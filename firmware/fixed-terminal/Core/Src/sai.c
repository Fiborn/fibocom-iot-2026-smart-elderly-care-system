#include "sai.h"
#include <stdio.h>
#include "wav_player.h"
extern void Error_Handler(void);

SAI_HandleTypeDef hsai_BlockB1;
DMA_HandleTypeDef hdma_sai1_b;

/**
  * @brief  SAI1初始化
  * @note   MCLK和BCLK短接，SAI不输出MCLK引脚
  *         STM32作为I2S Master，输出SCK(BCLK)和FS(WCLK)
  *         BCLK = 44100 * 32 = 1.4112 MHz
  */
void MX_SAI1_Init(uint32_t sampleRate)
{
    hsai_BlockB1.Instance = SAI1_Block_B;
    HAL_SAI_DeInit(&hsai_BlockB1);
    
    hsai_BlockB1.Instance = SAI1_Block_B;
    hsai_BlockB1.Init.AudioMode = SAI_MODEMASTER_TX;
    hsai_BlockB1.Init.Synchro = SAI_ASYNCHRONOUS;
    hsai_BlockB1.Init.OutputDrive = SAI_OUTPUTDRIVE_ENABLE;
    hsai_BlockB1.Init.FIFOThreshold = SAI_FIFOTHRESHOLD_1QF;
    hsai_BlockB1.Init.SynchroExt = SAI_SYNCEXT_DISABLE;
    hsai_BlockB1.Init.MonoStereoMode = SAI_STEREOMODE;
    hsai_BlockB1.Init.CompandingMode = SAI_NOCOMPANDING;
    hsai_BlockB1.Init.TriState = SAI_OUTPUT_NOTRELEASED;
    
    /* 关键：不输出MCLK，使用MasterDivider来产生正确的SCK */
    hsai_BlockB1.Init.MckOverSampling = SAI_MCK_OVERSAMPLING_DISABLE;
    hsai_BlockB1.Init.NoDivider = SAI_MASTERDIVIDER_ENABLE;
    hsai_BlockB1.Init.AudioFrequency = sampleRate;
    
    /* 协议配置 */
    hsai_BlockB1.Init.Protocol = SAI_FREE_PROTOCOL;
    hsai_BlockB1.Init.DataSize = SAI_DATASIZE_16;
    hsai_BlockB1.Init.FirstBit = SAI_FIRSTBIT_MSB;
    hsai_BlockB1.Init.ClockStrobing = SAI_CLOCKSTROBING_FALLINGEDGE;
    
    /* 帧配置 - I2S标准 Philips格式 */
    hsai_BlockB1.FrameInit.FrameLength = 32;
    hsai_BlockB1.FrameInit.ActiveFrameLength = 16;
    hsai_BlockB1.FrameInit.FSDefinition = SAI_FS_CHANNEL_IDENTIFICATION;
    hsai_BlockB1.FrameInit.FSPolarity = SAI_FS_ACTIVE_LOW;
    hsai_BlockB1.FrameInit.FSOffset = SAI_FS_BEFOREFIRSTBIT;
    
    /* Slot配置 */
    hsai_BlockB1.SlotInit.FirstBitOffset = 0;
    hsai_BlockB1.SlotInit.SlotSize = SAI_SLOTSIZE_DATASIZE;
    hsai_BlockB1.SlotInit.SlotNumber = 2;
    hsai_BlockB1.SlotInit.SlotActive = SAI_SLOTACTIVE_0 | SAI_SLOTACTIVE_1;
    
    if (HAL_SAI_Init(&hsai_BlockB1) != HAL_OK)
    {
        printf("SAI HAL_SAI_Init Error!\r\n");
        Error_Handler();
    }
    
    /* 禁用WCKCFG错误中断，因为我们不输出MCLK */
    __HAL_SAI_DISABLE_IT(&hsai_BlockB1, SAI_IT_WCKCFG);
    
    printf("SAI Init OK, SampleRate = %lu\r\n", sampleRate);
}

