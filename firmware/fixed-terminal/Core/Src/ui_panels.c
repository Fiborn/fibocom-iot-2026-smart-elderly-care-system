#include "ui_panels.h"
#include "ltdc.h"
#include "sensors.h"
#include "lunar.h"
#include <string.h>
#include <stdio.h>

/***************************************************************************************************************************************
*   Function: UI_DrawSwitchPanel
*
*   Description: Draw the right side panel in AI mode
*                Contains "Switch" title and "REC" button (150x90)
*****************************************************************************************************************************************/
void UI_DrawSwitchPanel(void)
{
    LCD_SetColor(LCD_WHITE);
    LCD_FillRect(640, 0, 160, 480);
	  LCD_SetColor(LCD_BLACK);
    LCD_SetFont(&Font32);
    LCD_DisplayString(656, 20, "HOME SYS", LCD_WHITE);
	  LCD_SetTextFont(&CH_Font32);
//	  LCD_DisplayText(712, 20, "\xCA\xB6\xB1\xF0", LCD_WHITE);
    
    LCD_SetColor(TECH_BLUE);
    LCD_FillRect(645, 120, 150, 90);
    LCD_SetColor(LCD_WHITE);
    LCD_SetFont(&CH_Font32);
    LCD_DisplayText(690, 149, "\xC5\xC4\xD5\xD5", TECH_BLUE);
    
    LCD_SetColor(LIGHT_CYAN);
    LCD_FillRect(645, 230, 150, 90);
    LCD_SetColor(LCD_MAGENTA);
    LCD_SetFont(&Font32);
    LCD_DisplayText(690, 259, "\xCD\xBC\xBF\xE2", LIGHT_CYAN);
}

/***************************************************************************************************************************************
*   Function: UI_DrawRecordPanel
*
*   Description: Draw the right side panel in Video recording mode
*                Contains 3 buttons: Start, Stop, Back AI (each 150x90)
*****************************************************************************************************************************************/
void UI_DrawRecordPanel(void)
{
    LCD_SetColor(LCD_WHITE);
    LCD_FillRect(640, 0, 160, 480);
    LCD_SetFont(&CH_Font32);
    LCD_SetColor(LCD_BLACK);
    LCD_DisplayText(660, 20, "\xC5\xC4\xD5\xD5\xC4\xA3\xCA\xBD", LCD_WHITE);
    
    // Capture button (top)
    LCD_SetColor(TECH_BLUE);
    LCD_FillRect(645, 80, 150, 90);
    LCD_SetColor(LCD_WHITE);
    LCD_SetFont(&CH_Font32);
    LCD_DisplayText(690, 115, "\xC5\xC4\xD5\xD5", TECH_BLUE);
    
    // Gallery button (middle)
    LCD_SetColor(LIGHT_CYAN);
    LCD_FillRect(645, 190, 150, 90);
    LCD_SetColor(LCD_MAGENTA);
    LCD_SetFont(&CH_Font32);
    LCD_DisplayText(690, 225, "\xCD\xBC\xBF\xE2", LIGHT_CYAN);
    
    // Back AI button (bottom)
    LCD_SetColor(LCD_GREY);
    LCD_FillRect(645, 300, 150, 90);
    LCD_SetColor(LIGHT_YELLOW);
    LCD_SetFont(&CH_Font32);
    LCD_DisplayText(690, 335, "\xB7\xB5\xBB\xD8", LCD_GREY);
}


void UI_DrawMainPanel(void)
{
    LCD_SetColor(LCD_BackColor);
    LCD_FillRect(200, 0, 400, 480);   /* 只清 200~600 这块信息区 */
}



/***************************************************************************************************************************************

*   Function: UI_UpdateDateTime

*

*   Description: 日期行 y=40：2026年6月27日 星期X  (左对齐,X从260开始)

*                时间行 y=160：HH:MM:SS         (左对齐)

*                所有内容控制在 X: 250~600 范围内

*****************************************************************************************************************************************/

