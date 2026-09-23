#include "gallery.h"
#include "ltdc.h"
#include "usart.h"
#include "dcmi.h"
#include "jpeg_utils.h"
#include "jpeg_encode.h"
#include "sd_lock.h"
#include <string.h>
#include <stdio.h>

#define CHUNK_SIZE_IN       ((uint32_t)(4096))
#define CHUNK_SIZE_OUT      ((uint32_t)(64 * 1024))

#define JPEG_BUFFER_EMPTY   0
#define JPEG_BUFFER_FULL    1

#define NB_INPUT_DATA_BUFFERS  2

typedef struct {
    uint8_t State;
    uint8_t *DataBuffer;
    uint32_t DataBufferSize;
} JPEG_Data_BufferTypeDef;

Gallery_ContextTypeDef gallery_ctx;
static FIL gallery_file;
FIL *gallery_pFile;

uint8_t Gallery_JPEG_InBuffer0[CHUNK_SIZE_IN];
uint8_t Gallery_JPEG_InBuffer1[CHUNK_SIZE_IN];

JPEG_Data_BufferTypeDef Gallery_JPEG_InBufferTab[NB_INPUT_DATA_BUFFERS] = {
    {JPEG_BUFFER_EMPTY, Gallery_JPEG_InBuffer0, 0},
    {JPEG_BUFFER_EMPTY, Gallery_JPEG_InBuffer1, 0}
};

uint32_t Gallery_JPEG_IN_Read_Index = 0;
uint32_t Gallery_JPEG_IN_Write_Index = 0;
__IO uint32_t Gallery_Input_Is_Paused = 0;
uint32_t Gallery_Jpeg_HWDecodingEnd = 0;

uint32_t Gallery_FrameBufferAddress;

static int32_t GALLERY_ScanFiles(void);
void JPEG_YCbCrToRGB(uint8_t *pSrc, uint16_t *pDst, uint32_t width, uint32_t height, uint32_t chroma_sampling);
static void GALLERY_DrawList(void);
static void GALLERY_DrawViewing(void);
static void GALLERY_DrawExitButton(void);
static void GALLERY_DrawFileInfo(void);

void GALLERY_Init(void)
{
    memset(&gallery_ctx, 0, sizeof(Gallery_ContextTypeDef));
    gallery_ctx.decode_buffer = (uint8_t*)(SDRAM_BANK_ADDR + 0x200000);
    printf("[GALLERY] Init, decode buffer at %p\r\n", gallery_ctx.decode_buffer);
}

void GALLERY_Enter(uint8_t prev_mode)
{
    gallery_ctx.state = GALLERY_STATE_LIST;
    gallery_ctx.prev_mode = prev_mode;
    gallery_ctx.scroll_offset = 0;
    gallery_ctx.current_page = 0;
    gallery_ctx.selected_file = 0;
    gallery_ctx.file_count = 0;
    gallery_ctx.decode_done = 0;
    gallery_ctx.need_redraw = 1;
    
    OV5640_DCMI_Suspend();
    printf("[GALLERY] Camera suspended\r\n");
    
    GALLERY_ScanFiles();
    GALLERY_DrawList();
    GALLERY_DrawExitButton();
    gallery_ctx.need_redraw = 0;
}

void GALLERY_Exit(void)
{
    gallery_ctx.state = GALLERY_STATE_LIST;
    gallery_ctx.decode_done = 0;
    Gallery_Jpeg_HWDecodingEnd = 0;
    
    if (gallery_pFile != NULL) {
        f_close(gallery_pFile);
        gallery_pFile = NULL;
    }
    SD_Lock_Release(SD_LOCK_OWNER_GALLERY_VIEW);
    
    OV5640_DCMI_Resume();
    printf("[GALLERY] Camera resumed\r\n");
}

