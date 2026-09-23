#include "jpeg_encode.h"
#include "jpeg_utils.h"
#include "photo_capture.h"
#include "gallery.h"
#include "sd_lock.h"
#include "usart.h"
#include <string.h>
#include <stdio.h>

#define BYTES_PER_PIXEL    2
#define CHUNK_SIZE_IN      ((uint32_t)(MAX_INPUT_WIDTH * BYTES_PER_PIXEL * MAX_INPUT_LINES))
#define CHUNK_SIZE_OUT     ((uint32_t)(4096))

#define JPEG_BUFFER_EMPTY  0
#define JPEG_BUFFER_FULL   1

typedef struct {
    __IO uint8_t State;
    uint8_t *DataBuffer;
    __IO uint32_t DataBufferSize;
} JPEG_Data_BufferTypeDef;

static JPEG_Data_BufferTypeDef Jpeg_OUT_BufferTab;
static JPEG_Data_BufferTypeDef Jpeg_IN_BufferTab[2];

static JPEG_RGBToYCbCr_Convert_Function pRGBToYCbCr_Convert_Function;

static uint8_t Input_Data_Buffer[CHUNK_SIZE_IN];
static uint8_t MCU_Data_InBuffer[CHUNK_SIZE_IN];
static uint8_t JPEG_Data_OutBuffer[CHUNK_SIZE_OUT];

static uint32_t MCU_TotalNb = 0;
static uint32_t MCU_BlockIndex = 0;
static __IO uint32_t Jpeg_HWEncodingEnd = 0;
static __IO uint32_t Output_Is_Paused = 0;
static __IO uint32_t Input_Is_Paused = 0;
static uint32_t CurrentLine = 1;

static void ReadRgbLines(uint8_t *pDataBuffer, uint32_t *BufferSize);

JPEG_EncodeContextTypeDef jpeg_encode_ctx;

void JPEG_Encode_Init(void)
{
    JPEG_InitColorTables();

    memset(&jpeg_encode_ctx.hjpeg, 0, sizeof(JPEG_HandleTypeDef));
    jpeg_encode_ctx.hjpeg.Instance = JPEG;

    if (HAL_JPEG_Init(&jpeg_encode_ctx.hjpeg) != HAL_OK) {
        printf("[JPEG] HAL_Init failed!\r\n");
        jpeg_encode_ctx.state = JPEG_ENCODE_ERROR;
        return;
    }

    Jpeg_OUT_BufferTab.DataBuffer = JPEG_Data_OutBuffer;
    Jpeg_IN_BufferTab[0].DataBuffer = MCU_Data_InBuffer;

    jpeg_encode_ctx.output_buffer = (uint8_t*)(PHOTO_CAPTURE_BUFFER_ADDR + PHOTO_CAPTURE_BUFFER_SIZE + 0x1000);
    jpeg_encode_ctx.state = JPEG_ENCODE_IDLE;
    printf("[JPEG] Init complete, output buffer at %p\r\n", jpeg_encode_ctx.output_buffer);
}

