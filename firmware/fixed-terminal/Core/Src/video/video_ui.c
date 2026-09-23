/**
 ******************************************************************************
 * @file    video_ui.c
 * @brief   Video recording UI module for STM32H7
 ******************************************************************************
 */

#include "video/video_ui.h"
#include <stdio.h>
#include <string.h>
#include "ltdc.h"
#include "touch_800x480.h"

/*============================================================================
 * Private Variables
 *==========================================================================*/

/* Button definitions - Right side vertical layout */
static ui_button_t g_buttons[UI_BUTTON_MAX] = {
    [UI_BUTTON_AI_MODE] = {
        .x = UI_BTN_X,
        .y = UI_BTN_AI_MODE_Y,
        .width = UI_BTN_WIDTH,
        .height = UI_BTN_HEIGHT,
        .text = "AI MODE",
        .visible = 1,
        .enabled = 2  /* Highlighted by default in AI mode */
    },
    [UI_BUTTON_REC] = {
        .x = UI_BTN_X,
        .y = UI_BTN_REC_Y,
        .width = UI_BTN_WIDTH,
        .height = UI_BTN_HEIGHT,
        .text = "REC",
        .visible = 1,
        .enabled = 1
    },
    [UI_BUTTON_STOP] = {
        .x = UI_BTN_X,
        .y = UI_BTN_STOP_Y,
        .width = UI_BTN_WIDTH,
        .height = UI_BTN_HEIGHT,
        .text = "STOP",
        .visible = 0,
        .enabled = 0
    },
    [UI_BUTTON_BACK_AI] = {
        .x = UI_BTN_X,
        .y = UI_BTN_BACK_AI_Y,
        .width = UI_BTN_WIDTH,
        .height = UI_BTN_HEIGHT,
        .text = "BACK AI",
        .visible = 0,
        .enabled = 0
    }
};

/* Touch state */
static volatile uint8_t g_touch_pressed = 0;
static volatile int8_t g_pressed_button = -1;

/* Current UI mode */
static volatile ui_mode_t g_current_mode = UI_MODE_AI;

/* Display data */
static volatile uint16_t g_current_fps = 0;
static volatile int32_t g_current_gesture = -1;
static volatile uint32_t g_recording_time_ms = 0;
static volatile uint32_t g_file_size_bytes = 0;
static volatile uint8_t g_is_recording = 0;
static volatile uint8_t g_recording_blink = 0;

/*============================================================================
 * Private Function Prototypes
 *==========================================================================*/

static void VIDEO_UI_DrawButtonFrame(ui_button_t *btn, uint16_t color);
static void VIDEO_UI_DrawButtonText(ui_button_t *btn, uint16_t color);
static void VIDEO_UI_ClearMainDisplay(void);
static void VIDEO_UI_DrawStatusBar(void);
static void VIDEO_UI_UpdateBlink(void);
static void VIDEO_UI_DrawSidePanel(void);

/*============================================================================
 * UI API Functions
 *==========================================================================*/

/**
 * @brief   Initialize video UI module
 */
int8_t VIDEO_UI_Init(void)
{
    printf("[VIDEO_UI] Initializing UI module...\r\n");

    g_current_mode = UI_MODE_AI;
    g_current_fps = 0;
    g_current_gesture = -1;
    g_recording_time_ms = 0;
    g_file_size_bytes = 0;
    g_is_recording = 0;

    /* Initial draw */
    VIDEO_UI_Draw();

    printf("[VIDEO_UI] UI module initialized\r\n");
    return 0;
}

/**
 * @brief   Deinitialize video UI module
 */
int8_t VIDEO_UI_DeInit(void)
{
    printf("[VIDEO_UI] UI module deinitialized\r\n");
    return 0;
}

/**
 * @brief   Update UI display
 */
void VIDEO_UI_Update(void)
{
    /* Only update blink state (has internal timing for 500ms interval) */
    VIDEO_UI_UpdateBlink();
}

/**
 * @brief   Draw complete UI based on current mode
 */