void GALLERY_Process(void)
{
    if (gallery_ctx.state == GALLERY_STATE_VIEWING && gallery_ctx.decode_done == 0) {
        static uint32_t process_count = 0;
        process_count++;
        if (process_count % 100 == 0) {
            printf("[GALLERY] Process: HWDecodingEnd=%d, decode_done=%d, ReadIdx=%lu, WriteIdx=%lu, InputPaused=%d\r\n",
                   Gallery_Jpeg_HWDecodingEnd, gallery_ctx.decode_done,
                   Gallery_JPEG_IN_Read_Index, Gallery_JPEG_IN_Write_Index, Gallery_Input_Is_Paused);
        }
        
        uint32_t result;
        do {
            result = JPEG_InputHandler(&jpeg_encode_ctx.hjpeg);
            if (Gallery_Jpeg_HWDecodingEnd == 1) {
                break;
            }
        } while (result == 0);
        
        if (Gallery_Jpeg_HWDecodingEnd == 1 && gallery_ctx.decode_done == 0) {
            gallery_ctx.decode_done = 1;
            gallery_ctx.need_redraw = 1;
            printf("[GALLERY] Decode complete, displaying image\r\n");
            
            f_close(gallery_pFile);
            gallery_pFile = NULL;
            SD_Lock_Release(SD_LOCK_OWNER_GALLERY_VIEW);
        }
    }
}

void GALLERY_Render(void)
{
    if (gallery_ctx.need_redraw) {
        if (gallery_ctx.state == GALLERY_STATE_LIST) {
            GALLERY_DrawList();
        } else if (gallery_ctx.state == GALLERY_STATE_VIEWING) {
            GALLERY_DrawViewing();
        }
        GALLERY_DrawExitButton();
        gallery_ctx.need_redraw = 0;
    }
}

static uint8_t gallery_touch_down = 0;
static uint16_t gallery_last_y = 0;
static uint32_t gallery_last_click_time = 0;
static int16_t gallery_total_delta = 0;
static uint32_t gallery_press_start_time = 0;
static uint16_t gallery_press_x = 0;
static uint16_t gallery_press_y = 0;

void GALLERY_HandleTouchRelease(void)
{
    if (gallery_touch_down && gallery_ctx.state == GALLERY_STATE_LIST) {
        if (gallery_total_delta < 5 && gallery_total_delta > -5) {
            uint32_t current_time = HAL_GetTick();
            if (current_time - gallery_last_click_time < 300) {
                uint32_t row = (gallery_press_y - 60) / 50;
                uint32_t col = (gallery_press_x < 320) ? 0 : 1;
                uint32_t file_index = gallery_ctx.current_page * 16 + row * 2 + col;
                
                if (file_index < gallery_ctx.file_count && gallery_press_x < 640 && gallery_press_y >= 60) {
                    printf("[GALLERY] Double-click on file %lu: %s\r\n", file_index, gallery_ctx.files[file_index].filename);
                    gallery_ctx.selected_file = file_index;
                    gallery_ctx.state = GALLERY_STATE_VIEWING;
                    gallery_ctx.decode_done = 0;
                    gallery_ctx.file_ended = 0;
                    gallery_ctx.total_bytes_read = 0;
                    Gallery_Jpeg_HWDecodingEnd = 0;
                    
                    FRESULT res;
                    char filepath[64];
                    
                    sprintf(filepath, "/%s", gallery_ctx.files[file_index].filename);
                    
                    if (SD_Lock_TryAcquire(SD_LOCK_OWNER_GALLERY_VIEW) == 0U) {
                        printf("[GALLERY] SD lock busy, owner=%s\r\n",
                               SD_Lock_OwnerName(SD_Lock_GetOwner()));
                        gallery_ctx.state = GALLERY_STATE_LIST;
                        gallery_ctx.need_redraw = 1;
                        gallery_touch_down = 0;
                        return;
                    }

                    res = f_open(&gallery_file, filepath, FA_READ);
                    if (res != FR_OK) {
                        printf("[GALLERY] Failed to open file: %s, error: %d\r\n", filepath, res);
                        SD_Lock_Release(SD_LOCK_OWNER_GALLERY_VIEW);
                        gallery_ctx.state = GALLERY_STATE_LIST;
                        gallery_ctx.need_redraw = 1;
                        gallery_touch_down = 0;
                        return;
                    }
                    
                    gallery_pFile = &gallery_file;
                    Gallery_FrameBufferAddress = (uint32_t)gallery_ctx.decode_buffer;
                    
                    Gallery_JPEG_IN_Read_Index = 0;
                    Gallery_JPEG_IN_Write_Index = 0;
                    Gallery_Input_Is_Paused = 0;
                    
                    for (uint32_t i = 0; i < NB_INPUT_DATA_BUFFERS; i++) {
                        UINT bytes_read;
                        if (f_read(gallery_pFile, Gallery_JPEG_InBufferTab[i].DataBuffer, CHUNK_SIZE_IN, &bytes_read) == FR_OK) {
                            Gallery_JPEG_InBufferTab[i].DataBufferSize = bytes_read;
                            Gallery_JPEG_InBufferTab[i].State = JPEG_BUFFER_FULL;
                        } else {
                            printf("[GALLERY] Failed to read file data!\r\n");
                            f_close(gallery_pFile);
                            gallery_pFile = NULL;
                            SD_Lock_Release(SD_LOCK_OWNER_GALLERY_VIEW);
                            gallery_ctx.state = GALLERY_STATE_LIST;
                            gallery_ctx.need_redraw = 1;
                            gallery_touch_down = 0;
                            return;
                        }
                    }
                    
                    printf("[GALLERY] Starting JPEG decode: %s\r\n", gallery_ctx.files[file_index].filename);
                    printf("[GALLERY] JPEG state before abort: %d\r\n", jpeg_encode_ctx.hjpeg.State);
                    
                    HAL_JPEG_Abort(&jpeg_encode_ctx.hjpeg);
                    HAL_Delay(10);
                    
                    printf("[GALLERY] JPEG state after abort: %d\r\n", jpeg_encode_ctx.hjpeg.State);
                    
                    HAL_JPEG_Decode_DMA(&jpeg_encode_ctx.hjpeg, Gallery_JPEG_InBufferTab[0].DataBuffer,
                                        Gallery_JPEG_InBufferTab[0].DataBufferSize,
                                        (uint8_t*)Gallery_FrameBufferAddress, CHUNK_SIZE_OUT);
                    
                    printf("[GALLERY] JPEG state after decode start: %d\r\n", jpeg_encode_ctx.hjpeg.State);
                    
                    gallery_ctx.need_redraw = 1;
                }
            }
            gallery_last_click_time = current_time;
        }
    }
    gallery_touch_down = 0;
}

