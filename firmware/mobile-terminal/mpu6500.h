#ifndef MPU6500_H_
#define MPU6500_H_

#include "ti_msp_dl_config.h"
#include <stdint.h>

/* MPU6500 的 7bit 地址 = 0x68, 移位后写地址 = 0xD0, 读地址 = 0xD1 */
#define MPU6500_I2C_ADDR_7BIT    0x68
#define MPU6500_ADDR_W           (MPU6500_I2C_ADDR_7BIT << 1)         // 0xD0
#define MPU6500_ADDR_R           ((MPU6500_I2C_ADDR_7BIT << 1) | 1)   // 0xD1

/* 寄存器 */
#define MPU6500_SMPLRT_DIV          0x19
#define MPU6500_CONFIG              0x1A
#define MPU6500_GYRO_CONFIG         0x1B
#define MPU6500_ACCEL_CONFIG        0x1C
#define MPU6500_ACCEL_CONFIG2       0x1D
#define MPU6500_ACCEL_XOUT_H        0x3B
#define MPU6500_TEMP_OUT_H          0x41
#define MPU6500_GYRO_XOUT_H         0x43
#define MPU6500_PWR_MGMT_1          0x6B
#define MPU6500_PWR_MGMT_2          0x6C
#define MPU6500_WHO_AM_I            0x75

typedef enum {
    ACC_RANGE_2G  = 0x00,
    ACC_RANGE_4G  = 0x08,
    ACC_RANGE_8G  = 0x10,
    ACC_RANGE_16G = 0x18
} MPU6500_AccRange_t;

typedef enum {
    GYR_RANGE_250  = 0x00,
    GYR_RANGE_500  = 0x08,
    GYR_RANGE_1000 = 0x10,
    GYR_RANGE_2000 = 0x18
} MPU6500_GyrRange_t;

typedef enum {
    MPU6500_DLPF_0 = 0, MPU6500_DLPF_1, MPU6500_DLPF_2, MPU6500_DLPF_3,
    MPU6500_DLPF_4,     MPU6500_DLPF_5, MPU6500_DLPF_6, MPU6500_DLPF_7
} MPU6500_DLPF_t;

typedef struct {
    float x; float y; float z;
} xyzFloat;

/* ========= 软件 I2C 引脚定义（PA28=SDA, PA31=SCL）========= */
#define MPU_I2C_PORT          GPIOA
#define MPU_I2C_SDA_PIN       DL_GPIO_PIN_28
#define MPU_I2C_SDA_IOMUX     IOMUX_PINCM3
#define MPU_I2C_SCL_PIN       DL_GPIO_PIN_31
#define MPU_I2C_SCL_IOMUX     IOMUX_PINCM6

/* API */
void     MPU6500_I2C_Init(void);
uint8_t  MPU6500_Init(void);
uint8_t  MPU6500_WriteReg(uint8_t reg, uint8_t data);
uint8_t  MPU6500_ReadReg(uint8_t reg);
uint8_t  MPU6500_ReadRegs(uint8_t reg, uint8_t *buf, uint8_t len);

void     MPU6500_SetAccRange(MPU6500_AccRange_t range);
void     MPU6500_SetGyrRange(MPU6500_GyrRange_t range);
void     MPU6500_SetAccDLPF(MPU6500_DLPF_t dlpf);
void     MPU6500_SetGyrDLPF(MPU6500_DLPF_t dlpf);

xyzFloat MPU6500_GetGValues(void);
xyzFloat MPU6500_GetGyrValues(void);
float    MPU6500_GetTemperature(void);
float    MPU6500_GetResultantG(xyzFloat g);

#endif
