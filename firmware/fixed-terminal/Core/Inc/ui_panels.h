#ifndef __UI_PANELS_H
#define __UI_PANELS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "lunar.h"

void UI_DrawSwitchPanel(void);
void UI_DrawRecordPanel(void);
void UI_DrawMainPanel(void);
void UI_UpdateDateTime(uint16_t year, uint8_t mon, uint8_t day,
                       uint8_t hour, uint8_t min, uint8_t sec, uint8_t week);
void UI_UpdateLunar(LunarDate *ld);
void UI_UpdateTempHumi(uint8_t temp, uint8_t humi);
void UI_UpdateSensors(uint8_t flame, uint8_t smoke, uint8_t water,
                      uint16_t ao_flame, uint16_t ao_smoke);

#ifdef __cplusplus
}
#endif

#endif /* __UI_PANELS_H */
