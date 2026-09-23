#ifndef __GALLERY_H
#define __GALLERY_H

#include "stm32h7xx_hal.h"
#include "fatfs.h"
#include "ltdc.h"

#define GALLERY_MAX_FILES       100
#define GALLERY_FILENAME_LEN    32
#define GALLERY_DECODE_BUFFER_SIZE  (640 * 480 * 2)

typedef enum {
    GALLERY_STATE_LIST,
    GALLERY_STATE_VIEWING
} Gallery_StateTypeDef;

typedef enum {
    GALLERY_MODE_NONE,
    GALLERY_MODE_AI,
    GALLERY_MODE_PHOTO
} Gallery_PrevModeTypeDef;

typedef struct {
    char filename[GALLERY_FILENAME_LEN];
    uint32_t filesize;
} Gallery_FileInfoTypeDef;

typedef struct {
    Gallery_StateTypeDef state;
    Gallery_PrevModeTypeDef prev_mode;
    Gallery_FileInfoTypeDef files[GALLERY_MAX_FILES];
    uint32_t file_count;
    int32_t scroll_offset;
    uint32_t current_page;
    uint32_t selected_file;
    uint8_t *decode_buffer;
    uint32_t image_width;
    uint32_t image_height;
    uint32_t chroma_sampling;
    uint8_t decode_done;
    uint8_t need_redraw;
    uint8_t file_ended;
    uint32_t total_bytes_read;
} Gallery_ContextTypeDef;

extern Gallery_ContextTypeDef gallery_ctx;
extern FIL *gallery_pFile;

void GALLERY_Init(void);
void GALLERY_Enter(uint8_t prev_mode);
void GALLERY_Exit(void);
void GALLERY_Process(void);
void GALLERY_Render(void);
void GALLERY_HandleTouchEvent(uint16_t x, uint16_t y);
void GALLERY_HandleTouchRelease(void);

#endif