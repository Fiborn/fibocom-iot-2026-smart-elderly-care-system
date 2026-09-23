/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    ltdc.h
  * @brief   This file contains all the function prototypes for
  *          the ltdc.c file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with the software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __LTDC_H__
#define __LTDC_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */
#include "usart.h"
#include "fmc.h"
#include "lcd_fonts.h"
#include <stdio.h>
#include "dma2d.h"

// LTDC clock frequency is set to 33MHz

#define     LCD_NUM_LAYERS  1   // Number of display layers

#define ColorMode_0   LTDC_PIXEL_FORMAT_RGB565       // Set layer0 color format
//#define ColorMode_0   LTDC_PIXEL_FORMAT_ARGB1555  
//#define ColorMode_0    LTDC_PIXEL_FORMAT_ARGB4444  
//#define ColorMode_0   LTDC_PIXEL_FORMAT_RGB888
//#define ColorMode_0   LTDC_PIXEL_FORMAT_ARGB8888   


#if  LCD_NUM_LAYERS == 2  // If dual layer, configure layer1 color format

//  #define ColorMode_1   LTDC_PIXEL_FORMAT_RGB565   
    #define ColorMode_1   LTDC_PIXEL_FORMAT_ARGB1555
//  #define ColorMode_1   LTDC_PIXEL_FORMAT_ARGB4444
// #define ColorMode_1   LTDC_PIXEL_FORMAT_RGB888   
//  #define ColorMode_1   LTDC_PIXEL_FORMAT_ARGB8888

#endif

// Display direction definitions
#define Direction_H 0       // LCD horizontal display
#define Direction_V 1       // LCD vertical display

// Number display mode: fill with 0 or space
#define  Fill_Zero  0       // Fill with 0
#define  Fill_Space 1       // Fill with space

/*---------------------------------------- Color Definitions ------------------------------------------------------*/

#define     LCD_WHITE       0xffFFFFFF     // White
#define     LCD_BLACK       0xff000000     // Black
#define     LCD_BLUE        0xff0000FF     // Blue
#define     LCD_GREEN       0xff00FF00     // Green
#define     LCD_RED         0xffFF0000     // Red
#define     LCD_CYAN        0xff00FFFF     // Cyan
#define     LCD_MAGENTA     0xffFF00FF     // Magenta
#define     LCD_YELLOW      0xffFFFF00     // Yellow
#define     LCD_GREY        0xff2C2C2C     // Grey

#define     LIGHT_BLUE      0xff8080FF     // Light blue
#define     LIGHT_GREEN     0xff80FF80     // Light green
#define     LIGHT_RED       0xffFF8080     // Light red
#define     LIGHT_CYAN      0xff80FFFF     // Light cyan
#define     LIGHT_MAGENTA   0xffFF80FF     // Light magenta
#define     LIGHT_YELLOW    0xffFFFF80     // Light yellow
#define     LIGHT_GREY      0xffA3A3A3     // Light grey

#define     DARK_BLUE       0xff000080     // Dark blue
#define     DARK_GREEN      0xff008000     // Dark green
#define     DARK_RED        0xff800000     // Dark red
#define     DARK_CYAN       0xff008080     // Dark cyan
#define     DARK_MAGENTA    0xff800080     // Dark magenta
#define     DARK_YELLOW     0xff808000     // Dark yellow
#define     DARK_GREY       0xff404040     // Dark grey
#define 		TECH_BLUE    		0xff0066ff
/*---------------------------------------------------------- Function Prototypes -------------------------------------------------------*/
    
void    LCD_RGB_Init(void);         // Initialize LCD screen
void    LCD_Clear(void);            // Clear screen
void    LCD_ClearRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height);

void  LCD_SetLayer(uint8_t Layerx);
void  LCD_SetColor(uint32_t Color);
void  LCD_SetBackColor(uint32_t Color);
extern uint32_t LCD_BackColor;      // Global background color variable
void  LCD_DisplayDirection(uint8_t direction);