int32_t JPEG_Encode_Start(uint8_t *rgb565_buffer, uint32_t width, uint32_t height)
{
    if (jpeg_encode_ctx.state != JPEG_ENCODE_IDLE) {
        printf("[JPEG] Busy, cannot start\r\n");
        return -1;
    }

    jpeg_encode_ctx.input_buffer = rgb565_buffer;
    jpeg_encode_ctx.image_width = width;
    jpeg_encode_ctx.image_height = height;
    jpeg_encode_ctx.output_size = 0;
    jpeg_encode_ctx.state = JPEG_ENCODE_RUNNING;

    MCU_TotalNb = 0;
    MCU_BlockIndex = 0;
    Jpeg_HWEncodingEnd = 0;
    Output_Is_Paused = 0;
    Input_Is_Paused = 0;
    CurrentLine = 1;

    JPEG_ConfTypeDef Conf;
    Conf.ImageWidth = width;
    Conf.ImageHeight = height;
    Conf.ChromaSubsampling = JPEG_422_SUBSAMPLING;
    Conf.ColorSpace = JPEG_YCBCR_COLORSPACE;
    Conf.ImageQuality = 80;

    JPEG_GetEncodeColorConvertFunc(&Conf, &pRGBToYCbCr_Convert_Function, &MCU_TotalNb);

    Jpeg_OUT_BufferTab.DataBufferSize = 0;
    Jpeg_OUT_BufferTab.State = JPEG_BUFFER_EMPTY;

    Jpeg_IN_BufferTab[0].DataBufferSize = 0;
    Jpeg_IN_BufferTab[0].State = JPEG_BUFFER_EMPTY;

    uint32_t dataBufferSize = 0;
    ReadRgbLines(Input_Data_Buffer, &dataBufferSize);
    if (dataBufferSize == 0) {
        printf("[JPEG] No input data\r\n");
        jpeg_encode_ctx.state = JPEG_ENCODE_ERROR;
        return -1;
    }

    MCU_BlockIndex += pRGBToYCbCr_Convert_Function(Input_Data_Buffer, Jpeg_IN_BufferTab[0].DataBuffer,
                                                    0, dataBufferSize, (uint32_t*)(&Jpeg_IN_BufferTab[0].DataBufferSize));
    
    SCB_CleanDCache_by_Addr((uint32_t*)Jpeg_IN_BufferTab[0].DataBuffer, Jpeg_IN_BufferTab[0].DataBufferSize);
    
    Jpeg_IN_BufferTab[0].State = JPEG_BUFFER_FULL;

    HAL_JPEG_ConfigEncoding(&jpeg_encode_ctx.hjpeg, &Conf);

    HAL_JPEG_Encode_DMA(&jpeg_encode_ctx.hjpeg, Jpeg_IN_BufferTab[0].DataBuffer,
                        Jpeg_IN_BufferTab[0].DataBufferSize,
                        Jpeg_OUT_BufferTab.DataBuffer, CHUNK_SIZE_OUT);

    printf("[JPEG] Encoding started: %dx%d, MCU: %lu\r\n", width, height, MCU_TotalNb);
    return 0;
}

void JPEG_Encode_Process(void)
{
    uint32_t JpegEncodeProcessing_End = 0;
    
    if (jpeg_encode_ctx.state != JPEG_ENCODE_RUNNING) {
        return;
    }

    do {
        JPEG_EncodeInputHandler();
        JpegEncodeProcessing_End = JPEG_EncodeOutputHandler();
        
        if (Jpeg_HWEncodingEnd != 0 && Output_Is_Paused == 1) {
            Output_Is_Paused = 0;
            HAL_JPEG_Resume(&jpeg_encode_ctx.hjpeg, JPEG_PAUSE_RESUME_OUTPUT);
        }
    } while (JpegEncodeProcessing_End == 0);
    
    uint32_t i;
    for (i = jpeg_encode_ctx.output_size - 2; i > 0; i--) {
        if (jpeg_encode_ctx.output_buffer[i] == 0xFF && jpeg_encode_ctx.output_buffer[i+1] == 0xD9) {
            if (i + 2 != jpeg_encode_ctx.output_size) {
                printf("[JPEG] EOI corrected: size %lu -> %lu\r\n", jpeg_encode_ctx.output_size, i + 2);
                jpeg_encode_ctx.output_size = i + 2;
            }
            break;
        }
    }
    
    jpeg_encode_ctx.state = JPEG_ENCODE_COMPLETE;
    printf("[JPEG] Encoding complete, size: %lu bytes\r\n", jpeg_encode_ctx.output_size);
}

