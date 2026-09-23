/**
 ******************************************************************************
 * @file    video_rec.c
 * @brief   Video recording core module for STM32H7
 ******************************************************************************
 */

#include "video/video_rec.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ff.h"
#include "main.h"

/*============================================================================
 * Private Variables
 *==========================================================================*/

static video_rec_handle_t g_video_rec = {
    .state = VIDEO_REC_STATE_IDLE,
    .frames_recorded = 0,
    .frames_dropped = 0,
    .bytes_written = 0,
    .recording_time_ms = 0,
    .frame_interval_ms = 1000 / VIDEO_REC_FPS,
    .last_frame_time_ms = 0,
    .frame_counter = 0,
    .last_error = -1,
    .current_buffer = 0,
    .write_buffer = 0,
    .buffer_full = 0,
};

/*============================================================================
 * Private Function Prototypes
 *==========================================================================*/

static int8_t VIDEO_REC_WriteFrame(const uint8_t *frame_data);
static void VIDEO_REC_UpdateStats(void);

/*============================================================================
 * Video Recording API Functions
 *==========================================================================*/

/**
 * @brief   Initialize video recording module
 */
int8_t VIDEO_REC_Init(void)
{
    printf("[VIDEO_REC] Initializing video recording module...\r\n");

    /* Initialize handle */
    g_video_rec.state = VIDEO_REC_STATE_IDLE;
    g_video_rec.frames_recorded = 0;
    g_video_rec.frames_dropped = 0;
    g_video_rec.bytes_written = 0;
    g_video_rec.recording_time_ms = 0;
    g_video_rec.frame_counter = 0;
    g_video_rec.last_error = -1;
    g_video_rec.current_buffer = 0;
    g_video_rec.write_buffer = 0;
    g_video_rec.buffer_full = 0;

    /* Set video directory path */
    strcpy(g_video_rec.video_path, "0:/VIDEO");

    /* Note: Frame buffers are not allocated here.
     * We use Frame_Buffer (SDRAM) directly for video recording.
     * The centered 640x480 area is obtained via MODE_GetFrameBuffer(). */

    printf("[VIDEO_REC] Initialization complete\r\n");
    printf("[VIDEO_REC]   Frame size: %d bytes\r\n", VIDEO_REC_FRAME_SIZE);
    printf("[VIDEO_REC]   FPS: %d\r\n", VIDEO_REC_FPS);
    printf("[VIDEO_REC]   Max file size: %d MB\r\n", VIDEO_MAX_FILE_SIZE / (1024 * 1024));

    return 0;
}

/**
 * @brief   Deinitialize video recording module
 */
int8_t VIDEO_REC_DeInit(void)
{
    /* Stop if recording */
    if (g_video_rec.state == VIDEO_REC_STATE_RECORDING) {
        VIDEO_REC_Stop();
    }

    g_video_rec.state = VIDEO_REC_STATE_IDLE;

    printf("[VIDEO_REC] Deinitialization complete\r\n");

    return 0;
}

/**
 * @brief   Start video recording
 */
int8_t VIDEO_REC_Start(void)
{
    char filename[64];
    int8_t result;
    
    if (g_video_rec.state == VIDEO_REC_STATE_RECORDING) {
        printf("[VIDEO_REC] WARNING: Already recording\r\n");
        return 0;
    }
    
    /* Create directory if not exists */
    if (VIDEO_REC_CreateDirectory() != 0) {
        printf("[VIDEO_REC] ERROR: Failed to create video directory\r\n");
        g_video_rec.last_error = 1;
        g_video_rec.state = VIDEO_REC_STATE_ERROR;
        return -1;
    }
    
    /* Generate filename */
    if (VIDEO_REC_GenerateFilename(filename, sizeof(filename)) != 0) {
        printf("[VIDEO_REC] ERROR: Failed to generate filename\r\n");
        g_video_rec.last_error = 1;
        g_video_rec.state = VIDEO_REC_STATE_ERROR;
        return -1;
    }
    
    printf("[VIDEO_REC] Starting recording...\r\n");
    printf("[VIDEO_REC]   Filename: %s\r\n", filename);
    
    /* Initialize AVI file */
    result = AVI_Init(&(g_video_rec.avi), filename,
                      VIDEO_REC_WIDTH, VIDEO_REC_HEIGHT, VIDEO_REC_FPS,
                      VIDEO_MAX_FILE_SIZE);
    
    if (result != 0) {
        printf("[VIDEO_REC] ERROR: Failed to create AVI file\r\n");
        g_video_rec.last_error = 1;
        g_video_rec.state = VIDEO_REC_STATE_ERROR;
        return -1;
    }
    
    /* Reset statistics */
    g_video_rec.frames_recorded = 0;
    g_video_rec.frames_dropped = 0;
    g_video_rec.bytes_written = 0;
    g_video_rec.recording_time_ms = 0;
    g_video_rec.frame_counter = 0;
    g_video_rec.buffer_full = 0;
    
    /* Update state */
    g_video_rec.state = VIDEO_REC_STATE_RECORDING;
    
    /* Get current time for frame timing */
    g_video_rec.last_frame_time_ms = HAL_GetTick();
    
    printf("[VIDEO_REC] Recording started\r\n");
    
    /* Call callback */
    VIDEO_REC_OnStart();
    
    return 0;
}

