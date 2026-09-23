/**
 ******************************************************************************
 * @file    mode_switch.h
 * @brief   Mode switching between AI recognition and video recording
 ******************************************************************************
 */

#ifndef __MODE_SWITCH_H__
#define __MODE_SWITCH_H__

#include <stdint.h>

/*============================================================================
 * Mode Definitions
 *==========================================================================*/

typedef enum {
    MODE_AI = 0,        /* AI gesture recognition mode */
    MODE_VIDEO_REC,      /* Video recording mode */
    MODE_MAX
} system_mode_t;

/*============================================================================
 * Mode Configuration
 *==========================================================================*/

/* AI Mode Configuration */
#define AI_MODE_WIDTH      480     /* AI processing width */
#define AI_MODE_HEIGHT     480     /* AI processing height */
#define AI_MODE_FRAME_SIZE (AI_MODE_WIDTH * AI_MODE_HEIGHT * 2)  /* 460800 bytes */

/* Video Recording Mode Configuration */
#define REC_MODE_WIDTH     640     /* Recording width */
#define REC_MODE_HEIGHT    480     /* Recording height */
#define REC_MODE_FRAME_SIZE (REC_MODE_WIDTH * REC_MODE_HEIGHT * 2)  /* 614400 bytes */

/*============================================================================
 * Mode Switch API Functions
 *==========================================================================*/

/**
 * @brief   Initialize mode switch module
 * @retval  0: Success, -1: Error
 */
int8_t MODE_Init(void);

/**
 * @brief   Deinitialize mode switch module
 * @retval  0: Success, -1: Error
 */
int8_t MODE_DeInit(void);

/**
 * @brief   Get current system mode
 * @retval  Current mode (system_mode_t)
 */
system_mode_t MODE_GetCurrent(void);

/**
 * @brief   Switch to AI recognition mode
 * @retval  0: Success, -1: Error
 */
int8_t MODE_SwitchToAI(void);

/**
 * @brief   Switch to video recording mode
 * @retval  0: Success, -1: Error
 */
int8_t MODE_SwitchToVideo(void);

/**
 * @brief   Check if mode switch is in progress
 * @retval  1: Switching, 0: Idle
 */
uint8_t MODE_IsSwitching(void);

/**
 * @brief   Get frame buffer for current mode
 * @retval  Pointer to frame buffer
 */
uint8_t* MODE_GetFrameBuffer(void);

/**
 * @brief   Get frame size for current mode
 * @retval  Frame size in bytes
 */
uint32_t MODE_GetFrameSize(void);

/**
 * @brief   Get display dimensions for current mode
 * @param   width: Pointer to store width
 * @param   height: Pointer to store height
 */
void MODE_GetDisplaySize(uint16_t *width, uint16_t *height);

/**
 * @brief   Get preview position for current mode (for LCD display)
 * @param   x: Pointer to store X position
 * @param   y: Pointer to store Y position
 * @param   width: Pointer to store width
 * @param   height: Pointer to store height
 */
void MODE_GetPreviewRect(uint16_t *x, uint16_t *y, uint16_t *width, uint16_t *height);

/*============================================================================
 * OV5640 Configuration Functions (to be implemented in dcmi.c)
 *==========================================================================*/

/**
 * @brief   Configure OV5640 for AI mode (480x480)
 * @retval  0: Success, -1: Error
 */
extern int8_t OV5640_SetMode_AI(void);

/**
 * @brief   Configure OV5640 for video recording mode (640x480)
 * @retval  0: Success, -1: Error
 */
extern int8_t OV5640_SetMode_Video(void);

/**
 * @brief   Configure DCMI for AI mode (with cropping)
 * @retval  0: Success, -1: Error
 */
extern int8_t DCMI_SetMode_AI(void);

/**
 * @brief   Configure DCMI for video recording mode (no cropping)
 * @retval  0: Success, -1: Error
 */
extern int8_t DCMI_SetMode_Video(void);

/*============================================================================
 * External Variables
 *==========================================================================*/

extern volatile uint8_t OV5640_FrameState;

#endif /* __MODE_SWITCH_H__ */
