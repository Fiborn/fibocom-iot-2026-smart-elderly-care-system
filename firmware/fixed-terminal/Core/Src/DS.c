#include "DS.h"
#include "delay.h"
#include <stdlib.h>

#define DS1302_DELAY_US    2   // 2??,?? =1祍 ??
#define DEC2BCD(x)   ((((x)/10)<<4) | ((x)%10))
#define COMPILE_OFFSET_SEC   150   // ?? 1?30?
static uint8_t parse_month(const char *s)
{
    switch(s[0]) {
        case 'J': return (s[1]=='a') ? 1 : (s[2]=='n' ? 6 : 7);   // Jan/Jun/Jul
        case 'F': return 2;
        case 'M': return (s[2]=='r') ? 3 : 5;                     // Mar/May
        case 'A': return (s[1]=='p') ? 4 : 8;                     // Apr/Aug
        case 'S': return 9;
        case 'O': return 10;
        case 'N': return 11;
        case 'D': return 12;
    }
    return 1;
}

/* ???? (Zeller ??,?? 1=Mon ... 7=Sun,DS1302 ??) */
static uint8_t calc_weekday(uint16_t y, uint8_t m, uint8_t d)
{
    if(m < 3) { m += 12; y -= 1; }
    uint8_t k = y % 100;
    uint8_t j = y / 100;
    int h = (d + 13*(m+1)/5 + k + k/4 + j/4 + 5*j) % 7;   // 0=Sat
    int w = ((h + 6 - 1) % 7) + 1;   // ??? 1=Mon..7=Sun
    return w;
}

static void build_time_from_compile(uint8_t *buf)
{
    const char *d = __DATE__;   // "Mmm dd yyyy"
    const char *t = __TIME__;   // "hh:mm:ss"
    
    uint16_t year  = (d[7]-'0')*1000 + (d[8]-'0')*100 + (d[9]-'0')*10 + (d[10]-'0');
    uint8_t  mon   = parse_month(d);
    uint8_t  day   = (d[4]==' ' ? 0 : (d[4]-'0')*10) + (d[5]-'0');
    uint8_t  hour  = (t[0]-'0')*10 + (t[1]-'0');
    uint8_t  minute= (t[3]-'0')*10 + (t[4]-'0');
    uint8_t  sec   = (t[6]-'0')*10 + (t[7]-'0');
    
    /* ---- ?????? ---- */
    uint32_t total = (uint32_t)hour*3600 + (uint32_t)minute*60 + sec + COMPILE_OFFSET_SEC;
    
    /* ?????(?????) */
    static const uint8_t mday[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
    
    while(total >= 86400) {           /* ?? */
        total -= 86400;
        day++;
        uint8_t dmax = mday[mon-1];
        if(mon == 2 && ((year%4==0 && year%100!=0) || year%400==0)) dmax = 29;
        if(day > dmax) {
            day = 1;
            mon++;
            if(mon > 12) { mon = 1; year++; }
        }
    }
    hour   = total / 3600;
    minute = (total % 3600) / 60;
    sec    = total % 60;
    /* ---------------------- */
    
    uint8_t week = calc_weekday(year, mon, day);
    
    buf[0] = DEC2BCD(year / 100);
    buf[1] = DEC2BCD(year % 100);
    buf[2] = DEC2BCD(mon);
    buf[3] = DEC2BCD(day);
    buf[4] = DEC2BCD(hour);
    buf[5] = DEC2BCD(minute);
    buf[6] = DEC2BCD(sec) & 0x7F;
    buf[7] = week;
}

uint8_t time_buf[8] = {0};

void DS1302_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    __HAL_RCC_GPIOB_CLK_ENABLE();
    
    /* RST, SCK ????,???? LOW ???? */
    GPIO_InitStruct.Pin = DS1302_RST_PIN | DS1302_SCK_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(DS1302_RST_PORT, &GPIO_InitStruct);
    
    /* IO ??????(?????) */
    GPIO_InitStruct.Pin = DS1302_IO_PIN;
    HAL_GPIO_Init(DS1302_IO_PORT, &GPIO_InitStruct);
    
    DS1302_RST_L();
    DS1302_SCK_L();
}

/* IO ?????? */
void DS1302_IO_Output(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DS1302_IO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(DS1302_IO_PORT, &GPIO_InitStruct);
}

/* IO ??????(???) */
void DS1302_IO_Input(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DS1302_IO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(DS1302_IO_PORT, &GPIO_InitStruct);
}

/* DS1302 ??? */
void DS1302_Init(void)
{
    DS1302_GPIO_Init();
    DS1302_RST_L();
    DS1302_SCK_L();
    delay_us(10);   // ??
}

