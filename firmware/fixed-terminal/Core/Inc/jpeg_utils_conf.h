#ifndef  __JPEG_UTILS_CONF_H__
#define  __JPEG_UTILS_CONF_H__

#include "stm32h7xx_hal.h"
#include "stm32h7xx_hal_jpeg.h"

#define JPEG_ARGB8888            0
#define JPEG_RGB888              1  
#define JPEG_RGB565              2

#define USE_JPEG_DECODER     1
#define USE_JPEG_ENCODER     1

#define JPEG_RGB_FORMAT      JPEG_RGB565
#define JPEG_SWAP_RB         0

#endif