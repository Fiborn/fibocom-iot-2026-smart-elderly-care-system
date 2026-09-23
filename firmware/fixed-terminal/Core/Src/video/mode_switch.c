/**
 ******************************************************************************
 * @file    mode_switch.c
 * @brief   Mode switching between AI recognition and video recording
 ******************************************************************************
 */

#include "mode_switch.h"
#include <stdio.h>
#include "main.h"
#include "dcmi.h"
#include "video_rec.h"

/*============================================================================
 * Private Variables
 *==========================================================================*/

static volatile system_mode_t g_current_mode = MODE_AI;
static volatile uint8_t g_switching = 0;

/*============================================================================
 * Mode Switch API Functions
 *==========================================================================*/

/**
 * @brief   Initialize mode switch module
 */
int8_t MODE_Init(void)
{
    printf("[MODE] Initializing mode switch module...\r\n");

    g_current_mode = MODE_AI;
    g_switching = 0;

    /* Note: OV5640 and DCMI are already initialized in DCMI_OV5640_Init()
     * with default AI mode configuration (480x480 with cropping) */

    printf("[MODE] Initialized in AI mode\r\n");
    printf("[MODE]   Resolution: %dx%d\r\n", AI_MODE_WIDTH, AI_MODE_HEIGHT);

    return 0;
}

/**
 * @brief   Deinitialize mode switch module
 */
int8_t MODE_DeInit(void)
{
    g_current_mode = MODE_AI;
    g_switching = 0;
    
    printf("[MODE] Mode switch deinitialized\r\n");
    
    return 0;
}

/**
 * @brief   Get current system mode
 */
system_mode_t MODE_GetCurrent(void)
{
    return g_current_mode;
}

/**
 * @brief   Switch to AI recognition mode
 */
int8_t MODE_SwitchToAI(void)
{
    if (g_current_mode == MODE_AI && !g_switching) {
        return 0;  /* Already in AI mode */
    }
    
    if (g_switching) {
        printf("[MODE] WARNING: Mode switch already in progress\r\n");
        return -1;
    }
    
    printf("[MODE] Switching to AI mode...\r\n");
    g_switching = 1;
    
    /* Stop video recording if active */
    if (VIDEO_REC_IsRecording()) {
        VIDEO_REC_Stop();
    }
    
    /* Reconfigure OV5640 */
    if (OV5640_SetMode_AI() != 0) {
        printf("[MODE] ERROR: Failed to set OV5640 AI mode\r\n");
        g_switching = 0;
        return -1;
    }
    
    /* Reconfigure DCMI */
    if (DCMI_SetMode_AI() != 0) {
        printf("[MODE] ERROR: Failed to set DCMI AI mode\r\n");
        g_switching = 0;
        return -1;
    }
    
    /* Restart DMA if needed */
    /* Note: DMA restart should be handled in DCMI configuration */
    
    g_current_mode = MODE_AI;
    g_switching = 0;
    
    printf("[MODE] Switched to AI mode\r\n");
    printf("[MODE]   Resolution: %dx%d\r\n", AI_MODE_WIDTH, AI_MODE_HEIGHT);
    
    return 0;
}

/**
 * @brief   Switch to video recording mode
 */
int8_t MODE_SwitchToVideo(void)
{
    if (g_current_mode == MODE_VIDEO_REC && !g_switching) {
        return 0;  /* Already in video mode */
    }
    
    if (g_switching) {
        printf("[MODE] WARNING: Mode switch already in progress\r\n");
        return -1;
    }
    
    printf("[MODE] Switching to video recording mode...\r\n");
    g_switching = 1;
    
    /* Stop video recording if active */
    if (VIDEO_REC_IsRecording()) {
        VIDEO_REC_Stop();
    }
    
    /* Reconfigure OV5640 */
    if (OV5640_SetMode_Video() != 0) {
        printf("[MODE] ERROR: Failed to set OV5640 video mode\r\n");
        g_switching = 0;
        return -1;
    }
    
    /* Reconfigure DCMI */
    if (DCMI_SetMode_Video() != 0) {
        printf("[MODE] ERROR: Failed to set DCMI video mode\r\n");
        g_switching = 0;
        return -1;
    }
    
    /* Restart DMA if needed */
    /* Note: DMA restart should be handled in DCMI configuration */
    
    g_current_mode = MODE_VIDEO_REC;
    g_switching = 0;
    
    printf("[MODE] Switched to video recording mode\r\n");
    printf("[MODE]   Resolution: %dx%d\r\n", REC_MODE_WIDTH, REC_MODE_HEIGHT);
    
    return 0;
}

/**
 * @brief   Check if mode switch is in progress
 */
uint8_t MODE_IsSwitching(void)
{
    return g_switching;
}

/**
 * @brief   Get frame buffer for current mode
 * @retval  Pointer to frame buffer
 * @note    AI mode: Returns Camera_Buffer (0x24000000)
 *          Video mode: Returns pointer to 640x480 area in Frame_Buffer (centered)
 */
uint8_t* MODE_GetFrameBuffer(void)
{
    if (g_current_mode == MODE_AI) {
        /* AI mode: Return Camera_Buffer directly */
        return (uint8_t*)Camera_Buffer;
    } else {
        /* Video mode: Return centered 640x480 area in Frame_Buffer */
        /* Frame_Buffer is 800x480, we need the centered 640x480 area */
        uint16_t x_offset = (800 - 640) / 2;  /* Center horizontally */
        uint16_t y_offset = (480 - 480) / 2;  /* No vertical offset */
        return (uint8_t*)(Frame_Buffer + (y_offset * 800 + x_offset) * 2);
    }
}

/**
 * @brief   Get frame size for current mode
 */
uint32_t MODE_GetFrameSize(void)
{
    if (g_current_mode == MODE_AI) {
        return AI_MODE_FRAME_SIZE;
    } else {
        return REC_MODE_FRAME_SIZE;
    }
}

/**
 * @brief   Get display dimensions for current mode
 */
void MODE_GetDisplaySize(uint16_t *width, uint16_t *height)
{
    if (g_current_mode == MODE_AI) {
        *width = AI_MODE_WIDTH;
        *height = AI_MODE_HEIGHT;
    } else {
        *width = REC_MODE_WIDTH;
        *height = REC_MODE_HEIGHT;
    }
}

/**
 * @brief   Get preview position for current mode
 */
void MODE_GetPreviewRect(uint16_t *x, uint16_t *y, uint16_t *width, uint16_t *height)
{
    uint16_t w, h;
    
    MODE_GetDisplaySize(&w, &h);
    
    /* Center the preview on LCD (800x480) */
    *x = (800 - w) / 2;
    *y = (480 - h) / 2;
    *width = w;
    *height = h;
    
    /* For AI mode, we display 96x96 processed image in the corner */
    /* This function returns the raw camera preview area */
}
