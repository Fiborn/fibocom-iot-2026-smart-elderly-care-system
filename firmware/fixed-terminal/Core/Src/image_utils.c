#include "image_utils.h"

/***************************************************************************************************************************************
*   Function: Image_Downscale_480to96
*
*   Input:   src - 480x480 RGB565 source buffer (Camera_Buffer)
*            dst - 96x96 RGB565 destination buffer (AI_Input_Buffer)
*
*   Output:  None
*
*   Description: Downscale 480x480 image to 96x96 using 5:1 pixel decimation
*                480 / 96 = 5, so we take every 5th pixel
*
*   Note:    Simple nearest-neighbor sampling for AI model input
*            RGB565 format: 2 bytes per pixel
*****************************************************************************************************************************************/
void Image_Downscale_480to96(uint16_t *src, uint16_t *dst)
{
    const uint32_t src_width = 480;
    const uint32_t dst_width = 96;
    const uint32_t step = 5;  // 480 / 96 = 5
    
    for (uint32_t dy = 0; dy < dst_width; dy++) {
        uint32_t src_y = dy * step;
        for (uint32_t dx = 0; dx < dst_width; dx++) {
            uint32_t src_x = dx * step;
            uint32_t src_idx = src_y * src_width + src_x;
            uint32_t dst_idx = dy * dst_width + dx;
            dst[dst_idx] = src[src_idx];
        }
    }
}

/***************************************************************************************************************************************
*   Function: RGB565_to_Gray
*
*   Input:   src_rgb565 - RGB565 source buffer
*            dst_gray   - grayscale destination buffer
*            width      - image width
*            height     - image height
*
*   Output:  None
*
*   Description: Convert RGB565 format to grayscale
*                Uses luminance formula: Y = 0.299*R + 0.587*G + 0.114*B
*                RGB565 format: R[15:11], G[10:5], B[4:0]
*****************************************************************************************************************************************/
void RGB565_to_Gray(uint16_t *src_rgb565, uint8_t *dst_gray, uint32_t width, uint32_t height)
{
    const uint32_t pixel_count = width * height;
    for (uint32_t i = 0; i < pixel_count; i++) {
        uint16_t rgb = src_rgb565[i];
        uint8_t r = (rgb >> 11) & 0x1F;
        uint8_t g = (rgb >> 5) & 0x3F;
        uint8_t b = rgb & 0x1F;
        
        uint16_t r_scaled = ((uint16_t)r << 3) | (r >> 2);
        uint16_t g_scaled = ((uint16_t)g << 2) | (g >> 4);
        uint16_t b_scaled = ((uint16_t)b << 3) | (b >> 2);
        
        uint16_t gray = (r_scaled * 77 + g_scaled * 150 + b_scaled * 29) >> 8;
        if (gray > 255) gray = 255;
        dst_gray[i] = (uint8_t)gray;
    }
}

/***************************************************************************************************************************************
*   Function: UINT8_to_INT8
*
*   Input:   src - pointer to uint8 source buffer (0-255 range)
*            dst - pointer to int8 destination buffer (-128~127 range)
*            size - number of elements to convert
*
*   Output:  None
*
*   Description: Convert uint8 (0-255) to int8 (-128~127) for AI model input
*                This is required for INT8 quantized neural network models
*                Formula: int8 = uint8 - 128
*
*   Note:    AI model (gesture_model) expects int8 input format
*****************************************************************************************************************************************/
void UINT8_to_INT8(uint8_t *src, int8_t *dst, uint32_t size)
{
    for (uint32_t i = 0; i < size; i++) {
        dst[i] = (int8_t)(src[i] - 128);
    }
}
