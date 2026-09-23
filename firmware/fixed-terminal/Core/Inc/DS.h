#ifndef __DS1302_H
#define __DS1302_H

#include "stm32h7xx_hal.h"

#define DS1302_ReadTime  ds1302_read_time
#define DS1302_WriteTime ds1302_write_time

void Parse_RTC_Time(uint16_t *year, uint8_t *mon, uint8_t *day,
                    uint8_t *hour, uint8_t *min, uint8_t *sec, uint8_t *week);
/* 引脚定义 */
#define DS1302_RST_PORT    GPIOB
#define DS1302_RST_PIN     GPIO_PIN_12
#define DS1302_SCK_PORT    GPIOB
#define DS1302_SCK_PIN     GPIO_PIN_13
#define DS1302_IO_PORT     GPIOB
#define DS1302_IO_PIN      GPIO_PIN_14

#define DS1302_RST_H()     HAL_GPIO_WritePin(DS1302_RST_PORT, DS1302_RST_PIN, GPIO_PIN_SET)
#define DS1302_RST_L()     HAL_GPIO_WritePin(DS1302_RST_PORT, DS1302_RST_PIN, GPIO_PIN_RESET)
#define DS1302_SCK_H()     HAL_GPIO_WritePin(DS1302_SCK_PORT, DS1302_SCK_PIN, GPIO_PIN_SET)
#define DS1302_SCK_L()     HAL_GPIO_WritePin(DS1302_SCK_PORT, DS1302_SCK_PIN, GPIO_PIN_RESET)
#define DS1302_IO_H()      HAL_GPIO_WritePin(DS1302_IO_PORT, DS1302_IO_PIN, GPIO_PIN_SET)
#define DS1302_IO_L()      HAL_GPIO_WritePin(DS1302_IO_PORT, DS1302_IO_PIN, GPIO_PIN_RESET)
#define DS1302_IO_READ()   HAL_GPIO_ReadPin(DS1302_IO_PORT, DS1302_IO_PIN)

/* 寄存器地址 */
#define ds1302_sec_add      0x80
#define ds1302_min_add      0x82
#define ds1302_hr_add       0x84
#define ds1302_date_add     0x86
#define ds1302_month_add    0x88
#define ds1302_day_add      0x8A
#define ds1302_year_add     0x8C
#define ds1302_control_add  0x8E

extern uint8_t time_buf[8];

void DS1302_Init(void);
void ds1302_write_byte(uint8_t addr, uint8_t d);
uint8_t ds1302_read_byte(uint8_t addr);
void ds1302_write_time(void);
void ds1302_read_time(void);
void ds1302_init_with_check();
#endif