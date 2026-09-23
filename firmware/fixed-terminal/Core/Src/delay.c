#include "delay.h"

// 初始化延时函数
// sysclk: 系统时钟频率，单位为 MHz (例如 H7 是 480)
void delay_init(uint16_t sysclk)
{
    // 1. 开启 ITM 探测相关的全局使能
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    
    // 2. 对于 Cortex-M7 (H7)，需要解锁 DWT 寄存器的访问权限
    // 0xC5ACCE55 是 ARM 规定的解锁码
    DWT->LAR = 0xC5ACCE55; 
    
    // 3. 清零计数器
    DWT->CYCCNT = 0;
    
    // 4. 开启 CYCCNT 使能
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

// 微秒级延时
void delay_us(uint32_t us)
{
    uint32_t ticks = us * (SystemCoreClock / 1000000);
    uint32_t start = DWT->CYCCNT;
    while ((DWT->CYCCNT - start) < ticks);
}