/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file         stm32h7xx_hal_msp.c
  * @brief        This file provides code for the MSP Initialization
  *               and de-Initialization codes.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN Define */

/* USER CODE END Define */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN Macro */

/* USER CODE END Macro */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* External functions --------------------------------------------------------*/
/* USER CODE BEGIN ExternalFunctions */

/* USER CODE END ExternalFunctions */

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */
/**
  * Initializes the Global MSP.
  */
void HAL_MspInit(void)
{

  /* USER CODE BEGIN MspInit 0 */

  /* USER CODE END MspInit 0 */

  __HAL_RCC_SYSCFG_CLK_ENABLE();

  /* System interrupt init*/

  /* USER CODE BEGIN MspInit 1 */

  /* USER CODE END MspInit 1 */
}

/* USER CODE BEGIN 1 */

void HAL_JPEG_MspInit(JPEG_HandleTypeDef *hjpeg)
{
    static MDMA_HandleTypeDef hmdmaIn;
    static MDMA_HandleTypeDef hmdmaOut;

    __HAL_RCC_JPGDECEN_CLK_ENABLE();
    __HAL_RCC_MDMA_CLK_ENABLE();

    HAL_NVIC_SetPriority(JPEG_IRQn, 0x07, 0x0F);
    HAL_NVIC_EnableIRQ(JPEG_IRQn);

    hmdmaIn.Init.Priority = MDMA_PRIORITY_HIGH;
    hmdmaIn.Init.Endianness = MDMA_LITTLE_ENDIANNESS_PRESERVE;
    hmdmaIn.Init.SourceInc = MDMA_SRC_INC_BYTE;
    hmdmaIn.Init.DestinationInc = MDMA_DEST_INC_DISABLE;
    hmdmaIn.Init.SourceDataSize = MDMA_SRC_DATASIZE_BYTE;
    hmdmaIn.Init.DestDataSize = MDMA_DEST_DATASIZE_WORD;
    hmdmaIn.Init.DataAlignment = MDMA_DATAALIGN_PACKENABLE;
    hmdmaIn.Init.SourceBurst = MDMA_SOURCE_BURST_32BEATS;
    hmdmaIn.Init.DestBurst = MDMA_DEST_BURST_16BEATS;
    hmdmaIn.Init.SourceBlockAddressOffset = 0;
    hmdmaIn.Init.DestBlockAddressOffset = 0;
    hmdmaIn.Init.Request = MDMA_REQUEST_JPEG_INFIFO_TH;
    hmdmaIn.Init.TransferTriggerMode = MDMA_BUFFER_TRANSFER;
    hmdmaIn.Init.BufferTransferLength = 32;

    hmdmaIn.Instance = MDMA_Channel7;

    __HAL_LINKDMA(hjpeg, hdmain, hmdmaIn);
    HAL_MDMA_DeInit(&hmdmaIn);
    HAL_MDMA_Init(&hmdmaIn);

    hmdmaOut.Init.Priority = MDMA_PRIORITY_VERY_HIGH;
    hmdmaOut.Init.Endianness = MDMA_LITTLE_ENDIANNESS_PRESERVE;
    hmdmaOut.Init.SourceInc = MDMA_SRC_INC_DISABLE;
    hmdmaOut.Init.DestinationInc = MDMA_DEST_INC_BYTE;
    hmdmaOut.Init.SourceDataSize = MDMA_SRC_DATASIZE_WORD;
    hmdmaOut.Init.DestDataSize = MDMA_DEST_DATASIZE_BYTE;
    hmdmaOut.Init.DataAlignment = MDMA_DATAALIGN_PACKENABLE;
    hmdmaOut.Init.SourceBurst = MDMA_SOURCE_BURST_32BEATS;
    hmdmaOut.Init.DestBurst = MDMA_DEST_BURST_32BEATS;
    hmdmaOut.Init.SourceBlockAddressOffset = 0;
    hmdmaOut.Init.DestBlockAddressOffset = 0;
    hmdmaOut.Init.Request = MDMA_REQUEST_JPEG_OUTFIFO_TH;
    hmdmaOut.Init.TransferTriggerMode = MDMA_BUFFER_TRANSFER;
    hmdmaOut.Init.BufferTransferLength = 32;

    hmdmaOut.Instance = MDMA_Channel6;
    HAL_MDMA_DeInit(&hmdmaOut);
    HAL_MDMA_Init(&hmdmaOut);

    __HAL_LINKDMA(hjpeg, hdmaout, hmdmaOut);

    HAL_NVIC_SetPriority(MDMA_IRQn, 0x08, 0x0F);
    HAL_NVIC_EnableIRQ(MDMA_IRQn);
}

void HAL_JPEG_MspDeInit(JPEG_HandleTypeDef *hjpeg)
{
    HAL_NVIC_DisableIRQ(MDMA_IRQn);
    HAL_MDMA_DeInit(hjpeg->hdmain);
    HAL_MDMA_DeInit(hjpeg->hdmaout);
}

/* USER CODE END 1 */
