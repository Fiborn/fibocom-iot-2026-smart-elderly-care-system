/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    dcmi.h
  * @brief   This file contains all the function prototypes for
  *          the dcmi.c file
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
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __DCMI_H__
#define __DCMI_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

#include "sccb.h"  
#include "usart.h"
#include "ltdc.h"
#include <string.h>



 // DCMI״̬��־��������֡�������ʱ����? HAL_DCMI_FrameEventCallback() �жϻص������� 1 
extern volatile uint8_t OV5640_FrameState;  // DCMI状态标志，新帧就绪时由HAL_DCMI_FrameEventCallback()中断回调设置为1
extern volatile uint8_t OV5640_FPS ;      // 帧率

extern volatile uint8_t  OV5640_LineState ;
extern volatile uint16_t OV5640_LineCount ;  

extern volatile uint16_t g_current_camera_width;  // 当前摄像头宽度（AI模式:480, 视频模式:640）

/*------------------------------------------------------------ Buffer Definitions ------------------------------------------------*/
/*
 * AI Image Processing Pipeline Memory Layout:
 *
 * Camera_Buffer (0x24000000)     - 480x480 RGB565 from DCMI DMA (裁剪�?)
 *     �? Image_Downscale_480to96()
 * AI_Input_Buffer (Frame+1MB)    - 96x96 RGB565 (降采样后，用于显示彩�?)
 *     �? RGB565_to_Gray()
 * AI_Gray_Buffer                 - 96x96 uint8 grayscale (AI模型输入)
 *     �? Gray_to_RGB565()
 * AI_Gray_RGB565_Buffer          - 96x96 RGB565 (灰度显示缓冲�?)
 *     �? UINT8_to_INT8() (future)
 * AI_Quant_Buffer                - 96x96 int8 quantized (TFLite输入)
 */

#define Camera_Buffer           0x24000000                                          // DCMI DMA buffer (480x480 RGB565)
#define Frame_Buffer            (SDRAM_BANK_ADDR + LCD_Width * LCD_Height * BytesPerPixel_0)  // LCD display buffer (800x480 RGB565)
#define AI_Input_Buffer         (Frame_Buffer + 0x100000)                           // Downsampled buffer (96x96 RGB565, offset 1MB)
#define AI_Gray_Buffer          (AI_Input_Buffer + 0x5000)                          // Grayscale buffer (96x96 uint8, offset 20KB)
#define AI_Gray_RGB565_Buffer   (AI_Gray_Buffer + 0x3000)                           // Grayscale RGB565 display buffer (96x96 RGB565, offset 12KB)
#define AI_Quant_Buffer         (AI_Gray_RGB565_Buffer + 0x5000)                    // INT8 quantized buffer (96x96 int8, offset 20KB)

/* Cloud upload SDRAM cache region
 * Keep these buffers above the existing frame/AI/photo/JPEG/gallery regions.
 * Base address 0xC02A1000 avoids the currently used low SDRAM area.
 */
#define CLOUD_UPLOAD_SDRAM_BASE      (SDRAM_BANK_ADDR + 0x2A1000)
#define CLOUD_HTTP_RAW_BUFFER_ADDR   (CLOUD_UPLOAD_SDRAM_BASE)
#define CLOUD_HTTP_B64_BUFFER_ADDR   (CLOUD_HTTP_RAW_BUFFER_ADDR + 0x4000)
#define CLOUD_HTTP_JSON_BUFFER_ADDR  (CLOUD_HTTP_B64_BUFFER_ADDR + 0x6000)

/*------------------------------------------------------------ Buffer Sizes ------------------------------------------------*/
#define CAMERA_BUFFER_SIZE          (480 * 480 * 2)     // 460,800 bytes (AI模式完整帧)
#define AI_INPUT_BUFFER_SIZE        (96 * 96 * 2)       // 18,432 bytes (0x4800)
#define AI_GRAY_BUFFER_SIZE         (96 * 96)           // 9,216 bytes (0x2400)
#define AI_GRAY_RGB565_BUFFER_SIZE  (96 * 96 * 2)       // 18,432 bytes (0x4800)
#define AI_QUANT_BUFFER_SIZE        (96 * 96)           // 9,216 bytes (0x2400)

// ĸʽ OV5640_Set_Pixformat() ����
#define 	Pixformat_RGB565   0
#define 	Pixformat_JPEG     1
#define 	Pixformat_GRAY		 2

