/**
 ******************************************************************************
 * @file    video_ui.h
 * @brief   Video recording UI module for STM32H7
 ******************************************************************************
 */

#ifndef __VIDEO_UI_H__
#define __VIDEO_UI_H__

#include <stdint.h>
#include "video_rec.h"

/*============================================================================
 * UI Configuration
 *==========================================================================*/

/* LCD Display Configuration */
#define LCD_WIDTH   800
#define LCD_HEIGHT  480

/* Screen Partition */
#define MAIN_DISPLAY_WIDTH   640    /* Main display area: 0~640 */
#define SIDE_PANEL_WIDTH     160    /* Button area: 640~800 */
#define SIDE_PANEL_X         640

/* Button Configuration - Right side vertical layout */
#define UI_BTN_WIDTH         100
#define UI_BTN_HEIGHT        40
#define UI_BTN_X             650     /* Button X position */
#define UI_BTN_SPACING       10      /* Vertical spacing between buttons */

/* Button Y positions */
#define UI_BTN_AI_MODE_Y     0
#define UI_BTN_REC_Y         (UI_BTN_AI_MODE_Y + UI_BTN_HEIGHT + UI_BTN_SPACING)  /* 50 */
#define UI_BTN_STOP_Y        UI_BTN_REC_Y                                  /* Same as REC */
#define UI_BTN_BACK_AI_Y    (LCD_HEIGHT - UI_BTN_HEIGHT)                   /* 440 */

/* Status bar - Bottom of main display area */
#define STATUS_BAR_Y         (LCD_HEIGHT - 50)
#define STATUS_BAR_HEIGHT    50
#define STATUS_TIME_X        0
#define STATUS_SIZE_X        100
#define STATUS_FPS_X         220
#define STATUS_REC_X         320
#define STATUS_MODE_X        420

/* AI Mode Display Position */
#define AI_RGB_X             100
#define AI_RGB_Y             100
#define AI_RGB_SIZE          96
#define AI_FPS_X             100
#define AI_FPS_Y             50
#define AI_GESTURE_X         100
#define AI_GESTURE_Y         210

/*============================================================================
 * UI Button Definitions
 *==========================================================================*/

typedef enum {
    UI_BUTTON_AI_MODE = 0,
    UI_BUTTON_REC,
    UI_BUTTON_STOP,
    UI_BUTTON_BACK_AI,
    UI_BUTTON_MAX
} ui_button_id_t;

/* Button States */
typedef enum {
    UI_BUTTON_STATE_NORMAL = 0,
    UI_BUTTON_STATE_PRESSED,
    UI_BUTTON_STATE_HIGHLIGHTED,
    UI_BUTTON_STATE_DISABLED
} ui_button_state_t;

/* Button Definition */
typedef struct {
    uint16_t x;              /* X position */
    uint16_t y;              /* Y position */
    uint16_t width;          /* Button width */
    uint16_t height;         /* Button height */
    const char *text;        /* Button text */
    uint8_t visible;         /* Visibility flag */
    uint8_t enabled;         /* Enable flag (0:disabled, 1:normal, 2:highlighted) */
} ui_button_t;

/*============================================================================
 * UI Colors
 *==========================================================================*/

/* Standard Colors */
#define UI_COLOR_BLACK       0x0000
#define UI_COLOR_WHITE       0xFFFF
#define UI_COLOR_GRAY        0x8410
#define UI_COLOR_DARK_GRAY   0x4208
#define UI_COLOR_LIGHT_GRAY  0xC618
#define UI_COLOR_RED         0xF800
#define UI_COLOR_GREEN       0x07E0
#define UI_COLOR_BLUE        0x001F
#define UI_COLOR_YELLOW      0xFFE0
#define UI_COLOR_ORANGE      0xFC00
#define UI_COLOR_CYAN        0x07FF

