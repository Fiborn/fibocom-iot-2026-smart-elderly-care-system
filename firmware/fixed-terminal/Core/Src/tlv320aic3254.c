#include "tlv320aic3254.h"
#include "i2c.h"
#include <stdio.h>

extern I2C_HandleTypeDef hi2c1;

/* TLV320AIC3254 I2C地址: 0x18 (7-bit) → 0x30 (8-bit write) */
#define TLV320_I2C_ADDR  (0x18 << 1)

/* RESET引脚 - 根据你的硬件修改 */
#define TLV320_RESET_PORT  GPIOC
#define TLV320_RESET_PIN   GPIO_PIN_0

static uint8_t current_page = 0xFF;

static void TLV320_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOC_CLK_ENABLE();
    
    GPIO_InitStruct.Pin = TLV320_RESET_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(TLV320_RESET_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(TLV320_RESET_PORT, TLV320_RESET_PIN, GPIO_PIN_SET);
}

static void TLV320_HW_Reset(void)
{
    TLV320_GPIO_Init();
    HAL_GPIO_WritePin(TLV320_RESET_PORT, TLV320_RESET_PIN, GPIO_PIN_RESET);
    HAL_Delay(20);
    HAL_GPIO_WritePin(TLV320_RESET_PORT, TLV320_RESET_PIN, GPIO_PIN_SET);
    HAL_Delay(20);
    current_page = 0xFF;
}

static HAL_StatusTypeDef TLV320_SelectPage(uint8_t page)
{
    uint8_t data[2] = {0x00, page};
    HAL_StatusTypeDef status;
    
    if(current_page == page)
        return HAL_OK;
    
    status = HAL_I2C_Master_Transmit(&hi2c1, TLV320_I2C_ADDR, data, 2, 100);
    if(status == HAL_OK)
        current_page = page;
    return status;
}

HAL_StatusTypeDef TLV320AIC3254_WriteRegister(uint8_t page, uint8_t reg, uint8_t value)
{
    uint8_t data[2] = {reg, value};
    HAL_StatusTypeDef status;
    
    status = TLV320_SelectPage(page);
    if(status != HAL_OK) return status;
    
    status = HAL_I2C_Master_Transmit(&hi2c1, TLV320_I2C_ADDR, data, 2, 100);
    if(status != HAL_OK)
    {
        printf("I2C Write Failed: P%d R%d V0x%02X\r\n", page, reg, value);
    }
    return status;
}

uint8_t TLV320AIC3254_ReadRegister(uint8_t page, uint8_t reg)
{
    uint8_t value = 0xFF;
    
    if(TLV320_SelectPage(page) != HAL_OK) return 0xFF;
    if(HAL_I2C_Master_Transmit(&hi2c1, TLV320_I2C_ADDR, &reg, 1, 100) != HAL_OK) return 0xFF;
    HAL_I2C_Master_Receive(&hi2c1, TLV320_I2C_ADDR, &value, 1, 100);
    return value;
}

/**
  * @brief  初始化TLV320AIC3254
  * 
  * 硬件连接:
  *   CLK引脚 = MCLK + BCLK 短接 → 由STM32 SAI的SCK驱动
  *   FSYNC = STM32 SAI的FS
  *   DIN = STM32 SAI的SD
  *   TLV320为I2S从模式
  *
  * 时钟分析:
  *   BCLK = fs * 32 = 44100 * 32 = 1,411,200 Hz
  *   MCLK = BCLK = 1,411,200 Hz (因为短接)
  *   
  *   TLV320内部需要 DAC_CLK/DOSR = fs
  *   如果 CODEC_CLKIN = MCLK = 1.4112MHz
  *   NDAC=1, MDAC=1, DOSR=32 → fs = 1411200/(1*1*32) = 44100 ✓
  *   
  *   但是DOSR=32太小，DAC滤波器可能工作不好
  *   TLV320 datasheet推荐 DOSR >= 64 (最好128)
  *   
  *   所以必须使用PLL:
  *   PLL_CLKIN = BCLK = 1.4112MHz
  *   PLL输出 = PLL_CLKIN * R * J.D / P
  *   目标: PLL输出 ≈ 90.3168MHz (= 2048 * 44100)
  *   
  *   P=1, R=1, J=64, D=0:
  *   PLL_CLK = 1.4112M * 1 * 64 / 1 = 90.3168 MHz ✓ (正好!)
  *   
  *   然后: NDAC=2, MDAC=7, DOSR=128
  *   DAC_fs = 90316800 / (2 * 7 * 128) = 90316800 / 1792 = 50400 ≠ 44100
  *   
  *   试试: NDAC=1, MDAC=2, DOSR=128, BCLK_N_divider用于产生BCLK
  *   DAC_MOD_CLK = 90316800 / (1*2) = 45158400
  *   DAC_fs = 45158400 / 128 = 352800? 不对
  *   
  *   重新算: CODEC_CLKIN经PLL后 = 90.3168MHz
  *   NDAC * MDAC * DOSR = 90316800 / 44100 = 2048
  *   可选: NDAC=2, MDAC=8, DOSR=128 → 2*8*128=2048 ✓
  *   或者: NDAC=4, MDAC=4, DOSR=128 → 4*4*128=2048 ✓
  *   或者: NDAC=8, MDAC=2, DOSR=128 → 8*2*128=2048 ✓
  *   
  *   用 NDAC=2, MDAC=8, DOSR=128:
  *   DAC_MOD_CLK = 90316800 / (2*8) = 5644800 Hz
  *   DAC_fs = 5644800 / 128 = 44100 Hz ✓ 完美!
  */
	    /*
     * 修正后的采样率公式:
     * PLL_CLK = BCLK * J = fs * 32 * 56 = fs * 1792
     * DAC_fs = PLL_CLK / (NDAC * MDAC * DOSR) = fs * 1792 / 1792 = fs ✓
     * 
     * 所有采样率通用（因为BCLK随fs变化，PLL自动跟随）
     */