void UI_UpdateDateTime(uint16_t year, uint8_t mon, uint8_t day,

                       uint8_t hour, uint8_t min, uint8_t sec, uint8_t week)
{

    char buf[16];
    uint16_t x;

    /* 星期中文表(GB2312编码) */
    static const char *week_cn_tbl[] = {
        "\xC8\xD5",  /* 0 或 7: 日 */
        "\xD2\xBB",  /* 1: 一 */
        "\xB6\xFE",  /* 2: 二 */
        "\xC8\xFD",  /* 3: 三 */
        "\xCB\xC4",  /* 4: 四 */
        "\xCE\xE5",  /* 5: 五 */
        "\xC1\xF9",  /* 6: 六 */
    };

    /* ===== 日期行 y=40：2024年6月27日 星期X ===== */
    LCD_SetColor(LCD_BackColor);
    LCD_FillRect(150, 40, 350, 40);   /* 清 250~600 */

    LCD_SetColor(LIGHT_CYAN);
    x = 160;   /* 左对齐起点 */

    /* 年份数字 */
    LCD_SetFont(&Font32);
    snprintf(buf, sizeof(buf), "%d", year);
    LCD_DisplayString(x, 40, (uint8_t *)buf, LCD_BackColor);
    x += strlen(buf) * Font32.Width;

    /* "年" */
    LCD_SetTextFont(&CH_Font32);
    LCD_DisplayText(x, 40, "\xC4\xEA", LCD_BackColor);
    x += CH_Font32.Width;

    /* 月份数字 */
    LCD_SetFont(&Font32);
    snprintf(buf, sizeof(buf), "%d", mon);
    LCD_DisplayString(x, 40, (uint8_t *)buf, LCD_BackColor);
    x += strlen(buf) * Font32.Width;

    /* "月" */
    LCD_SetTextFont(&CH_Font32);
    LCD_DisplayText(x, 40, "\xD4\xC2", LCD_BackColor);
    x += CH_Font32.Width;

    /* 日数字 */
    LCD_SetFont(&Font32);
    snprintf(buf, sizeof(buf), "%d", day);
    LCD_DisplayString(x, 40, (uint8_t *)buf, LCD_BackColor);
    x += strlen(buf) * Font32.Width;

    /* "日" */
    LCD_SetTextFont(&CH_Font32);
    LCD_DisplayText(x, 40, "\xC8\xD5", LCD_BackColor);
    x += CH_Font32.Width;

    /* 间隔一个空格 */
    x += 8;
    /* "星期X" - 紧挨日期后 */
    {
        uint8_t w_idx = week;
        if (w_idx == 0 || w_idx > 7) w_idx = 0;
        if (w_idx == 7) w_idx = 0;   /* 7 也归到日 */

        LCD_SetTextFont(&CH_Font32);
        LCD_DisplayText(x, 40, "\xD0\xC7\xC6\xDA", LCD_BackColor);   /* "星期" */
        x += CH_Font32.Width * 2;
        LCD_DisplayText(x, 40, (char *)week_cn_tbl[w_idx], LCD_BackColor);
    }

    /* ===== 时间行 y=160：HH:MM:SS（左对齐） ===== */
    LCD_SetColor(LCD_BackColor);
    LCD_FillRect(150, 160, 350, 40);

    LCD_SetFont(&Font32);
    LCD_SetColor(LIGHT_GREEN);
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d", hour, min, sec);
    LCD_DisplayString(160, 160, (uint8_t *)buf, LCD_BackColor);
}

/***************************************************************************************************************************************

*   Function: UI_UpdateLunar

*

*   Description: 农历X月X (y=100), 左对齐, X: 250~600

*****************************************************************************************************************************************/

void UI_UpdateLunar(LunarDate *ld)
{
    char buf[64];

    /* 清行 - 限制在 250~600 范围 */
    LCD_SetColor(LCD_BackColor);
    LCD_FillRect(150, 100, 350, 40);
    LCD_SetColor(LIGHT_CYAN);
    LCD_SetTextFont(&CH_Font32);
    snprintf(buf, sizeof(buf), "\xC5\xA9\xC0\xFA%s%s", ld->l_month_cn, ld->l_day_cn);
    LCD_DisplayText(160, 100, (char *)buf, LCD_BackColor);
}

