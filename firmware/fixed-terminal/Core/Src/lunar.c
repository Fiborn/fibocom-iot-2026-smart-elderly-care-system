#include "lunar.h"
#include <string.h>
#include <stdio.h>

/* ============ 农历数据表 1900-2100 ============ */
/* 每年 1 个 uint32：低 4 位为闰月月份(0=无闰)，5-16 位为 12 个月(每月1=大30天, 0=小29天)，
   17 位为闰月大小，18-21 位保留。
   bit 0..3:  闰月月份
   bit 4..15: 普通月大小（bit15=正月, bit4=腊月，1=大）
   bit 16:    闰月大小
*/
static const uint32_t lunar_info[201] = {
    0x04bd8,0x04ae0,0x0a570,0x054d5,0x0d260,0x0d950,0x16554,0x056a0,0x09ad0,0x055d2, // 1900-1909
    0x04ae0,0x0a5b6,0x0a4d0,0x0d250,0x1d255,0x0b540,0x0d6a0,0x0ada2,0x095b0,0x14977, // 1910-1919
    0x04970,0x0a4b0,0x0b4b5,0x06a50,0x06d40,0x1ab54,0x02b60,0x09570,0x052f2,0x04970, // 1920-1929
    0x06566,0x0d4a0,0x0ea50,0x06e95,0x05ad0,0x02b60,0x186e3,0x092e0,0x1c8d7,0x0c950, // 1930-1939
    0x0d4a0,0x1d8a6,0x0b550,0x056a0,0x1a5b4,0x025d0,0x092d0,0x0d2b2,0x0a950,0x0b557, // 1940-1949
    0x06ca0,0x0b550,0x15355,0x04da0,0x0a5b0,0x14573,0x052b0,0x0a9a8,0x0e950,0x06aa0, // 1950-1959
    0x0aea6,0x0ab50,0x04b60,0x0aae4,0x0a570,0x05260,0x0f263,0x0d950,0x05b57,0x056a0, // 1960-1969
    0x096d0,0x04dd5,0x04ad0,0x0a4d0,0x0d4d4,0x0d250,0x0d558,0x0b540,0x0b6a0,0x195a6, // 1970-1979
    0x095b0,0x049b0,0x0a974,0x0a4b0,0x0b27a,0x06a50,0x06d40,0x0af46,0x0ab60,0x09570, // 1980-1989
    0x04af5,0x04970,0x064b0,0x074a3,0x0ea50,0x06b58,0x055c0,0x0ab60,0x096d5,0x092e0, // 1990-1999
    0x0c960,0x0d954,0x0d4a0,0x0da50,0x07552,0x056a0,0x0abb7,0x025d0,0x092d0,0x0cab5, // 2000-2009
    0x0a950,0x0b4a0,0x0baa4,0x0ad50,0x055d9,0x04ba0,0x0a5b0,0x15176,0x052b0,0x0a930, // 2010-2019
    0x07954,0x06aa0,0x0ad50,0x05b52,0x04b60,0x0a6e6,0x0a4e0,0x0d260,0x0ea65,0x0d530, // 2020-2029
    0x05aa0,0x076a3,0x096d0,0x04afb,0x04ad0,0x0a4d0,0x1d0b6,0x0d250,0x0d520,0x0dd45, // 2030-2039
    0x0b5a0,0x056d0,0x055b2,0x049b0,0x0a577,0x0a4b0,0x0aa50,0x1b255,0x06d20,0x0ada0, // 2040-2049
    0x14b63,0x09370,0x049f8,0x04970,0x064b0,0x168a6,0x0ea50,0x06b20,0x1a6c4,0x0aae0, // 2050-2059
    0x0a2e0,0x0d2e3,0x0c960,0x0d557,0x0d4a0,0x0da50,0x05d55,0x056a0,0x0a6d0,0x055d4, // 2060-2069
    0x052d0,0x0a9b8,0x0a950,0x0b4a0,0x0b6a6,0x0ad50,0x055a0,0x0aba4,0x0a5b0,0x052b0, // 2070-2079
    0x0b273,0x06930,0x07337,0x06aa0,0x0ad50,0x14b55,0x04b60,0x0a570,0x054e4,0x0d160, // 2080-2089
    0x0e968,0x0d520,0x0daa0,0x16aa6,0x056d0,0x04ae0,0x0a9d4,0x0a2d0,0x0d150,0x0f252, // 2090-2099
    0x0d520                                                                            // 2100
};

