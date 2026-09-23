#ifndef __SENSORS_H__
#define __SENSORS_H__
#include "main.h"
#include "stm32h7xx_hal.h"

/* ============ 语音指令输入引脚（分布在 GPIOA 和 GPIOB） ============ */

/* ==== K230 跌倒报警输入引脚 ==== */
#define K230_FALL_PORT           GPIOD
#define K230_FALL_PIN            GPIO_PIN_13
#define K230_FALL_READ()         HAL_GPIO_ReadPin(K230_FALL_PORT, K230_FALL_PIN)

/* --- GPIOA 部分 (cmd1, cmd2) --- */
#define VOICE_IN_TIME_PORT      GPIOA
#define VOICE_IN_TIME_PIN       GPIO_PIN_11

#define VOICE_IN_WEATHER_PORT   GPIOA
#define VOICE_IN_WEATHER_PIN    GPIO_PIN_12

/* --- GPIOB 部分 (cmd3, cmd4, cmd5) --- */
#define VOICE_IN_OPERA_PORT     GPIOB
#define VOICE_IN_OPERA_PIN      GPIO_PIN_4

#define VOICE_IN_SONG_PORT      GPIOB
#define VOICE_IN_SONG_PIN       GPIO_PIN_5

#define VOICE_IN_STOP_PORT      GPIOB
#define VOICE_IN_STOP_PIN       GPIO_PIN_15

void Voice_Input_Init(void);

/* ============== 火焰传感器 ============== */
#define FLAME_DO_PORT         GPIOB
#define FLAME_DO_PIN          GPIO_PIN_0
#define FLAME_DO_READ()       HAL_GPIO_ReadPin(FLAME_DO_PORT, FLAME_DO_PIN)

/* ============== MQ-2 烟雾传感器 (DO改为PB3) ============== */
#define SMOKE_DO_PORT         GPIOB
#define SMOKE_DO_PIN          GPIO_PIN_3
#define SMOKE_DO_READ()       HAL_GPIO_ReadPin(SMOKE_DO_PORT, SMOKE_DO_PIN)

/* ============== 雨滴/漏水裸板 ============== */
#define RAIN_DETECT_PORT      GPIOB
#define RAIN_DETECT_PIN       GPIO_PIN_2
#define RAIN_READ()           HAL_GPIO_ReadPin(RAIN_DETECT_PORT, RAIN_DETECT_PIN)

/* 报警阈值 */
#define FLAME_AO_THRESHOLD    30000
#define SMOKE_AO_THRESHOLD    20000

/* ADC通道索引 */
#define ADC_CH_FLAME          0
#define ADC_CH_SMOKE          1

/* ============== 继电器控制输出 (PA5) ============== */
#define RELAY_PORT            GPIOA
#define RELAY_PIN             GPIO_PIN_5
#define RELAY_ON()            HAL_GPIO_WritePin(RELAY_PORT, RELAY_PIN, GPIO_PIN_SET)
#define RELAY_OFF()           HAL_GPIO_WritePin(RELAY_PORT, RELAY_PIN, GPIO_PIN_RESET)


///* ============ K230 相关（只做 extern 声明！） ============ */
#define K230_BUF_LEN 128

//extern uint8_t  g_k230_rx_byte;
//extern char     g_k230_result[K230_BUF_LEN];
//extern uint8_t  g_k230_new_data;
//extern uint8_t  g_k230_buf[K230_BUF_LEN];
//extern uint16_t g_k230_buf_len;
//extern uint32_t g_k230_last_rx_time;
//extern uint8_t  g_k230_rx_state;

void Sensors_GPIO_Init(void);
void Sensors_ADC_Init(void);
uint16_t Sensors_ADC_Read(uint8_t ch);

uint8_t Flame_DO_Detect(void);
uint8_t Smoke_DO_Detect(void);
uint8_t Rain_Detect(void);
void K230_Fall_GPIO_Init(void);
//void K230_Init(void);
//void K230_CheckTimeout(void);
void Relay_GPIO_Init(void);
#endif