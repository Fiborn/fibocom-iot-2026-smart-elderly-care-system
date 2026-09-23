#ifndef DHT11_H
#define DHT11_H

#include "stm32h7xx_hal.h"   

/* 定义DHT11连接的引脚和端口 */
#define DHT11_PORT          GPIOA
#define DHT11_PIN           GPIO_PIN_15

/* 引脚操作宏 */
#define DHT11_DQ_OUT(x)     HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, (x) ? GPIO_PIN_SET : GPIO_PIN_RESET)
#define DHT11_DQ_READ()     HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN)

/* 方向切换函数 */
void DHT11_IO_OUT(void);
void DHT11_IO_IN(void);

/* 原有功能函数声明 */
uint8_t DHT11_Init(void);
uint8_t DHT11_Read_Data(uint8_t *temp, uint8_t *humi);
uint8_t DHT11_Read_Byte(void);
uint8_t DHT11_Read_Bit(void);
uint8_t DHT11_Check(void);
void DHT11_Rst(void);

#endif