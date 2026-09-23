#ifndef __DISPLAY_UTILS_H
#define __DISPLAY_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void LCD_CopyBuffer(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t *color, uint16_t src_stride);
void LCD_DisplayGrayscale(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t *src_gray);

#ifdef __cplusplus
}
#endif

#endif /* __DISPLAY_UTILS_H */
