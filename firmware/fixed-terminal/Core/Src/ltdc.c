/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    ltdc.c
  * @brief   This file provides code for the configuration
  *          of the LTDC instances.
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
/* Includes ------------------------------------------------------------------*/
#include "ltdc.h"
#include "lcd_fonts.h"
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

LTDC_HandleTypeDef hltdc;

/* LTDC init function */
void MX_LTDC_Init(void)
{

  /* USER CODE BEGIN LTDC_Init 0 */

  /* USER CODE END LTDC_Init 0 */

  LTDC_LayerCfgTypeDef pLayerCfg = {0};

  /* USER CODE BEGIN LTDC_Init 1 */

  /* USER CODE END LTDC_Init 1 */
  hltdc.Instance = LTDC;
  hltdc.Init.HSPolarity = LTDC_HSPOLARITY_AL;
  hltdc.Init.VSPolarity = LTDC_VSPOLARITY_AL;
  hltdc.Init.DEPolarity = LTDC_DEPOLARITY_AL;
  hltdc.Init.PCPolarity = LTDC_PCPOLARITY_IPC;
  hltdc.Init.HorizontalSync = 0;
  hltdc.Init.VerticalSync = 0;
  hltdc.Init.AccumulatedHBP = 80;
  hltdc.Init.AccumulatedVBP = 20;
  hltdc.Init.AccumulatedActiveW = 880;
  hltdc.Init.AccumulatedActiveH = 500;
  hltdc.Init.TotalWidth = 1080;
  hltdc.Init.TotalHeigh = 522;
  hltdc.Init.Backcolor.Blue = 0;
  hltdc.Init.Backcolor.Green = 0;
  hltdc.Init.Backcolor.Red = 0;
  if (HAL_LTDC_Init(&hltdc) != HAL_OK)
  {
    Error_Handler();
  }
  pLayerCfg.WindowX0 = 0;
  pLayerCfg.WindowX1 = 800;
  pLayerCfg.WindowY0 = 0;
  pLayerCfg.WindowY1 = 480;
  pLayerCfg.PixelFormat = LTDC_PIXEL_FORMAT_RGB565;
  pLayerCfg.Alpha = 255;
  pLayerCfg.Alpha0 = 0;
  pLayerCfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_CA;
  pLayerCfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_CA;
  pLayerCfg.FBStartAdress = 0xC0000000;
  pLayerCfg.ImageWidth = 800;
  pLayerCfg.ImageHeight = 480;
  pLayerCfg.Backcolor.Blue = 0;
  pLayerCfg.Backcolor.Green = 0;
  pLayerCfg.Backcolor.Red = 0;
  if (HAL_LTDC_ConfigLayer(&hltdc, &pLayerCfg, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN LTDC_Init 2 */

  /* USER CODE END LTDC_Init 2 */

}

void HAL_LTDC_MspInit(LTDC_HandleTypeDef* ltdcHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
  if(ltdcHandle->Instance==LTDC)
  {
  /* USER CODE BEGIN LTDC_MspInit 0 */

  /* USER CODE END LTDC_MspInit 0 */

  /** Initializes the peripherals clock
  */
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
    PeriphClkInitStruct.PLL3.PLL3M = 25;
    PeriphClkInitStruct.PLL3.PLL3N = 330;
    PeriphClkInitStruct.PLL3.PLL3P = 2;
    PeriphClkInitStruct.PLL3.PLL3Q = 2;
    PeriphClkInitStruct.PLL3.PLL3R = 10;
    PeriphClkInitStruct.PLL3.PLL3RGE = RCC_PLL3VCIRANGE_0;
    PeriphClkInitStruct.PLL3.PLL3VCOSEL = RCC_PLL3VCOWIDE;
    PeriphClkInitStruct.PLL3.PLL3FRACN = 0;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    {
      Error_Handler();
    }

    /* LTDC clock enable */
    __HAL_RCC_LTDC_CLK_ENABLE();

    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOI_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    /**LTDC GPIO Configuration
    PE5     ------> LTDC_G0
    PE6     ------> LTDC_G1
    PI9     ------> LTDC_VSYNC
    PI10     ------> LTDC_HSYNC
    PF10     ------> LTDC_DE
    PA2     ------> LTDC_R1
    PH8     ------> LTDC_R2
    PH9     ------> LTDC_R3
    PH10     ------> LTDC_R4
    PH11     ------> LTDC_R5
    PH12     ------> LTDC_R6
    PG6     ------> LTDC_R7
    PG7     ------> LTDC_CLK
    PA8     ------> LTDC_B3
    PH13     ------> LTDC_G2
    PH14     ------> LTDC_G3
    PH15     ------> LTDC_G4
    PI0     ------> LTDC_G5
    PI1     ------> LTDC_G6
    PI2     ------> LTDC_G7
    PD6     ------> LTDC_B2
    PG12     ------> LTDC_B1
    PG13     ------> LTDC_R0
    PG14     ------> LTDC_B0
    PI4     ------> LTDC_B4
    PI5     ------> LTDC_B5
    PI6     ------> LTDC_B6
    PI7     ------> LTDC_B7
    */
    GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF14_LTDC;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_0|GPIO_PIN_1
                          |GPIO_PIN_2|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6
                          |GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF14_LTDC;
    HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF14_LTDC;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF14_LTDC;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11
                          |GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF14_LTDC;
    HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_12|GPIO_PIN_13
                          |GPIO_PIN_14;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF14_LTDC;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF13_LTDC;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF14_LTDC;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /* USER CODE BEGIN LTDC_MspInit 1 */

  /* USER CODE END LTDC_MspInit 1 */
  }
}

void HAL_LTDC_MspDeInit(LTDC_HandleTypeDef* ltdcHandle)
{

  if(ltdcHandle->Instance==LTDC)
  {
  /* USER CODE BEGIN LTDC_MspDeInit 0 */

  /* USER CODE END LTDC_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_LTDC_CLK_DISABLE();

    /**LTDC GPIO Configuration
    PE5     ------> LTDC_G0
    PE6     ------> LTDC_G1
    PI9     ------> LTDC_VSYNC
    PI10     ------> LTDC_HSYNC
    PF10     ------> LTDC_DE
    PA2     ------> LTDC_R1
    PH8     ------> LTDC_R2
    PH9     ------> LTDC_R3
    PH10     ------> LTDC_R4
    PH11     ------> LTDC_R5
    PH12     ------> LTDC_R6
    PG6     ------> LTDC_R7
    PG7     ------> LTDC_CLK
    PA8     ------> LTDC_B3
    PH13     ------> LTDC_G2
    PH14     ------> LTDC_G3
    PH15     ------> LTDC_G4
    PI0     ------> LTDC_G5
    PI1     ------> LTDC_G6
    PI2     ------> LTDC_G7
    PD6     ------> LTDC_B2
    PG12     ------> LTDC_B1
    PG13     ------> LTDC_R0
    PG14     ------> LTDC_B0
    PI4     ------> LTDC_B4
    PI5     ------> LTDC_B5
    PI6     ------> LTDC_B6
    PI7     ------> LTDC_B7
    */
    HAL_GPIO_DeInit(GPIOE, GPIO_PIN_5|GPIO_PIN_6);

    HAL_GPIO_DeInit(GPIOI, GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_0|GPIO_PIN_1
                          |GPIO_PIN_2|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6
                          |GPIO_PIN_7);

    HAL_GPIO_DeInit(GPIOF, GPIO_PIN_10);

    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2|GPIO_PIN_8);

    HAL_GPIO_DeInit(GPIOH, GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11
                          |GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15);

    HAL_GPIO_DeInit(GPIOG, GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_12|GPIO_PIN_13
                          |GPIO_PIN_14);

    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_6);

  /* USER CODE BEGIN LTDC_MspDeInit 1 */

  /* USER CODE END LTDC_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

extern DMA2D_HandleTypeDef hdma2d;  // DMA2D handle
extern LTDC_HandleTypeDef hltdc;    // LTDC handle


static pFONT *LCD_Fonts;       // English font
static pFONT *LCD_CHFonts;     // Chinese font

// LCD controller structure
struct
{
    uint32_t Color;            // Current pen color
    uint32_t BackColor;        // Background color
    uint32_t ColorMode;        // Color format
    uint32_t LayerMemoryAdd;   // Layer memory address
    uint8_t  Layer;            // Current layer
    uint8_t  Direction;        // Display direction
    uint8_t  BytesPerPixel;    // Bytes per pixel
    uint8_t  ShowNum_Mode;     // Number display mode
}LCD;

uint32_t LCD_BackColor = 0;    // Global background color variable


/*************************************************************************************************
*   Function: LCD_GPIO_Init
*   Description: Initialize LTDC backlight GPIO pin
*************************************************************************************************/

void LCD_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_LDC_Backlight_CLK_ENABLE;  // Enable backlight GPIO clock

    GPIO_InitStruct.Pin     = LCD_Backlight_PIN;
    GPIO_InitStruct.Mode    = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull    = GPIO_NOPULL;
    GPIO_InitStruct.Speed   = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LCD_Backlight_PORT, &GPIO_InitStruct);

    LCD_Backlight_OFF;  // Turn off backlight first, will turn on after LTDC init
}