HAL_StatusTypeDef TLV320AIC3254_Init(void)
{
    uint8_t regVal;
    
    printf("\r\n=== TLV320AIC3254 Initialization ===\r\n");
    
    TLV320_HW_Reset();
    HAL_Delay(50);
    
    regVal = TLV320AIC3254_ReadRegister(0, 0x00);
    printf("Page Select Reg: 0x%02X (expect 0x00)\r\n", regVal);
    
    /* 软复位 */
    TLV320AIC3254_WriteRegister(0, 0x01, 0x01);
    HAL_Delay(10);
    
    /* CODEC_CLKIN = PLL_CLK */
    TLV320AIC3254_WriteRegister(0, 0x04, 0x03);
    
    /*
     * PLL配置:
     * PLL_CLKIN = BCLK (默认, Reg 0x04 bit[3:2] = 01 for BCLK)
     * 
     * 等等！Reg 0x04 的 bit[3:2] 控制 PLL 输入源！
     * bit[1:0] = 11 → CODEC_CLKIN = PLL_CLK  ← 这个OK
     * bit[3:2] = 00 → PLL_CLKIN = MCLK
     * bit[3:2] = 01 → PLL_CLKIN = BCLK
     * 
     * 你写的 0x03 → bit[3:2]=00 → PLL_CLKIN = MCLK引脚
     * 但你的 MCLK 和 BCLK 短接在一起，所以恰好也OK
     * 不过更明确的做法是选 BCLK:
     */
    TLV320AIC3254_WriteRegister(0, 0x04, 0x07);  /* PLL_CLKIN=BCLK, CODEC_CLKIN=PLL */
    
    /* PLL on, P=1, R=1 */
    TLV320AIC3254_WriteRegister(0, 0x05, 0x91);
    
    /*
     * J = 56 (0x38)
     * PLL_CLK = BCLK * R * J / P = 1,411,200 * 1 * 56 / 1 = 79,027,200 Hz
     */
    TLV320AIC3254_WriteRegister(0, 0x06, 0x38);  /* J = 56 */
    
    TLV320AIC3254_WriteRegister(0, 0x07, 0x00);  /* D MSB = 0 */
    TLV320AIC3254_WriteRegister(0, 0x08, 0x00);  /* D LSB = 0 */
    
    HAL_Delay(20);
    
    /*
     * 分频器:
     * NDAC = 2, MDAC = 7, DOSR = 128
     * DAC_fs = 79,027,200 / (2 * 7 * 128) = 79,027,200 / 1792 = 44,100 Hz ✓
     */
    TLV320AIC3254_WriteRegister(0, 0x0B, 0x82);  /* NDAC = 2, powered up */
    TLV320AIC3254_WriteRegister(0, 0x0C, 0x87);  /* MDAC = 7, powered up */
    TLV320AIC3254_WriteRegister(0, 0x0D, 0x00);  /* DOSR MSB = 0 */
    TLV320AIC3254_WriteRegister(0, 0x0E, 0x80);  /* DOSR LSB = 128 */
    
    /* I2S, 16-bit, slave mode */
    TLV320AIC3254_WriteRegister(0, 0x1B, 0x00);
    
    /* DAC Processing Block = PRB_P1 */
    TLV320AIC3254_WriteRegister(0, 0x3C, 0x01);
    
    /* ============ Page 1: 模拟配置 ============ */
    
    TLV320AIC3254_WriteRegister(1, 0x01, 0x08);
    TLV320AIC3254_WriteRegister(1, 0x02, 0x01);
    HAL_Delay(50);
    
    TLV320AIC3254_WriteRegister(1, 0x7B, 0x01);
    HAL_Delay(10);
    
    /* HPL/HPR Power-on Length */
    TLV320AIC3254_WriteRegister(1, 0x0A, 0x03);
    
    /* DAC PTM */
    TLV320AIC3254_WriteRegister(1, 0x03, 0x00);
    TLV320AIC3254_WriteRegister(1, 0x04, 0x00);
    
    /* 输出路由 */
    TLV320AIC3254_WriteRegister(1, 0x0C, 0x08);  /* DAC_L → HPL */
    TLV320AIC3254_WriteRegister(1, 0x0D, 0x08);  /* DAC_R → HPR */
    TLV320AIC3254_WriteRegister(1, 0x0E, 0x08);  /* DAC_L → LOL */
    TLV320AIC3254_WriteRegister(1, 0x0F, 0x08);  /* DAC_R → LOR */
    
    /* 
     * 输出增益 - 注意寄存器格式！
     * Page 1, Reg 0x10 (HPL): bit[5:0] = gain, bit[6] = unmute flag 需要看datasheet
     * 实际上对于 AIC3254:
     * Reg 16 (0x10): HPL driver gain
     *   Bit[5:0] = 二补码增益 (-6 to +29 dB, 步进1dB)
     *   Bit[6] = 0: unmuted, 1: muted  ← 关键！默认可能是muted!
     * 
     * 写 0x00 = unmuted, 0dB → 这是对的
     * 但有些版本默认bit6=1(muted)，复位后需要显式清零
     */
    TLV320AIC3254_WriteRegister(1, 0x10, 0x00);  /* HPL gain = 0dB, unmuted */
    TLV320AIC3254_WriteRegister(1, 0x11, 0x00);  /* HPR gain = 0dB, unmuted */
    TLV320AIC3254_WriteRegister(1, 0x12, 0x00);  /* LOL gain = 0dB, unmuted */
    TLV320AIC3254_WriteRegister(1, 0x13, 0x00);  /* LOR gain = 0dB, unmuted */
    
    /* 上电输出驱动 */
    TLV320AIC3254_WriteRegister(1, 0x09, 0x3C);
    HAL_Delay(500);
    
    /* ============ Page 0: 使能DAC ============ */
    
    TLV320AIC3254_WriteRegister(0, 0x3F, 0xD6);
    HAL_Delay(50);
    
    /* Unmute DAC */
    TLV320AIC3254_WriteRegister(0, 0x40, 0x00);
    
    /* DAC数字音量 = 0dB */
    TLV320AIC3254_WriteRegister(0, 0x41, 0x00);
    TLV320AIC3254_WriteRegister(0, 0x42, 0x00);
    
    /* 验证DAC Flag */
    regVal = TLV320AIC3254_ReadRegister(0, 0x25);
    printf("DAC Flag (P0 R37): 0x%02X\r\n", regVal);
    /* 期望: bit7=1(L DAC powered), bit3=1(R DAC powered) → 0x88 mask */
    if((regVal & 0x88) != 0x88)
    {
        printf("WARNING: DAC not powered! Flag=0x%02X\r\n", regVal);
    }
    
    regVal = TLV320AIC3254_ReadRegister(0, 0x05);
    printf("PLL Ctrl (P0 R5): 0x%02X\r\n", regVal);
    
    regVal = TLV320AIC3254_ReadRegister(0, 0x06);
    printf("PLL J (P0 R6): 0x%02X\r\n", regVal);
    
    printf("=== TLV320AIC3254 Init Complete ===\r\n\r\n");
    
    return HAL_OK;
}

