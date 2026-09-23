#ifndef __SAI_H
#define __SAI_H

#include "stm32h7xx_hal.h"

extern SAI_HandleTypeDef hsai_BlockB1;
extern DMA_HandleTypeDef hdma_sai1_b;

void MX_SAI1_Init(uint32_t sampleRate);
void SAI_Play(uint16_t *pBuffer, uint32_t Size);
void SAI_Stop(void);
void SAI_Pause(void);
void SAI_Resume(void);

#endif