void VIDEO_UI_Draw(void)
{
    /* Clear main display area */
    VIDEO_UI_ClearMainDisplay();

    /* Draw side panel (buttons) */
    VIDEO_UI_DrawSidePanel();

    /* Draw mode-specific elements */
    if (g_current_mode == UI_MODE_AI) {
        /* AI mode: draw gesture text position indicator */
        LCD_SetColor(UI_COLOR_WHITE);
        LCD_DisplayString(AI_FPS_X, AI_FPS_Y, "FPS: --", LCD_BackColor);
        LCD_DisplayString(AI_GESTURE_X, AI_GESTURE_Y, "Gesture: --", LCD_BackColor);
    } else {
        /* Recording preview mode: draw status bar */
        VIDEO_UI_DrawStatusBar();
    }
}

/**
 * @brief   Draw side panel (all buttons)
 */
void VIDEO_UI_DrawSidePanel(void)
{
    uint8_t i;
    for (i = 0; i < UI_BUTTON_MAX; i++) {
        VIDEO_UI_DrawButton((ui_button_id_t)i);
    }
}

/**
 * @brief   Draw single button
 */
void VIDEO_UI_DrawButton(ui_button_id_t button)
{
    uint16_t bg_color;
    uint16_t text_color;
    ui_button_t *btn = &g_buttons[button];

    /* Clear button area if not visible */
    if (!btn->visible) {
        LCD_SetColor(UI_COLOR_BLACK);
        LCD_FillRect(btn->x, btn->y, btn->width, btn->height);
        return;
    }

    /* Determine colors based on state */
    if (btn->enabled == 0) {
        /* Disabled */
        bg_color = UI_COLOR_BTN_DISABLED;
        text_color = UI_COLOR_DARK_GRAY;
    } else if (btn->enabled == 2) {
        /* Highlighted (AI MODE button) */
        bg_color = UI_COLOR_BTN_HIGHLIGHT;
        text_color = UI_COLOR_BTN_TEXT;
    } else {
        /* Normal or pressed */
        if (btn->text && strcmp(btn->text, "REC") == 0 && g_recording_blink) {
            bg_color = UI_COLOR_REC_ACTIVE;
        } else {
            bg_color = UI_COLOR_BTN_NORMAL;
        }
        text_color = UI_COLOR_BTN_TEXT;
    }

    VIDEO_UI_DrawButtonFrame(btn, bg_color);
    VIDEO_UI_DrawButtonText(btn, text_color);
}

/**
 * @brief   Get button at specified coordinates
 */