/*************************************************************************************************
*   Function: LCD_RGB_Init
*   Description: Initialize LCD screen
*************************************************************************************************/

void LCD_RGB_Init(void)
{
    LTDC_LayerCfgTypeDef pLayerCfg = {0};

    /*---------------------------------- layer0 config --------------------------------*/

    pLayerCfg.WindowX0       = 0;
    pLayerCfg.WindowX1       = LCD_Width;
    pLayerCfg.WindowY0       = 0;
    pLayerCfg.WindowY1       = LCD_Height;
    pLayerCfg.ImageWidth     = LCD_Width;
    pLayerCfg.ImageHeight    = LCD_Height;
    pLayerCfg.PixelFormat    = ColorMode_0;

    pLayerCfg.Alpha          = 255;  // Range 0~255, 255=opaque, 0=transparent
    pLayerCfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_CA;
    pLayerCfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_CA;

    pLayerCfg.FBStartAdress  = LCD_MemoryAdd;

    pLayerCfg.Alpha0         = 0;
    pLayerCfg.Backcolor.Blue  = 0;
    pLayerCfg.Backcolor.Green = 0;
    pLayerCfg.Backcolor.Red   = 0;

    HAL_LTDC_ConfigLayer(&hltdc, &pLayerCfg, 0);

#if ( ( ColorMode_0 == LTDC_PIXEL_FORMAT_RGB888 )||( ColorMode_0 == LTDC_PIXEL_FORMAT_ARGB8888 ) )
    HAL_LTDC_EnableDither(&hltdc);  // Enable dithering for 24/32-bit color
#endif


    /*---------------------------------- layer1 config --------------------------------*/

#if ( LCD_NUM_LAYERS == 2 )

    LTDC_LayerCfgTypeDef pLayerCfg1 = {0};

    pLayerCfg1.WindowX0      = 0;
    pLayerCfg1.WindowX1      = LCD_Width;
    pLayerCfg1.WindowY0      = 0;
    pLayerCfg1.WindowY1      = LCD_Height;
    pLayerCfg1.ImageWidth    = LCD_Width;
    pLayerCfg1.ImageHeight   = LCD_Height;
    pLayerCfg1.PixelFormat   = ColorMode_1;

    pLayerCfg1.Alpha         = 255;
    pLayerCfg1.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;
    pLayerCfg1.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA;

    pLayerCfg1.FBStartAdress = LCD_MemoryAdd + LCD_MemoryAdd_OFFSET;

    pLayerCfg1.Alpha0        = 0;
    pLayerCfg1.Backcolor.Red   = 0;
    pLayerCfg1.Backcolor.Green = 0;
    pLayerCfg1.Backcolor.Blue  = 0;

    HAL_LTDC_ConfigLayer(&hltdc, &pLayerCfg1, 1);

    #if ( ( ColorMode_1 == LTDC_PIXEL_FORMAT_RGB888 )||( ColorMode_1 == LTDC_PIXEL_FORMAT_ARGB8888 ) )
        HAL_LTDC_EnableDither(&hltdc);
    #endif

#endif

    LCD_GPIO_Init();  // Initialize backlight GPIO

    /*---------------------------------- Default settings --------------------------------*/

    LCD_DisplayDirection(Direction_H);
    LCD_SetFont(&Font24);
    LCD_ShowNumMode(Fill_Space);

    LCD_SetLayer(0);
    LCD_SetBackColor(LCD_BLACK);
    LCD_SetColor(LCD_WHITE);
    LCD_Clear();

    /*---------------------------------- Foreground layer init --------------------------------*/

#if LCD_NUM_LAYERS == 2
    LCD_SetLayer(1);
    LCD_SetBackColor(LCD_BLACK);
    LCD_SetColor(LCD_WHITE);
    LCD_Clear();
#endif

    LCD_Backlight_ON;  // Turn on backlight

}