void GALLERY_HandleTouchEvent(uint16_t x, uint16_t y)
{
    if (x >= 640 && x <= 800 && y >= 400 && y <= 480) {
        return;
    }
    
    if (gallery_ctx.state == GALLERY_STATE_LIST) {
        if (!gallery_touch_down) {
            gallery_touch_down = 1;
            gallery_last_y = y;
            gallery_total_delta = 0;
            gallery_press_x = x;
            gallery_press_y = y;
            gallery_press_start_time = HAL_GetTick();
            
            if (x >= 650 && x <= 710 && y >= 200 && y <= 260) {
                if (gallery_ctx.current_page > 0) {
                    gallery_ctx.current_page--;
                    gallery_ctx.need_redraw = 1;
                    printf("[GALLERY] Page %lu\r\n", gallery_ctx.current_page + 1);
                }
            } else if (x >= 730 && x <= 790 && y >= 200 && y <= 260) {
                uint32_t max_page = (gallery_ctx.file_count + 15) / 16;
                if (max_page == 0) max_page = 1;
                if (gallery_ctx.current_page < max_page - 1) {
                    gallery_ctx.current_page++;
                    gallery_ctx.need_redraw = 1;
                    printf("[GALLERY] Page %lu\r\n", gallery_ctx.current_page + 1);
                }
            }
            
            return;
        }
        
        int16_t delta = gallery_last_y - y;
        gallery_last_y = y;
        gallery_total_delta += delta;
    } else if (gallery_ctx.state == GALLERY_STATE_VIEWING) {
        if (x >= 640 && x <= 800 && y >= 320 && y <= 390) {
            printf("[GALLERY] Return to list\r\n");
            HAL_JPEG_Abort(&jpeg_encode_ctx.hjpeg);
            f_close(gallery_pFile);
            gallery_pFile = NULL;
            SD_Lock_Release(SD_LOCK_OWNER_GALLERY_VIEW);
            gallery_ctx.state = GALLERY_STATE_LIST;
            gallery_ctx.decode_done = 0;
            gallery_ctx.need_redraw = 1;
        }
    }
}