void HAL_SAI_MspInit(SAI_HandleTypeDef* saiHandle)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
    
    if(saiHandle->Instance == SAI1_Block_B)
    {
        /* SAI_CLK = 45.1584 MHz (= 4 × 256 × 44100)，MCKDIV=4 精确得到 fs=44100 */
        PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SAI1;
        PeriphClkInitStruct.Sai1ClockSelection = RCC_SAI1CLKSOURCE_PLL2;
        PeriphClkInitStruct.PLL2.PLL2M = 5;      /* 25/5 = 5 MHz 输入 */
        PeriphClkInitStruct.PLL2.PLL2N = 144;    /* VCO 整数部分 */
        PeriphClkInitStruct.PLL2.PLL2P = 16;     /* SAI_CLK = VCO/16 */
        PeriphClkInitStruct.PLL2.PLL2Q = 2;
        PeriphClkInitStruct.PLL2.PLL2R = 2;
        PeriphClkInitStruct.PLL2.PLL2RGE = RCC_PLL2VCIRANGE_2;   /* 4–8 MHz 输入 */
        PeriphClkInitStruct.PLL2.PLL2VCOSEL = RCC_PLL2VCOWIDE;   /* 192–836 MHz */
        PeriphClkInitStruct.PLL2.PLL2FRACN = 4153;  /* 小数部分,得到 722.5344 MHz VCO */
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
        {
            printf("SAI PLL2 Clock Config Error!\r\n");
            Error_Handler();
        }
        
        /* 使能外设时钟 */
        __HAL_RCC_SAI1_CLK_ENABLE();
        __HAL_RCC_GPIOE_CLK_ENABLE();
        __HAL_RCC_GPIOF_CLK_ENABLE();
        __HAL_RCC_DMA1_CLK_ENABLE();
        
        /**
         * GPIO配置 (SAI1 Block B):
         * PF9 - SAI1_FS_B   (WCLK/FSYNC)
         * PF8 - SAI1_SCK_B  (BCLK) → 硬件上同时连接TLV320的MCLK和BCLK
         * PE3 - SAI1_SD_B   (DIN到TLV320)
         */
        /* PF8, PF9 */
        GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF6_SAI1;
        HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
        
        /* PE3 */
        GPIO_InitStruct.Pin = GPIO_PIN_3;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF6_SAI1;
        HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
        
        /* DMA配置 - DMA1 Stream1 用于 SAI1_B */
        hdma_sai1_b.Instance = DMA1_Stream1;
        hdma_sai1_b.Init.Request = DMA_REQUEST_SAI1_B;
        hdma_sai1_b.Init.Direction = DMA_MEMORY_TO_PERIPH;
        hdma_sai1_b.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_sai1_b.Init.MemInc = DMA_MINC_ENABLE;
        hdma_sai1_b.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
        hdma_sai1_b.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
        hdma_sai1_b.Init.Mode = DMA_CIRCULAR;
        hdma_sai1_b.Init.Priority = DMA_PRIORITY_HIGH;
        hdma_sai1_b.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
        hdma_sai1_b.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
        hdma_sai1_b.Init.MemBurst = DMA_MBURST_SINGLE;
        hdma_sai1_b.Init.PeriphBurst = DMA_PBURST_SINGLE;
        
        if (HAL_DMA_Init(&hdma_sai1_b) != HAL_OK)
        {
            printf("SAI DMA Init Error!\r\n");
            Error_Handler();
        }
        
        __HAL_LINKDMA(saiHandle, hdmatx, hdma_sai1_b);
        
        /* 中断配置 */
        HAL_NVIC_SetPriority(DMA1_Stream1_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(DMA1_Stream1_IRQn);
        
        HAL_NVIC_SetPriority(SAI1_IRQn, 6, 0);
        HAL_NVIC_EnableIRQ(SAI1_IRQn);
    }
}

void HAL_SAI_MspDeInit(SAI_HandleTypeDef* saiHandle)
{
    if(saiHandle->Instance == SAI1_Block_B)
    {
        __HAL_RCC_SAI1_CLK_DISABLE();
        HAL_GPIO_DeInit(GPIOF, GPIO_PIN_8 | GPIO_PIN_9);
        HAL_GPIO_DeInit(GPIOE, GPIO_PIN_3);
        HAL_DMA_DeInit(saiHandle->hdmatx);
        HAL_NVIC_DisableIRQ(DMA1_Stream1_IRQn);
        HAL_NVIC_DisableIRQ(SAI1_IRQn);
    }
}

extern WAV_HeaderTypeDef wavHeader;
void SAI_Play(uint16_t *pBuffer, uint32_t Size)
{
    HAL_StatusTypeDef status;
    
    printf("SAI_Play: Buffer=0x%08lX, Size=%lu halfwords\r\n", (uint32_t)pBuffer, Size);
    printf("SAI State: %d\r\n", (int)hsai_BlockB1.State);
    
    /* 如果SAI不在READY状态，强制复位 */
    if(hsai_BlockB1.State != HAL_SAI_STATE_READY)
    {
        printf("SAI not ready (state=%d), forcing reset...\r\n", (int)hsai_BlockB1.State);
        HAL_SAI_DMAStop(&hsai_BlockB1);
        HAL_SAI_DeInit(&hsai_BlockB1);
        HAL_Delay(2);
        MX_SAI1_Init(wavHeader.SampleRate);
    }
    
    SCB_CleanDCache_by_Addr((uint32_t*)pBuffer, Size * 2);
    
    /* 清除SAI错误标志 */
    __HAL_SAI_CLEAR_FLAG(&hsai_BlockB1, SAI_FLAG_OVRUDR | SAI_FLAG_WCKCFG);
    
    status = HAL_SAI_Transmit_DMA(&hsai_BlockB1, (uint8_t *)pBuffer, Size);
    if(status != HAL_OK)
    {
        printf("SAI_Play HAL Error: %d, SAI Error: 0x%lX\r\n", status, hsai_BlockB1.ErrorCode);
        printf("SAI SR: 0x%08lX\r\n", hsai_BlockB1.Instance->SR);
    }
    else
    {
        printf("SAI DMA started successfully\r\n");
    }
}

void SAI_Stop(void)
{
    HAL_SAI_DMAStop(&hsai_BlockB1);
}

void SAI_Pause(void)
{
    HAL_SAI_DMAPause(&hsai_BlockB1);
}

void SAI_Resume(void)
{
    HAL_SAI_DMAResume(&hsai_BlockB1);
}

/* 中断处理函数 */
void DMA1_Stream1_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_sai1_b);
}

void SAI1_IRQHandler(void)
{
    HAL_SAI_IRQHandler(&hsai_BlockB1);
}