/*************************************************************************************************
*   Function: LCD_SetLayer
*   Parameters: layer - Layer to select (0 or 1)
*   Description: Select display layer, switch corresponding memory address and color format
*************************************************************************************************/

void LCD_SetLayer(uint8_t layer)
{
#if LCD_NUM_LAYERS == 2

    if (layer == 0)
    {
        LCD.LayerMemoryAdd = LCD_MemoryAdd;
        LCD.ColorMode      = ColorMode_0;
        LCD.BytesPerPixel  = BytesPerPixel_0;
    }
    else if(layer == 1)
    {
        LCD.LayerMemoryAdd = LCD_MemoryAdd + LCD_MemoryAdd_OFFSET;
        LCD.ColorMode      = ColorMode_1;
        LCD.BytesPerPixel  = BytesPerPixel_1;
    }
    LCD.Layer = layer;

#else

    LCD.LayerMemoryAdd = LCD_MemoryAdd;
    LCD.ColorMode      = ColorMode_0;
    LCD.BytesPerPixel  = BytesPerPixel_0;
    LCD.Layer = 0;

#endif
}

/*************************************************************************************************
*   Function: LCD_ConvertColor
*   Parameters: Color - 32-bit ARGB color value
*   Return: Converted color value matching current ColorMode
*   Description: Convert 32-bit ARGB color to current pixel format
*************************************************************************************************/
static uint32_t LCD_ConvertColor(uint32_t Color)
{
    uint16_t Alpha_Value = 0, Red_Value = 0, Green_Value = 0, Blue_Value = 0;

    if( LCD.ColorMode == LTDC_PIXEL_FORMAT_RGB565 )
    {
        Red_Value   = (uint16_t)((Color&0x00F80000)>>8);
        Green_Value = (uint16_t)((Color&0x0000FC00)>>5);
        Blue_Value  = (uint16_t)((Color&0x000000F8)>>3);
        return (uint32_t)(Red_Value | Green_Value | Blue_Value);
    }
    else if( LCD.ColorMode == LTDC_PIXEL_FORMAT_ARGB1555 )
    {
        if( (Color & 0xFF000000) == 0 )
            Alpha_Value = 0x0000;
        else
            Alpha_Value = 0x8000;

        Red_Value   = (uint16_t)((Color&0x00F80000)>>9);
        Green_Value = (uint16_t)((Color&0x0000F800)>>6);
        Blue_Value  = (uint16_t)((Color&0x000000F8)>>3);
        return (uint32_t)(Alpha_Value | Red_Value | Green_Value | Blue_Value);
    }
    else if( LCD.ColorMode == LTDC_PIXEL_FORMAT_ARGB4444 )
    {
        Alpha_Value = (uint16_t)((Color&0xf0000000)>>16);
        Red_Value   = (uint16_t)((Color&0x00F00000)>>12);
        Green_Value = (uint16_t)((Color&0x0000F000)>>8);
        Blue_Value  = (uint16_t)((Color&0x000000F8)>>4);
        return (uint32_t)(Alpha_Value | Red_Value | Green_Value | Blue_Value);
    }
    else
    {
        return Color;
    }
}