static int32_t GALLERY_ScanFiles(void)
{
    FRESULT res;
    FILINFO fno;
    DIR dir;
    int32_t count = 0;
    
    if (SD_Lock_TryAcquire(SD_LOCK_OWNER_GALLERY_SCAN) == 0U) {
        printf("[GALLERY] Scan blocked, SD lock owner=%s\r\n",
               SD_Lock_OwnerName(SD_Lock_GetOwner()));
        return -1;
    }

    res = f_opendir(&dir, "/");
    if (res != FR_OK) {
        printf("[GALLERY] Failed to open directory!\r\n");
        SD_Lock_Release(SD_LOCK_OWNER_GALLERY_SCAN);
        return -1;
    }
    
    gallery_ctx.file_count = 0;
    
    while (1) {
        res = f_readdir(&dir, &fno);
        if (res != FR_OK || fno.fname[0] == 0) break;
        
        if (fno.fname[0] == '.') continue;
        
        uint32_t len = strlen(fno.fname);
        if (len > 4 && strcmp(&fno.fname[len-4], ".jpg") == 0) {
            strncpy(gallery_ctx.files[count].filename, fno.fname, GALLERY_FILENAME_LEN-1);
            gallery_ctx.files[count].filename[GALLERY_FILENAME_LEN-1] = 0;
            gallery_ctx.files[count].filesize = fno.fsize;
            count++;
            
            if (count >= GALLERY_MAX_FILES) break;
        }
    }
    
    f_closedir(&dir);
    SD_Lock_Release(SD_LOCK_OWNER_GALLERY_SCAN);
    
    gallery_ctx.file_count = count;
    
    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            int num_i = 0, num_j = 0;
            sscanf(gallery_ctx.files[i].filename, "IMG_%d.jpg", &num_i);
            sscanf(gallery_ctx.files[j].filename, "IMG_%d.jpg", &num_j);
            
            if (num_j < num_i) {
                Gallery_FileInfoTypeDef temp = gallery_ctx.files[i];
                gallery_ctx.files[i] = gallery_ctx.files[j];
                gallery_ctx.files[j] = temp;
            }
        }
    }
    
    return count;
}

uint32_t JPEG_InputHandler(JPEG_HandleTypeDef *hjpeg)
{
    if (Gallery_Jpeg_HWDecodingEnd == 0) {
        if (Gallery_JPEG_InBufferTab[Gallery_JPEG_IN_Write_Index].State == JPEG_BUFFER_EMPTY) {
            UINT bytes_read;
            
            if (!gallery_ctx.file_ended) {
                if (f_read(gallery_pFile, Gallery_JPEG_InBufferTab[Gallery_JPEG_IN_Write_Index].DataBuffer,
                          CHUNK_SIZE_IN, &bytes_read) == FR_OK) {
                    Gallery_JPEG_InBufferTab[Gallery_JPEG_IN_Write_Index].DataBufferSize = bytes_read;
                    Gallery_JPEG_InBufferTab[Gallery_JPEG_IN_Write_Index].State = JPEG_BUFFER_FULL;
                    gallery_ctx.total_bytes_read += bytes_read;
                    
                    printf("[GALLERY] Read chunk: %lu bytes, total: %lu bytes, WriteIdx=%lu\r\n", 
                           bytes_read, gallery_ctx.total_bytes_read, Gallery_JPEG_IN_Write_Index);
                    
                    if (bytes_read < CHUNK_SIZE_IN) {
                        printf("[GALLERY] File read complete, last chunk: %lu bytes\r\n", bytes_read);
                        gallery_ctx.file_ended = 1;
                    }
                } else {
                    printf("[GALLERY] Error reading JPEG data!\r\n");
                    gallery_ctx.file_ended = 1;
                    f_close(gallery_pFile);
                    gallery_pFile = NULL;
                    SD_Lock_Release(SD_LOCK_OWNER_GALLERY_VIEW);
                    gallery_ctx.state = GALLERY_STATE_LIST;
                    gallery_ctx.decode_done = 0;
                    gallery_ctx.need_redraw = 1;
                }
            } else {
                Gallery_JPEG_InBufferTab[Gallery_JPEG_IN_Write_Index].DataBufferSize = 0;
                Gallery_JPEG_InBufferTab[Gallery_JPEG_IN_Write_Index].State = JPEG_BUFFER_FULL;
                printf("[GALLERY] Sending empty buffer to signal end of data\r\n");
                gallery_ctx.file_ended = 0;
            }
            
            if ((Gallery_Input_Is_Paused == 1) && (Gallery_JPEG_IN_Write_Index == Gallery_JPEG_IN_Read_Index)) {
                Gallery_Input_Is_Paused = 0;
                HAL_JPEG_ConfigInputBuffer(hjpeg, Gallery_JPEG_InBufferTab[Gallery_JPEG_IN_Read_Index].DataBuffer,
                                           Gallery_JPEG_InBufferTab[Gallery_JPEG_IN_Read_Index].DataBufferSize);
                
                HAL_JPEG_Resume(hjpeg, JPEG_PAUSE_RESUME_INPUT);
                printf("[GALLERY] Resumed JPEG input, ReadIdx=%lu, size=%lu\r\n", 
                       Gallery_JPEG_IN_Read_Index, Gallery_JPEG_InBufferTab[Gallery_JPEG_IN_Read_Index].DataBufferSize);
            }
            
            Gallery_JPEG_IN_Write_Index++;
            if (Gallery_JPEG_IN_Write_Index >= NB_INPUT_DATA_BUFFERS) {
                Gallery_JPEG_IN_Write_Index = 0;
            }
            
            return 0;
        } else {
            return 2;
        }
    } else {
        gallery_ctx.file_ended = 0;
        gallery_ctx.total_bytes_read = 0;
        return 1;
    }
}