int8_t VIDEO_UI_GetButtonAt(uint16_t x, uint16_t y)
{
    int8_t i;

    /* Only check buttons in side panel area */
    if (x < SIDE_PANEL_X) {
        return -1;
    }

    for (i = 0; i < UI_BUTTON_MAX; i++) {
        if (!g_buttons[i].visible || g_buttons[i].enabled == 0) {
            continue;
        }
        if (x >= g_buttons[i].x && x < g_buttons[i].x + g_buttons[i].width &&
            y >= g_buttons[i].y && y < g_buttons[i].y + g_buttons[i].height) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief   Handle touch event
 */
void VIDEO_UI_HandleTouch(uint16_t x, uint16_t y, uint8_t pressed)
{
    int8_t button;

    if (pressed) {
        /* Touch pressed */
        if (!g_touch_pressed) {
            g_touch_pressed = 1;
            button = VIDEO_UI_GetButtonAt(x, y);
            if (button >= 0) {
                g_pressed_button = button;
                /* Draw pressed state */
                VIDEO_UI_DrawButtonFrame(&g_buttons[button], UI_COLOR_BTN_PRESSED);
                VIDEO_UI_DrawButtonText(&g_buttons[button], UI_COLOR_BTN_TEXT);
            }
        }
    } else {
        /* Touch released */
        if (g_touch_pressed && g_pressed_button >= 0) {
            button = VIDEO_UI_GetButtonAt(x, y);
            if (button == g_pressed_button) {
                /* Execute button action */
                switch (g_pressed_button) {
                    case UI_BUTTON_AI_MODE:
                        VIDEO_UI_OnAIButton();
                        break;
                    case UI_BUTTON_REC:
                        VIDEO_UI_OnRecButton();
                        break;
                    case UI_BUTTON_STOP:
                        VIDEO_UI_OnStopButton();
                        break;
                    case UI_BUTTON_BACK_AI:
                        VIDEO_UI_OnBackAIButton();
                        break;
                    default:
                        break;
                }
            }
            /* Redraw button to normal state */
            VIDEO_UI_DrawButton((ui_button_id_t)g_pressed_button);
        }
        g_touch_pressed = 0;
        g_pressed_button = -1;
    }
}

/**
 * @brief   Set current UI mode
 */
void VIDEO_UI_SetMode(ui_mode_t mode)
{
    if (g_current_mode == mode) {
        return;
    }

    g_current_mode = mode;

    /* Update button visibility based on mode */
    if (mode == UI_MODE_AI) {
        /* AI mode */
        g_buttons[UI_BUTTON_AI_MODE].enabled = 2;  /* Highlighted */
        g_buttons[UI_BUTTON_REC].visible = 1;
        g_buttons[UI_BUTTON_REC].enabled = 1;
        g_buttons[UI_BUTTON_STOP].visible = 0;
        g_buttons[UI_BUTTON_STOP].enabled = 0;
        g_buttons[UI_BUTTON_BACK_AI].visible = 0;
        g_buttons[UI_BUTTON_BACK_AI].enabled = 0;
    } else {
        /* Recording preview mode */
        g_buttons[UI_BUTTON_AI_MODE].enabled = 1;  /* Normal */
        g_buttons[UI_BUTTON_REC].visible = 0;
        g_buttons[UI_BUTTON_REC].enabled = 0;
        g_buttons[UI_BUTTON_STOP].visible = 1;
        g_buttons[UI_BUTTON_STOP].enabled = 1;
        g_buttons[UI_BUTTON_BACK_AI].visible = 1;
        g_buttons[UI_BUTTON_BACK_AI].enabled = 1;
    }

    /* Redraw UI */
    VIDEO_UI_Draw();
}

/**
 * @brief   Get current UI mode
 */
ui_mode_t VIDEO_UI_GetMode(void)
{
    return g_current_mode;
}

/**
 * @brief   Update FPS display
 */
void VIDEO_UI_UpdateFPS(uint16_t fps)
{
    static uint16_t last_fps = 0xFFFF;
    static ui_mode_t last_mode = (ui_mode_t)0xFF;
    char str[16];
    uint16_t x, y;

    /* Only redraw if value or mode changed */
    if (fps == last_fps && g_current_mode == last_mode) {
        return;
    }
    last_fps = fps;
    last_mode = g_current_mode;

    /* Position depends on current mode */
    if (g_current_mode == UI_MODE_AI) {
        x = AI_FPS_X;
        y = AI_FPS_Y;
    } else {
        x = STATUS_FPS_X;
        y = STATUS_BAR_Y;
    }

    LCD_SetColor(UI_COLOR_BLACK);
    LCD_FillRect(x, y, 80, 20);

    LCD_SetColor(UI_COLOR_WHITE);
    sprintf(str, "FPS:%d", fps);
    LCD_DisplayString(x, y, str, LCD_BackColor);

    g_current_fps = fps;
}

/**
 * @brief   Update gesture result display
 */
void VIDEO_UI_UpdateGesture(int32_t gesture)
{
    static int32_t last_gesture = -2;
    char str[32];

    /* Only redraw if value changed */
    if (gesture == last_gesture) {
        return;
    }
    last_gesture = gesture;

    LCD_SetColor(UI_COLOR_BLACK);
    LCD_FillRect(AI_GESTURE_X, AI_GESTURE_Y, 150, 20);

    LCD_SetColor(UI_COLOR_WHITE);
    if (gesture >= 0) {
        sprintf(str, "Gesture:%d", (int)gesture);
    } else {
        sprintf(str, "Gesture:--");
    }
    LCD_DisplayString(AI_GESTURE_X, AI_GESTURE_Y, str, LCD_BackColor);

    g_current_gesture = gesture;
}

/**
 * @brief   Update recording status display
 */
void VIDEO_UI_UpdateRecordingStatus(uint32_t time_ms, uint32_t size_bytes, uint8_t is_recording)
{
    static uint32_t last_seconds = 0xFFFFFFFF;
    static uint32_t last_size_mb = 0xFFFFFFFF;
    static uint8_t last_is_recording = 0xFF;
    char str[32];
    uint16_t x, y;

    /* Update recording state */
    g_recording_time_ms = time_ms;
    g_file_size_bytes = size_bytes;
    g_is_recording = is_recording;

    /* Only redraw if seconds, size (MB), or recording state changed */
    uint32_t seconds = time_ms / 1000;
    uint32_t size_mb = size_bytes / (1024 * 1024);
    if (seconds == last_seconds && size_mb == last_size_mb && is_recording == last_is_recording) {
        return;
    }
    last_seconds = seconds;
    last_size_mb = size_mb;
    last_is_recording = is_recording;

    /* Draw status bar background */
    LCD_SetColor(UI_COLOR_BLACK);
    LCD_FillRect(0, STATUS_BAR_Y, MAIN_DISPLAY_WIDTH, STATUS_BAR_HEIGHT);

    /* Time */
    uint8_t mins = seconds / 60;
    uint8_t secs = seconds % 60;
    x = STATUS_TIME_X;
    y = STATUS_BAR_Y + 15;
    LCD_SetColor(UI_COLOR_GRAY);
    LCD_DisplayString(x, y, "Time:", LCD_BackColor);
    x = STATUS_TIME_X + 40;
    sprintf(str, "%02d:%02d", mins, secs);
    LCD_SetColor(UI_COLOR_WHITE);
    LCD_DisplayString(x, y, str, LCD_BackColor);

    /* File size */
    x = STATUS_SIZE_X;
    uint32_t size_kb = size_bytes / 1024;
    uint32_t size_mb_frac = size_kb / 1024;
    size_kb = size_kb % 1024;
    LCD_SetColor(UI_COLOR_GRAY);
    LCD_DisplayString(x, y, "Size:", LCD_BackColor);
    x = STATUS_SIZE_X + 40;
    sprintf(str, "%lu.%luMB", size_mb_frac, size_kb);
    LCD_SetColor(UI_COLOR_WHITE);
    LCD_DisplayString(x, y, str, LCD_BackColor);

    /* Mode indicator */
    x = STATUS_MODE_X;
    LCD_SetColor(UI_COLOR_GRAY);
    LCD_DisplayString(x, y, "Mode:", LCD_BackColor);
    x = STATUS_MODE_X + 40;
    LCD_SetColor(UI_COLOR_WHITE);
    LCD_DisplayString(x, y, "REC", LCD_WHITE);

    /* Recording indicator (blinking dot) */
    x = STATUS_REC_X;
    LCD_SetColor(UI_COLOR_GRAY);
    LCD_DisplayString(x, y, "REC:", LCD_WHITE);
    x = STATUS_REC_X + 40;
    if (is_recording) {
        LCD_SetColor(g_recording_blink ? UI_COLOR_RED : UI_COLOR_BLACK);
    } else {
        LCD_SetColor(UI_COLOR_BLACK);
    }
    LCD_DrawCircle(x + 5, y + 10, 5);
}

/*============================================================================
 * Private Functions
 *==========================================================================*/

/**
 * @brief   Clear main display area
 */
static void VIDEO_UI_ClearMainDisplay(void)
{
    LCD_SetColor(UI_COLOR_BLACK);
    LCD_FillRect(0, 0, MAIN_DISPLAY_WIDTH, LCD_HEIGHT);
}

/**
 * @brief   Draw status bar (for recording preview mode)
 */
static void VIDEO_UI_DrawStatusBar(void)
{
    /* Draw separator line */
    LCD_SetColor(UI_COLOR_DARK_GRAY);
    LCD_DrawLine(0, STATUS_BAR_Y, MAIN_DISPLAY_WIDTH, STATUS_BAR_Y);

    /* Initialize status display */
    VIDEO_UI_UpdateRecordingStatus(0, 0, 0);
}

/**
 * @brief   Update blink state for recording indicator
 */
static void VIDEO_UI_UpdateBlink(void)
{
    static uint32_t last_tick = 0;
    uint32_t current_tick;

    if (!g_is_recording) {
        if (g_recording_blink) {
            g_recording_blink = 0;
        }
        return;
    }

    current_tick = HAL_GetTick();

    if (current_tick - last_tick >= 500) {
        g_recording_blink = !g_recording_blink;
        last_tick = current_tick;

        /* Update recording indicator */
        uint16_t x = STATUS_REC_X + 40;
        uint16_t y = STATUS_BAR_Y + 15;
        LCD_SetColor(g_recording_blink ? UI_COLOR_RED : UI_COLOR_BLACK);
        LCD_DrawCircle(x + 5, y + 10, 5);

        /* Update REC button if visible */
        if (g_buttons[UI_BUTTON_REC].visible) {
            VIDEO_UI_DrawButton(UI_BUTTON_REC);
        }
    }
}

/**
 * @brief   Draw button frame (background + border)
 */
static void VIDEO_UI_DrawButtonFrame(ui_button_t *btn, uint16_t color)
{
    LCD_SetColor(color);
    LCD_FillRect(btn->x, btn->y, btn->width, btn->height);

    LCD_SetColor(UI_COLOR_BTN_BORDER);
    LCD_DrawRect(btn->x, btn->y, btn->width, btn->height);
}

/**
 * @brief   Draw button text
 */
static void VIDEO_UI_DrawButtonText(ui_button_t *btn, uint16_t color)
{
    uint16_t text_width;
    uint16_t text_x, text_y;

    text_width = strlen(btn->text) * 8;
    text_x = btn->x + (btn->width - text_width) / 2;
    text_y = btn->y + (btn->height - 16) / 2;

    LCD_SetColor(color);
    LCD_DisplayString(text_x, text_y, btn->text, LCD_WHITE);
}

/*============================================================================
 * Button Callbacks (weak functions - can be overridden in main.c)
 *==========================================================================*/

/**
 * @brief   AI MODE button pressed callback
 */
__weak void VIDEO_UI_OnAIButton(void)
{
    printf("[VIDEO_UI] AI MODE button pressed\r\n");

    /* Already in AI mode, no action needed */
    if (g_current_mode == UI_MODE_AI) {
        return;
    }

    /* Stop recording if active */
    if (g_is_recording) {
        VIDEO_UI_OnStopButton();
    }

    /* Switch to AI mode */
    VIDEO_UI_SetMode(UI_MODE_AI);
}

/**
 * @brief   REC button pressed callback
 */
__weak void VIDEO_UI_OnRecButton(void)
{
    printf("[VIDEO_UI] REC button pressed\r\n");

    /* Switch to recording preview mode */
    VIDEO_UI_SetMode(UI_MODE_REC_PREVIEW);

    /* Start recording - caller should implement this */
}

/**
 * @brief   STOP button pressed callback
 */
__weak void VIDEO_UI_OnStopButton(void)
{
    printf("[VIDEO_UI] STOP button pressed\r\n");

    /* Stop recording - caller should implement this */

    /* Reset recording state */
    g_is_recording = 0;
    g_recording_time_ms = 0;
    g_file_size_bytes = 0;

    /* Switch to AI mode */
    VIDEO_UI_SetMode(UI_MODE_AI);
}

/**
 * @brief   Back to AI button pressed callback
 */
__weak void VIDEO_UI_OnBackAIButton(void)
{
    printf("[VIDEO_UI] Back AI button pressed\r\n");

    /* Same as STOP button - stop recording and return to AI mode */
    VIDEO_UI_OnStopButton();
}
