#include "photo_capture.h"
#include "jpeg_encode.h"
#include "cloud_upload.h"
#include "sd_lock.h"
#include "usart.h"
#include "string.h"
#include "stdio.h"

Photo_CaptureContextTypeDef photo_capture_ctx;
uint8_t *Photo_Capture_Buffer = (uint8_t*)PHOTO_CAPTURE_BUFFER_ADDR;

static void PHOTO_CAPTURE_FindNextFilename(void);
static FRESULT PHOTO_CAPTURE_WriteToSD(void);

void PHOTO_CAPTURE_Init(void)
{
    photo_capture_ctx.state = PHOTO_STATE_IDLE;
    photo_capture_ctx.frame_buffer = Photo_Capture_Buffer;
    photo_capture_ctx.photo_count = 1;

    PHOTO_CAPTURE_FindNextFilename();

    printf("[PHOTO] Init complete, buffer at 0x%08X, next filename: %s\r\n", 
           (uint32_t)Photo_Capture_Buffer, photo_capture_ctx.filename);
}

static void PHOTO_CAPTURE_FindNextFilename(void)
{
    FRESULT res;
    DIR dir;
    FILINFO fno;
    uint32_t max_num = 0;
    uint32_t num;

    if (SD_Lock_TryAcquire(SD_LOCK_OWNER_PHOTO_WRITE) == 0U) {
        printf("[PHOTO] Cannot scan filename, SD lock owner=%s\r\n",
               SD_Lock_OwnerName(SD_Lock_GetOwner()));
        return;
    }

    res = f_opendir(&dir, "0:/");
    if (res == FR_OK) {
        while (f_readdir(&dir, &fno) == FR_OK && fno.fname[0] != 0) {
            if (strncmp(fno.fname, "IMG_", 4) == 0 && 
                sscanf(fno.fname, "IMG_%04lu.jpg", &num) == 1) {
                if (num > max_num) {
                    max_num = num;
                }
            }
        }
        f_closedir(&dir);
    }
    SD_Lock_Release(SD_LOCK_OWNER_PHOTO_WRITE);

    photo_capture_ctx.photo_count = max_num + 1;
    sprintf(photo_capture_ctx.filename, "IMG_%04lu.jpg", photo_capture_ctx.photo_count);
}

void PHOTO_CAPTURE_Request(void)
{
    if (photo_capture_ctx.state != PHOTO_STATE_IDLE) {
        printf("[PHOTO] Busy, cannot request\r\n");
        return;
    }

    photo_capture_ctx.state = PHOTO_STATE_WAIT_FRAME;
    printf("[PHOTO] Capture requested, waiting for frame...\r\n");
}

void PHOTO_CAPTURE_OnFrameComplete(void)
{
    uint32_t i;
    uint8_t *src, *dst;
    
    if (photo_capture_ctx.state == PHOTO_STATE_WAIT_FRAME) {
        OV5640_DCMI_Suspend();
        
        src = (uint8_t*)Frame_Buffer;
        dst = photo_capture_ctx.frame_buffer;
        
        SCB_InvalidateDCache_by_Addr((uint32_t*)Frame_Buffer, Display_Width * PHOTO_CAPTURE_HEIGHT * 2);
        
        for (i = 0; i < PHOTO_CAPTURE_HEIGHT; i++) {
            memcpy(dst, src, PHOTO_CAPTURE_WIDTH * 2);
            src += Display_Width * 2;
            dst += PHOTO_CAPTURE_WIDTH * 2;
        }
        
        SCB_CleanDCache_by_Addr((uint32_t*)photo_capture_ctx.frame_buffer, PHOTO_CAPTURE_WIDTH * PHOTO_CAPTURE_HEIGHT * 2);
        
        photo_capture_ctx.state = PHOTO_STATE_ENCODING;
        
        if (JPEG_Encode_Start(photo_capture_ctx.frame_buffer, 
                              PHOTO_CAPTURE_WIDTH, 
                              PHOTO_CAPTURE_HEIGHT) != 0) {
            printf("[PHOTO] JPEG encode start failed\r\n");
            OV5640_DCMI_Resume();
            photo_capture_ctx.state = PHOTO_STATE_IDLE;
        } else {
            printf("[PHOTO] Frame captured, encoding started\r\n");
        }
    }
}