/**
 * @brief   Stop video recording
 */
int8_t VIDEO_REC_Stop(void)
{
    uint32_t total_frames;
    uint32_t file_size;
    
    if (g_video_rec.state != VIDEO_REC_STATE_RECORDING) {
        printf("[VIDEO_REC] WARNING: Not recording\r\n");
        return 0;
    }
    
    printf("[VIDEO_REC] Stopping recording...\r\n");
    printf("[VIDEO_REC]   Frames recorded: %lu\r\n", g_video_rec.frames_recorded);
    printf("[VIDEO_REC]   Frames dropped: %lu\r\n", g_video_rec.frames_dropped);
    printf("[VIDEO_REC]   File size: %lu bytes\r\n", g_video_rec.bytes_written);
    
    /* Save statistics before closing */
    total_frames = g_video_rec.frames_recorded;
    file_size = g_video_rec.bytes_written;
    
    /* Close AVI file */
    if (AVI_Close(&(g_video_rec.avi)) != 0) {
        printf("[VIDEO_REC] ERROR: Failed to close AVI file\r\n");
        g_video_rec.last_error = 1;
        g_video_rec.state = VIDEO_REC_STATE_ERROR;
        return -1;
    }
    
    /* Update state */
    g_video_rec.state = VIDEO_REC_STATE_IDLE;
    
    printf("[VIDEO_REC] Recording stopped successfully\r\n");
    
    /* Call callback */
    VIDEO_REC_OnStop(total_frames, file_size);
    
    return 0;
}

/**
 * @brief   Get current recording state
 */
video_rec_state_t VIDEO_REC_GetState(void)
{
    return g_video_rec.state;
}

/**
 * @brief   Check if currently recording
 */
uint8_t VIDEO_REC_IsRecording(void)
{
    return (g_video_rec.state == VIDEO_REC_STATE_RECORDING) ? 1 : 0;
}

/**
 * @brief   Get number of frames recorded
 */
uint32_t VIDEO_REC_GetFrameCount(void)
{
    return g_video_rec.frames_recorded;
}

/**
 * @brief   Get recording time in milliseconds
 */
uint32_t VIDEO_REC_GetRecordingTime(void)
{
    return g_video_rec.recording_time_ms;
}

/**
 * @brief   Get current file size in bytes
 */
uint32_t VIDEO_REC_GetFileSize(void)
{
    return g_video_rec.bytes_written;
}

/**
 * @brief   Get last error code
 */
int8_t VIDEO_REC_GetLastError(void)
{
    return g_video_rec.last_error;
}

/**
 * @brief   Process a frame from DMA
 * @note    Frame data is read from Frame_Buffer (centered 640x480 area)
 */
void VIDEO_REC_ProcessFrame(const uint8_t *frame_data, uint32_t frame_size)
{
    if (g_video_rec.state != VIDEO_REC_STATE_RECORDING) {
        return;
    }

    /* Validate frame size */
    if (frame_size != VIDEO_REC_FRAME_SIZE) {
        g_video_rec.frames_dropped++;
        return;
    }

    /* Write frame to AVI file */
    if (VIDEO_REC_WriteFrame(frame_data) != 0) {
        /* Error occurred, stop recording */
        printf("[VIDEO_REC] ERROR: Failed to write frame, stopping recording\r\n");
        VIDEO_REC_Stop();
        return;
    }

    /* Update statistics */
    VIDEO_REC_UpdateStats();

    /* Check if max file size reached */
    if (AVI_IsMaxSizeReached(&(g_video_rec.avi))) {
        printf("[VIDEO_REC] Max file size reached, auto-stopping\r\n");
        VIDEO_REC_Stop();
    }
}