/*************************************************************************************************
*   Function: LCD_SetColor
*   Parameters: Color - 32-bit ARGB color value
*   Description: Set pen color for drawing
*************************************************************************************************/

void LCD_SetColor(uint32_t Color)
{
    uint16_t Alpha_Value = 0, Red_Value = 0, Green_Value = 0, Blue_Value = 0;

    if( LCD.ColorMode == LTDC_PIXEL_FORMAT_RGB565 )
    {
        Red_Value   = (uint16_t)((Color&0x00F80000)>>8);
        Green_Value = (uint16_t)((Color&0x0000FC00)>>5);
        Blue_Value  = (uint16_t)((Color&0x000000F8)>>3);
        LCD.Color = (uint16_t)(Red_Value | Green_Value | Blue_Value);
    }
    else if( LCD.ColorMode == LTDC_PIXEL_FORMAT_ARGB1555 )
    {
        if( (Color & 0xFF000000) == 0 )
            Alpha_Value = 0x0000;
        else
            Alpha_Value = 0x8000;

        Red_Value   = (uint16_t)((Color&0x00F80000)>>9);
        Green_Value = (uint16_t)((Color&0x0000F800)>>6);
        Blue_Value  = (uint16_t)((Color&0x000000F8)>>3);
        LCD.Color = (uint16_t)(Alpha_Value | Red_Value | Green_Value | Blue_Value);
    }
    else if( LCD.ColorMode == LTDC_PIXEL_FORMAT_ARGB4444 )
    {
        Alpha_Value = (uint16_t)((Color&0xf0000000)>>16);
        Red_Value   = (uint16_t)((Color&0x00F00000)>>12);
        Green_Value = (uint16_t)((Color&0x0000F000)>>8);
        Blue_Value  = (uint16_t)((Color&0x000000F8)>>4);
        LCD.Color = (uint16_t)(Alpha_Value | Red_Value | Green_Value | Blue_Value);
    }
    else
        LCD.Color = Color;
}

/*************************************************************************************************
*   Function: LCD_SetBackColor
*   Parameters: Color - 32-bit ARGB color value
*   Description: Set background color
*************************************************************************************************/

void LCD_SetBackColor(uint32_t Color)
{
    uint16_t Alpha_Value = 0, Red_Value = 0, Green_Value = 0, Blue_Value = 0;

    if( LCD.ColorMode == LTDC_PIXEL_FORMAT_RGB565 )
    {
        Red_Value    = (uint16_t)((Color&0x00F80000)>>8);
        Green_Value  = (uint16_t)((Color&0x0000FC00)>>5);
        Blue_Value   = (uint16_t)((Color&0x000000F8)>>3);
        LCD.BackColor = (uint16_t)(Red_Value | Green_Value | Blue_Value);
    }
    else if( LCD.ColorMode == LTDC_PIXEL_FORMAT_ARGB1555 )
    {
        if( (Color & 0xFF000000) == 0 )
            Alpha_Value = 0x0000;
        else
            Alpha_Value = 0x8000;

        Red_Value    = (uint16_t)((Color&0x00F80000)>>9);
        Green_Value  = (uint16_t)((Color&0x0000F800)>>6);
        Blue_Value   = (uint16_t)((Color&0x000000F8)>>3);
        LCD.BackColor = (uint16_t)(Alpha_Value | Red_Value | Green_Value | Blue_Value);
    }
    else if( LCD.ColorMode == LTDC_PIXEL_FORMAT_ARGB4444 )
    {
        Alpha_Value  = (uint16_t)((Color&0xf0000000)>>16);
        Red_Value    = (uint16_t)((Color&0x00F00000)>>12);
        Green_Value  = (uint16_t)((Color&0x0000F000)>>8);
        Blue_Value   = (uint16_t)((Color&0x000000F8)>>4);
        LCD.BackColor = (uint16_t)(Alpha_Value | Red_Value | Green_Value | Blue_Value);
    }

    else
        LCD.BackColor = Color;

    LCD_BackColor = Color;  // Update global background color variable
}

/*************************************************************************************************
*   Function: LCD_SetFont
*   Parameters: *fonts - ASCII font to set
*   Description: Set ASCII font (3216/2412/2010/1608/1206)
*************************************************************************************************/

void LCD_SetFont(pFONT *fonts)
{
  LCD_Fonts = fonts;
}

/*************************************************************************************************
*   Function: LCD_DisplayDirection
*   Parameters: direction - Display direction
*   Description: Set display direction (horizontal or vertical)
*************************************************************************************************/

void LCD_DisplayDirection(uint8_t direction)
{
    LCD.Direction = direction;
}

/*************************************************************************************************
*   Function: LCD_Clear
*   Description: Clear screen with LCD.BackColor using DMA2D
*************************************************************************************************/