static void GALLERY_DrawList(void)
{
    LCD_SetColor(LCD_BLACK);
    LCD_FillRect(0, 0, 640, 480);
    
    LCD_SetColor(LCD_WHITE);
    LCD_SetFont(&Font32);
    LCD_DisplayString(200, 10, "Photo Gallery", LCD_BLACK);
    
    LCD_SetFont(&Font32);
    
    if (gallery_ctx.file_count == 0) {
        LCD_SetColor(LCD_GREY);
        LCD_DisplayString(10, 100, "No photos found", LCD_BLACK);
        return;
    }
    
    uint32_t page_start = gallery_ctx.current_page * 16;
    uint32_t cols = 2;
    uint32_t rows = 8;
    uint32_t col_width = 320;
    uint32_t row_height = 50;
    
    for (uint32_t row = 0; row < rows; row++) {
        for (uint32_t col = 0; col < cols; col++) {
            uint32_t file_index = page_start + row * cols + col;
            if (file_index >= gallery_ctx.file_count) {
                break;
            }
            
            uint32_t x = col * col_width + 10;
            uint32_t y = row * row_height + 60;
            
            char info[48];
            sprintf(info, "%s", gallery_ctx.files[file_index].filename);
            
            LCD_SetColor(LCD_WHITE);
            LCD_DisplayString(x, y, info, LCD_BLACK);
        }
    }
    
    LCD_SetColor(LCD_WHITE);
    LCD_SetFont(&Font16);
    LCD_DisplayString(240, 450, "Double-click to view", LCD_BLACK);
}

