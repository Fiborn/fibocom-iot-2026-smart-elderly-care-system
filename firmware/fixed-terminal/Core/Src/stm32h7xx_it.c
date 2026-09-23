/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32h7xx_it.c
  * @brief   Interrupt Service Routines.
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
#include "stm32h7xx_it.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern DMA_HandleTypeDef hdma_dcmi;
extern DCMI_HandleTypeDef hdcmi;
extern SD_HandleTypeDef hsd1;
/* USER CODE BEGIN EV */

#include "dcmi.h"  

/* Private types -------------------------------------------------------------*/
typedef struct
{
  __IO uint32_t ISR;   /*!< DMA interrupt status register */
  __IO uint32_t Reserved0;
  __IO uint32_t IFCR;  /*!< DMA interrupt flag clear register */
} DMA_Registers;


/* USER CODE END EV */

/******************************************************************************/
/*           Cortex Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
   while (1)
  {
  }
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
  /* USER CODE BEGIN HardFault_IRQn 0 */

  /* USER CODE END HardFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_HardFault_IRQn 0 */
    /* USER CODE END W1_HardFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */

  /* USER CODE END MemoryManagement_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
    /* USER CODE END W1_MemoryManagement_IRQn 0 */
  }
}

/**
  * @brief This function handles Pre-fetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  /* USER CODE BEGIN BusFault_IRQn 0 */

  /* USER CODE END BusFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_BusFault_IRQn 0 */
    /* USER CODE END W1_BusFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */

  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
    /* USER CODE END W1_UsageFault_IRQn 0 */
  }
}

/**
  * @brief This function handles System service call via SWI instruction.
  */
void SVC_Handler(void)
{
  /* USER CODE BEGIN SVCall_IRQn 0 */

  /* USER CODE END SVCall_IRQn 0 */
  /* USER CODE BEGIN SVCall_IRQn 1 */

  /* USER CODE END SVCall_IRQn 1 */
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */

  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /* USER CODE END DebugMonitor_IRQn 1 */
}

/**
  * @brief This function handles Pendable request for system service.
  */
void PendSV_Handler(void)
{
  /* USER CODE BEGIN PendSV_IRQn 0 */

  /* USER CODE END PendSV_IRQn 0 */
  /* USER CODE BEGIN PendSV_IRQn 1 */

  /* USER CODE END PendSV_IRQn 1 */
}

/**
  * @brief This function handles System tick timer.
  */
void SysTick_Handler(void)
{
  /* USER CODE BEGIN SysTick_IRQn 0 */

  /* USER CODE END SysTick_IRQn 0 */
  HAL_IncTick();
  /* USER CODE BEGIN SysTick_IRQn 1 */

  /* USER CODE END SysTick_IRQn 1 */
}

/******************************************************************************/
/* STM32H7xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32h7xx.s).                    */
/******************************************************************************/

/**
  * @brief This function handles SDMMC1 global interrupt.
  */
void SDMMC1_IRQHandler(void)
{
  /* USER CODE BEGIN SDMMC1_IRQn 0 */

  /* USER CODE END SDMMC1_IRQn 0 */
  HAL_SD_IRQHandler(&hsd1);
  /* USER CODE BEGIN SDMMC1_IRQn 1 */

  /* USER CODE END SDMMC1_IRQn 1 */
}

/**
  * @brief This function handles DMA2 stream7 global interrupt.
  */
void DMA2_Stream7_IRQHandler(void)
{
  /* USER CODE BEGIN DMA2_Stream7_IRQn 0 */

  uint32_t tmpisr_dma;

//HAL ���DMA�Ļص��������Ÿ��û���DCMIʹ���ˣ����������ֱ������DMA�жϽ������ݵĴ���
//>>>>>>>> �����Զ����жϴ���	
  DMA_Registers  *regs_dma  = (DMA_Registers *)(&hdma_dcmi)->StreamBaseAddress;
  tmpisr_dma  = regs_dma->ISR;	

	if (( tmpisr_dma & (DMA_FLAG_TCIF0_4 << ((&hdma_dcmi)->StreamIndex & 0x1FU))) != 0U)	// 检查标志
		{
      if(__HAL_DMA_GET_IT_SOURCE(&hdma_dcmi, DMA_IT_TC) != 0U)
      {		
			if (g_current_camera_width == 640)
			{
				for (uint32_t row = 0; row < DMA_Height; row++)
				{
					uint32_t src_offset = row * g_current_camera_width * 2;
					uint32_t dst_offset = (OV5640_LineCount * DMA_Height + row) * Display_Width * 2;
					memcpy((uint16_t *)(Frame_Buffer + dst_offset),
						   (uint16_t *)(Camera_Buffer + src_offset),
						   g_current_camera_width * 2);
				}

				OV5640_LineCount++;

				if (OV5640_LineCount >= (Display_Height / DMA_Height))
				{
					OV5640_LineCount = 0;
				}
			}
			else
			{
				memcpy((uint16_t *)Frame_Buffer,
					   (uint16_t *)Camera_Buffer,
					   g_current_camera_width * Display_Height * 2);
			}
		}
		}			
	
  /* USER CODE END DMA2_Stream7_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_dcmi);
  /* USER CODE BEGIN DMA2_Stream7_IRQn 1 */

  /* USER CODE END DMA2_Stream7_IRQn 1 */
}

/**
  * @brief This function handles DCMI global interrupt.
  */
void DCMI_IRQHandler(void)
{
  /* USER CODE BEGIN DCMI_IRQn 0 */

  /* USER CODE END DCMI_IRQn 0 */
  HAL_DCMI_IRQHandler(&hdcmi);
  /* USER CODE BEGIN DCMI_IRQn 1 */

  /* USER CODE END DCMI_IRQn 1 */
}

/* USER CODE BEGIN 1 */
#include "jpeg_encode.h"
#include "cloud_upload.h"

extern UART_HandleTypeDef huart4;

void JPEG_IRQHandler(void)
{
    HAL_JPEG_IRQHandler(&jpeg_encode_ctx.hjpeg);
}

void MDMA_IRQHandler(void)
{
    HAL_MDMA_IRQHandler(jpeg_encode_ctx.hjpeg.hdmain);
    HAL_MDMA_IRQHandler(jpeg_encode_ctx.hjpeg.hdmaout);
}

void UART4_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart4);
}
//void USART3_IRQHandler(void)
//{
//    HAL_UART_IRQHandler(&huart3);
//}

//#define K230_BUF_LEN 128
//extern uint8_t  g_k230_rx_byte;
//extern char     g_k230_result[K230_BUF_LEN];
//extern uint8_t  g_k230_new_data;
//extern uint8_t  g_k230_buf[K230_BUF_LEN];
//extern uint16_t g_k230_buf_len;
//extern uint32_t g_k230_last_rx_time;
//extern uint8_t  g_k230_rx_state;


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == UART4)
    {
        Cloud_UART_RxCallback(huart);
    }   
//		if (huart->Instance == USART3)
//    {
//    if (g_k230_buf_len < K230_BUF_LEN - 1)
//        {
//            g_k230_result[g_k230_buf_len++] = g_k230_rx_byte;
//            g_k230_result[g_k230_buf_len] = '\0';
//        }
//        g_k230_last_rx_time = HAL_GetTick();
//        HAL_UART_Receive_IT(&huart3, &g_k230_rx_byte, 1);
//    }
		
}
/* USER CODE END 1 */