uint32_t JPEG_EncodeOutputHandler(void)
{
    uint8_t buffer_state;
    uint32_t data_size;
    uint32_t encoding_end;
    uint32_t output_paused;
    
    __disable_irq();
    buffer_state = Jpeg_OUT_BufferTab.State;
    data_size = Jpeg_OUT_BufferTab.DataBufferSize;
    encoding_end = Jpeg_HWEncodingEnd;
    output_paused = Output_Is_Paused;
    
    if (buffer_state == JPEG_BUFFER_FULL) {
        Jpeg_OUT_BufferTab.State = JPEG_BUFFER_EMPTY;
        Jpeg_OUT_BufferTab.DataBufferSize = 0;
    }
    __enable_irq();
    
    if (buffer_state == JPEG_BUFFER_FULL) {
        SCB_InvalidateDCache_by_Addr((uint32_t*)Jpeg_OUT_BufferTab.DataBuffer, CHUNK_SIZE_OUT);
        
        if (jpeg_encode_ctx.output_size + data_size <= JPEG_OUTPUT_BUFFER_SIZE) {
            memcpy(jpeg_encode_ctx.output_buffer + jpeg_encode_ctx.output_size,
                   Jpeg_OUT_BufferTab.DataBuffer,
                   data_size);
            jpeg_encode_ctx.output_size += data_size;
        }

        if (encoding_end != 0) {
            return 1;
        } else if (output_paused == 1) {
            Output_Is_Paused = 0;
            HAL_JPEG_Resume(&jpeg_encode_ctx.hjpeg, JPEG_PAUSE_RESUME_OUTPUT);
        }
    }
    return 0;
}

void JPEG_EncodeInputHandler(void)
{
    uint32_t dataBufferSize = 0;

    if ((Jpeg_IN_BufferTab[0].State == JPEG_BUFFER_EMPTY) && (MCU_BlockIndex <= MCU_TotalNb)) {
        ReadRgbLines(Input_Data_Buffer, &dataBufferSize);
        if (dataBufferSize != 0) {
            MCU_BlockIndex += pRGBToYCbCr_Convert_Function(Input_Data_Buffer, Jpeg_IN_BufferTab[0].DataBuffer,
                                                            0, dataBufferSize, (uint32_t*)(&Jpeg_IN_BufferTab[0].DataBufferSize));
            
            SCB_CleanDCache_by_Addr((uint32_t*)Jpeg_IN_BufferTab[0].DataBuffer, Jpeg_IN_BufferTab[0].DataBufferSize);
            
            Jpeg_IN_BufferTab[0].State = JPEG_BUFFER_FULL;

            if (Input_Is_Paused == 1) {
                Input_Is_Paused = 0;
                HAL_JPEG_ConfigInputBuffer(&jpeg_encode_ctx.hjpeg, Jpeg_IN_BufferTab[0].DataBuffer,
                                          Jpeg_IN_BufferTab[0].DataBufferSize);
                HAL_JPEG_Resume(&jpeg_encode_ctx.hjpeg, JPEG_PAUSE_RESUME_INPUT);
            }
        } else {
            MCU_BlockIndex++;
        }
    }
}

uint32_t JPEG_Encode_GetOutputSize(void)
{
    return jpeg_encode_ctx.output_size;
}

uint8_t* JPEG_Encode_GetOutputBuffer(void)
{
    return jpeg_encode_ctx.output_buffer;
}

void JPEG_Encode_Reset(void)
{
    jpeg_encode_ctx.state = JPEG_ENCODE_IDLE;
    jpeg_encode_ctx.output_size = 0;
    MCU_TotalNb = 0;
    MCU_BlockIndex = 0;
    Jpeg_HWEncodingEnd = 0;
    Output_Is_Paused = 0;
    Input_Is_Paused = 0;
    CurrentLine = 1;
    
    Jpeg_IN_BufferTab[0].State = JPEG_BUFFER_EMPTY;
    Jpeg_IN_BufferTab[0].DataBufferSize = 0;
    Jpeg_OUT_BufferTab.State = JPEG_BUFFER_EMPTY;
    Jpeg_OUT_BufferTab.DataBufferSize = 0;
    
    printf("[JPEG] Reset complete\r\n");
}