static void GALLERY_DrawViewing(void)
{
    LCD_SetColor(LCD_BLACK);
    LCD_FillRect(0, 0, 640, 480);
    
    if (!gallery_ctx.decode_done) {
        LCD_SetColor(LCD_WHITE);
        LCD_SetFont(&Font32);
        LCD_DisplayString(10, 200, "Decoding...", LCD_BLACK);
    } else {
        uint16_t x = (640 - gallery_ctx.image_width) / 2;
        uint16_t y = (480 - gallery_ctx.image_height) / 2;
        
        extern DMA2D_HandleTypeDef hdma2d;
        
        uint32_t cssMode;
        uint32_t inputLineOffset = gallery_ctx.image_width % 16;
        if (inputLineOffset != 0) {
            inputLineOffset = 16 - inputLineOffset;
        }
        
        if (gallery_ctx.chroma_sampling == JPEG_422_SUBSAMPLING) {
            cssMode = DMA2D_CSS_422;
        } else if (gallery_ctx.chroma_sampling == JPEG_420_SUBSAMPLING) {
            cssMode = DMA2D_CSS_420;
        } else {
            cssMode = DMA2D_NO_CSS;
        }
        
        hdma2d.Init.Mode = DMA2D_M2M_PFC;
        hdma2d.Init.ColorMode = DMA2D_OUTPUT_RGB565;
        hdma2d.Init.OutputOffset = LCD_Width - gallery_ctx.image_width;
        hdma2d.Init.AlphaInverted = DMA2D_REGULAR_ALPHA;
        hdma2d.Init.RedBlueSwap = DMA2D_RB_REGULAR;
        
        hdma2d.LayerCfg[1].AlphaMode = DMA2D_REPLACE_ALPHA;
        hdma2d.LayerCfg[1].InputAlpha = 0xFF;
        hdma2d.LayerCfg[1].InputColorMode = DMA2D_INPUT_YCBCR;
        hdma2d.LayerCfg[1].ChromaSubSampling = cssMode;
        hdma2d.LayerCfg[1].InputOffset = inputLineOffset;
        hdma2d.LayerCfg[1].RedBlueSwap = DMA2D_RB_REGULAR;
        hdma2d.LayerCfg[1].AlphaInverted = DMA2D_REGULAR_ALPHA;
        
        HAL_DMA2D_Init(&hdma2d);
        HAL_DMA2D_ConfigLayer(&hdma2d, 1);
        
        uint32_t destination = LCD_MemoryAdd + 2 * (LCD_Width * y + x);
        HAL_DMA2D_Start(&hdma2d, (uint32_t)gallery_ctx.decode_buffer, destination, gallery_ctx.image_width, gallery_ctx.image_height);
        HAL_DMA2D_PollForTransfer(&hdma2d, 25);
        
        GALLERY_DrawFileInfo();
    }
}

static void GALLERY_DrawArrow(uint16_t x, uint16_t y, uint8_t direction, uint32_t color)
{
    LCD_SetColor(color);
    if (direction == 0) {
        LCD_DrawLine(x + 25, y + 10, x + 10, y + 30);
        LCD_DrawLine(x + 25, y + 50, x + 10, y + 30);
        LCD_DrawLine(x + 25, y + 10, x + 25, y + 50);
    } else {
        LCD_DrawLine(x + 35, y + 10, x + 50, y + 30);
        LCD_DrawLine(x + 35, y + 50, x + 50, y + 30);
        LCD_DrawLine(x + 35, y + 10, x + 35, y + 50);
    }
}

static void GALLERY_DrawRoundedRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color)
{
    uint16_t r = 15;
    LCD_SetColor(color);
    LCD_FillRect(x + r, y, w - 2 * r, h);
    LCD_FillRect(x, y + r, w, h - 2 * r);
    LCD_FillRect(x, y, r, r);
    LCD_FillRect(x + w - r, y, r, r);
    LCD_FillRect(x, y + h - r, r, r);
    LCD_FillRect(x + w - r, y + h - r, r, r);
}

static void GALLERY_DrawExitButton(void)
{
    LCD_SetColor(LCD_WHITE);
    LCD_FillRect(640, 0, 160, 480);
    
    GALLERY_DrawRoundedRect(650, 400, 140, 70, LCD_RED);
    LCD_SetColor(LCD_WHITE);
    LCD_SetFont(&Font24);
    LCD_DisplayString(696, 425, "EXIT", LCD_RED);
    
    if (gallery_ctx.state == GALLERY_STATE_VIEWING) {
        GALLERY_DrawRoundedRect(650, 320, 140, 70, LCD_BLUE);
        LCD_SetColor(LCD_WHITE);
        LCD_SetFont(&Font24);
        LCD_DisplayString(696, 345, "BACK", LCD_BLUE);
    } else {
        uint32_t max_page = (gallery_ctx.file_count + 15) / 16;
        if (max_page == 0) max_page = 1;
        
        if (gallery_ctx.current_page > 0) {
            GALLERY_DrawRoundedRect(650, 200, 60, 60, LCD_GREEN);
            GALLERY_DrawArrow(650, 200, 0, LCD_BLACK);
        } else {
            GALLERY_DrawRoundedRect(650, 200, 60, 60, LCD_GREY);
            GALLERY_DrawArrow(650, 200, 0, LCD_WHITE);
        }
        
        if (gallery_ctx.current_page < max_page - 1) {
            GALLERY_DrawRoundedRect(730, 200, 60, 60, LCD_GREEN);
            GALLERY_DrawArrow(730, 200, 1, LCD_BLACK);
        } else {
            GALLERY_DrawRoundedRect(730, 200, 60, 60, LCD_GREY);
            GALLERY_DrawArrow(730, 200, 1, LCD_WHITE);
        }
        
        char page_info[16];
        sprintf(page_info, "%lu/%lu", gallery_ctx.current_page + 1, max_page);
        LCD_SetColor(LCD_BLACK);
        LCD_SetFont(&Font16);
        LCD_DisplayString(710, 280, page_info, LCD_WHITE);
    }
}

