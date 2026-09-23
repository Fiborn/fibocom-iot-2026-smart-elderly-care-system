#include "dht11.h"
#include "delay.h"

// 切换为输出
void DHT11_IO_OUT(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT11_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP; // 推挽输出
    GPIO_InitStruct.Pull = GPIO_PULLUP;         // 建议上拉
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStruct);
}

// 切换为输入
void DHT11_IO_IN(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT11_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;         // 输入也保持上拉
    HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStruct);
}

// 复位 DHT11
void DHT11_Rst(void)
{
    DHT11_IO_OUT();
    DHT11_DQ_OUT(0);        // 拉低 DQ
    HAL_Delay(20);          // 拉低至少 18ms (这里可以用 HAL_Delay)
    DHT11_DQ_OUT(1);        // DQ=1
    delay_us(30);           // 【修正】主机拉高 20~40us，必须用微秒
}

// 检测响应
uint8_t DHT11_Check(void)
{
    uint8_t retry = 0;
    DHT11_IO_IN();          
    // 等待 DHT11 拉低信号 (80us)
    while (DHT11_DQ_READ() && retry < 100)
    {
        retry++;
        delay_us(1);
    }
    if (retry >= 100) return 1;
    
    retry = 0;
    // 等待 DHT11 释放总线 (80us)
    while (!DHT11_DQ_READ() && retry < 100)
    {
        retry++;
        delay_us(1);
    }
    if (retry >= 100) return 1;
    return 0;
}

// 读取一个位
uint8_t DHT11_Read_Bit(void)
{
    uint8_t retry = 0;
    // 等待变低电平（每位的开始）
    while (DHT11_DQ_READ() && retry < 100)
    {
        retry++;
        delay_us(1);
    }
    retry = 0;
    // 等待变高电平
    while (!DHT11_DQ_READ() && retry < 100)
    {
        retry++;
        delay_us(1);
    }
    delay_us(40); // 关键：DHT11 传输 0 是 26-28us，传输 1 是 70us
                  // 延时 40us 后判断，如果是 1 则此时仍为高，如果是 0 则已变为低
    if (DHT11_DQ_READ()) return 1;
    else return 0;
}


/**
 * @brief 从DHT11读取一个字节
 * @return 读到的数据
 */
uint8_t DHT11_Read_Byte(void)
{
    uint8_t i, dat = 0;
    for (i = 0; i < 8; i++)
    {
        dat <<= 1;
        dat |= DHT11_Read_Bit();
    }
    return dat;
}

/**
 * @brief 从DHT11读取一次温湿度数据
 * @param temp 温度值(0~50°)
 * @param humi 湿度值(20%~90%)
 * @retval 0 正常
 * @retval 1 读取失败
 */
uint8_t DHT11_Read_Data(uint8_t *temp, uint8_t *humi)
{
    uint8_t buf[5];
    uint8_t i;
    DHT11_Rst();
    if (DHT11_Check() == 0)
    {
        for (i = 0; i < 5; i++)          // 读取40位数据
        {
            buf[i] = DHT11_Read_Byte();
        }
        if ((buf[0] + buf[1] + buf[2] + buf[3]) == buf[4])
        {
            *humi = buf[0];
            *temp = buf[2];
        }
    }
    else return 1;
    return 0;
}

/**
 * @brief 初始化DHT11的IO口并检测存在性
 * @retval 1 不存在
 * @retval 0 存在
 */
uint8_t DHT11_Init(void)
{
    /* 使能GPIOA时钟 */
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* 初始化引脚为输出模式，默认高电平 */
    DHT11_IO_OUT();
    DHT11_DQ_OUT(1);

    DHT11_Rst();                        // 复位DHT11
    return DHT11_Check();               // 检测DHT11
}