void PHOTO_CAPTURE_Process(void)
{
    switch (photo_capture_ctx.state) {
        case PHOTO_STATE_ENCODING:
            JPEG_Encode_Process();
            
            if (jpeg_encode_ctx.state == JPEG_ENCODE_COMPLETE) {
                photo_capture_ctx.state = PHOTO_STATE_WRITING;
                printf("[PHOTO] Encoding complete, writing to SD...\r\n");
            } else if (jpeg_encode_ctx.state == JPEG_ENCODE_ERROR) {
                printf("[PHOTO] Encoding error, resuming...\r\n");
                OV5640_DCMI_Resume();
                photo_capture_ctx.state = PHOTO_STATE_IDLE;
                JPEG_Encode_Reset();
            }
            break;

        case PHOTO_STATE_WRITING:
            if (PHOTO_CAPTURE_WriteToSD() == FR_OK) {
                photo_capture_ctx.state = PHOTO_STATE_COMPLETE;
                printf("[PHOTO] Write complete\r\n");
            } else {
                printf("[PHOTO] Write failed, resuming...\r\n");
                OV5640_DCMI_Resume();
                photo_capture_ctx.state = PHOTO_STATE_IDLE;
            }
            break;

        case PHOTO_STATE_COMPLETE:
            if (Cloud_RequestLatestPhotoUpload(photo_capture_ctx.filename) != 0U) {
                printf("[PHOTO] Auto upload requested: %s\r\n", photo_capture_ctx.filename);
            } else {
                printf("[PHOTO] Auto upload skipped: %s\r\n", photo_capture_ctx.filename);
            }
            photo_capture_ctx.photo_count++;
            sprintf(photo_capture_ctx.filename, "IMG_%04lu.jpg", photo_capture_ctx.photo_count);
            JPEG_Encode_Reset();
            OV5640_DCMI_Resume();
            photo_capture_ctx.state = PHOTO_STATE_IDLE;
            printf("[PHOTO] Capture complete, resumed preview\r\n");
            break;

        default:
            break;
    }
}

uint8_t PHOTO_CAPTURE_IsBusy(void)
{
    return (photo_capture_ctx.state != PHOTO_STATE_IDLE);
}

static FRESULT PHOTO_CAPTURE_WriteToSD(void)
{
    FRESULT res;
    UINT bytes_written;
    uint8_t *jpeg_data = JPEG_Encode_GetOutputBuffer();
    uint32_t jpeg_size = JPEG_Encode_GetOutputSize();

    if (SD_Lock_TryAcquire(SD_LOCK_OWNER_PHOTO_WRITE) == 0U) {
        printf("[PHOTO] SD lock busy, owner=%s\r\n",
               SD_Lock_OwnerName(SD_Lock_GetOwner()));
        return FR_TIMEOUT;
    }

    SCB_CleanDCache_by_Addr((uint32_t*)jpeg_data, jpeg_size);

    printf("[PHOTO] JPEG data: addr=%p, size=%lu\r\n", jpeg_data, jpeg_size);
    printf("[PHOTO] JPEG header: 0x%02X%02X, footer: 0x%02X%02X\r\n",
           jpeg_data[0], jpeg_data[1],
           jpeg_data[jpeg_size-2], jpeg_data[jpeg_size-1]);
    
    if (jpeg_data[0] != 0xFF || jpeg_data[1] != 0xD8) {
        printf("[PHOTO] ERROR: Invalid JPEG SOI marker!\r\n");
    }
    if (jpeg_size < 4 || jpeg_data[jpeg_size-2] != 0xFF || jpeg_data[jpeg_size-1] != 0xD9) {
        printf("[PHOTO] ERROR: Invalid JPEG EOI marker!\r\n");
        uint32_t i;
        for (i = 0; i < jpeg_size - 1; i++) {
            if (jpeg_data[i] == 0xFF && jpeg_data[i+1] == 0xD9) {
                printf("[PHOTO] EOI found at offset %lu (expected at %lu)\r\n", i, jpeg_size - 2);
                break;
            }
        }
        if (i >= jpeg_size - 1) {
            printf("[PHOTO] EOI not found in buffer!\r\n");
        }
    }

    res = f_open(&photo_capture_ctx.jpeg_file, photo_capture_ctx.filename, FA_CREATE_ALWAYS | FA_WRITE);
    if (res != FR_OK) {
        printf("[PHOTO] f_open failed: %d\r\n", res);
        SD_Lock_Release(SD_LOCK_OWNER_PHOTO_WRITE);
        return res;
    }

    res = f_write(&photo_capture_ctx.jpeg_file, jpeg_data, jpeg_size, &bytes_written);
    if (res != FR_OK || bytes_written != jpeg_size) {
        printf("[PHOTO] f_write failed: %d, written: %lu/%lu\r\n", res, bytes_written, jpeg_size);
        f_close(&photo_capture_ctx.jpeg_file);
        SD_Lock_Release(SD_LOCK_OWNER_PHOTO_WRITE);
        return res;
    }

    res = f_close(&photo_capture_ctx.jpeg_file);
    if (res != FR_OK) {
        printf("[PHOTO] f_close failed: %d\r\n", res);
        SD_Lock_Release(SD_LOCK_OWNER_PHOTO_WRITE);
        return res;
    }

    SD_Lock_Release(SD_LOCK_OWNER_PHOTO_WRITE);
    printf("[PHOTO] Written %s, size: %lu bytes\r\n", photo_capture_ctx.filename, jpeg_size);
    return FR_OK;
}