/**
 * @brief   Get frame buffer for DMA destination
 */
uint8_t* VIDEO_REC_GetFrameBuffer(uint8_t buffer_index)
{
    if (buffer_index >= VIDEO_REC_NUM_BUFFERS) {
        return NULL;
    }
    return g_video_rec.frame_buffers[buffer_index];
}

/**
 * @brief   Switch to next frame buffer
 */
void VIDEO_REC_SwitchBuffer(void)
{
    g_video_rec.write_buffer = !g_video_rec.current_buffer;
    g_video_rec.buffer_full = 1;
}

/**
 * @brief   Get current active frame buffer index
 */
uint8_t VIDEO_REC_GetCurrentBuffer(void)
{
    return g_video_rec.current_buffer;
}

/**
 * @brief   Create video directory if not exists
 */
int8_t VIDEO_REC_CreateDirectory(void)
{
    FRESULT res;
    DIR dir;
    
    /* Try to open directory */
    res = f_opendir(&dir, g_video_rec.video_path);
    if (res == FR_OK) {
        /* Directory exists */
        f_closedir(&dir);
        return 0;
    }
    
    /* Create directory */
    res = f_mkdir(g_video_rec.video_path);
    if (res != FR_OK) {
        printf("[VIDEO_REC] ERROR: Failed to create directory: %d\r\n", res);
        return -1;
    }
    
    printf("[VIDEO_REC] Created directory: %s\r\n", g_video_rec.video_path);
    
    return 0;
}

/**
 * @brief   Generate filename with timestamp
 */
int8_t VIDEO_REC_GenerateFilename(char *filename, uint16_t max_len)
{
    if (filename == NULL || max_len < 32) {
        return -1;
    }
    
    uint32_t tick = HAL_GetTick();
    
    snprintf(filename, max_len, "0:/VIDEO/VID_%06lu.avi", tick);
    
    return 0;
}

/*============================================================================
 * External Callbacks (Weak implementations, can be overridden)
 *==========================================================================*/

__weak void VIDEO_REC_OnStart(void)
{
    /* User can override this function */
}

__weak void VIDEO_REC_OnStop(uint32_t total_frames, uint32_t file_size)
{
    /* User can override this function */
}

__weak void VIDEO_REC_OnError(int8_t error_code)
{
    /* User can override this function */
}

/*============================================================================
 * Private Functions
 *==========================================================================*/

/**
 * @brief   Write frame to AVI file
 */
static int8_t VIDEO_REC_WriteFrame(const uint8_t *frame_data)
{
    int8_t result;
    
    if (frame_data == NULL) {
        return -1;
    }
    
    /* Write frame to AVI */
    result = AVI_WriteFrame(&(g_video_rec.avi), frame_data);
    
    if (result == 0) {
        /* Success */
        g_video_rec.frames_recorded++;
        g_video_rec.bytes_written = AVI_GetFileSize(&(g_video_rec.avi));
    } else if (result == 1) {
        /* Max size reached */
        g_video_rec.frames_recorded++;
        g_video_rec.bytes_written = AVI_GetFileSize(&(g_video_rec.avi));
    } else {
        /* Error */
        g_video_rec.last_error = 0;
        return -1;
    }
    
    return 0;
}

/**
 * @brief   Update recording statistics
 */
static void VIDEO_REC_UpdateStats(void)
{
    uint32_t current_time;
    
    g_video_rec.frame_counter++;
    
    /* Calculate recording time */
    current_time = HAL_GetTick();
    g_video_rec.recording_time_ms = current_time;
    
    /* Calculate actual frame rate (every 30 frames) */
    if (g_video_rec.frame_counter % 30 == 0) {
        uint32_t elapsed = current_time - g_video_rec.last_frame_time_ms;
        if (elapsed > 0) {
            g_video_rec.frame_interval_ms = elapsed / 30;
        }
        g_video_rec.last_frame_time_ms = current_time;
    }
}
