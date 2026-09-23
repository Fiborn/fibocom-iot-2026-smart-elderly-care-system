#ifndef __LUNAR_H
#define __LUNAR_H

#include <stdint.h>

typedef struct {
    uint16_t l_year;
    uint8_t  l_month;
    uint8_t  l_day;
    uint8_t  is_leap;
    char     gz_year[12];     // 干支年（可选保留，不使用）
    char     zodiac[8];       // 生肖（可选保留，不使用）
    char     l_month_cn[12];  // 农历月中文 如"正月"/"闰五月"
    char     l_day_cn[12];    // 农历日中文 如"初一"
} LunarDate;

/* 公历转农历 */
void Solar2Lunar(uint16_t sy, uint8_t sm, uint8_t sd, LunarDate *ld);

/* 兼容接口（main.c 中调用名） */
void SolarToLunar(uint16_t sy, uint8_t sm, uint8_t sd, LunarDate *ld);

#endif