void LCD_Clear(void)
{
    DMA2D->CR     &= ~(DMA2D_CR_START);
    DMA2D->CR      = DMA2D_R2M;
    DMA2D->OPFCCR  = LCD.ColorMode;
    DMA2D->OOR     = 0;
    DMA2D->OMAR    = LCD.LayerMemoryAdd;
    DMA2D->NLR     = (LCD_Width<<16)|(LCD_Height);
    DMA2D->OCOLR   = LCD.BackColor;

    while( LTDC->CDSR != 0X00000001);

    DMA2D->CR     |= DMA2D_CR_START;

    while (DMA2D->CR & DMA2D_CR_START) ;
}

/*************************************************************************************************
*   Function: LCD_ClearRect
*   Description: Clear rectangular area with LCD.BackColor
*************************************************************************************************/

void LCD_ClearRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    DMA2D->CR     &= ~(DMA2D_CR_START);
    DMA2D->CR      = DMA2D_R2M;
    DMA2D->OPFCCR  = LCD.ColorMode;
    DMA2D->OCOLR   = LCD.BackColor;

    if(LCD.Direction == Direction_H)
    {
        DMA2D->OOR    = LCD_Width - width;
        DMA2D->OMAR   = LCD.LayerMemoryAdd + LCD.BytesPerPixel*(LCD_Width * y + x);
        DMA2D->NLR    = (width<<16)|(height);
    }
    else
    {
        DMA2D->OOR    = LCD_Width - height;
        DMA2D->OMAR   = LCD.LayerMemoryAdd + LCD.BytesPerPixel*((LCD_Height - x - 1 - width)*LCD_Width + y);
        DMA2D->NLR    = (width)|(height<<16);
    }

    DMA2D->CR     |= DMA2D_CR_START;

    while (DMA2D->CR & DMA2D_CR_START) ;
}


/*************************************************************************************************
*   Function: LCD_DrawPoint
*   Parameters: x,y - coordinates, color - 32-bit ARGB color
*   Description: Draw a point at specified coordinates
*************************************************************************************************/

void LCD_DrawPoint(uint16_t x,uint16_t y,uint32_t color)
{
    if( LCD.ColorMode == LTDC_PIXEL_FORMAT_ARGB8888 )
    {
        if (LCD.Direction == Direction_H)
        {
            *(__IO uint32_t*)( LCD.LayerMemoryAdd + 4*(x + y*LCD_Width) ) = color ;
        }
        else if(LCD.Direction == Direction_V)
        {
            *(__IO uint32_t*)( LCD.LayerMemoryAdd + 4*((LCD_Height - x - 1)*LCD_Width + y) ) = color ;
        }
    }
    else if ( LCD.ColorMode == LTDC_PIXEL_FORMAT_RGB888 )
    {
        if (LCD.Direction == Direction_H)
        {
            *(__IO uint16_t*)( LCD.LayerMemoryAdd + 3*(x + y*LCD_Width) ) = color ;
            *(__IO uint8_t*)( LCD.LayerMemoryAdd + 3*(x + y*LCD_Width) + 2 ) = color>>16 ;
        }
        else if(LCD.Direction == Direction_V)
        {
            *(__IO uint16_t*)( LCD.LayerMemoryAdd + 3*((LCD_Height - x - 1)*LCD_Width + y) ) = color ;
            *(__IO uint8_t*)( LCD.LayerMemoryAdd + 3*((LCD_Height - x - 1)*LCD_Width + y) +2) = color>>16 ;
        }
    }
    else
    {
        if (LCD.Direction == Direction_H)
        {
            *(__IO uint16_t*)( LCD.LayerMemoryAdd + 2*(x + y*LCD_Width) ) = color ;
        }
        else if(LCD.Direction == Direction_V)
        {
            *(__IO uint16_t*)( LCD.LayerMemoryAdd + 2*((LCD_Height - x - 1)*LCD_Width + y) ) = color ;
        }
    }
}

/*************************************************************************************************
*   Function: LCD_ReadPoint
*   Parameters: x,y - coordinates
*   Return: Color value at specified point
*************************************************************************************************/

uint32_t LCD_ReadPoint(uint16_t x,uint16_t y)
{
    uint32_t color = 0;

    if( LCD.ColorMode == LTDC_PIXEL_FORMAT_ARGB8888 )
    {
        if (LCD.Direction == Direction_H)
        {
            color = *(__IO uint32_t*)( LCD.LayerMemoryAdd + 4*(x + y*LCD_Width) );
        }
        else if(LCD.Direction == Direction_V)
        {
            color = *(__IO uint32_t*)( LCD.LayerMemoryAdd + 4*((LCD_Height - x - 1)*LCD_Width + y) );
        }
    }
    else if ( LCD.ColorMode == LTDC_PIXEL_FORMAT_RGB888 )
    {
        if (LCD.Direction == Direction_H)
        {
            color = *(__IO uint32_t*)( LCD.LayerMemoryAdd + 3*(x + y*LCD_Width) ) &0x00ffffff;
        }
        else if(LCD.Direction == Direction_V)
        {
            color = *(__IO uint32_t*)( LCD.LayerMemoryAdd + 3*((LCD_Height - x - 1)*LCD_Width + y) ) &0x00ffffff;
        }
    }
    else
    {
        if (LCD.Direction == Direction_H)
        {
            color = *(__IO uint16_t*)( LCD.LayerMemoryAdd + 2*(x + y*LCD_Width) );
        }
        else if(LCD.Direction == Direction_V)
        {
            color = *(__IO uint16_t*)( LCD.LayerMemoryAdd + 2*((LCD_Height - x - 1)*LCD_Width + y) );
        }
    }
    return color;
}

