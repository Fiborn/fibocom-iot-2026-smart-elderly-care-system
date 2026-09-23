#include "sensors.h"
#include "usart.h"

ADC_HandleTypeDef hadc1;

/* ============ K230 全局变量的唯一定义 ============ */
uint8_t  g_k230_rx_byte = 0;
char     g_k230_result[K230_BUF_LEN] = {0};
uint8_t  g_k230_new_data = 0;
uint8_t  g_k230_buf[K230_BUF_LEN] = {0};
uint16_t g_k230_buf_len = 0;
uint32_t g_k230_last_rx_time = 0;
uint8_t  g_k230_rx_state = 0;

/*****************************************************************
* 函数: Sensors_GPIO_Init
* 功能: 初始化三路DO数字输入 (火焰PB0, 烟雾PB3, 雨滴PB2)
*****************************************************************/
void Sensors_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitStruct.Pin   = FLAME_DO_PIN | SMOKE_DO_PIN | RAIN_DETECT_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/*****************************************************************
* 函数: Sensors_ADC_Init
* 功能: 初始化ADC1，配置两路通道（PB1 火焰 + PA7 烟雾）
*   PB1 -> ADC1_INP5
*   PA7 -> ADC1_INP7
*****************************************************************/
void Sensors_ADC_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_ADC12_CLK_ENABLE();

    /* PB1 -> ADC1_INP5 模拟输入 */
    GPIO_InitStruct.Pin  = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* PA7 -> ADC1_INP7 模拟输入 */
    GPIO_InitStruct.Pin  = GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    hadc1.Instance                      = ADC1;
    hadc1.Init.ClockPrescaler           = ADC_CLOCK_ASYNC_DIV4;
    hadc1.Init.Resolution               = ADC_RESOLUTION_16B;
    hadc1.Init.ScanConvMode             = ADC_SCAN_DISABLE;
    hadc1.Init.EOCSelection             = ADC_EOC_SINGLE_CONV;
    hadc1.Init.LowPowerAutoWait         = DISABLE;
    hadc1.Init.ContinuousConvMode       = DISABLE;
    hadc1.Init.NbrOfConversion          = 1;
    hadc1.Init.DiscontinuousConvMode    = DISABLE;
    hadc1.Init.ExternalTrigConv         = ADC_SOFTWARE_START;
    hadc1.Init.ExternalTrigConvEdge     = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DR;
    hadc1.Init.Overrun                  = ADC_OVR_DATA_PRESERVED;
    hadc1.Init.OversamplingMode         = DISABLE;
    if (HAL_ADC_Init(&hadc1) != HAL_OK) {
        Error_Handler();
    }

    /* ADC自校准 */
    if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED) != HAL_OK) {
        Error_Handler();
    }
}

/*****************************************************************
* 函数: Sensors_ADC_Read
* 功能: 单通道单次采样
*****************************************************************/
uint16_t Sensors_ADC_Read(uint8_t ch)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    uint16_t val = 0;

    sConfig.Rank                = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime        = ADC_SAMPLETIME_64CYCLES_5;
    sConfig.SingleDiff          = ADC_SINGLE_ENDED;
    sConfig.OffsetNumber        = ADC_OFFSET_NONE;
    sConfig.Offset              = 0;
    sConfig.OffsetRightShift    = DISABLE;
    sConfig.OffsetSignedSaturation = DISABLE;

    if (ch == ADC_CH_FLAME)
        sConfig.Channel = ADC_CHANNEL_5;    // PB1 -> ADC1_INP5
    else
        sConfig.Channel = ADC_CHANNEL_7;    // PA7 -> ADC1_INP7

    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
        return 0;
    }

    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
        val = (uint16_t)HAL_ADC_GetValue(&hadc1);
    }
    HAL_ADC_Stop(&hadc1);

    return val;
}

/* DO状态：返回1表示报警 */
uint8_t Flame_DO_Detect(void)
{
    return (FLAME_DO_READ() == GPIO_PIN_RESET) ? 1 : 0;
}

uint8_t Smoke_DO_Detect(void)
{
    return (SMOKE_DO_READ() == GPIO_PIN_RESET) ? 1 : 0;
}

/* 雨滴检测：低电平=有水导通=漏水报警 */
uint8_t Rain_Detect(void)
{
    return (RAIN_READ() == GPIO_PIN_RESET) ? 1 : 0;
}

/*****************************************************************
* 函数: Voice_Input_Init
* 功能: 初始化5路语音识别输入引脚
*****************************************************************/
void Voice_Input_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* --- GPIOA: PA11 (cmd1), PA12 (cmd2) --- */
    GPIO_InitStruct.Pin   = VOICE_IN_TIME_PIN | VOICE_IN_WEATHER_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* --- GPIOB: PB10 (cmd3), PB11 (cmd4), PB15 (cmd5) --- */
    GPIO_InitStruct.Pin   = VOICE_IN_OPERA_PIN | VOICE_IN_SONG_PIN | VOICE_IN_STOP_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/*****************************************************************
* 函数: K230_Fall_GPIO_Init
* 功能: 初始化 K230 跌倒信号输入引脚（下拉，等外部拉高）
*****************************************************************/
void K230_Fall_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOD_CLK_ENABLE();   /* 换引脚记得同步换时钟 */

    GPIO_InitStruct.Pin   = K230_FALL_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_PULLDOWN;   /* 默认拉低，K230 拉高时才触发 */
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(K230_FALL_PORT, &GPIO_InitStruct);

    printf("[K230] Fall detect GPIO init done (input pull-down)\r\n");
}

/*****************************************************************
* 函数: Relay_GPIO_Init
* 功能: 初始化继电器控制引脚 PA5，推挽输出，默认低电平（继电器断开）
*****************************************************************/
void Relay_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();   // ? 改成GPIOA

    GPIO_InitStruct.Pin   = RELAY_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(RELAY_PORT, &GPIO_InitStruct);

    RELAY_OFF();
    printf("[RELAY] GPIO init done (PA5, default OFF)\r\n");
}

//void K230_Init(void)
//{
//    g_k230_result[0] = '\0';
//    g_k230_new_data = 0;
//    g_k230_buf_len = 0;
//    g_k230_rx_state = 0;

//    HAL_UART_Receive_IT(&huart3, &g_k230_rx_byte, 1);
//    printf("[K230] Init done, waiting for data...\n");
//}
//void K230_CheckTimeout(void)
//{
//    if (g_k230_buf_len == 0)
//    {
//        return;
//    }

//    uint32_t now = HAL_GetTick();
//    if (now - g_k230_last_rx_time > 100)
//    {
//        printf("[K230] Timeout, data: %s (len=%d)\n", g_k230_result, g_k230_buf_len);
//        g_k230_new_data = 1;
//        g_k230_buf_len = 0;
//        g_k230_rx_state = 0;
//    }
//}
