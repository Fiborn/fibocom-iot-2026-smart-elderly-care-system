#include "display_utils.h"
#include "ltdc.h"
#include "dcmi.h"

/***************************************************************************************************************************************
*   Function: LCD_CopyBuffer
*
*   Input: x - horizontal position
*          y - vertical position
*          width - horizontal size of the image
*          height - vertical size of the image
*          *color - pointer to the image buffer to display
*
*   Output: None
*
*   Description: Display image buffer starting at (x,y) position
*
*   Note: 1. Uses DMA2D for implementation
*         2. The image to be displayed must be smaller than the LCD display size
*
*****************************************************************************************************************************************/

void LCD_CopyBuffer(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t *color, uint16_t src_stride)
{
	DMA2D->CR	  &=	~(DMA2D_CR_START);				// Stop DMA2D
	DMA2D->CR		=	DMA2D_M2M;							// Memory to memory mode
	DMA2D->FGPFCCR	=	ColorMode_0;						// Foreground color format
	DMA2D->FGOR    =  src_stride - width;					// Foreground offset (source line offset in pixels)
	DMA2D->OOR		=	LCD_Width - width;				// Output offset 	
	DMA2D->FGMAR   =  (uint32_t)color;		
	DMA2D->OMAR		=	LCD_MemoryAdd + BytesPerPixel_0*(LCD_Width * y + x);	// Output address
	DMA2D->NLR		=	(width<<16)|(height);			// Set width and height		

	DMA2D->CR	  |=	DMA2D_CR_START;					// Start DMA2D
	while (DMA2D->CR & DMA2D_CR_START) ;				// Wait for transfer complete
	
}

/***************************************************************************************************************************************
*   Function: LCD_DisplayGrayscale
*
*   Input:   x - horizontal position
*            y - vertical position
*            width - image width
*            height - image height
*            *src_gray - pointer to grayscale buffer (uint8)
*
*   Output:  None
*
*   Description: Display grayscale image on LCD
*                Converts grayscale to RGB565 and stores in AI_Gray_RGB565_Buffer
*                Then displays using DMA2D
*****************************************************************************************************************************************/
void LCD_DisplayGrayscale(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t *src_gray)
{
    // Use dedicated grayscale RGB565 display buffer
    uint16_t *dst_rgb565 = (uint16_t*)AI_Gray_RGB565_Buffer;
    
    for (uint32_t i = 0; i < (uint32_t)width * height; i++) {
        uint8_t gray = src_gray[i];
        uint16_t r = (gray >> 3) & 0x1F;
        uint16_t g = (gray >> 2) & 0x3F;
        uint16_t b = (gray >> 3) & 0x1F;
        dst_rgb565[i] = (r << 11) | (g << 5) | b;
    }
    LCD_CopyBuffer(x, y, width, height, dst_rgb565, width);
}