/*************************************************************************************************
*   Function: LCD_DisplayChar
*   Parameters: x,y - coordinates, c - ASCII character, bgColor - background color
*   Description: Display single ASCII character
*************************************************************************************************/
void LCD_DisplayChar(uint16_t x, uint16_t y,uint8_t c, uint32_t bgColor)
{
    uint16_t  index = 0, counter = 0;
    uint8_t   disChar;
    uint16_t  Xaddress = x;
    uint32_t  bg = LCD_ConvertColor(bgColor);  

    c = c - 32;

    for(index = 0; index < LCD_Fonts->Sizes; index++)
    {
        disChar = LCD_Fonts->pTable[c*LCD_Fonts->Sizes + index];
        for(counter = 0; counter < 8; counter++)
        {
            if(disChar & 0x01)
            {
                LCD_DrawPoint(Xaddress,y,LCD.Color);
            }
            else
            {
                LCD_DrawPoint(Xaddress,y,bg);   // 改用转换后的颜色
            }
            disChar >>= 1;
            Xaddress++;

            if( (Xaddress - x)==LCD_Fonts->Width )
            {
                Xaddress = x;
                y++;
                break;
            }
        }
    }
}
/*************************************************************************************************
*   Function: LCD_DisplayString
*   Parameters: x,y - coordinates, p - string pointer, bgColor - background color
*   Description: Display ASCII string
*************************************************************************************************/

void LCD_DisplayString( uint16_t x, uint16_t y, char *p, uint32_t bgColor)
{
    while ((x < LCD_Width) && (*p != 0))
    {
         LCD_DisplayChar( x,y,*p, bgColor);
         x += LCD_Fonts->Width;
         p++;
    }
}

/*************************************************************************************************
*   Function: LCD_SetTextFont
*   Parameters: *fonts - Text font to set
*   Description: Set text font (Chinese and ASCII)
*************************************************************************************************/

void LCD_SetTextFont(pFONT *fonts)
{
    LCD_CHFonts = fonts;
    LCD_Fonts = fonts;
}

/*************************************************************************************************
*   Function: LCD_DisplayChinese
*   Parameters: x,y - coordinates, pText - Chinese character string, bgColor - background color
*   Description: Display single Chinese character
*************************************************************************************************/
void LCD_DisplayChinese(uint16_t x, uint16_t y, char *pText, uint32_t bgColor)
{
    uint16_t  i=0,index = 0, counter = 0;
    uint16_t  addr;
    uint8_t   disChar;
    uint16_t  Xaddress = x;
    uint32_t  bg = LCD_ConvertColor(bgColor);  

    while(1)
    {
        if ( *(LCD_CHFonts->pTable + (i+1)*LCD_CHFonts->Sizes + 0)==*pText && *(LCD_CHFonts->pTable + (i+1)*LCD_CHFonts->Sizes + 1)==*(pText+1) )
        {
            addr=i;
            break;
        }
        i+=2;

        if(i >= LCD_CHFonts->Table_Rows) break;
    }

    for(index = 0; index <LCD_CHFonts->Sizes; index++)
    {
        disChar = *(LCD_CHFonts->pTable + (addr)*LCD_CHFonts->Sizes + index);

        for(counter = 0; counter < 8; counter++)
        {
            if(disChar & 0x01)
            {
                LCD_DrawPoint(Xaddress,y,LCD.Color);
            }
            else
            {
                LCD_DrawPoint(Xaddress,y,bg);   // 改用转换后的颜色
            }
            disChar >>= 1;
            Xaddress++;

            if( (Xaddress - x)==LCD_CHFonts->Width )
            {
                Xaddress = x;
                y++;
                break;
            }
        }
    }
}

/*************************************************************************************************
*   Function: LCD_DisplayText
*   Parameters: x,y - coordinates, pText - text string, bgColor - background color
*   Description: Display string (Chinese and ASCII)
*************************************************************************************************/

void LCD_DisplayText(uint16_t x, uint16_t y, char *pText, uint32_t bgColor)
{
    while(*pText != 0)
    {
        if(*pText<=0x7F)
        {
            LCD_DisplayChar(x,y,*pText, bgColor);
            x+=LCD_Fonts->Width;
            pText++;
        }
        else
        {
            LCD_DisplayChinese(x,y,pText, bgColor);
            x+=LCD_CHFonts->Width;
            pText+=2;
        }
    }
}


/*************************************************************************************************
*   Function: LCD_ShowNumMode
*   Parameters: mode - Number display mode
*   Description: Set number display mode (fill with 0 or space)
*************************************************************************************************/

void LCD_ShowNumMode(uint8_t mode)
{
    LCD.ShowNum_Mode = mode;
}

/*************************************************************************************************
*   Function: LCD_DisplayNumber
*   Parameters: x,y - coordinates, number - integer value, len - length, bgColor - background color
*   Description: Display integer number
*************************************************************************************************/