/* 干支 / 生肖 / 农历月名 / 农历日名 (使用 GBK 十六进制转义，避免编码问题) */
static const char* tg[10]  = {
    "\xBC\xD7", // 甲
    "\xD2\xD2", // 乙
    "\xB1\xFB", // 丙
    "\xB6\xA1", // 丁
    "\xCE\xEC", // 戊
    "\xBC\xBA", // 己
    "\xB8\xFD", // 庚
    "\xD0\xC1", // 辛
    "\xC8\xC9", // 壬
    "\xB9\xEF"  // 癸
};
static const char* dz[12]  = {
    "\xD7\xD3", // 子
    "\xB3\xF3", // 丑
    "\xD2\xF8", // 寅
    "\xC3\xAE", // 卯
    "\xB3\xBD", // 辰
    "\xCB\xC8", // 巳
    "\xCE\xE7", // 午
    "\xCE\xB4", // 未
    "\xC9\xEA", // 申
    "\xD3\xCF", // 酉
    "\xD0\xE7", // 戌
    "\xBA\xA5"  // 亥
};
static const char* sx[12]  = {
    "\xCA\xF3", // 鼠
    "\xC5\xA3", // 牛
    "\xBB\xA2", // 虎
    "\xCD\xC3", // 兔
    "\xC1\xFA", // 龙
    "\xC9\xDF", // 蛇
    "\xC2\xED", // 马
    "\xD1\xF2", // 羊
    "\xBA\xEF", // 猴
    "\xBC\xA6", // 鸡
    "\xB9\xB7", // 狗
    "\xD6\xED"  // 猪
};
static const char* mn[12]  = {
    "\xD5\xFD\xD4\xC2", // 正月
    "\xB6\xFE\xD4\xC2", // 二月
    "\xC8\xFD\xD4\xC2", // 三月
    "\xCB\xC4\xD4\xC2", // 四月
    "\xCE\xE5\xD4\xC2", // 五月
    "\xC1\xF9\xD4\xC2", // 六月
    "\xC6\xDF\xD4\xC2", // 七月
    "\xB0\xCB\xD4\xC2", // 八月
    "\xBE\xC5\xD4\xC2", // 九月
    "\xCA\xAE\xD4\xC2", // 十月
    "\xB6\xAC\xD4\xC2", // 冬月
    "\xC0\xB0\xD4\xC2"  // 腊月
};
static const char* dn1[10] = {
    "\xB3\xF5\xCA\xAE", // 初十
    "\xB3\xF5\xD2\xBB", // 初一
    "\xB3\xF5\xB6\xFE", // 初二
    "\xB3\xF5\xC8\xFD", // 初三
    "\xB3\xF5\xCB\xC4", // 初四
    "\xB3\xF5\xCE\xE5", // 初五
    "\xB3\xF5\xC1\xF9", // 初六
    "\xB3\xF5\xC6\xDF", // 初七
    "\xB3\xF5\xB0\xCB", // 初八
    "\xB3\xF5\xBE\xC5"  // 初九
};
static const char* dn2[10] = {
    "\xB6\xFE\xCA\xAE", // 二十
    "\xD8\xA5\xD2\xBB", // 廿一
    "\xD8\xA5\xB6\xFE", // 廿二
    "\xD8\xA5\xC8\xFD", // 廿三
    "\xD8\xA5\xCB\xC4", // 廿四
    "\xD8\xA5\xCE\xE5", // 廿五
    "\xD8\xA5\xC1\xF9", // 廿六
    "\xD8\xA5\xC6\xDF", // 廿七
    "\xD8\xA5\xB0\xCB", // 廿八
    "\xD8\xA5\xBE\xC5"  // 廿九
};
static const char* dn3[2]  = {
    "\xC8\xFD\xCA\xAE", // 三十
    "\xD8\xA6\xD2\xBB"  // 卅一
};
/* 节气对应的公历月份 */
static const uint8_t jq_month[24] = {
    1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12
};

/* 节气表：每年 24 个 uint8（日期 1..31），共 201 年，从 SD 卡 JIEQI.BIN 加载 */
static uint8_t jieqi_tab[201][24];
static int jieqi_loaded = 0;

/* ====================== 农历核心算法 ====================== */

/* 该年农历总天数 */
static int LunarYearDays(int y)
{
    int i, sum = 348;
    uint32_t info = lunar_info[y - 1900];
    for (i = 0x8000; i > 0x8; i >>= 1) if (info & i) sum += 1;
    int leap_m = info & 0xf;
    if (leap_m) sum += (info & 0x10000) ? 30 : 29;
    return sum;
}

/* 闰月月份 0=无 */
static int LeapMonth(int y){ return lunar_info[y-1900] & 0xf; }

/* 闰月天数 */
static int LeapDays(int y){
    if (LeapMonth(y)) return (lunar_info[y-1900] & 0x10000) ? 30 : 29;
    return 0;
}