/***************************************************************************************************************************************
*   Function: UI_UpdateTempHumi
*
*   Description: 温度和湿度同一行显示 y=220,   X: 250~600
*                格式:  温度 XX℃    湿度 XX%
*****************************************************************************************************************************************/
void UI_UpdateTempHumi(uint8_t temp, uint8_t humi)
{
    char buf[16];
    uint16_t x;

    /* 清一行（原来两行区域一起清干净） */
    LCD_SetColor(LCD_BackColor);
    LCD_FillRect(150, 220, 350, 76);   /* 清 220~296 */

    LCD_SetColor(LIGHT_GREEN);
    x = 160;

    /* "温度" */
    LCD_SetTextFont(&CH_Font32);
    LCD_DisplayText(x, 220, "\xCE\xC2\xB6\xC8", LCD_BackColor);
    x += CH_Font32.Width * 2;
    x += 8;

    /* 数字 */
    LCD_SetFont(&Font32);
    snprintf(buf, sizeof(buf), "%d", temp);
    LCD_DisplayString(x, 220, (uint8_t *)buf, LCD_BackColor);
    x += strlen(buf) * Font32.Width;

    /* "℃" */
    LCD_SetTextFont(&CH_Font32);
    LCD_DisplayText(x, 220, "\xA1\xE6", LCD_BackColor);
    x += CH_Font32.Width;

    /* 间隔 */
    x += 20;

    /* "湿度" */
    LCD_SetTextFont(&CH_Font32);
    LCD_DisplayText(x, 220, "\xCA\xAA\xB6\xC8", LCD_BackColor);
    x += CH_Font32.Width * 2;
    x += 8;

    /* 数字 */
    LCD_SetFont(&Font32);
    snprintf(buf, sizeof(buf), "%d", humi);
    LCD_DisplayString(x, 220, (uint8_t *)buf, LCD_BackColor);
    x += strlen(buf) * Font32.Width;

    /* "%" */
    LCD_DisplayString(x, 220, (uint8_t *)"%", LCD_BackColor);
}
/***************************************************************************************************************************************
*   Function: UI_UpdateSensors
*
*   Description: 火焰/烟雾/水泄露/系统状态 中文显示, 32号字体
*                y=280 火焰   y=320 烟雾   y=360 水泄露   y=410 系统
*****************************************************************************************************************************************/
void UI_UpdateSensors(uint8_t flame, uint8_t smoke, uint8_t water,
                      uint16_t ao_flame, uint16_t ao_smoke)
{
    uint8_t sys_warn = 0;
    uint16_t x;

    LCD_SetTextFont(&CH_Font32);

    /* ===== 火焰行 y=280 ===== */
    LCD_SetColor(LCD_BackColor);
    LCD_FillRect(150, 280, 350, 36);
    x = 160;

    /* "火焰" 标签 - 白色/青色 */
    LCD_SetColor(LIGHT_CYAN);
    LCD_DisplayText(x, 280, "\xBB\xF0\xD1\xE6", LCD_BackColor);   /* 火焰 */
    x += CH_Font32.Width * 2;
    x += 12;

    if (flame || ao_flame < FLAME_AO_THRESHOLD) {
        LCD_SetColor(LCD_RED);
        LCD_DisplayText(x, 280, "\xB1\xA8\xBE\xAF", LCD_BackColor);  /* 报警 */
        sys_warn |= 0x01;
    } else {
        LCD_SetColor(LIGHT_GREEN);
        LCD_DisplayText(x, 280, "\xD5\xFD\xB3\xA3", LCD_BackColor);  /* 正常 */
    }

    /* ===== 烟雾行 y=320 ===== */
    LCD_SetColor(LCD_BackColor);
    LCD_FillRect(150, 320, 350, 36);
    x = 160;

    LCD_SetColor(LIGHT_CYAN);
    LCD_DisplayText(x, 320, "\xD1\xCC\xCE\xED", LCD_BackColor);   /* 烟雾 */
    x += CH_Font32.Width * 2;
    x += 12;

    if (smoke || ao_smoke > SMOKE_AO_THRESHOLD) {
        LCD_SetColor(LCD_RED);
        LCD_DisplayText(x, 320, "\xB1\xA8\xBE\xAF", LCD_BackColor);  /* 报警 */
        sys_warn |= 0x02;
    } else {
        LCD_SetColor(LIGHT_GREEN);
        LCD_DisplayText(x, 320, "\xD5\xFD\xB3\xA3", LCD_BackColor);  /* 正常 */
    }

    /* ===== 水泄露行 y=360 ===== */
    LCD_SetColor(LCD_BackColor);
    LCD_FillRect(150, 360, 350, 36);
    x = 160;

    LCD_SetColor(LIGHT_CYAN);
    LCD_DisplayText(x, 360, "\xCB\xAE\xD0\xB9\xC2\xB6", LCD_BackColor); /* 水泄露 */
    x += CH_Font32.Width * 3;
    x += 12;

    if (water) {
        LCD_SetColor(LCD_RED);
        LCD_DisplayText(x, 360, "\xB1\xA8\xBE\xAF", LCD_BackColor);  /* 报警 */
        sys_warn |= 0x04;
    } else {
        LCD_SetColor(LIGHT_GREEN);
        LCD_DisplayText(x, 360, "\xD5\xFD\xB3\xA3", LCD_BackColor);  /* 正常 */
    }

    /* ===== 系统状态行 y=410 ===== */
    LCD_SetColor(LCD_BackColor);
    LCD_FillRect(150, 410, 350, 36);
    x = 160;

    LCD_SetColor(LIGHT_CYAN);
    LCD_DisplayText(x, 410, "\xCF\xB5\xCD\xB3", LCD_BackColor);   /* 系统 */
    x += CH_Font32.Width * 2;
    x += 12;

    if (sys_warn) {
        LCD_SetColor(LCD_RED);
        LCD_DisplayText(x, 410, "\xB1\xA8\xBE\xAF", LCD_BackColor);  /* 报警 */
    } else {
        LCD_SetColor(LIGHT_GREEN);
        LCD_DisplayText(x, 410, "\xB0\xB2\xC8\xAB", LCD_BackColor);  /* 安全 */
    }
}