void  LCD_DisplayNumber( uint16_t x, uint16_t y, int32_t number, uint8_t len, uint32_t bgColor)
{
    char   Number_Buffer[15];

    if( LCD.ShowNum_Mode == Fill_Zero)
    {
        sprintf( Number_Buffer , "%0.*d",len, number );
    }
    else
    {
        sprintf( Number_Buffer , "%*d",len, number );
    }

    LCD_DisplayString( x, y,(char *)Number_Buffer, bgColor) ;
}

/*************************************************************************************************
*   Function: LCD_DisplayDecimals
*   Parameters: x,y - coordinates, decimals - float value, len - length, decs - decimal places, bgColor - background color
*   Description: Display decimal number
*************************************************************************************************/

void  LCD_DisplayDecimals( uint16_t x, uint16_t y, double decimals, uint8_t len, uint8_t decs, uint32_t bgColor)
{
    char  Number_Buffer[20];

    if( LCD.ShowNum_Mode == Fill_Zero)
    {
        sprintf( Number_Buffer , "%0*.*lf",len,decs, decimals );
    }
    else
    {
        sprintf( Number_Buffer , "%*.*lf",len,decs, decimals );
    }

    LCD_DisplayString( x, y,(char *)Number_Buffer, bgColor) ;
}


/*************************************************************************************************
*   Function: LCD_DrawImage
*   Parameters: x,y - coordinates, width,height - image size, pImage - image data pointer
*   Description: Display image at specified position
*************************************************************************************************/

void    LCD_DrawImage(uint16_t x,uint16_t y,uint16_t width,uint16_t height,const uint8_t *pImage)
{
    uint8_t   disChar;
    uint16_t  Xaddress = x;
    uint16_t  i=0,j=0,m=0;

    for(i = 0; i <height; i++)
    {
        for(j = 0; j <(float)width/8; j++)
        {
            disChar = *pImage;

            for(m = 0; m < 8; m++)
            {
                if(disChar & 0x01)
                {
                    LCD_DrawPoint(Xaddress,y,LCD.Color);
                }
                else
                {
                    LCD_DrawPoint(Xaddress,y,LCD.BackColor);
                }
                disChar >>= 1;
                Xaddress++;

                if( (Xaddress - x)==width )
                {
                    Xaddress = x;
                    y++;
                    break;
                }
            }
            pImage++;
        }
    }
}


/*************************************************************************************************
*   Function: LCD_DrawLine
*   Parameters: x1,y1 - start coordinates, x2,y2 - end coordinates
*   Description: Draw line between two points
*************************************************************************************************/

#define ABS(X)  ((X) > 0 ? (X) : -(X))

void LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    int16_t deltax = 0, deltay = 0, x = 0, y = 0, xinc1 = 0, xinc2 = 0,
    yinc1 = 0, yinc2 = 0, den = 0, num = 0, numadd = 0, numpixels = 0,
    curpixel = 0;

    deltax = ABS(x2 - x1);
    deltay = ABS(y2 - y1);
    x = x1;
    y = y1;

    if (x2 >= x1)
    {
     xinc1 = 1;
     xinc2 = 1;
    }
    else
    {
     xinc1 = -1;
     xinc2 = -1;
    }

    if (y2 >= y1)
    {
     yinc1 = 1;
     yinc2 = 1;
    }
    else
    {
     yinc1 = -1;
     yinc2 = -1;
    }

    if (deltax >= deltay)
    {
     xinc1 = 0;
     yinc2 = 0;
     den = deltax;
     num = deltax / 2;
     numadd = deltay;
     numpixels = deltax;
    }
    else
    {
     xinc2 = 0;
     yinc1 = 0;
     den = deltay;
     num = deltay / 2;
     numadd = deltax;
     numpixels = deltay;
    }
    for (curpixel = 0; curpixel <= numpixels; curpixel++)
    {
     LCD_DrawPoint(x,y,LCD.Color);
     num += numadd;
     if (num >= den)
     {
        num -= den;
        x += xinc1;
        y += yinc1;
     }
     x += xinc2;
     y += yinc2;
    }
}

/*************************************************************************************************
*   Function: LCD_DrawRect
*   Parameters: x,y - coordinates, width,height - rectangle size
*   Description: Draw rectangle outline
*************************************************************************************************/

void LCD_DrawRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    LCD_DrawLine(x, y, x+width, y);
    LCD_DrawLine(x, y+height, x+width, y+height);
    LCD_DrawLine(x, y, x, y+height);
    LCD_DrawLine(x+width, y, x+width, y+height);
}

/*************************************************************************************************
*   Function: LCD_DrawCircle
*   Parameters: x,y - center coordinates, r - radius
*   Description: Draw circle outline
*************************************************************************************************/

void LCD_DrawCircle(uint16_t x, uint16_t y, uint16_t r)
{
    int Xadd = -r, Yadd = 0, err = 2-2*r, e2;
    do {

        LCD_DrawPoint(x-Xadd,y+Yadd,LCD.Color);
        LCD_DrawPoint(x+Xadd,y+Yadd,LCD.Color);
        LCD_DrawPoint(x+Xadd,y-Yadd,LCD.Color);
        LCD_DrawPoint(x-Xadd,y-Yadd,LCD.Color);

        e2 = err;
        if (e2 <= Yadd) {
            err += ++Yadd*2+1;
            if (-Xadd == Yadd && e2 <= Xadd) e2 = 0;
        }
        if (e2 > Xadd) err += ++Xadd*2+1;
    }
    while (Xadd <= 0);
}

