#include "mpu6500.h"
#include <math.h>

/* 32MHz 主频下的位延时，100kHz I2C 半周期约 5us = 160 cycles */
#define I2C_DELAY()    delay_cycles(80)

static float acc_sensitivity = 16384.0f;
static float gyr_sensitivity = 131.0f;

/* ============ 引脚电平控制 ============ */
static void SCL_H(void)
{
    DL_GPIO_disableOutput(MPU_I2C_PORT, MPU_I2C_SCL_PIN);
    DL_GPIO_initDigitalInputFeatures(MPU_I2C_SCL_IOMUX,
        DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
        DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
    I2C_DELAY();
}
static void SCL_L(void)
{
    DL_GPIO_initDigitalOutput(MPU_I2C_SCL_IOMUX);
    DL_GPIO_clearPins(MPU_I2C_PORT, MPU_I2C_SCL_PIN);
    DL_GPIO_enableOutput(MPU_I2C_PORT, MPU_I2C_SCL_PIN);
    I2C_DELAY();
}
static void SDA_H(void)
{
    DL_GPIO_disableOutput(MPU_I2C_PORT, MPU_I2C_SDA_PIN);
    DL_GPIO_initDigitalInputFeatures(MPU_I2C_SDA_IOMUX,
        DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
        DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
    I2C_DELAY();
}
static void SDA_L(void)
{
    DL_GPIO_initDigitalOutput(MPU_I2C_SDA_IOMUX);
    DL_GPIO_clearPins(MPU_I2C_PORT, MPU_I2C_SDA_PIN);
    DL_GPIO_enableOutput(MPU_I2C_PORT, MPU_I2C_SDA_PIN);
    I2C_DELAY();
}
static uint8_t SDA_Read(void)
{
    DL_GPIO_disableOutput(MPU_I2C_PORT, MPU_I2C_SDA_PIN);
    DL_GPIO_initDigitalInputFeatures(MPU_I2C_SDA_IOMUX,
        DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
        DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
    return (DL_GPIO_readPins(MPU_I2C_PORT, MPU_I2C_SDA_PIN) &
            MPU_I2C_SDA_PIN) ? 1 : 0;
}

/* ============ I2C 时序 ============ */
void MPU6500_I2C_Init(void)
{
    /* 初始为总线空闲状态 */
    SCL_L();
    SDA_L();
    SCL_H();
    SDA_H();
    delay_cycles(32000);   // ~1ms
}

static void I2C_Start(void)
{
    SDA_H();
    SCL_H();
    SDA_L();
    SCL_L();
}

static void I2C_Stop(void)
{
    SDA_L();
    SCL_H();
    SDA_H();
}

/* 主机读应答，返回 0=ACK, 1=NACK */
static uint8_t I2C_WaitAck(void)
{
    uint8_t ack;
    SDA_H();          // 释放 SDA
    SCL_H();
    ack = SDA_Read();
    SCL_L();
    return ack;
}

/* 主机发 ACK（0）或 NACK（1） */
static void I2C_SendAck(uint8_t nack)
{
    if (nack) SDA_H();
    else      SDA_L();
    SCL_H();
    SCL_L();
    SDA_H();
}

/* 写一个字节，不管应答 */
static void I2C_WriteByte(uint8_t dat)
{
    uint8_t i;
    for (i = 0; i < 8; i++) {
        if (dat & 0x80) SDA_H();
        else            SDA_L();
        dat <<= 1;
        SCL_H();
        SCL_L();
    }
}

/* 读一个字节 */
static uint8_t I2C_ReadByte(void)
{
    uint8_t i, dat = 0;
    SDA_H();          // 释放 SDA，让从机驱动
    for (i = 0; i < 8; i++) {
        SCL_L();      // 让从机准备数据
        SCL_H();      // 数据稳定
        dat <<= 1;
        if (SDA_Read()) dat |= 0x01;
    }
    SCL_L();
    return dat;
}

/* ============ MPU6500 读写接口 ============ */

uint8_t MPU6500_WriteReg(uint8_t reg, uint8_t data)
{
    I2C_Start();
    I2C_WriteByte(MPU6500_ADDR_W);
    if (I2C_WaitAck()) { I2C_Stop(); return 1; }
    I2C_WriteByte(reg);
    if (I2C_WaitAck()) { I2C_Stop(); return 1; }
    I2C_WriteByte(data);
    if (I2C_WaitAck()) { I2C_Stop(); return 1; }
    I2C_Stop();
    return 0;
}

uint8_t MPU6500_ReadRegs(uint8_t reg, uint8_t *buf, uint8_t len)
{
    uint8_t i;
    I2C_Start();
    I2C_WriteByte(MPU6500_ADDR_W);
    if (I2C_WaitAck()) { I2C_Stop(); return 1; }
    I2C_WriteByte(reg);
    if (I2C_WaitAck()) { I2C_Stop(); return 1; }

    I2C_Start();      // 重启
    I2C_WriteByte(MPU6500_ADDR_R);
    if (I2C_WaitAck()) { I2C_Stop(); return 1; }

    for (i = 0; i < len; i++) {
        buf[i] = I2C_ReadByte();
        if (i == len - 1) I2C_SendAck(1);   // 最后一字节 NACK
        else              I2C_SendAck(0);   // ACK
    }
    I2C_Stop();
    return 0;
}

uint8_t MPU6500_ReadReg(uint8_t reg)
{
    uint8_t data = 0;
    MPU6500_ReadRegs(reg, &data, 1);
    return data;
}

/* ============ 初始化 ============ */
uint8_t MPU6500_Init(void)
{
    MPU6500_I2C_Init();

    uint8_t who = MPU6500_ReadReg(MPU6500_WHO_AM_I);
    if (who != 0x70 && who != 0x71 && who != 0x68) {
        return 0;
    }

    MPU6500_WriteReg(MPU6500_PWR_MGMT_1, 0x80);  // reset
    delay_cycles(3200000);                       // ~100ms

    MPU6500_WriteReg(MPU6500_PWR_MGMT_1, 0x01);  // PLL
    delay_cycles(320000);

    MPU6500_WriteReg(MPU6500_PWR_MGMT_2, 0x00);
    MPU6500_WriteReg(MPU6500_SMPLRT_DIV, 0x07);

    MPU6500_SetAccRange(ACC_RANGE_4G);
    MPU6500_SetGyrRange(GYR_RANGE_500);
    MPU6500_SetGyrDLPF(MPU6500_DLPF_3);
    MPU6500_SetAccDLPF(MPU6500_DLPF_3);

    return 1;
}

/* ============ 配置 ============ */
void MPU6500_SetAccRange(MPU6500_AccRange_t range)
{
    uint8_t reg = MPU6500_ReadReg(MPU6500_ACCEL_CONFIG);
    reg = (reg & 0xE7) | range;
    MPU6500_WriteReg(MPU6500_ACCEL_CONFIG, reg);

    switch (range) {
        case ACC_RANGE_2G:  acc_sensitivity = 16384.0f; break;
        case ACC_RANGE_4G:  acc_sensitivity = 8192.0f;  break;
        case ACC_RANGE_8G:  acc_sensitivity = 4096.0f;  break;
        case ACC_RANGE_16G: acc_sensitivity = 2048.0f;  break;
    }
}

void MPU6500_SetGyrRange(MPU6500_GyrRange_t range)
{
    uint8_t reg = MPU6500_ReadReg(MPU6500_GYRO_CONFIG);
    reg = (reg & 0xE7) | range;
    MPU6500_WriteReg(MPU6500_GYRO_CONFIG, reg);

    switch (range) {
        case GYR_RANGE_250:  gyr_sensitivity = 131.0f; break;
        case GYR_RANGE_500:  gyr_sensitivity = 65.5f;  break;
        case GYR_RANGE_1000: gyr_sensitivity = 32.8f;  break;
        case GYR_RANGE_2000: gyr_sensitivity = 16.4f;  break;
    }
}

void MPU6500_SetAccDLPF(MPU6500_DLPF_t dlpf)
{
    MPU6500_WriteReg(MPU6500_ACCEL_CONFIG2, dlpf & 0x07);
}

void MPU6500_SetGyrDLPF(MPU6500_DLPF_t dlpf)
{
    MPU6500_WriteReg(MPU6500_CONFIG, dlpf & 0x07);
}

/* ============ 数据读取 ============ */
xyzFloat MPU6500_GetGValues(void)
{
    uint8_t buf[6];
    xyzFloat g = {0};
    MPU6500_ReadRegs(MPU6500_ACCEL_XOUT_H, buf, 6);
    int16_t x = (int16_t)((buf[0] << 8) | buf[1]);
    int16_t y = (int16_t)((buf[2] << 8) | buf[3]);
    int16_t z = (int16_t)((buf[4] << 8) | buf[5]);
    g.x = x / acc_sensitivity;
    g.y = y / acc_sensitivity;
    g.z = z / acc_sensitivity;
    return g;
}

xyzFloat MPU6500_GetGyrValues(void)
{
    uint8_t buf[6];
    xyzFloat gyr = {0};
    MPU6500_ReadRegs(MPU6500_GYRO_XOUT_H, buf, 6);
    int16_t x = (int16_t)((buf[0] << 8) | buf[1]);
    int16_t y = (int16_t)((buf[2] << 8) | buf[3]);
    int16_t z = (int16_t)((buf[4] << 8) | buf[5]);
    gyr.x = x / gyr_sensitivity;
    gyr.y = y / gyr_sensitivity;
    gyr.z = z / gyr_sensitivity;
    return gyr;
}

float MPU6500_GetTemperature(void)
{
    uint8_t buf[2];
    MPU6500_ReadRegs(MPU6500_TEMP_OUT_H, buf, 2);
    int16_t raw = (int16_t)((buf[0] << 8) | buf[1]);
    return ((float)raw / 333.87f) + 21.0f;
}

float MPU6500_GetResultantG(xyzFloat g)
{
    return sqrtf(g.x*g.x + g.y*g.y + g.z*g.z);
}
