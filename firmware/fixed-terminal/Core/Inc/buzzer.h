#ifndef __BUZZER_H
#define __BUZZER_H

#include "main.h"
#include "stm32h7xx_hal.h"

#define BUZZER_PORT   GPIOD
#define BUZZER_PIN    GPIO_PIN_7

/* 初始化蜂鸣器（GPIO + TIM7 中断） */
void Buzzer_Init(void);

/* 开启鸣叫（定时器会自动产生方波） */
void Buzzer_On(void);

/* 关闭鸣叫 */
void Buzzer_Off(void);

/* 设置频率，hz 建议 500~4000 */
void Buzzer_SetFreq(uint16_t hz);

/* 查询当前状态 */
uint8_t Buzzer_IsOn(void);

#endif