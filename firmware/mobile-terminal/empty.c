/*
 * Copyright (c) 2021, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "ti_msp_dl_config.h"
#include "mpu6500.h"
#include "uart_print.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

/* 向 4G 模块发送字符串 */
static void UART4G_SendString(const char *s)
{
    while (*s) {
        while (DL_UART_isBusy(UART_4g_INST)) { ; }
        DL_UART_Main_transmitData(UART_4g_INST, (uint8_t)(*s));
        s++;
    }
}

void delay_ms(uint32_t ms)
{
    delay_cycles(ms * 32000);
}

typedef enum {
    FALL_CALIB = 0,
    FALL_MONITOR,
    FALL_SUSPECT,
    FALL_CONFIRM,
    FALL_ALARM
} FallState;

typedef struct {
    float x;
    float y;
    float z;
} Vec3;

static xyzFloat g;
static xyzFloat gyr;
static float temp;
static float resG;
int fall_flag=0;
static FallState fallState = FALL_CALIB;

static xyzFloat g_ref = {0};
static xyzFloat g_lp  = {0};
static xyzFloat g_sum = {0};

static uint16_t calib_cnt  = 0;
static uint16_t state_ticks = 0;
static uint16_t still_ticks = 0;
static uint16_t print_ticks = 0;
static uint16_t blink_ticks = 0;

static uint8_t lp_init = 0;
static uint8_t impact_seen = 0;
static uint8_t rotate_seen = 0;
static uint8_t fall_sent = 0;

/* 按键中断标志，主循环里处理 */
volatile uint8_t key_pressed_flag = 0;

/* ===== 采样与阈值 ===== */
#define SAMPLE_PERIOD_MS       8
#define PRINT_PERIOD_TICKS     25
#define BLINK_PERIOD_TICKS     31
#define CALIB_SAMPLES          100

#define TH_FREEFALL_G          0.45f
#define TH_IMPACT_G            1.80f
#define TH_ROTATE_DPS          120.0f
#define TH_FAST_ROTATE_DPS     180.0f
#define TH_PREIMPACT_G         1.50f
#define TH_PREIMPACT_GYR       100.0f

#define TH_LIE_ANGLE_DEG       45.0f
#define TH_STILL_G_ERR         0.18f
#define TH_STILL_GYR_DPS       20.0f

#define TH_SUSPECT_TIMEOUT     50
#define TH_CONFIRM_TIMEOUT     250
#define TH_STILL_TICKS         50

static void led_on(void)
{
    DL_GPIO_setPins(GPIOB, DL_GPIO_PIN_14);
}

static void led_off(void)
{
    DL_GPIO_clearPins(GPIOB, DL_GPIO_PIN_14);
}

/* ===== 蜂鸣器控制 (PB7 有源蜂鸣器) ===== */
static void buzzer_on(void)
{
    DL_GPIO_setPins(GPIOB, DL_GPIO_PIN_7);
}

static void buzzer_off(void)
{
    DL_GPIO_clearPins(GPIOB, DL_GPIO_PIN_7);
}