static void GALLERY_DrawFileInfo(void)
{
    LCD_SetColor(LCD_WHITE);
    LCD_SetFont(&Font16);
    char info[48];
    sprintf(info, "%s (%dx%d)", gallery_ctx.files[gallery_ctx.selected_file].filename,
            gallery_ctx.image_width, gallery_ctx.image_height);
    LCD_DisplayString(10, 10, info, LCD_BLACK);
}

void JPEG_YCbCrToRGB(uint8_t *pSrc, uint16_t *pDst, uint32_t width, uint32_t height, uint32_t chroma_sampling)
{
    if (chroma_sampling == 2) {
        for (uint32_t y = 0; y < height; y++) {
            for (uint32_t x = 0; x < width; x += 2) {
                uint8_t Y0 = pSrc[0];
                uint8_t Cb = pSrc[1];
                uint8_t Y1 = pSrc[2];
                uint8_t Cr = pSrc[3];
                pSrc += 4;
                
                int16_t R0 = Y0 + ((Cr - 128) * 359) / 256;
                int16_t G0 = Y0 - ((Cb - 128) * 88) / 256 - ((Cr - 128) * 183) / 256;
                int16_t B0 = Y0 + ((Cb - 128) * 454) / 256;
                
                int16_t R1 = Y1 + ((Cr - 128) * 359) / 256;
                int16_t G1 = Y1 - ((Cb - 128) * 88) / 256 - ((Cr - 128) * 183) / 256;
                int16_t B1 = Y1 + ((Cb - 128) * 454) / 256;
                
                R0 = (R0 < 0) ? 0 : ((R0 > 255) ? 255 : R0);
                G0 = (G0 < 0) ? 0 : ((G0 > 255) ? 255 : G0);
                B0 = (B0 < 0) ? 0 : ((B0 > 255) ? 255 : B0);
                
                R1 = (R1 < 0) ? 0 : ((R1 > 255) ? 255 : R1);
                G1 = (G1 < 0) ? 0 : ((G1 > 255) ? 255 : G1);
                B1 = (B1 < 0) ? 0 : ((B1 > 255) ? 255 : B1);
                
                *pDst++ = ((R0 >> 3) << 11) | ((G0 >> 2) << 5) | (B0 >> 3);
                *pDst++ = ((R1 >> 3) << 11) | ((G1 >> 2) << 5) | (B1 >> 3);
            }
        }
    } else {
        for (uint32_t i = 0; i < width * height; i++) {
            uint8_t Y = pSrc[0];
            uint8_t Cb = pSrc[1];
            uint8_t Cr = pSrc[2];
            pSrc += 3;
            
            int16_t R = Y + ((Cr - 128) * 359) / 256;
            int16_t G = Y - ((Cb - 128) * 88) / 256 - ((Cr - 128) * 183) / 256;
            int16_t B = Y + ((Cb - 128) * 454) / 256;
            
            R = (R < 0) ? 0 : ((R > 255) ? 255 : R);
            G = (G < 0) ? 0 : ((G > 255) ? 255 : G);
            B = (B < 0) ? 0 : ((B > 255) ? 255 : B);
            
            *pDst++ = ((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3);
        }
    }
}