HAL_StatusTypeDef TLV320AIC3254_SetVolume(uint8_t volume)
{
    TLV320AIC3254_WriteRegister(0, 0x41, volume);
    TLV320AIC3254_WriteRegister(0, 0x42, volume);
    return HAL_OK;
}

HAL_StatusTypeDef TLV320AIC3254_SetSampleRate(uint32_t sampleRate)
{
    printf("TLV320 SetSampleRate: %lu Hz\r\n", sampleRate);
    
    /*
     * 采样率由PLL和分频器决定:
     * PLL_CLK = BCLK * J / P = fs * 32 * 64 = fs * 2048
     * DAC_fs = PLL_CLK / (NDAC * MDAC * DOSR)
     * 
     * 对于44100Hz: PLL=90316800, NDAC=2, MDAC=8, DOSR=128 → 44100
     * 对于48000Hz: BCLK=1536000, PLL=1536000*64=98304000
     *             NDAC=2, MDAC=6, DOSR=128 → 98304000/1536=64000 不对
     *             NDAC=4, MDAC=6, DOSR=128 → 98304000/3072=32000 不对
     *             NDAC=2, MDAC=8, DOSR=128 → 98304000/2048=48000 ✓
     * 
     * 结论: 只要 NDAC*MDAC*DOSR = 2048, J=64, PLL会自动适应BCLK变化
     * 所以各种采样率都用相同的分频器配置即可(因为SAI会改变BCLK频率)
     */
    
    /* PLL配置不变（J=64, 从模式BCLK自动跟随） */
    /* 分频器不变 */
    
    return HAL_OK;
}