static void ReadRgbLines(uint8_t *pDataBuffer, uint32_t *BufferSize)
{
    uint32_t CurrentBlockLine = 1;
    *BufferSize = 0;

    while ((CurrentLine <= jpeg_encode_ctx.image_height) && (CurrentBlockLine <= MAX_INPUT_LINES)) {
        memcpy(pDataBuffer,
               jpeg_encode_ctx.input_buffer + (CurrentLine - 1) * jpeg_encode_ctx.image_width * BYTES_PER_PIXEL,
               jpeg_encode_ctx.image_width * BYTES_PER_PIXEL);
        pDataBuffer += jpeg_encode_ctx.image_width * BYTES_PER_PIXEL;
        *BufferSize += jpeg_encode_ctx.image_width * BYTES_PER_PIXEL;
        CurrentLine++;
        CurrentBlockLine++;
    }
}

void HAL_JPEG_GetDataCallback(JPEG_HandleTypeDef *hjpeg, uint32_t NbEncodedData)
{
    extern Gallery_ContextTypeDef gallery_ctx;
    
    if (gallery_ctx.state == GALLERY_STATE_VIEWING) {
        printf("[GALLERY] GetDataCallback: NbEncodedData=%lu\r\n", NbEncodedData);
        extern uint8_t Gallery_JPEG_InBuffer0[];
        extern uint8_t Gallery_JPEG_InBuffer1[];
        extern uint32_t Gallery_JPEG_IN_Read_Index;
        extern uint32_t Gallery_JPEG_IN_Write_Index;
        extern __IO uint32_t Gallery_Input_Is_Paused;
        extern uint32_t Gallery_Jpeg_HWDecodingEnd;
        
        typedef struct {
            uint8_t State;
            uint8_t *DataBuffer;
            uint32_t DataBufferSize;
        } JPEG_Data_BufferTypeDef;
        
        extern JPEG_Data_BufferTypeDef Gallery_JPEG_InBufferTab[2];
        
        if (NbEncodedData == Gallery_JPEG_InBufferTab[Gallery_JPEG_IN_Read_Index].DataBufferSize) {
            Gallery_JPEG_InBufferTab[Gallery_JPEG_IN_Read_Index].State = JPEG_BUFFER_EMPTY;
            Gallery_JPEG_InBufferTab[Gallery_JPEG_IN_Read_Index].DataBufferSize = 0;
            
            Gallery_JPEG_IN_Read_Index++;
            if (Gallery_JPEG_IN_Read_Index >= 2) {
                Gallery_JPEG_IN_Read_Index = 0;
            }
            
            if (Gallery_JPEG_InBufferTab[Gallery_JPEG_IN_Read_Index].State == JPEG_BUFFER_FULL) {
                HAL_JPEG_ConfigInputBuffer(hjpeg, Gallery_JPEG_InBufferTab[Gallery_JPEG_IN_Read_Index].DataBuffer,
                                           Gallery_JPEG_InBufferTab[Gallery_JPEG_IN_Read_Index].DataBufferSize);
                Gallery_Input_Is_Paused = 0;
                printf("[GALLERY] Switched to buffer %lu, size=%lu\r\n", 
                       Gallery_JPEG_IN_Read_Index, Gallery_JPEG_InBufferTab[Gallery_JPEG_IN_Read_Index].DataBufferSize);
            } else {
                HAL_JPEG_Pause(hjpeg, JPEG_PAUSE_RESUME_INPUT);
                Gallery_Input_Is_Paused = 1;
                printf("[GALLERY] Input paused, waiting for data\r\n");
            }
        } else {
            HAL_JPEG_ConfigInputBuffer(hjpeg, Gallery_JPEG_InBufferTab[Gallery_JPEG_IN_Read_Index].DataBuffer + NbEncodedData,
                                       Gallery_JPEG_InBufferTab[Gallery_JPEG_IN_Read_Index].DataBufferSize - NbEncodedData);
        }
    } else {
        if (NbEncodedData == Jpeg_IN_BufferTab[0].DataBufferSize) {
            Jpeg_IN_BufferTab[0].State = JPEG_BUFFER_EMPTY;
            Jpeg_IN_BufferTab[0].DataBufferSize = 0;
            HAL_JPEG_Pause(hjpeg, JPEG_PAUSE_RESUME_INPUT);
            Input_Is_Paused = 1;
        } else {
            HAL_JPEG_ConfigInputBuffer(hjpeg, Jpeg_IN_BufferTab[0].DataBuffer + NbEncodedData,
                                       Jpeg_IN_BufferTab[0].DataBufferSize - NbEncodedData);
        }
    }
}