/* 农历某月份天数 */
static int MonthDays(int y, int m){
    return (lunar_info[y-1900] & (0x10000 >> m)) ? 30 : 29;
}

/* 公历日期距 1900-1-31 的天数 */
static long DaysFrom19000131(int y, int m, int d)
{
    static const int md[12]={31,28,31,30,31,30,31,31,30,31,30,31};
    long days = 0;
    int i;
    for (i=1900; i<y; i++){
        days += ((i%4==0 && i%100!=0)||i%400==0) ? 366 : 365;
    }
    for (i=0; i<m-1; i++){
        days += md[i];
        if (i==1 && (((y%4==0 && y%100!=0)||y%400==0))) days += 1;
    }
    days += d - 31;
    return days;
}

void Solar2Lunar(uint16_t sy, uint8_t sm, uint8_t sd, LunarDate *ld)
{
    int i, leap = 0, temp = 0;
    long offset;

    /* ---------- 1) 计算距 1900-01-31 的天数 ---------- */
    offset = DaysFrom19000131(sy, sm, sd);
    printf("offset = %ld\r\n", offset);

    /* ---------- 2) 定位农历年 ---------- */
    int y;
    for (y = 1900; y < 2100; y++) {
        temp = LunarYearDays(y);
        if (offset < temp) break;
        offset -= temp;
    }
    ld->l_year = y;

    /* ---------- 3) 定位农历月（处理闰月） ---------- */
    leap = LeapMonth(y);
    int isLeap = 0;

    for (i = 1; i < 13; i++) {
        /* 闰月：在 leap 月之后插入 */
        if (leap > 0 && i == (leap + 1) && isLeap == 0) {
            --i;
            isLeap = 1;
            temp = LeapDays(y);
        } else {
            temp = MonthDays(y, i);
        }

        /* 解除闰月标记 */
        if (isLeap == 1 && i == (leap + 1)) isLeap = 0;

        if (offset < temp) break;
        offset -= temp;
    }

    /* 处理闰月当月退出循环时的情况 */
    if (offset == 0 && leap > 0 && i == leap + 1) {
        if (isLeap) {
            isLeap = 0;
        } else {
            isLeap = 1;
            --i;
        }
    }

    ld->l_month = i;
    ld->l_day   = (uint8_t)(offset + 1);
    ld->is_leap = isLeap;

    /* ---------- 4) 干支年（1900 = 庚子年；干支序号 36） ---------- */
    int gz = (y - 1900 + 36) % 60;
    snprintf(ld->gz_year, sizeof(ld->gz_year), "%s%s",
             tg[gz % 10], dz[gz % 12]);

    /* ---------- 5) 生肖（1900 = 鼠年） ---------- */
    snprintf(ld->zodiac, sizeof(ld->zodiac), "%s",
             sx[(y - 1900) % 12]);

    /* ---------- 6) 中文月份 ---------- */
    if (ld->is_leap)
        snprintf(ld->l_month_cn, sizeof(ld->l_month_cn),
                 "\xC8\xF2%s", mn[ld->l_month - 1]);   /* 闰X月 */
    else
        snprintf(ld->l_month_cn, sizeof(ld->l_month_cn),
                 "%s", mn[ld->l_month - 1]);
    /* ---------- 7) 中文日期 ---------- */
    int d = ld->l_day;
    if (d <= 10) {
        // 1-10日：初一 到 初十
        snprintf(ld->l_day_cn, sizeof(ld->l_day_cn), "%s", dn1[d % 10]);
    } 
    else if (d < 20) {
        // 11-19日：十一 到 十九
        // 逻辑：GBK编码下，取“十”(\xCA\xAE) 拼接 dn1[d-10] 的后两个字节（即去掉“初”字）
        char temp_day[10];
        sprintf(temp_day, "\xCA\xAE%s", &dn1[d - 10][2]); 
        snprintf(ld->l_day_cn, sizeof(ld->l_day_cn), "%s", temp_day);
    } 
    else if (d == 20) {
        // 20日：二十
        snprintf(ld->l_day_cn, sizeof(ld->l_day_cn), "%s", dn2[0]);
    } 
    else if (d < 30) {
        // 21-29日：廿一 到 廿九
        snprintf(ld->l_day_cn, sizeof(ld->l_day_cn), "%s", dn2[d - 20]);
    } 
    else {
        // 30日：三十
        snprintf(ld->l_day_cn, sizeof(ld->l_day_cn), "%s", dn3[0]);
    }
}
void SolarToLunar(uint16_t sy, uint8_t sm, uint8_t sd, LunarDate *ld)
{
    Solar2Lunar(sy, sm, sd, ld);
}