/* Button Colors */
#define UI_COLOR_BTN_NORMAL     0x0948  /* Dark blue */
#define UI_COLOR_BTN_PRESSED    0x0000  /* Black */
#define UI_COLOR_BTN_HIGHLIGHT  0x2965  /* Light blue */
#define UI_COLOR_BTN_DISABLED   0x8410  /* Gray */
#define UI_COLOR_BTN_TEXT       0xFFFF  /* White text */
#define UI_COLOR_BTN_BORDER     0x0000  /* Black border */

/* Recording Colors */
#define UI_COLOR_REC_NORMAL   0xF800  /* Red */
#define UI_COLOR_REC_ACTIVE   0xFC00  /* Bright red/orange */

/*============================================================================
 * Mode Definitions
 *==========================================================================*/

typedef enum {
    UI_MODE_AI = 0,           /* AI recognition mode */
    UI_MODE_REC_PREVIEW      /* Recording preview mode */
} ui_mode_t;

/*============================================================================
 * UI API Functions
 *==========================================================================*/

/**
 * @brief   Initialize video UI module
 * @retval  0: Success
 */
int8_t VIDEO_UI_Init(void);

/**
 * @brief   Deinitialize video UI module
 * @retval  0: Success
 */
int8_t VIDEO_UI_DeInit(void);

/**
 * @brief   Update UI display (called periodically in main loop)
 */
void VIDEO_UI_Update(void);

/**
 * @brief   Draw complete UI based on current mode
 */
void VIDEO_UI_Draw(void);

/**
 * @brief   Draw all buttons
 */
void VIDEO_UI_DrawButtons(void);

/**
 * @brief   Draw single button
 * @param   button: Button ID
 */
void VIDEO_UI_DrawButton(ui_button_id_t button);

/**
 * @brief   Get button at specified coordinates
 * @param   x: Touch X coordinate
 * @param   y: Touch Y coordinate
 * @retval  Button ID or -1 if not on any button
 */
int8_t VIDEO_UI_GetButtonAt(uint16_t x, uint16_t y);

/**
 * @brief   Handle touch event
 * @param   x: Touch X coordinate
 * @param   y: Touch Y coordinate
 * @param   pressed: 1 if pressed, 0 if released
 */
void VIDEO_UI_HandleTouch(uint16_t x, uint16_t y, uint8_t pressed);

/**
 * @brief   Set current UI mode
 * @param   mode: UI_MODE_AI or UI_MODE_REC_PREVIEW
 */
void VIDEO_UI_SetMode(ui_mode_t mode);

/**
 * @brief   Get current UI mode
 * @retval  Current UI mode
 */
ui_mode_t VIDEO_UI_GetMode(void);

/**
 * @brief   Update FPS display
 * @param   fps: Current FPS value
 */
void VIDEO_UI_UpdateFPS(uint16_t fps);

/**
 * @brief   Update gesture result display
 * @param   gesture: Gesture number (0-11)
 */
void VIDEO_UI_UpdateGesture(int32_t gesture);

/**
 * @brief   Update recording status display
 * @param   time_ms: Recording time in milliseconds
 * @param   size_bytes: File size in bytes
 * @param   is_recording: 1 if currently recording
 */
void VIDEO_UI_UpdateRecordingStatus(uint32_t time_ms, uint32_t size_bytes, uint8_t is_recording);

/*============================================================================
 * External Callbacks (implemented in main.c)
 *==========================================================================*/

/**
 * @brief   Called when AI MODE button is pressed
 */
extern void VIDEO_UI_OnAIButton(void);

/**
 * @brief   Called when REC button is pressed
 */
extern void VIDEO_UI_OnRecButton(void);

/**
 * @brief   Called when STOP button is pressed
 */
extern void VIDEO_UI_OnStopButton(void);

/**
 * @brief   Called when Back to AI button is pressed
 */
extern void VIDEO_UI_OnBackAIButton(void);

#endif /* __VIDEO_UI_H__ */
