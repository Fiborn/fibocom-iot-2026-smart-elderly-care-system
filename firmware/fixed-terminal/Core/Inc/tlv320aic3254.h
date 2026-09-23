#ifndef __TLV320AIC3254_H
#define __TLV320AIC3254_H

#include "stm32h7xx_hal.h"

HAL_StatusTypeDef TLV320AIC3254_Init(void);
HAL_StatusTypeDef TLV320AIC3254_WriteRegister(uint8_t page, uint8_t reg, uint8_t value);
uint8_t TLV320AIC3254_ReadRegister(uint8_t page, uint8_t reg);
HAL_StatusTypeDef TLV320AIC3254_SetVolume(uint8_t volume);
HAL_StatusTypeDef TLV320AIC3254_SetSampleRate(uint32_t sampleRate);

#endif