#define  OV5640_AF_Focusing     	2           // ���ڴ����Զ��Խ���
#define  OV5640_AF_End				1				// �Զ��Խ�����
#define  OV5640_Success   			0           // ͨѶ�ɹ���־
#define  OV5640_Error     			-1          // ͨѶ����

#define  OV5640_Enable    1
#define  OV5640_Disable   0


// OV5640����Чģʽ���� OV5640_Set_Effect() ����
#define  OV5640_Effect_Normal       0  // ����ģʽ
#define  OV5640_Effect_Negative     1  // ��Ƭģʽ��Ҳ������ɫȫ��ȡ��
#define  OV5640_Effect_BW           2  // �ڰ�ģʽ
#define  OV5640_Effect_Solarize  	3  // ����Ƭ����ģʽ

// 1. ����OV5640ʵ�������ͼ���С�����Ը���ʵ�ʵ�Ӧ�û�����ʾ�����е���
// 2. ��������������Ӱ��֡��
// 3. ��Ϊ���õ�OV5640��ISP���ڱ���Ϊ16:9(1920*1080)���û����õ�����ߴ�ҲӦ�����������
// 4. �����Ҫ������������Ҫ�޸ĳ�ʼ��������Ĳ���
#define 	OV5640_Width          880   // ͼ�񳤶� 
#define 	OV5640_Height         495   // ͼ�����?

// 1. ����Ҫ��ʾ�Ļ����С����ֵһ��Ҫ�ܱ�?4��������
// 2. RGB565��ʽ�£����ջ���DCMI��OV5640�����?4:3ͼ��ü�Ϊ��Ӧ��Ļ�ı���?
// 3. JPGģʽ�£���ֵһ��Ҫ�ܱ�8��������
#define 	Display_Width          800   	// ͼ�񳤶� 
#define 	Display_Height         480   // ͼ�����?

#define 	DMA_Height         80   

#define 	Display_BufferSize     Display_Width * Display_Height*2 /4   // DMA�������ݴ�С��32λ����

/*------------------------------------------------------------ ���üĴ��� ------------------------------------------------*/

#define 	OV5640_ChipID_H          	0x300A  	// оƬID�Ĵ��� ���ֽ�
#define 	OV5640_ChipID_L          	0x300B  	// оƬID�Ĵ��� ���ֽ�

#define	OV5640_FORMAT_CONTROL		0x4300	// �������ݽӿ�����ĸ��?	
#define 	OV5640_FORMAT_CONTROL_MUX  0x501F	// ����ISP�ĸ�ʽ

#define	OV5640_JPEG_MODE_SELECT		0x4713	// JPEGģʽѡ����1~6ģʽ���û��������ֲ����˵��?
#define	OV5640_JPEG_VFIFO_CTRL00 	0x4600	// ��������JPEGģʽ2�Ƿ�̶��������
#define	OV5640_JPEG_VFIFO_HSIZE_H	0x4602	// JPEG���ˮƽ�ߴ�?,���ֽ�
#define	OV5640_JPEG_VFIFO_HSIZE_L	0x4603	// JPEG���ˮƽ�ߴ�?,���ֽ�
#define	OV5640_JPEG_VFIFO_VSIZE_H	0x4604	// JPEG�����ֱ�ߴ�?,���ֽ�
#define	OV5640_JPEG_VFIFO_VSIZE_L	0x4605	// JPEG�����ֱ�ߴ�?,���ֽ�

#define 	OV5640_GroupAccess			0X3212	// �Ĵ��������?
#define 	OV5640_TIMING_DVPHO_H		0x3808	// ���ˮƽ�ߴ�?,���ֽ�
#define 	OV5640_TIMING_DVPHO_L		0x3809	// ���ˮƽ�ߴ�?,���ֽ�
#define 	OV5640_TIMING_DVPVO_H		0x380A	// �����ֱ�ߴ�?,���ֽ�
#define 	OV5640_TIMING_DVPVO_L		0x380B   // �����ֱ�ߴ�?,���ֽ�
#define 	OV5640_TIMING_Flip			0x3820	// Bit[2:1]���������Ƿ�ֱ��ת
#define 	OV5640_TIMING_Mirror			0x3821	// Bit[2:1]���������Ƿ�ˮƽ����

#define 	OV5640_AF_CMD_MAIN			0x3022	// AF ������
#define 	OV5640_AF_CMD_ACK				0x3023	// AF ����ȷ��
#define 	OV5640_AF_FW_STATUS			0x3029	// �Խ�״̬�Ĵ���

/*------------------------------------------------------------ �������� ------------------------------------------------*/

int8_t   DCMI_OV5640_Init(void);	// ��ʼSCCB��DCMI��DMA�Լ�����OV5640

void     OV5640_DMA_Transmit_Continuous(uint32_t DMA_Buffer,uint32_t DMA_BufferSize);	// ����DMA���䣬����ģʽ
void     OV5640_DMA_Transmit_Snapshot(uint32_t DMA_Buffer,uint32_t DMA_BufferSize);		//  ����DMA���䣬����ģʽ������һ֡ͼ���ͣ�?
void     OV5640_DCMI_Suspend(void);		// ����DCMI��ֹͣ��������
void     OV5640_DCMI_Resume(void);		// �ָ�DCMI����ʼ��������
void     OV5640_DCMI_Stop(void);			// ��ֹDCMI��DMA����ֹͣDCMI���񣬽�ֹDCMI����
int8_t 	OV5640_DCMI_Crop(uint16_t Displey_XSize,uint16_t Displey_YSize,uint16_t Sensor_XSize,uint16_t Sensor_YSize ); // �ü�����

void     OV5640_Reset(void);				//	ִ��������λ
uint16_t OV5640_ReadID(void);				// ��ȡ����ID
void		OV5640_Config(void);				// ����OV5640�������?
	
void		OV5640_Set_Pixformat(uint8_t pixformat);					// ����ͼ��������?	
void 		OV5640_Set_JPEG_QuantizationScale(uint8_t scale);		// ����JPEG��ʽ��ѹ���ȼ�,ȡֵ 0x01~0x3F
int8_t 	OV5640_Set_Framesize(uint16_t width,uint16_t height);	// ����ʵ�������ͼ���С
int8_t 	OV5640_Set_Horizontal_Mirror( int8_t ConfigState );	// �������������ͼ���Ƿ����ˮƽ����
int8_t 	OV5640_Set_Vertical_Flip( int8_t ConfigState );			//	�������������ͼ���Ƿ���д�ֱ��ת 
void 		OV5640_Set_Brightness(int8_t Brightness);					// ��������
void		OV5640_Set_Contrast(int8_t Contrast);						// ���öԱȶ�
void 		OV5640_Set_Effect(uint8_t effect_Mode);					// 设置特殊效果（彩色�?�黑白等模式�?
void 		OV5640_Set_Exposure(uint8_t level);							// 设置曝光等级(0~100)�?50为默认，>50增加曝光

int8_t 	OV5640_AF_Download_Firmware(void);		//	下载自动对焦固件到OV5640
int8_t 	OV5640_AF_QueryStatus(void);				//	对焦状态查询
void 		OV5640_AF_Trigger_Constant(void);		// 自动对焦 连续 模式
void 		OV5640_AF_Trigger_Single(void);			// 自动对焦 单次 模式 
void 		OV5640_AF_Release(void);					// 释放镜头，释放后回到初始对焦为无限远状态

/*------------------------------------------------------------ 模式切换函数 ------------------------------------------------*/
int8_t DCMI_SetMode_AI(void);       // 切换到AI模式（480x480裁剪，完整帧DMA传输）
int8_t DCMI_SetMode_Video(void);    // 切换到视频模式（640x480裁剪，行级DMA传输）

/*-------------------------------------------------------------- �������ú� ---------------------------------------------*/

#define OV5640_PWDN_PIN            			 GPIO_PIN_3        				 	// PWDN ����      
#define OV5640_PWDN_PORT           			 GPIOE                 			 	// PWDN GPIO�˿�     
#define GPIO_OV5640_PWDN_CLK_ENABLE    	__HAL_RCC_GPIOE_CLK_ENABLE() 		// PWDN GPIO�˿�ʱ��

// �͵�ƽ������������ģʽ������ͷ��������
#define	OV5640_PWDN_OFF	HAL_GPIO_WritePin(OV5640_PWDN_PORT, OV5640_PWDN_PIN, GPIO_PIN_RESET)	

// �ߵ�ƽ���������ģʽ������ͷֹͣ��������ʱ���Ľ������
#define 	OV5640_PWDN_ON		HAL_GPIO_WritePin(OV5640_PWDN_PORT, OV5640_PWDN_PIN, GPIO_PIN_SET)	
  


/* USER CODE END Includes */

extern DCMI_HandleTypeDef hdcmi;

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

void MX_DCMI_Init(void);

/* USER CODE BEGIN Prototypes */

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __DCMI_H__ */