static float vec_norm(xyzFloat v)
{
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

static float clampf(float x, float minv, float maxv)
{
    if (x < minv) return minv;
    if (x > maxv) return maxv;
    return x;
}

static float angle_between_deg(xyzFloat a, xyzFloat b)
{
    float na = vec_norm(a);
    float nb = vec_norm(b);
    if (na < 0.001f || nb < 0.001f) return 0.0f;

    float dot = a.x * b.x + a.y * b.y + a.z * b.z;
    float c = dot / (na * nb);
    c = clampf(c, -1.0f, 1.0f);
    return acosf(c) * 57.29578f;
}

static float gyro_norm(xyzFloat v)
{
    return vec_norm(v);
}

static void update_gravity_lp(xyzFloat in)
{
    const float alpha = 0.15f;
    if (!lp_init) {
        g_lp = in;
        lp_init = 1;
    } else {
        g_lp.x += alpha * (in.x - g_lp.x);
        g_lp.y += alpha * (in.y - g_lp.y);
        g_lp.z += alpha * (in.z - g_lp.z);
    }
}

static void led_blink_service(void)
{
    if (fallState == FALL_ALARM) {
        led_on();
        return;
    }

    blink_ticks++;
    if (blink_ticks >= BLINK_PERIOD_TICKS) {
        blink_ticks = 0;
        DL_GPIO_togglePins(GPIOB, DL_GPIO_PIN_14);
    }
}

/* 按键解除报警：把状态机整个复位回监测模式 */
static void reset_to_monitor(void)
{
    fallState   = FALL_MONITOR;

    state_ticks = 0;
    still_ticks = 0;
    blink_ticks = 0;

    impact_seen = 0;
    rotate_seen = 0;
    fall_sent   = 0;

    /* 让 LED 立即回到"熄灭状态"，随后 blink service 会开始闪 */
    led_off();

    /* 关闭蜂鸣器 */
    buzzer_off();

    UART_SendString("[KEY] Alarm cleared, back to monitor.\r\n");
    fall_flag=0;
    UART4G_SendString("{\"services\": [{\"service_id\": \"data\", \"properties\": {\"fall_flag\":0}}]}");
}

int main(void)
{
    SYSCFG_DL_init();

    /* 上电确保蜂鸣器关闭 */
    buzzer_off();
    led_off();

    delay_ms(200);
    UART_SendString("\r\n[TEST] UART TX OK!\r\n");
    UART_SendString("[TEST] If you see this, UART hardware works.\r\n");
    delay_ms(100);

    NVIC_EnableIRQ(GPIOA_INT_IRQn);

    UART_SendString("\r\n===== MPU6500 Fall Detection Start =====\r\n");
    UART4G_SendString("BOOT\r\n");
    MPU6500_I2C_Init();

    uint8_t who = MPU6500_ReadReg(MPU6500_WHO_AM_I);
    UART_Printf("WHO_AM_I = 0x%02X\r\n", who);

    if (MPU6500_Init()) {
        UART_SendString("MPU6500 Init OK!\r\n");
    } else {
        UART_SendString("MPU6500 Init FAILED!\r\n");
        UART_SendString("[ERROR] MPU6500 not found. Check wiring!\r\n");
        while (1) {
            UART_SendString("[ALIVE] UART works, but MPU6500 init failed.\r\n");
            DL_GPIO_togglePins(GPIOB, DL_GPIO_PIN_14);
            delay_ms(1000);
        }
    }

    MPU6500_SetAccRange(ACC_RANGE_8G);
    MPU6500_SetGyrRange(GYR_RANGE_500);
    MPU6500_SetAccDLPF(MPU6500_DLPF_4);
    MPU6500_SetGyrDLPF(MPU6500_DLPF_4);

    UART_SendString("Keep device steady on chest/collar for calibration...\r\n");

    while (1)
    {
        float gyroN, tiltDeg, resG_lp;

        /* ==== 按键事件处理（放在最开始，最高优先级）==== */
        if (key_pressed_flag) {
            key_pressed_flag = 0;

            /* 简单消抖：延时一点再确认 */
            delay_ms(20);

            /* 无论当前处在哪个状态，只要按下按键就回到监测 */
            if (fallState == FALL_ALARM ||
                fallState == FALL_CONFIRM ||
                fallState == FALL_SUSPECT) {
                reset_to_monitor();
            } else {
                UART_SendString("[KEY] Pressed (no alarm to clear).\r\n");
            }
        }

        g   = MPU6500_GetGValues();
        gyr = MPU6500_GetGyrValues();
        temp = MPU6500_GetTemperature();
        resG = MPU6500_GetResultantG(g);

        update_gravity_lp(g);

        gyroN = gyro_norm(gyr);
        resG_lp = vec_norm(g_lp);
        tiltDeg = angle_between_deg(g_lp, g_ref);

        switch (fallState)
        {
        case FALL_CALIB:
            if (fabsf(resG_lp - 1.0f) < 0.12f && gyroN < 12.0f) {
                g_sum.x += g_lp.x;
                g_sum.y += g_lp.y;
                g_sum.z += g_lp.z;
                calib_cnt++;

                if (calib_cnt >= CALIB_SAMPLES) {
                    g_ref.x = g_sum.x / calib_cnt;
                    g_ref.y = g_sum.y / calib_cnt;
                    g_ref.z = g_sum.z / calib_cnt;
                    fallState = FALL_MONITOR;
                    UART_SendString("Calibration OK, monitoring...\r\n");
                }
            } else {
                calib_cnt = 0;
                g_sum.x = g_sum.y = g_sum.z = 0;
            }
            break;

        case FALL_MONITOR:
            if ((resG < TH_FREEFALL_G) ||
                ((resG > TH_PREIMPACT_G) && (gyroN > TH_PREIMPACT_GYR)) ||
                (gyroN > TH_FAST_ROTATE_DPS)) {
                fallState = FALL_SUSPECT;
                state_ticks = 0;
                still_ticks = 0;
                impact_seen = 0;
                rotate_seen = 0;
            }
            break;

        case FALL_SUSPECT:
            state_ticks++;

            if (resG > TH_IMPACT_G) {
                impact_seen = 1;
            }
            if (gyroN > TH_ROTATE_DPS) {
                rotate_seen = 1;
            }

            if (impact_seen && rotate_seen) {
                fallState = FALL_CONFIRM;
                state_ticks = 0;
                still_ticks = 0;
            }

            if (state_ticks > TH_SUSPECT_TIMEOUT) {
                fallState = FALL_MONITOR;
            }
            break;

        case FALL_CONFIRM:
            state_ticks++;

            if ((tiltDeg > TH_LIE_ANGLE_DEG) &&
                (fabsf(resG_lp - 1.0f) < TH_STILL_G_ERR) &&
                (gyroN < TH_STILL_GYR_DPS)) {
                still_ticks++;
            } else {
                if (still_ticks > 0) still_ticks--;
            }

            if (still_ticks >= TH_STILL_TICKS) {
                fallState = FALL_ALARM;
            }

            if (state_ticks > TH_CONFIRM_TIMEOUT) {
                fallState = FALL_MONITOR;
            }
            break;

        case FALL_ALARM:
            led_on();
            /* 蜂鸣器持续鸣叫 */
            buzzer_on();
            if (!fall_sent) {
                UART_SendString("FALL!\r\n");
                fall_flag=1;
                UART4G_SendString("{\"services\": [{\"service_id\": \"data\", \"properties\": {\"fall_flag\":1}}]}");
                fall_sent = 1;
            }
            break;

        default:
            fallState = FALL_MONITOR;
            break;
        }

        led_blink_service();

        print_ticks++;
        if (print_ticks >= PRINT_PERIOD_TICKS) {
            print_ticks = 0;

            UART_Printf("Acc(g): %.3f  %.3f  %.3f\r\n", g.x, g.y, g.z);
            UART_Printf("Resultant g: %.3f\r\n", resG);
            UART_Printf("Gyr(dps): %.3f  %.3f  %.3f\r\n", gyr.x, gyr.y, gyr.z);
            UART_Printf("GyroNorm: %.3f  Tilt: %.1f  State: %d\r\n", gyroN, tiltDeg, fallState);
            UART_Printf("Temp: %.2f C\r\n", temp);
            UART_Printf("********************************\r\n");
        }

        delay_ms(SAMPLE_PERIOD_MS);
    }
}

void GROUP1_IRQHandler(void)
{
    uint32_t gpioB = DL_GPIO_getEnabledInterruptStatus(GPIOB, DL_GPIO_PIN_8);

    if ((gpioB & DL_GPIO_PIN_8) == DL_GPIO_PIN_8) {
        DL_GPIO_clearInterruptStatus(GPIOB, DL_GPIO_PIN_8);

        /* 只置一个 flag，实际处理放到主循环里 */
        key_pressed_flag = 1;
    }
}
