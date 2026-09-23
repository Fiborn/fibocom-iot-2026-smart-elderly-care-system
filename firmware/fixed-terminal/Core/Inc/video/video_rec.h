/**
 ******************************************************************************
 * @file    video_rec.h
 * @brief   Video recording core module for STM32H7
 ******************************************************************************
 */

#ifndef __VIDEO_REC_H__
#define __VIDEO_REC_H__

#include <stdint.h>
#include "video_avi.h"

/*============================================================================
 * Video Recording Configuration
 *==========================================================================*/

/* Frame buffer configuration for video recording mode */
#define VIDEO_REC_WIDTH      640     /* Recording width */
#define VIDEO_REC_HEIGHT     480     /* Recording height */
#define VIDEO_REC_FPS        30      /* Frames per second */
#define VIDEO_REC_FRAME_SIZE (VIDEO_REC_WIDTH * VIDEO_REC_HEIGHT * 2)  /* 614400 bytes */

/* Double buffer configuration */
#define VIDEO_REC_NUM_BUFFERS    2   /* Number of frame buffers */

/* Task configuration */
#define VIDEO_REC_TASK_STACK_SIZE 4096
#define VIDEO_REC_TASK_PRIORITY  10   /* Higher priority than main task */

/*============================================================================
 * Video Recording State Machine
 *==========================================================================*/

typedef enum {
    VIDEO_REC_STATE_IDLE = 0,    /* Not recording */
    VIDEO_REC_STATE_READY,       /* Ready to start recording */
    VIDEO_REC_STATE_RECORDING,   /* Currently recording */
    VIDEO_REC_STATE_STOPPING,    /* Stop requested, finishing */
    VIDEO_REC_STATE_ERROR        /* Error occurred */
} video_rec_state_t;

/*============================================================================
 * Video Recording Handle
 *==========================================================================*/

typedef struct {
    /* Recording state */
    volatile video_rec_state_t state;
    
    /* AVI file handle */
    avi_handle_t avi;
    
    /* Frame buffers (DMA accessible) */
    uint8_t *frame_buffers[VIDEO_REC_NUM_BUFFERS];
    volatile uint8_t current_buffer;
    volatile uint8_t write_buffer;
    volatile uint8_t buffer_full;
    
    /* Statistics */
    volatile uint32_t frames_recorded;
    volatile uint32_t frames_dropped;
    volatile uint32_t bytes_written;
    volatile uint32_t recording_time_ms;
    
    /* Timing */
    volatile uint32_t frame_interval_ms;
    volatile uint32_t last_frame_time_ms;
    volatile uint32_t frame_counter;
    
    /* Error handling */
    volatile int8_t last_error;
    
    /* Directory path */
    char video_path[64];
    
} video_rec_handle_t;

/*============================================================================
 * Video Recording API Functions
 *==========================================================================*/

/**
 * @brief   Initialize video recording module
 * @retval  0: Success, -1: Error
 */
int8_t VIDEO_REC_Init(void);

/**
 * @brief   Deinitialize video recording module
 * @retval  0: Success, -1: Error
 */
int8_t VIDEO_REC_DeInit(void);

/**
 * @brief   Start video recording
 * @retval  0: Success, -1: Error
 */
int8_t VIDEO_REC_Start(void);

/**
 * @brief   Stop video recording
 * @retval  0: Success, -1: Error
 */
int8_t VIDEO_REC_Stop(void);

/**
 * @brief   Get current recording state
 * @retval  Current state (video_rec_state_t)
 */
video_rec_state_t VIDEO_REC_GetState(void);

/**
 * @brief   Check if currently recording
 * @retval  1: Recording, 0: Not recording
 */
uint8_t VIDEO_REC_IsRecording(void);

/**
 * @brief   Get number of frames recorded
 * @retval  Frame count
 */
uint32_t VIDEO_REC_GetFrameCount(void);

/**
 * @brief   Get recording time in milliseconds
 * @retval  Recording time in ms
 */
uint32_t VIDEO_REC_GetRecordingTime(void);

/**
 * @brief   Get current file size in bytes
 * @retval  File size in bytes
 */
uint32_t VIDEO_REC_GetFileSize(void);

/**
 * @brief   Get last error code
 * @retval  Last error (-1: No error, 0: Write error, 1: File error, 2: Memory error)
 */
int8_t VIDEO_REC_GetLastError(void);

/**
 * @brief   Process a frame from DMA (called from frame callback)
 * @param   frame_data: Pointer to frame data
 * @param   frame_size: Size of frame data in bytes
 * @retval  None
 */
void VIDEO_REC_ProcessFrame(const uint8_t *frame_data, uint32_t frame_size);

/**
 * @brief   Get frame buffer for DMA destination
 * @param   buffer_index: Buffer index (0 or 1)
 * @retval  Pointer to frame buffer
 */
uint8_t* VIDEO_REC_GetFrameBuffer(uint8_t buffer_index);

/**
 * @brief   Switch to next frame buffer (called from DMA interrupt)
 */
void VIDEO_REC_SwitchBuffer(void);

/**
 * @brief   Get current active frame buffer index
 * @retval  Current buffer index (0 or 1)
 */
uint8_t VIDEO_REC_GetCurrentBuffer(void);

/**
 * @brief   Create video directory if not exists
 * @retval  0: Success, -1: Error
 */
int8_t VIDEO_REC_CreateDirectory(void);

/**
 * @brief   Generate filename with timestamp
 * @param   filename: Output buffer for filename
 * @param   max_len: Maximum length of buffer
 * @retval  0: Success, -1: Error
 */
int8_t VIDEO_REC_GenerateFilename(char *filename, uint16_t max_len);

/*============================================================================
 * External Callbacks (to be implemented in main.c)
 *==========================================================================*/

/**
 * @brief   Called when recording starts
 */
void VIDEO_REC_OnStart(void);

/**
 * @brief   Called when recording stops
 * @param   total_frames: Total frames recorded
 * @param   file_size: Final file size in bytes
 */
void VIDEO_REC_OnStop(uint32_t total_frames, uint32_t file_size);

/**
 * @brief   Called on recording error
 * @param   error_code: Error code
 */
void VIDEO_REC_OnError(int8_t error_code);

/*============================================================================
 * External Variables (defined in main.c)
 *==========================================================================*/

extern volatile uint8_t OV5640_FrameState;

#endif /* __VIDEO_REC_H__ */
