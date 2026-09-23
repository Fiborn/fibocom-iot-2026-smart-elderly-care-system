#ifndef __DELAY_H
#define __DELAY_H

#include "stm32h7xx_hal.h"

void delay_init(uint16_t sysclk);
void delay_us(uint32_t us);

#endif