void HAL_JPEG_DataReadyCallback(JPEG_HandleTypeDef *hjpeg, uint8_t *pDataOut, uint32_t OutDataLength)
{
    extern Gallery_ContextTypeDef gallery_ctx;
    extern uint32_t Gallery_FrameBufferAddress;
    
    if (gallery_ctx.state == GALLERY_STATE_VIEWING) {
        printf("[GALLERY] DataReadyCallback: OutDataLength=%lu, pDataOut=%p\r\n", OutDataLength, pDataOut);
        Gallery_FrameBufferAddress += OutDataLength;
        HAL_JPEG_ConfigOutputBuffer(hjpeg, (uint8_t*)Gallery_FrameBufferAddress, 64 * 1024);
    } else {
        Jpeg_OUT_BufferTab.State = JPEG_BUFFER_FULL;
        Jpeg_OUT_BufferTab.DataBufferSize = OutDataLength;
        HAL_JPEG_Pause(hjpeg, JPEG_PAUSE_RESUME_OUTPUT);
        Output_Is_Paused = 1;
        HAL_JPEG_ConfigOutputBuffer(hjpeg, Jpeg_OUT_BufferTab.DataBuffer, CHUNK_SIZE_OUT);
    }
}

void HAL_JPEG_ErrorCallback(JPEG_HandleTypeDef *hjpeg)
{
    extern Gallery_ContextTypeDef gallery_ctx;
    extern FIL *gallery_pFile;
    
    if (gallery_ctx.state == GALLERY_STATE_VIEWING) {
        printf("[GALLERY] JPEG decode error!\r\n");
        f_close(gallery_pFile);
        gallery_pFile = NULL;
        SD_Lock_Release(SD_LOCK_OWNER_GALLERY_VIEW);
        gallery_ctx.state = GALLERY_STATE_LIST;
    } else {
        printf("[JPEG] Error callback\r\n");
        jpeg_encode_ctx.state = JPEG_ENCODE_ERROR;
    }
}

void HAL_JPEG_EncodeCpltCallback(JPEG_HandleTypeDef *hjpeg)
{
    Jpeg_HWEncodingEnd = 1;
}

void HAL_JPEG_DecodeCpltCallback(JPEG_HandleTypeDef *hjpeg)
{
    extern Gallery_ContextTypeDef gallery_ctx;
    extern uint32_t Gallery_Jpeg_HWDecodingEnd;
    
    printf("[GALLERY] DecodeCpltCallback called, state=%d\r\n", gallery_ctx.state);
    
    if (gallery_ctx.state == GALLERY_STATE_VIEWING) {
        Gallery_Jpeg_HWDecodingEnd = 1;
        printf("[GALLERY] JPEG decode complete!\r\n");
    }
}

void HAL_JPEG_InfoReadyCallback(JPEG_HandleTypeDef *hjpeg, JPEG_ConfTypeDef *pInfo)
{
    extern Gallery_ContextTypeDef gallery_ctx;
    
    if (gallery_ctx.state == GALLERY_STATE_VIEWING) {
        printf("[GALLERY] JPEG Info: %dx%d, subsampling: %d, colorspace: %d\r\n",
               pInfo->ImageWidth, pInfo->ImageHeight, pInfo->ChromaSubsampling, pInfo->ColorSpace);
        
        gallery_ctx.image_width = pInfo->ImageWidth;
        gallery_ctx.image_height = pInfo->ImageHeight;
        gallery_ctx.chroma_sampling = pInfo->ChromaSubsampling;
    }
}
