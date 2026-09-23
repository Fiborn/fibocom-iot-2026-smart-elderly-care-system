#include "buzzer.h"


TIM_HandleTypeDef htim_buzzer;
static volatile uint8_t s_buzzer_on = 0;

/*------------------------------------------------------------------
 * 蜂鸣器初始化
 *  - PD7 配置为推挽输出
 *  - TIM7 基本定时器，2kHz 翻转 => 1kHz 方波
 * H7 主频 480MHz，APB1 定时器时钟 = 240MHz
 *-----------------------------------------------------------------*/
void Buzzer_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOD_CLK_ENABLE();
    gpio.Pin   = BUZZER_PIN;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(BUZZER_PORT, &gpio);
    HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);

    __HAL_RCC_TIM7_CLK_ENABLE();

    /* 240MHz / 240 = 1MHz 计数频率  */
    /* Period = 500 => 500us 中断，翻转产生 1kHz 方波 */
    htim_buzzer.Instance               = TIM7;
    htim_buzzer.Init.Prescaler         = 240 - 1;
    htim_buzzer.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim_buzzer.Init.Period            = 500 - 1;
    htim_buzzer.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_Base_Init(&htim_buzzer);

    HAL_NVIC_SetPriority(TIM7_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(TIM7_IRQn);

    HAL_TIM_Base_Start_IT(&htim_buzzer);
}

void Buzzer_On(void)
{
    s_buzzer_on = 1;
}

void Buzzer_Off(void)
{
    s_buzzer_on = 0;
    HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);
}

uint8_t Buzzer_IsOn(void)
{
    return s_buzzer_on;
}

/* 动态改变频率，hz 建议 500~4000 */
void Buzzer_SetFreq(uint16_t hz)
{
    if (hz < 100 || hz > 10000) return;
    /* 翻转频率 = 2 * hz，1MHz 计数 => Period = 1000000/(2*hz) */
    uint32_t period = 1000000UL / (2UL * hz);
    __HAL_TIM_SET_AUTORELOAD(&htim_buzzer, period - 1);
    __HAL_TIM_SET_COUNTER(&htim_buzzer, 0);
}

/*------------------------------------------------------------------
 * 中断服务
 *-----------------------------------------------------------------*/
void TIM7_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&htim_buzzer);
}

/* 定时溢出回调 —— 翻转 PD7，产生方波 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM7) {
        if (s_buzzer_on) {
            HAL_GPIO_TogglePin(BUZZER_PORT, BUZZER_PIN);
        } else {
            HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);
        }
    }
}