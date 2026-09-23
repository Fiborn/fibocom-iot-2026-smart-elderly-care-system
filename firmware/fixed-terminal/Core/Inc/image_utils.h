#ifndef __IMAGE_UTILS_H
#define __IMAGE_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void Image_Downscale_480to96(uint16_t *src, uint16_t *dst);
void RGB565_to_Gray(uint16_t *src_rgb565, uint8_t *dst_gray, uint32_t width, uint32_t height);
void UINT8_to_INT8(uint8_t *src, int8_t *dst, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif /* __IMAGE_UTILS_H */