// DS1302.c ????????
void ds1302_write_byte(uint8_t addr, uint8_t d)
{
    uint8_t i;
    DS1302_IO_Output();
    DS1302_RST_H();
    delay_us(4);
    
    addr &= 0xFE;
    for(i=0;i<8;i++) {
        if(addr & 0x01) DS1302_IO_H(); else DS1302_IO_L();
        delay_us(1);
        DS1302_SCK_H();
        delay_us(2);
        DS1302_SCK_L();
        delay_us(1);
        addr >>= 1;
    }
    
    for(i=0;i<8;i++) {
        if(d & 0x01) DS1302_IO_H(); else DS1302_IO_L();
        delay_us(1);
        DS1302_SCK_H();
        delay_us(2);
        DS1302_SCK_L();
        delay_us(1);
        d >>= 1;
    }
    
    DS1302_RST_L();
    delay_us(2);
}

uint8_t ds1302_read_byte(uint8_t addr)
{
    uint8_t i, temp=0;
    DS1302_IO_Output();
    DS1302_RST_H();
    delay_us(4);
    
    addr |= 0x01;
    for(i=0;i<8;i++) {
        if(addr & 0x01) DS1302_IO_H(); else DS1302_IO_L();
        delay_us(1);
        DS1302_SCK_H();
        delay_us(2);
        DS1302_SCK_L();
        delay_us(1);
        addr >>= 1;
    }
    
    DS1302_IO_Input();
    delay_us(1);   // ??????,??!
    
    for(i=0;i<8;i++) {
        temp >>= 1;
        if(DS1302_IO_READ()) temp |= 0x80;
        DS1302_SCK_H();
        delay_us(2);
        DS1302_SCK_L();
        delay_us(1);
    }
    
    DS1302_RST_L();
    delay_us(2);
    return temp;
}

// ?????(??????????)
void ds1302_init_with_check(void)
{
    uint8_t sec;
    DS1302_Init();
    
    sec = ds1302_read_byte(ds1302_sec_add | 0x01);
    /* CH=1 ???????(???? ? ????) */
    if(sec & 0x80) {
        ds1302_write_time();
    }
}
void ds1302_write_time(void)
{
    /* ??????? time_buf */
    build_time_from_compile(time_buf);
    
    ds1302_write_byte(ds1302_control_add, 0x00);   // ?????
    ds1302_write_byte(ds1302_sec_add,    0x80);    // ?????
    ds1302_write_byte(ds1302_year_add,   time_buf[1]);
    ds1302_write_byte(ds1302_month_add,  time_buf[2]);
    ds1302_write_byte(ds1302_date_add,   time_buf[3]);
    ds1302_write_byte(ds1302_hr_add,     time_buf[4]);
    ds1302_write_byte(ds1302_min_add,    time_buf[5]);
    ds1302_write_byte(ds1302_sec_add,    time_buf[6]);  // ???? (CH=0)
    ds1302_write_byte(ds1302_day_add,    time_buf[7]);
    ds1302_write_byte(ds1302_control_add, 0x80);   // ?????
}

/* ??????? time_buf */
void ds1302_read_time(void)
{
    time_buf[0] = 0x20;   /* ????? 20(DS1302 ?????) */
    time_buf[1] = ds1302_read_byte(ds1302_year_add);
    time_buf[2] = ds1302_read_byte(ds1302_month_add);
    time_buf[3] = ds1302_read_byte(ds1302_date_add);
    time_buf[4] = ds1302_read_byte(ds1302_hr_add);
    time_buf[5] = ds1302_read_byte(ds1302_min_add);
    time_buf[6] = ds1302_read_byte(ds1302_sec_add) & 0x7F;
    time_buf[7] = ds1302_read_byte(ds1302_day_add);
}

/* BCD 转十进制 */
static uint8_t BCD2DEC(uint8_t b){ return (b>>4)*10 + (b&0x0F); }

void Parse_RTC_Time(uint16_t *year, uint8_t *mon, uint8_t *day,
                    uint8_t *hour, uint8_t *min, uint8_t *sec, uint8_t *week)
{
    *year = 2000 + BCD2DEC(time_buf[1]);
    *mon  = BCD2DEC(time_buf[2]);
    *day  = BCD2DEC(time_buf[3]);
    *hour = BCD2DEC(time_buf[4]);
    *min  = BCD2DEC(time_buf[5]);
    *sec  = BCD2DEC(time_buf[6] & 0x7F);
    *week = time_buf[7];   /* DS1302 中 1=Mon ... 7=Sun */
}