/*************************************************************************************************
*   Function: LCD_DrawEllipse
*   Parameters: x,y - center coordinates, r1,r2 - ellipse radii
*   Description: Draw ellipse outline
*************************************************************************************************/

void LCD_DrawEllipse(int x, int y, int r1, int r2)
{
  int Xadd = -r1, Yadd = 0, err = 2-2*r1, e2;
  float K = 0, rad1 = 0, rad2 = 0;

  rad1 = r1;
  rad2 = r2;

  if (r1 > r2)
  {
    do {
      K = (float)(rad1/rad2);

        LCD_DrawPoint(x-Xadd,y+(uint16_t)(Yadd/K),LCD.Color);
        LCD_DrawPoint(x+Xadd,y+(uint16_t)(Yadd/K),LCD.Color);
        LCD_DrawPoint(x+Xadd,y-(uint16_t)(Yadd/K),LCD.Color);
        LCD_DrawPoint(x-Xadd,y-(uint16_t)(Yadd/K),LCD.Color);

      e2 = err;
      if (e2 <= Yadd) {
        err += ++Yadd*2+1;
        if (-Xadd == Yadd && e2 <= Xadd) e2 = 0;
      }
      if (e2 > Xadd) err += ++Xadd*2+1;
    }
    while (Xadd <= 0);
  }
  else
  {
    Yadd = -r2;
    Xadd = 0;
    do {
      K = (float)(rad2/rad1);

        LCD_DrawPoint(x-(uint16_t)(Xadd/K),y+Yadd,LCD.Color);
        LCD_DrawPoint(x+(uint16_t)(Xadd/K),y+Yadd,LCD.Color);
        LCD_DrawPoint(x+(uint16_t)(Xadd/K),y-Yadd,LCD.Color);
        LCD_DrawPoint(x-(uint16_t)(Xadd/K),y-Yadd,LCD.Color);

      e2 = err;
      if (e2 <= Xadd) {
        err += ++Xadd*3+1;
        if (-Yadd == Xadd && e2 <= Yadd) e2 = 0;
      }
      if (e2 > Yadd) err += ++Yadd*3+1;
    }
    while (Yadd <= 0);
  }
}

/*************************************************************************************************
*   Function: LCD_FillRect
*   Parameters: x,y - coordinates, width,height - rectangle size
*   Description: Fill rectangle using DMA2D
*************************************************************************************************/

void LCD_FillRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    DMA2D->CR     &= ~(DMA2D_CR_START);
    DMA2D->CR      = DMA2D_R2M;
    DMA2D->OPFCCR  = LCD.ColorMode;
    DMA2D->OCOLR   = LCD.Color;

    if(LCD.Direction == Direction_H)
    {
        DMA2D->OOR    = LCD_Width - width;
        DMA2D->OMAR   = LCD.LayerMemoryAdd + LCD.BytesPerPixel*(LCD_Width * y + x);
        DMA2D->NLR    = (width<<16)|(height);
    }
    else
    {
        DMA2D->OOR    = LCD_Width - height;
        DMA2D->OMAR   = LCD.LayerMemoryAdd + LCD.BytesPerPixel*((LCD_Height - x - 1 - width)*LCD_Width + y);
        DMA2D->NLR    = (width)|(height<<16);
    }

    DMA2D->CR     |= DMA2D_CR_START;

    while (DMA2D->CR & DMA2D_CR_START) ;
}

/*************************************************************************************************
*   Function: LCD_FillCircle
*   Parameters: x,y - center coordinates, r - radius
*   Description: Fill circle
*************************************************************************************************/

void LCD_FillCircle(uint16_t x, uint16_t y, uint16_t r)
{
  int32_t  D;     /* Decision Variable */
  uint32_t  CurX; /* Current X Value */
  uint32_t  CurY; /* Current Y Value */

  D = 3 - (r << 1);
  CurX = 0;
  CurY = r;

  while (CurX <= CurY)
  {
    if(LCD.Direction == Direction_H)
    {
      LCD_DrawLine(x + CurX, y - CurY, x + CurX, y + CurY);
      LCD_DrawLine(x - CurX, y - CurY, x - CurX, y + CurY);
      LCD_DrawLine(x + CurY, y - CurX, x + CurY, y + CurX);
      LCD_DrawLine(x - CurY, y - CurX, x - CurY, y + CurX);
    }
    else
    {
      LCD_DrawLine(x - CurY, y + CurX, x + CurY, y + CurX);
      LCD_DrawLine(x - CurY, y - CurX, x + CurY, y - CurX);
      LCD_DrawLine(x - CurX, y + CurY, x + CurX, y + CurY);
      LCD_DrawLine(x - CurX, y - CurY, x + CurX, y - CurY);
    }
    if (D < 0)
    {
      D += (CurX << 2) + 6;
    }
    else
    {
      D += ((CurX - CurY) << 2) + 10;
      CurY--;
    }
    CurX++;
  }

  LCD_DrawCircle(x, y, r);
}

/* USER CODE END 1 */