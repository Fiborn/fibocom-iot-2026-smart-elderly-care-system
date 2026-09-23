/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    dcmi.c
  * @brief   This file provides code for the configuration
  *          of the DCMI instances.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "dcmi.h"

/* USER CODE BEGIN 0 */
#include "dcmi_ov5640_cfg.h"  
#include "photo_capture.h"
/* USER CODE END 0 */

DCMI_HandleTypeDef hdcmi;
DMA_HandleTypeDef hdma_dcmi;

/* DCMI init function */
void MX_DCMI_Init(void)
{

  /* USER CODE BEGIN DCMI_Init 0 */

  /* USER CODE END DCMI_Init 0 */

  /* USER CODE BEGIN DCMI_Init 1 */

  /* USER CODE END DCMI_Init 1 */
  hdcmi.Instance = DCMI;
  hdcmi.Init.SynchroMode = DCMI_SYNCHRO_HARDWARE;
  hdcmi.Init.PCKPolarity = DCMI_PCKPOLARITY_RISING;
  hdcmi.Init.VSPolarity = DCMI_VSPOLARITY_LOW;
  hdcmi.Init.HSPolarity = DCMI_HSPOLARITY_LOW;
  hdcmi.Init.CaptureRate = DCMI_CR_ALL_FRAME;
  hdcmi.Init.ExtendedDataMode = DCMI_EXTEND_DATA_8B;
  hdcmi.Init.JPEGMode = DCMI_JPEG_DISABLE;
  hdcmi.Init.ByteSelectMode = DCMI_BSM_ALL;
  hdcmi.Init.ByteSelectStart = DCMI_OEBS_ODD;
  hdcmi.Init.LineSelectMode = DCMI_LSM_ALL;
  hdcmi.Init.LineSelectStart = DCMI_OELS_ODD;
  if (HAL_DCMI_Init(&hdcmi) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DCMI_Init 2 */

  /* USER CODE END DCMI_Init 2 */

}

void HAL_DCMI_MspInit(DCMI_HandleTypeDef* dcmiHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(dcmiHandle->Instance==DCMI)
  {
  /* USER CODE BEGIN DCMI_MspInit 0 */

  /* USER CODE END DCMI_MspInit 0 */
    /* DCMI clock enable */
    __HAL_RCC_DCMI_CLK_ENABLE();

    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**DCMI GPIO Configuration
    PE4     ------> DCMI_D4
    PA4     ------> DCMI_HSYNC
    PA6     ------> DCMI_PIXCLK
    PC6     ------> DCMI_D0
    PC7     ------> DCMI_D1
    PD3     ------> DCMI_D5
    PG9     ------> DCMI_VSYNC
    PG10     ------> DCMI_D2
    PG11     ------> DCMI_D3
    PB8     ------> DCMI_D6
    PB9     ------> DCMI_D7
    */
    GPIO_InitStruct.Pin = GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF13_DCMI;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF13_DCMI;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF13_DCMI;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF13_DCMI;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF13_DCMI;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF13_DCMI;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* DCMI DMA Init */
    /* DCMI Init */
    hdma_dcmi.Instance = DMA2_Stream7;
    hdma_dcmi.Init.Request = DMA_REQUEST_DCMI;
    hdma_dcmi.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_dcmi.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_dcmi.Init.MemInc = DMA_MINC_ENABLE;
    hdma_dcmi.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hdma_dcmi.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    hdma_dcmi.Init.Mode = DMA_CIRCULAR;
    hdma_dcmi.Init.Priority = DMA_PRIORITY_LOW;
    hdma_dcmi.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
    hdma_dcmi.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
    hdma_dcmi.Init.MemBurst = DMA_MBURST_SINGLE;
    hdma_dcmi.Init.PeriphBurst = DMA_PBURST_SINGLE;
    if (HAL_DMA_Init(&hdma_dcmi) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(dcmiHandle,DMA_Handle,hdma_dcmi);

    /* DCMI interrupt Init */
    HAL_NVIC_SetPriority(DCMI_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(DCMI_IRQn);
  /* USER CODE BEGIN DCMI_MspInit 1 */

  /* USER CODE END DCMI_MspInit 1 */
  }
}

void HAL_DCMI_MspDeInit(DCMI_HandleTypeDef* dcmiHandle)
{

  if(dcmiHandle->Instance==DCMI)
  {
  /* USER CODE BEGIN DCMI_MspDeInit 0 */

  /* USER CODE END DCMI_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_DCMI_CLK_DISABLE();

    /**DCMI GPIO Configuration
    PE4     ------> DCMI_D4
    PA4     ------> DCMI_HSYNC
    PA6     ------> DCMI_PIXCLK
    PC6     ------> DCMI_D0
    PC7     ------> DCMI_D1
    PD3     ------> DCMI_D5
    PG9     ------> DCMI_VSYNC
    PG10     ------> DCMI_D2
    PG11     ------> DCMI_D3
    PB8     ------> DCMI_D6
    PB9     ------> DCMI_D7
    */
    HAL_GPIO_DeInit(GPIOE, GPIO_PIN_4);

    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_4|GPIO_PIN_6);

    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_6|GPIO_PIN_7);

    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_3);

    HAL_GPIO_DeInit(GPIOG, GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11);

    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_8|GPIO_PIN_9);

    /* DCMI DMA DeInit */
    HAL_DMA_DeInit(dcmiHandle->DMA_Handle);

    /* DCMI interrupt Deinit */
    HAL_NVIC_DisableIRQ(DCMI_IRQn);
  /* USER CODE BEGIN DCMI_MspDeInit 1 */

  /* USER CODE END DCMI_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */




extern DMA_HandleTypeDef hdma_dcmi;

volatile uint8_t OV5640_FrameState = 0;  // DCMI state flag, set to 1 by HAL_DCMI_FrameEventCallback() when new frame is ready     
volatile uint8_t OV5640_FPS ;          // Frame rate

volatile uint8_t  OV5640_LineState = 0;
volatile uint16_t OV5640_LineCount = 0;         	

volatile uint16_t g_current_camera_width = 480;  // Current camera width (AI mode:480, Video mode:640)	

/*****************************************************************************************************************************************
*	Function:	DCMI_GPIO_Init
*
*	Description:	Initialize DCMI GPIO pins
*
*****************************************************************************************************************************************/
void DCMI_GPIO_Init(void)
{
   GPIO_InitTypeDef GPIO_InitStruct = {0};

	GPIO_OV5640_PWDN_CLK_ENABLE;    // Enable PWDN pin GPIO clock

/****************************************************************************  

   GPIO Configuration:
   PH13   ------> PWDN
	
******************************************************************************/

// Initialize PWDN pin  
	OV5640_PWDN_ON;	// High level enters power-down mode, camera stops, power consumption of analog part is minimum

	GPIO_InitStruct.Pin 		= OV5640_PWDN_PIN;				// PWDN pin
	GPIO_InitStruct.Mode 	= GPIO_MODE_OUTPUT_PP;			// Push-pull output mode
	GPIO_InitStruct.Pull 	= GPIO_PULLUP;						// Pull-up
	GPIO_InitStruct.Speed 	= GPIO_SPEED_FREQ_LOW;			// Low speed
	HAL_GPIO_Init(OV5640_PWDN_PORT, &GPIO_InitStruct);	   // Initialize  

}


/***************************************************************************************************************************************
*	Function: OV5640_Delay
*	Parameter: Delay - delay time, unit ms 
*	Description: Delay function, not very accurate
*	Note: For camera initialization sequence, this delay function is sufficient. Can be replaced with RTOS delay or HAL delay.
***************************************************************************************************************************************/
void OV5640_Delay(uint32_t Delay)
{
	volatile uint16_t i;

	while (Delay --)				
	{
		for (i = 0; i < 40000; i++);
	}	
//	HAL_Delay(Delay);	  // Use HAL delay instead
}

/***************************************************************************************************************************************
*	Function: DCMI_OV5640_Init
*
*	Description: Initialize SCCB, DCMI, DMA and configure OV5640
*
***************************************************************************************************************************************/
int8_t DCMI_OV5640_Init(void)
{
	uint16_t	Device_ID;		// Variable to store device ID
	
   SCCB_GPIO_Config();		               // SCCB interface initialization
	DCMI_GPIO_Init();                       // Initialize GPIO

	OV5640_Reset();	                     // Execute software reset
	Device_ID =  OV5640_ReadID();		      // Read device ID

	if( Device_ID == 0x5640 )		// ID match
	{
		printf ("OV5640 OK,ID:0x%X\r\n",Device_ID);		      // Match success

		OV5640_Config();													// Configure camera settings
		OV5640_Set_Framesize(OV5640_Width, OV5640_Height);			// Set OV5640 frame size
		OV5640_DCMI_Crop(480, 480, OV5640_Width, OV5640_Height);	// Center crop to 480x480 for AI input
				
		return OV5640_Success;	 // Return success flag		
	}
	else
	{
		printf ("OV5640 ERROR!!!!!  ID:%X\r\n",Device_ID);	   // ID read failed
		return  OV5640_Error;	 // Return error flag
	}	
}

/***************************************************************************************************************************************
*	Function: OV5640_DMA_Transmit_Continuous
*
*	Parameter:  DMA_Buffer - DMA destination address, used to store the starting address of the image data storage buffer
*            DMA_BufferSize - Size of transfer data, unit 32-bit word
*
*	Description: Start DMA transfer, continuous mode
*
*	Note: 1. After continuous mode is started, it will keep transferring until DCMI is stopped
*            2. When OV5640 uses RGB565 mode, 1 pixel requires 2 bytes of storage
*				 3. Because the DMA transfer unit is 32-bit words, when calculating DMA_BufferSize, it needs to be divided by 4. For example:
*               To capture a 240*240 pixel image, the image needs to occupy 240*240*2 = 115200 bytes of storage space,
*               then DMA_BufferSize = 115200 / 4 = 28800 words
***************************************************************************************************************************************/
void OV5640_DMA_Transmit_Continuous(uint32_t DMA_Buffer,uint32_t DMA_BufferSize)
{
   hdma_dcmi.Init.Mode  = DMA_CIRCULAR;  // Circular mode					

   HAL_DMA_Init(&hdma_dcmi);    // Reinitialize DMA

  // Enable DCMI capture, continuous capture mode
   HAL_DCMI_Start_DMA(&hdcmi, DCMI_MODE_CONTINUOUS, (uint32_t)DMA_Buffer,DMA_BufferSize);	
}

/***************************************************************************************************************************************
*	Function: OV5640_DMA_Transmit_Snapshot
*
*	Parameter:  DMA_Buffer - DMA destination address, used to store the starting address of the image data storage buffer
*            DMA_BufferSize - Size of transfer data, unit 32-bit word
*
*	Description: Start DMA transfer, snapshot mode, capture one frame and stop
*
*	Note: 1. Snapshot mode only captures one frame
*            2. When OV5640 uses RGB565 mode, 1 pixel requires 2 bytes of storage
*				 3. Because the DMA transfer unit is 32-bit words, when calculating DMA_BufferSize, it needs to be divided by 4. For example:
*               To capture a 240*240 pixel image, the image needs to occupy 240*240*2 = 115200 bytes of storage space,
*               then DMA_BufferSize = 115200 / 4 = 28800 words
*            4. After using snapshot mode, DCMI will be suspended. Before starting capture again, you need to call OV5640_DCMI_Resume() to resume DCMI
*
***************************************************************************************************************************************/
void OV5640_DMA_Transmit_Snapshot(uint32_t DMA_Buffer,uint32_t DMA_BufferSize)
{
   hdma_dcmi.Init.Mode  = DMA_NORMAL;  // Normal mode					

   HAL_DMA_Init(&hdma_dcmi);    // Reinitialize DMA

   HAL_DCMI_Start_DMA(&hdcmi, DCMI_MODE_SNAPSHOT, (uint32_t)DMA_Buffer,DMA_BufferSize);
}

/***************************************************************************************************************************************
*	Function: OV5640_DCMI_Suspend
*
*	Description: Suspend DCMI, stop image capture
*
*	Note: 1. After using snapshot mode, call this function to stop DCMI capture
*            2. Can call OV5640_DCMI_Resume() to resume DCMI
*				 3. Note that in suspend mode, DCMI stops, but DMA does not stop
***************************************************************************************************************************************/
void OV5640_DCMI_Suspend(void) 
{
   HAL_DCMI_Suspend(&hdcmi);    // Suspend DCMI
}

/***************************************************************************************************************************************
*	Function: OV5640_DCMI_Resume
*
*	Description: Resume DCMI and start image capture
*
*	Note: 1. When DCMI is suspended, you can call this function to resume
*            2. After using OV5640_DMA_Transmit_Snapshot() snapshot mode, DCMI will also be suspended. Before starting capture again,
*				    you need to call this function to resume DCMI first
*
***************************************************************************************************************************************/
void  OV5640_DCMI_Resume(void) 
{
   (&hdcmi)->State = HAL_DCMI_STATE_BUSY;       // Set DCMI state
   (&hdcmi)->Instance->CR |= DCMI_CR_CAPTURE;   // Start DCMI capture
}

/***************************************************************************************************************************************
*	Function: OV5640_DCMI_Stop
*
*	Description: Stop DCMI and DMA, completely stop DCMI capture and clock
*
***************************************************************************************************************************************/
void  OV5640_DCMI_Stop(void) 
{
   HAL_DCMI_Stop(&hdcmi);
}


/***************************************************************************************************************************************
*	Function: OV5640_DCMI_Crop
*
*	Parameter:  Display_XSize, Display_YSize - Display image size
*				  Sensor_XSize, Sensor_YSize - Camera sensor output image size
*
*	Description: Use DCMI cropping function to crop the camera output image to the corresponding display size
*
*	Note: 1. Because the camera output resolution may not match the display, cropping is required
*				 2. The camera output resolution is set by OV5640_Config() or OV5640_Set_Framesize()
*            3. The horizontal effective pixel of DCMI must also be divisible by 4
*				 4. Horizontal and vertical offsets are calculated, no cropping is done if not needed
***************************************************************************************************************************************/
int8_t OV5640_DCMI_Crop(uint16_t Display_XSize,uint16_t Display_YSize,uint16_t Sensor_XSize,uint16_t Sensor_YSize )
{
	uint16_t DCMI_X_Offset,DCMI_Y_Offset;	// Horizontal and vertical offsets, used to center the image when DCMI is cropping horizontally
	uint16_t DCMI_CAPCNT;		// Horizontal effective pixels, used to determine how many PCLKs to sample when capturing horizontally
	uint16_t DCMI_VLINE;			// Vertical effective pixels

	if( (Display_XSize>=Sensor_XSize)|| (Display_YSize>=Sensor_YSize) )
	{
		return OV5640_Error;  // If the actual display size is larger than the camera output size, exit the current function without cropping
	}
// When the format is RGB565, horizontal offset needs to be divisible by 2 to ensure correct color data alignment
// Because one effective pixel has 2 bytes, which requires 2 PCLK cycles, so the starting position should be an even number, then center alignment
// Note: The register value starts from 0, so subtract 1	
	DCMI_X_Offset = Sensor_XSize - Display_XSize; // Actual calculation should be (Sensor_XSize - LCD_XSize)/2*2

// Calculate vertical offset, no cropping is done if not needed, otherwise the register value needs to be adjusted	
	DCMI_Y_Offset = (Sensor_YSize - Display_YSize)/2-1; // Register value starts from 0, so subtract 1

// Because one effective pixel has 2 bytes, which requires 2 PCLK cycles, so divide by 2
// The obtained register value also needs to be divisible by 4	
	DCMI_CAPCNT = Display_XSize*2-1;	// Register value starts from 0, so subtract 1
	
	DCMI_VLINE = Display_YSize-1;		// Vertical effective pixels
	
	HAL_DCMI_ConfigCrop (&hdcmi,DCMI_X_Offset,DCMI_Y_Offset,DCMI_CAPCNT,DCMI_VLINE);// Configure cropping parameters
	HAL_DCMI_EnableCrop(&hdcmi);		// Enable cropping
	
	return OV5640_Success;
}

/***************************************************************************************************************************************
*	Function: OV5640_Reset
*
*	Description: Execute software reset
*
*	Note: Wait for stable power supply during reset          
*
***************************************************************************************************************************************/
void OV5640_Reset(void)
{
	OV5640_Delay(30);  // Wait for module power supply to stabilize for at least 5ms, then pull up PWDN  	
	OV5640_PWDN_OFF;  // PWDN pin pulls low to enter working mode, camera starts working, at this time the camera module's indicator LED is on?
  
// After OV5640 is powered on, PWDN pulls low, need to wait 1ms before pulling up RESET. After RESET pulls low, OV5640 module performs hardware RC reset, and the reset time is about 6~10ms
// That is, wait for the hardware reset to be stable at this time
	OV5640_Delay(5);    
	
// At least >=20ms after reset before executing SCCB operations	
	OV5640_Delay(20);    
	
	SCCB_WriteReg_16Bit(0x3103, 0x11);	// Disable auto frame, wait for SCCB clock before reset instead of waiting for timing automatically
	SCCB_WriteReg_16Bit(0x3008, 0x82);	// Execute a soft reset
	OV5640_Delay(5);  // Delay 5ms
	
}

/***************************************************************************************************************************************
*	Function: OV5640_ReadID
*
*	Description: Read OV5640 device ID
*
***************************************************************************************************************************************/
uint16_t OV5640_ReadID(void)
{
   uint8_t PID_H,PID_L;     // ID bytes
	
   PID_H = SCCB_ReadReg_16Bit(OV5640_ChipID_H); // Read ID high byte
   PID_L = SCCB_ReadReg_16Bit(OV5640_ChipID_L); // Read ID low byte
	
	return(PID_H<<8)|PID_L; // Combine to get complete ID
}

/***************************************************************************************************************************************
*	Function: OV5640_Config
*
*	Description: Configure OV5640 camera related registers
*
*	Note: Refer to dcmi_ov5640_cfg.h for configuration values
*            
***************************************************************************************************************************************/

void OV5640_Config(void)
{
	uint32_t i;	// Loop counter
	
	for(i=0; i<(sizeof(OV5640_INIT_Config)/4); i++)
	{
		SCCB_WriteReg_16Bit(OV5640_INIT_Config[i][0], OV5640_INIT_Config[i][1]); // Write register
		
		OV5640_Delay(1);
	}
}

/***************************************************************************************************************************************
*	Function: OV5640_Set_Pixformat
*
*	Parameter:  pixformat - pixel format, select from: Pixformat_RGB565, Pixformat_GRAY, Pixformat_JPEG
*
*	Description: Set camera pixel format
*
***************************************************************************************************************************************/

void OV5640_Set_Pixformat(uint8_t pixformat)
{
   uint8_t OV5640_Reg;  // Register value

	if( pixformat == Pixformat_JPEG )
	{
		SCCB_WriteReg_16Bit(OV5640_FORMAT_CONTROL, 		0x30);	//	Set data interface output format	
		SCCB_WriteReg_16Bit(OV5640_FORMAT_CONTROL_MUX, 	0x00);	// Set ISP format	
 
		SCCB_WriteReg_16Bit(OV5640_JPEG_MODE_SELECT, 0x02);	 	// JPEG mode 2

		SCCB_WriteReg_16Bit(OV5640_JPEG_VFIFO_CTRL00, 0xA0); 		// JPEG FIFO control
		
		SCCB_WriteReg_16Bit(OV5640_JPEG_VFIFO_HSIZE_H, OV5640_Width>>8);			// JPEG output horizontal size, high byte
		SCCB_WriteReg_16Bit(OV5640_JPEG_VFIFO_HSIZE_L, (uint8_t)OV5640_Width);	// JPEG output horizontal size, low byte
		SCCB_WriteReg_16Bit(OV5640_JPEG_VFIFO_VSIZE_H, OV5640_Height>>8);			// JPEG output vertical size, high byte
		SCCB_WriteReg_16Bit(OV5640_JPEG_VFIFO_VSIZE_L, (uint8_t)OV5640_Height);	// JPEG output vertical size, low byte	
		
	}
	else if( pixformat == Pixformat_GRAY )
	{
		SCCB_WriteReg_16Bit(OV5640_FORMAT_CONTROL, 		0x10);	//	Set data interface output format
		SCCB_WriteReg_16Bit(OV5640_FORMAT_CONTROL_MUX, 	0x00);	// Set ISP format		
	}
	else	// RGB565
	{
		SCCB_WriteReg_16Bit(OV5640_FORMAT_CONTROL, 		0x6F);	// Here set to RGB565 format, output as G[2:0]B[4:0], R[4:0]G[5:3]	
		SCCB_WriteReg_16Bit(OV5640_FORMAT_CONTROL_MUX, 	0x01);	// Set ISP format	
	}
	
   OV5640_Reg = SCCB_ReadReg_16Bit(0x3821);   // Read register value, Bit[5] determines whether to use JPEG mode
	SCCB_WriteReg_16Bit(0x3821,(OV5640_Reg & 0xDF) | ((pixformat == Pixformat_JPEG) ? 0x20 : 0x00));
	 
   OV5640_Reg = SCCB_ReadReg_16Bit(0x3002);   // Read register value, Bit[7], Bit[4], Bit[2] control VFIFO, JFIFO, JPG
	SCCB_WriteReg_16Bit(0x3002,(OV5640_Reg & 0xE3) | ((pixformat == Pixformat_JPEG) ? 0x00 : 0x1C));
	 
   OV5640_Reg = SCCB_ReadReg_16Bit(0x3006);   // Read register value, Bit[5], Bit[3] determines whether to use JPG timing
	SCCB_WriteReg_16Bit(0x3006,(OV5640_Reg & 0xD7) | ((pixformat == Pixformat_JPEG) ? 0x28 : 0x00));

}

/***************************************************************************************************************************************
*	Function: OV5640_Set_JPEG_QuantizationScale
*
*	Parameter: scale - compression level, range 0x01~0x3F
*
*	Description: The larger the value, the stronger the compression, the smaller the image size, but the corresponding image quality will decrease
*
***************************************************************************************************************************************/

void OV5640_Set_JPEG_QuantizationScale(uint8_t scale)
{
	SCCB_WriteReg_16Bit(0x4407, scale); 	// JPEG compression level
}


/***************************************************************************************************************************************
*	Function: OV5640_Set_Framesize
*
*	Parameter:  width - actual output image width, height - actual output image height
*
*	Description: Set actual output image size and timing
*
*	Note: 1. Note that if you want to change the image size, you need to reconfigure the ISP settings at the same time, otherwise the image will be distorted
*            2. The smaller the output image width and height, the higher the frame rate. The frame rate only starts to increase when PLL, HTS, VTS change
*
***************************************************************************************************************************************/

int8_t OV5640_Set_Framesize(uint16_t width,uint16_t height)
{
// Many registers of OV5640 need to be written in group access mode	
    SCCB_WriteReg_16Bit(OV5640_GroupAccess,0X03);  	// Start group 3 access
	
    SCCB_WriteReg_16Bit(OV5640_TIMING_DVPHO_H,width>>8);			// DVPHO sets horizontal size
    SCCB_WriteReg_16Bit(OV5640_TIMING_DVPHO_L,width&0xff);
    SCCB_WriteReg_16Bit(OV5640_TIMING_DVPVO_H,height>>8);		// DVPVO sets vertical size
    SCCB_WriteReg_16Bit(OV5640_TIMING_DVPVO_L,height&0xff);

    SCCB_WriteReg_16Bit(OV5640_GroupAccess,0X13);		// End group access
    SCCB_WriteReg_16Bit(OV5640_GroupAccess,0Xa3);		// Trigger update
	
	return OV5640_Success; 
}

/***************************************************************************************************************************************
*	Function: OV5640_Set_Horizontal_Mirror
*
*	Parameter:  ConfigState - 1 to enable horizontal mirror, 0 to disable
*
*	Description: Configure whether to enable horizontal mirroring of the image
*
***************************************************************************************************************************************/
int8_t OV5640_Set_Horizontal_Mirror( int8_t ConfigState )
{
   uint8_t OV5640_Reg;  // Register value

   OV5640_Reg = SCCB_ReadReg_16Bit(OV5640_TIMING_Mirror);   // Read register value

// Bit[2:1] controls whether to enable horizontal mirror
   if ( ConfigState == OV5640_Enable )    // Enable mirror?
   { 
      OV5640_Reg |= 0X06;  
   } 
   else                    // Disable mirror
   {
      OV5640_Reg &= 0xF9; 	
   }
   return  SCCB_WriteReg_16Bit(OV5640_TIMING_Mirror,OV5640_Reg);   // Write register
}

/***************************************************************************************************************************************
*	Function: OV5640_Set_Vertical_Flip
*
*	Parameter:  ConfigState - 1 to enable vertical flip, 0 to disable
*
*	Description: Configure whether to enable vertical flip of the image
*
***************************************************************************************************************************************/
int8_t OV5640_Set_Vertical_Flip( int8_t ConfigState )
{
   uint8_t OV5640_Reg;  // Register value

   OV5640_Reg = SCCB_ReadReg_16Bit(OV5640_TIMING_Flip);          // Read register value

// Bit[2:1] controls whether to enable vertical flip
   if ( ConfigState == OV5640_Enable )   
   { 
		OV5640_Reg |= 0X06;       
   } 
   else   // Disable flip
   {
      OV5640_Reg &= 0xF9; 	
   }
   return  SCCB_WriteReg_16Bit(OV5640_TIMING_Flip,OV5640_Reg);   // Write register
}


/***************************************************************************************************************************************
*	Function: OV5640_Set_Brightness
*
*	Parameter:  Brightness - brightness level, range -4 to 4. Higher value means brighter image
*
*	Note: 1. Directly use OV5640 built-in brightness register
*            2. Higher brightness value means brighter image, but too bright will wash out details
*				 3. Too low will be too dark
*
***************************************************************************************************************************************/
void OV5640_Set_Brightness(int8_t Brightness)
{
	Brightness = Brightness+4;
	SCCB_WriteReg_16Bit(OV5640_GroupAccess,0X03);  	// Start group 3 access

	SCCB_WriteReg_16Bit( 0x5587, OV5640_Brightness_Config[Brightness][0]);	
	SCCB_WriteReg_16Bit( 0x5588, OV5640_Brightness_Config[Brightness][1]);
	
	SCCB_WriteReg_16Bit(OV5640_GroupAccess,0X13);		// End group access
	SCCB_WriteReg_16Bit(OV5640_GroupAccess,0Xa3);		// Trigger update	
}

/***************************************************************************************************************************************
*	Function: OV5640_Set_Contrast
*
*	Parameter: Contrast - contrast level, range -3 to 3                  
*
*	Note: 1. Directly use OV5640 built-in contrast register
*            2. Higher contrast means more vivid image, but too high will lose details
*
***************************************************************************************************************************************/
void OV5640_Set_Contrast(int8_t Contrast)
{
	Contrast = Contrast+3;
	SCCB_WriteReg_16Bit(OV5640_GroupAccess,0X03);  	// Start group 3 access

	SCCB_WriteReg_16Bit( 0x5586, OV5640_Contrast_Config[Contrast][0]);	
	SCCB_WriteReg_16Bit( 0x5585, OV5640_Contrast_Config[Contrast][1]);
	
	SCCB_WriteReg_16Bit(OV5640_GroupAccess,0X13);		// End group access
	SCCB_WriteReg_16Bit(OV5640_GroupAccess,0Xa3);		// Trigger update	
}
/***************************************************************************************************************************************
*	Function: OV5640_Set_Effect
*
*	Parameter:  effect_Mode - effect mode, select from: OV5640_Effect_Normal, OV5640_Effect_Negative,
*                          OV5640_Effect_BW, OV5640_Effect_Solarize
*
*	Description: Configure OV5640 image effects, such as normal, negative, black and white modes
*
*	Note: There are currently 4 effect modes, for more effects, refer to camera datasheet
*
***************************************************************************************************************************************/
void OV5640_Set_Effect(uint8_t effect_Mode)
{
	SCCB_WriteReg_16Bit(OV5640_GroupAccess,0X03);  	// Start group 3 access

	SCCB_WriteReg_16Bit( 0x5580, OV5640_Effect_Config[effect_Mode][0]);	
	SCCB_WriteReg_16Bit( 0x5583, OV5640_Effect_Config[effect_Mode][1]);
	SCCB_WriteReg_16Bit( 0x5584, OV5640_Effect_Config[effect_Mode][2]);	
	SCCB_WriteReg_16Bit( 0x5003, OV5640_Effect_Config[effect_Mode][3]);	
	SCCB_WriteReg_16Bit(OV5640_GroupAccess,0X13);		// End group access
	SCCB_WriteReg_16Bit(OV5640_GroupAccess,0Xa3);		// Trigger update	
}
/***************************************************************************************************************************************
*	Function: OV5640_Set_Exposure
*
*	Parameter: level - exposure level, range 0~100. Higher value means more exposure
*
*	Description: Set OV5640 camera exposure level
*
*	Note: 1. Increase exposure time by adjusting VTS(frame length) - most effective way
*            2. Increase gain in low light by adjusting AGC gain limit
*            3. Change exposure direction by adjusting AEC target brightness
*            4. level=50 is default normal exposure, greater than 50 increases exposure
*
***************************************************************************************************************************************/
void OV5640_Set_Exposure(uint8_t level)
{
    if (level > 100) level = 100;

    // Get current VTS value (0x380e, 0x380f)
    uint16_t current_vts = SCCB_ReadReg_16Bit(0x380e) << 8;
    current_vts |= SCCB_ReadReg_16Bit(0x380f);

    uint16_t new_vts = current_vts;
    uint16_t aec_target = 0x43;
    uint16_t agc_limit = 0x60;

    if (level > 50) {
        // Increase exposure
        uint8_t offset = level - 50;

        // VTS adjustment: increase frame length to extend exposure time
        // Larger offset means more VTS increase, up to 2.5x
        uint32_t vts_multiplier = 100 + (offset * 3);  // 100~400
        new_vts = (current_vts * vts_multiplier) / 100;
        if (new_vts > 0x0fff) new_vts = 0x0fff;  // VTS max 12 bits

        // AEC target brightness: smaller value means brighter target
        aec_target = 0x43 - (offset / 3);
        if (aec_target < 0x10) aec_target = 0x10;

        // AGC gain limit: larger value allows higher gain
        agc_limit = 0x60 + (offset / 2);
        if (agc_limit > 0xfc) agc_limit = 0xfc;
    } else if (level < 50) {
        // Decrease exposure
        uint8_t offset = 50 - level;

        // VTS decrease
        new_vts = (current_vts * (100 - offset)) / 100;
        if (new_vts < 0x02ee) new_vts = 0x02ee;  // Minimum limit

        // AEC target brightness increase
        aec_target = 0x43 + (offset / 3);
        if (aec_target > 0x60) aec_target = 0x60;

        // AGC gain limit decrease
        agc_limit = 0x60 - (offset / 2);
        if (agc_limit < 0x20) agc_limit = 0x20;
    }

    // Write VTS value
    SCCB_WriteReg_16Bit(0x380e, (new_vts >> 8) & 0x0F);
    SCCB_WriteReg_16Bit(0x380f, new_vts & 0xFF);

    // Write AEC target
    SCCB_WriteReg_16Bit(0x3a13, aec_target);

    // Write AGC gain limit
    SCCB_WriteReg_16Bit(0x3a11, agc_limit);

    // AEC upper limit remains unchanged
    SCCB_WriteReg_16Bit(0x3a19, 0xf8);

    printf("Exposure:%d VTS:%d->%d AEC:0x%02X AGC:0x%02X\r\n",
           level, current_vts, new_vts, aec_target, agc_limit);
}

/***************************************************************************************************************************************
*	Function: OV5640_Download_AF_Firmware
*
*	Description: Download autofocus firmware to OV5640
*
*	Note: Since OV5640 chip has no flash, it needs to be written every power-on
*
***************************************************************************************************************************************/

int8_t OV5640_AF_Download_Firmware(void)
{ 
	uint8_t  AF_Status = 0;		// AF status
	uint16_t i = 0; 				// Loop counter
	uint16_t OV5640_MCU_Addr = 0x8000;	// OV5640 MCU storage area start address is 0x8000, size is 4KB
	
	SCCB_WriteReg_16Bit(0x3000, 0x20);	// Bit[5] reset MCU, before writing firmware, need to execute this command
// Start writing firmware, write in blocks for faster speed
	SCCB_WriteBuffer_16Bit( OV5640_MCU_Addr,(uint8_t *)OV5640_AF_Firmware,sizeof(OV5640_AF_Firmware) );
	SCCB_WriteReg_16Bit(0x3000,0x00);  // Bit[5] write complete, set to 0 to enable MCU
	
// After writing firmware, wait for MCU to start running. Try to read status up to 100 times to determine status	
	for(i=0;i<100;i++)	
	{
		AF_Status = SCCB_ReadReg_16Bit(OV5640_AF_FW_STATUS);	// Read status register
		if( AF_Status == 0x7E)
		{
			printf("AF firmware starting>>>\r\n");	
		}			
		if( AF_Status == 0x70)	// Focus release, camera lens starts to focus automatically, which means firmware download is successful
		{
			printf("AF firmware download success\r\n");
			return OV5640_Success;  
		}			
	}
// If AF_Status has not reached 0x70 status after 100 reads, it means firmware download failed	
	printf("Autofocus firmware download failed, return error\r\n");	
	return OV5640_Error;	
}  

/***************************************************************************************************************************************
*	Function: OV5640_AF_QueryStatus
*
*	Return value: OV5640_AF_End - AF completed, OV5640_AF_Focusing - focusing
*
*	Description: Query AF status
*
*	Note: 1. AF time takes about 500ms
*				 2. If AF is not completed, the displayed image may be blurry
*
***************************************************************************************************************************************/

int8_t OV5640_AF_QueryStatus(void)
{
	uint8_t  AF_Status = 0;		// AF status	
	
	AF_Status = SCCB_ReadReg_16Bit(OV5640_AF_FW_STATUS);	// Read status register
	printf("AF_Status:0x%x\r\n",AF_Status);

// AF mode status: 0x10 for single AF mode, 0x20 for continuous AF mode	
	if( (AF_Status == 0x10)||(AF_Status == 0x20) )		
	{
		return OV5640_AF_End;	// AF completed flag
	}
	else
	{
		return OV5640_AF_Focusing;	// Focusing flag
	}
}

/***************************************************************************************************************************************
*	Function: OV5640_AF_Trigger_Constant
*
*	Description: Enable continuous autofocus, OV5640 will keep focusing when the current scene changes until the user stops
*
*	Note: 1. Call OV5640_AF_QueryStatus() to query AF status
*				 2. Call OV5640_AF_Release() to exit continuous AF mode
*				 3. AF time takes about 500ms
*				 4. If the response time is too slow, OV5640 will ignore some AF requests, so it is recommended to use single AF mode
*				
***************************************************************************************************************************************/

void OV5640_AF_Trigger_Constant(void)
{
	SCCB_WriteReg_16Bit(0x3022,0x04);	//	Start continuous AF
}

/***************************************************************************************************************************************
*	Function: OV5640_AF_Trigger_Single
*
*	Description: Trigger a single autofocus
*
*	Note: AF time takes about 500ms, user can call OV5640_AF_QueryStatus() to query AF status
*
***************************************************************************************************************************************/

void OV5640_AF_Trigger_Single(void)
{
	SCCB_WriteReg_16Bit(OV5640_AF_CMD_MAIN,0x03);	// Trigger single autofocus 
}

/***************************************************************************************************************************************
*	Function: OV5640_AF_Release
*
*	Description: Release lens, camera lens starts to focus automatically to the default position
*
***************************************************************************************************************************************/

void OV5640_AF_Release(void)
{
	SCCB_WriteReg_16Bit(OV5640_AF_CMD_MAIN,0x08);	// AF release command		
}

/**
  * @brief  Line Event callback.
  * @param  hdcmi: pointer to a DCMI_HandleTypeDef structure that contains
  *                the configuration information for DCMI.
  * @retval None
  */
void HAL_DCMI_LineEventCallback(DCMI_HandleTypeDef *hdcmi)
{
}

/***************************************************************************************************************************************
*	Function: HAL_DCMI_FrameEventCallback
*
*	Description: Frame event callback, called when each frame of data is received, used for frame synchronization
*
*	Note: The corresponding flag bit should be cleared in other places when processing each frame
***************************************************************************************************************************************/

extern uint8_t ui_mode;
void HAL_DCMI_FrameEventCallback(DCMI_HandleTypeDef *hdcmi)
{
	static uint32_t DCMI_Tick = 0;
   static uint8_t  DCMI_Frame_Count = 0;

 	if(HAL_GetTick() - DCMI_Tick >= 1000)
	{
		DCMI_Tick = HAL_GetTick();
		OV5640_FPS = DCMI_Frame_Count;
		DCMI_Frame_Count = 0;
	}
	DCMI_Frame_Count ++;

	if (ui_mode == 1) {
		PHOTO_CAPTURE_OnFrameComplete();
	}

   OV5640_FrameState = 1;
}

/***************************************************************************************************************************************
*	Function: HAL_DCMI_ErrorCallback
*
*	Description: Error event callback
*
*	Note: Common errors include DMA transfer error, FIFO overflow, etc.
***************************************************************************************************************************************/

void  HAL_DCMI_ErrorCallback(DCMI_HandleTypeDef *hdcmi)
{
}

/*============================================================================
 * Mode Switch Functions for AI and Video Recording
 *==========================================================================*/

/**
 * @brief   Configure OV5640 for AI mode (480x480 with cropping)
 * @retval  0: Success, -1: Error
 */
int8_t OV5640_SetMode_AI(void)
{
    printf("[OV5640] Setting AI mode (480x480)...\r\n");

    /* Stop DCMI first */
    OV5640_DCMI_Stop();

    /* Set frame size to 480x480 */
    if (OV5640_Set_Framesize(480, 480) != OV5640_Success) {
        printf("[OV5640] ERROR: Failed to set frame size\r\n");
        return -1;
    }

    /* Enable DCMI cropping (already configured in DCMI_OV5640_Init) */
    HAL_DCMI_EnableCrop(&hdcmi);

    /* Restart DCMI with continuous mode */
    OV5640_DMA_Transmit_Continuous((uint32_t)Camera_Buffer, Display_Width * DMA_Height * 2);

    printf("[OV5640] AI mode configured\r\n");
    return 0;
}

/**
 * @brief   Configure OV5640 for video recording mode (640x480, no cropping)
 * @retval  0: Success, -1: Error
 */
int8_t OV5640_SetMode_Video(void)
{
    printf("[OV5640] Setting video mode (640x480)...\r\n");

    /* Stop DCMI first */
    OV5640_DCMI_Stop();

    /* Set frame size to 640x480 */
    if (OV5640_Set_Framesize(640, 480) != OV5640_Success) {
        printf("[OV5640] ERROR: Failed to set frame size\r\n");
        return -1;
    }

    /* Disable DCMI cropping for full frame */
    HAL_DCMI_DisableCrop(&hdcmi);

    /* Restart DCMI with continuous mode */
    OV5640_DMA_Transmit_Continuous((uint32_t)Camera_Buffer, Display_Width * DMA_Height * 2);

    printf("[OV5640] Video mode configured\r\n");
    return 0;
}

/**
 * @brief   Configure DCMI for AI mode (480x480 cropping, full frame DMA)
 * @retval  0: Success, -1: Error
 */
int8_t DCMI_SetMode_AI(void)
{
    printf("[DCMI] Configuring for AI mode (480x480 cropping)...\r\n");

    OV5640_DCMI_Stop();

    g_current_camera_width = 480;
    OV5640_DCMI_Crop(480, 480, OV5640_Width, OV5640_Height);

    OV5640_LineCount = 0;
    OV5640_FrameState = 0;

    OV5640_DMA_Transmit_Continuous(Camera_Buffer, 480 * 480 * 2 / 4);

    return 0;
}

/**
 * @brief   Configure DCMI for video recording mode (640x480 cropping, line-by-line DMA)
 * @retval  0: Success, -1: Error
 */
int8_t DCMI_SetMode_Video(void)
{
    printf("[DCMI] Configuring for video mode (640x480 cropping)...\r\n");

    OV5640_DCMI_Stop();

    g_current_camera_width = 640;
    OV5640_DCMI_Crop(640, 480, OV5640_Width, OV5640_Height);

    OV5640_LineCount = 0;
    OV5640_FrameState = 0;

    OV5640_DMA_Transmit_Continuous(Camera_Buffer, 640 * DMA_Height / 2);

    return 0;
}


/* USER CODE END 1 */