// Display ASCII characters
void  LCD_SetFont(pFONT *fonts);
void    LCD_DisplayChar(uint16_t x, uint16_t y, uint8_t c, uint32_t bgColor);
void    LCD_DisplayString(uint16_t x, uint16_t y, char *p, uint32_t bgColor);

// Display Chinese characters (including ASCII)
void    LCD_SetTextFont(pFONT *fonts);
void    LCD_DisplayChinese(uint16_t x, uint16_t y, char *pText, uint32_t bgColor);
void    LCD_DisplayText(uint16_t x, uint16_t y, char *pText, uint32_t bgColor);

// Display integers and decimals
void  LCD_ShowNumMode(uint8_t mode);
void  LCD_DisplayNumber(uint16_t x, uint16_t y, int32_t number, uint8_t len, uint32_t bgColor);
void  LCD_DisplayDecimals(uint16_t x, uint16_t y, double number, uint8_t len, uint8_t decs, uint32_t bgColor);

// Display images
void    LCD_DrawImage(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint8_t *pImage);

// 2D graphics drawing functions
void  LCD_DrawPoint(uint16_t x, uint16_t y, uint32_t color);
uint32_t    LCD_ReadPoint(uint16_t x, uint16_t y);
void  LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void  LCD_DrawRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
void  LCD_DrawCircle(uint16_t x, uint16_t y, uint16_t r);
void  LCD_DrawEllipse(int x, int y, int r1, int r2);

// Fill functions
void  LCD_FillRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
void  LCD_FillCircle(uint16_t x, uint16_t y, uint16_t r);

/*-------------------------------------------------------- LCD Parameters -------------------------------------------------------*/

#define HBP  80
#define VBP  40
#define HSW  1
#define VSW  1
#define HFP  200
#define VFP  22

// FK743M2-IIT6 board uses external SDRAM as video memory
// Start address: 0xC0000000, Size: 32MB
// Video memory = resolution * bytes per pixel

#define LCD_Width         800
#define LCD_Height        480
#define LCD_MemoryAdd     SDRAM_BANK_ADDR

#if ( ColorMode_0 == LTDC_PIXEL_FORMAT_RGB565 || ColorMode_0 == LTDC_PIXEL_FORMAT_ARGB1555 || ColorMode_0 ==LTDC_PIXEL_FORMAT_ARGB4444 )
    #define BytesPerPixel_0     2   // 16-bit color mode
#elif ColorMode_0 == LTDC_PIXEL_FORMAT_RGB888
    #define BytesPerPixel_0     3   // 24-bit color mode
#else
    #define BytesPerPixel_0     4   // 32-bit color mode
#endif

#if LCD_NUM_LAYERS == 2
    #if ( ColorMode_1 == LTDC_PIXEL_FORMAT_RGB565 || ColorMode_1 == LTDC_PIXEL_FORMAT_ARGB1555 || ColorMode_1 == LTDC_PIXEL_FORMAT_ARGB4444 )
        #define BytesPerPixel_1     2
    #elif ColorMode_1 == LTDC_PIXEL_FORMAT_RGB888
        #define BytesPerPixel_1     3
    #else
        #define BytesPerPixel_1     4
    #endif
    #define LCD_MemoryAdd_OFFSET   LCD_Width * LCD_Height * BytesPerPixel_0
#endif

/*-------------------------------------------------------- LCD Backlight Control -------------------------------------------------------*/

#define  LCD_Backlight_PIN           GPIO_PIN_6
#define LCD_Backlight_PORT           GPIOH
#define  GPIO_LDC_Backlight_CLK_ENABLE  __HAL_RCC_GPIOH_CLK_ENABLE()

#define LCD_Backlight_OFF  HAL_GPIO_WritePin(LCD_Backlight_PORT, LCD_Backlight_PIN, GPIO_PIN_RESET)
#define LCD_Backlight_ON   HAL_GPIO_WritePin(LCD_Backlight_PORT, LCD_Backlight_PIN, GPIO_PIN_SET)

/* USER CODE END Includes */

extern LTDC_HandleTypeDef hltdc;

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

void MX_LTDC_Init(void);

/* USER CODE BEGIN Prototypes */

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __LTDC_H__ */