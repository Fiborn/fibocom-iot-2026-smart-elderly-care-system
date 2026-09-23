#ifndef __JPEG_ENCODE_H
#define __JPEG_ENCODE_H

#include "stm32h7xx_hal.h"
#include "stm32h7xx_hal_jpeg.h"

#define JPEG_OUTPUT_BUFFER_SIZE    (256 * 1024)
#define JPEG_INPUT_CHUNK_SIZE      (640 * 16 * 2)
#define MAX_INPUT_WIDTH            640
#define MAX_INPUT_LINES            16
#define BYTES_PER_PIXEL            2

typedef enum {
    JPEG_ENCODE_IDLE,
    JPEG_ENCODE_RUNNING,
    JPEG_ENCODE_COMPLETE,
    JPEG_ENCODE_ERROR
} JPEG_EncodeStateTypeDef;

typedef struct {
    JPEG_HandleTypeDef hjpeg;
    JPEG_EncodeStateTypeDef state;
    uint8_t *input_buffer;
    uint8_t *output_buffer;
    uint32_t output_size;
    uint32_t image_width;
    uint32_t image_height;
    uint32_t mcu_total;
    uint32_t mcu_index;
    uint32_t current_line;
    __IO uint32_t encoding_end;
    __IO uint32_t output_paused;
    __IO uint32_t input_paused;
} JPEG_EncodeContextTypeDef;

extern JPEG_EncodeContextTypeDef jpeg_encode_ctx;
extern uint8_t *JPEG_Work_Buffer;
extern uint8_t *JPEG_Output_Buffer;

void JPEG_Encode_Init(void);
int32_t JPEG_Encode_Start(uint8_t *rgb565_buffer, uint32_t width, uint32_t height);
void JPEG_Encode_Process(void);
uint32_t JPEG_EncodeOutputHandler(void);
uint32_t JPEG_Encode_GetOutputSize(void);
uint8_t* JPEG_Encode_GetOutputBuffer(void);
void JPEG_Encode_Reset(void);

#endif
