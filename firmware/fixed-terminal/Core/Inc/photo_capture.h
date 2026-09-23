#ifndef __PHOTO_CAPTURE_H
#define __PHOTO_CAPTURE_H

#include "stm32h7xx_hal.h"
#include "fatfs.h"
#include "dcmi.h"

#define PHOTO_CAPTURE_WIDTH         640
#define PHOTO_CAPTURE_HEIGHT        480
#define PHOTO_CAPTURE_BUFFER_SIZE   (PHOTO_CAPTURE_WIDTH * PHOTO_CAPTURE_HEIGHT * 2)
#define PHOTO_CAPTURE_BUFFER_ADDR   (AI_Quant_Buffer + AI_QUANT_BUFFER_SIZE + 0x1000)

typedef enum {
    PHOTO_STATE_IDLE,
    PHOTO_STATE_WAIT_FRAME,
    PHOTO_STATE_ENCODING,
    PHOTO_STATE_WRITING,
    PHOTO_STATE_COMPLETE
} Photo_CaptureStateTypeDef;

typedef struct {
    volatile Photo_CaptureStateTypeDef state;
    uint8_t *frame_buffer;
    uint32_t photo_count;
    char filename[32];
    FIL jpeg_file;
} Photo_CaptureContextTypeDef;

extern Photo_CaptureContextTypeDef photo_capture_ctx;
extern uint8_t *Photo_Capture_Buffer;

void PHOTO_CAPTURE_Init(void);
void PHOTO_CAPTURE_Request(void);
void PHOTO_CAPTURE_OnFrameComplete(void);
void PHOTO_CAPTURE_Process(void);
uint8_t PHOTO_CAPTURE_IsBusy(void);

#endif
