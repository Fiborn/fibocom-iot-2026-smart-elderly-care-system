/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include "main.h"
#include "dcmi.h"
#include "dma.h"
#include "dma2d.h"
#include "fatfs.h"
#include "ltdc.h"
#include "sdmmc.h"
#include "usart.h"
#include "gpio.h"
#include "fmc.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "led.h"  
#include "touch_800x480.h"
#include "app_x-cube-ai.h"
#include "network_sdram.h"
#include "jpeg_encode.h"
#include "photo_capture.h"
#include "gallery.h"

#include "sensors.h"
#include "dht11.h"
#include "delay.h"
#include "lunar.h"
#include "DS.h"
#include "lcd_fonts.h"
#include "sai.h"
#include "i2c.h"
#include "wav_player.h"
#include "tlv320aic3254.h"
#include "image_utils.h"
#include "display_utils.h"
#include "ui_panels.h"
#include "cloud_upload.h"
#include "buzzer.h"

#include <string.h>
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
// 定义科技蓝颜色
/* 语音停止引脚状态记录 */
uint8_t last_stop_val = 0;
/* WAV 播放器状态 */
uint8_t wav_player_ready = 0;
uint8_t last_cmd = 0;   /* 用于检测cmd变化，避免重复触发 */
uint8_t cmd = 0;  // 当前指令值

//extern char g_k230_result[128];
//extern uint8_t g_k230_new_data;

uint8_t last_k230_fall_val = 0;   /* K230 跌倒引脚上一次电平 */



uint8_t g_fall_alarm_active = 0; // 跌倒报警激活标志
uint32_t alarm_beep_cnt = 0;     // 用于控制滴滴响的计数器

uint8_t g_key_alarm_active = 0;  // 按键报警激活标志（蜂鸣器持续响）
uint8_t HELP_flag = 0;           // HELP标志位：1=按键按下报警中，0=空闲
GPIO_PinState g_key_last_state = GPIO_PIN_SET;
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint16_t rec_press_count = 0;
uint16_t last_press_x = 0;
uint16_t last_press_y = 0;
uint8_t ui_mode = 0;  // 0 = AI mode, 1 = Video recording mode

uint8_t force_refresh_main = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
/* USER CODE BEGIN PFP */
FATFS SD_FatFs;    
FRESULT MyFile_Res; 
static void MAIN_EnterPhotoCaptureMode(const char *source);
static uint8_t MAIN_RequestPhotoCapture(const char *source);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* ============ 音频播放全局变量（放到 main() 函数外或前面） ============ */
/* 建议放到文件顶部 USER CODE BEGIN PV 区域 */
#define MUSIC_COUNT   5    /* 歌曲总数 */
#define JINGJU_COUNT  5    /* 京剧总数 */
#define PHONE_NUMBER            "19996815306"

uint8_t  play_mode      = 0;  /* 0=未播放, 1=歌曲模式, 2=京剧模式 */
uint8_t  music_index    = 1;  /* 当前歌曲索引 1~5 */
uint8_t  jingju_index   = 1;  /* 当前京剧索引 1~5 */

static uint8_t smoke_stable_cnt = 0;
static uint8_t smoke_clear_cnt = 0;
static uint8_t relay_state = 0;
/* 播放当前模式指定索引的曲目 */
static void Play_Current_Track(void)
{
    char filename[32];
    if (play_mode == 1) {
        snprintf(filename, sizeof(filename), "music%d.wav", music_index);
        printf("[AUDIO] Play %s ...\r\n", filename);
    }
    else if (play_mode == 2) {
        snprintf(filename, sizeof(filename), "jingju%d.wav", jingju_index);
        printf("[AUDIO] Play %s ...\r\n", filename);
    }
    else {
        return;  /* 未处于播放模式 */
    }

    WAV_Player_Stop();
    HAL_Delay(10);
    if (WAV_Player_Start(filename) != 0) {
        printf("[AUDIO] Failed to play %s\r\n", filename);
    }
}

static PlayerStateTypeDef last_player_state = PLAYER_IDLE;

static void MAIN_EnterPhotoCaptureMode(const char *source)
{
    if (ui_mode != 0U) {
        return;
    }

    LCD_SetColor(LCD_BackColor);
    LCD_FillRect(0, 0, 640, 480);
    ui_mode = 1;
    UI_DrawRecordPanel();
    DCMI_SetMode_Video();
    printf("[UI] %s switched to Photo Capture mode!\r\n", source);
}

static uint8_t MAIN_RequestPhotoCapture(const char *source)
{
    if (ui_mode == 0U) {
        MAIN_EnterPhotoCaptureMode(source);
    }

    if (ui_mode != 1U) {
        printf("[PHOTO] %s trigger skipped: ui_mode=%u\r\n", source, ui_mode);
        return 0U;
    }

    if (PHOTO_CAPTURE_IsBusy() != 0U) {
        LCD_SetColor(LCD_BLACK);
        LCD_SetFont(&Font24);
        LCD_DisplayString(645, 400, "BUSY...", LCD_WHITE);
        printf("[PHOTO] %s trigger skipped: camera busy\r\n", source);
        return 0U;
    }

    PHOTO_CAPTURE_Request();
    LCD_SetColor(LCD_BLACK);
    LCD_SetFont(&Font24);
    LCD_DisplayString(645, 400, "CAPTURE!", LCD_WHITE);
    printf("[PHOTO] %s trigger accepted\r\n", source);

    return 1U;
}

    uint16_t adc_flame, adc_smoke;
    uint8_t  flame_flag, smoke_flag, rain_flag;
    uint8_t  last_flame = 0xFF, last_smoke = 0xFF, last_rain = 0xFF;
    uint32_t sensor_cnt = 0;
    uint32_t last_sensor_tick = 0;
    uint8_t temperature = 0;
    uint8_t humidity = 0;

    /* 用于记录4个语音引脚上一次的电平 */	
    uint8_t last_time_val    = 0;
    uint8_t last_weather_val = 0;
    uint8_t last_opera_val   = 0;
    uint8_t last_song_val    = 0;

    uint8_t curr_val;
    uint16_t cur_year = 2024;
    uint8_t  cur_mon = 1, cur_day = 1;
    uint8_t  cur_hour = 0, cur_min = 0, cur_sec = 0, cur_week = 1;
    uint8_t  last_sec = 0xFF;     /* 用于判断秒变化 */
    uint8_t  last_day = 0xFF;     /* 用于判断"日"变化（农历只需每天算一次） */
    uint32_t last_rtc_tick = 0;
    LunarDate ld;                 /* 农历日期结构体 */
    char     time_str[64];
    char     lunar_str[64];

		const char *week_cn[] = {
				"",
				"\xD2\xBB",  /* 一 */
				"\xB6\xFE",  /* 二 */
				"\xC8\xFD",  /* 三 */
				"\xCB\xC4",  /* 四 */
  			"\xCE\xE5",  /* 五 */
				"\xC1\xF9",  /* 六 */
				"\xC8\xD5"   /* 日 */
		};


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
    uint16_t adc_flame, adc_smoke;
    uint8_t  flame_flag, smoke_flag, rain_flag;
    uint8_t  last_flame = 0xFF, last_smoke = 0xFF, last_rain = 0xFF;
    uint32_t sensor_cnt = 0;
    uint32_t last_sensor_tick = 0;
    uint8_t temperature = 0;
    uint8_t humidity = 0;

    /* 用于记录4个语音引脚上一次的电平 */	
    uint8_t last_time_val    = 0;
    uint8_t last_weather_val = 0;
    uint8_t last_opera_val   = 0;
    uint8_t last_song_val    = 0;

    uint8_t curr_val;
    uint16_t cur_year = 2024;
    uint8_t  cur_mon = 1, cur_day = 1;
    uint8_t  cur_hour = 0, cur_min = 0, cur_sec = 0, cur_week = 1;
    uint8_t  last_sec = 0xFF;     /* 用于判断秒变化 */
    uint8_t  last_day = 0xFF;     /* 用于判断"日"变化（农历只需每天算一次） */
    uint32_t last_rtc_tick = 0;
    LunarDate ld;                 /* 农历日期结构体 */
    char     time_str[64];
    char     lunar_str[64];

		const char *week_cn[] = {
				"",
				"\xD2\xBB",  /* 一 */
				"\xB6\xFE",  /* 二 */
				"\xC8\xFD",  /* 三 */
				"\xCB\xC4",  /* 四 */
  			"\xCE\xE5",  /* 五 */
				"\xC1\xF9",  /* 六 */
				"\xC8\xD5"   /* 日 */
		};

  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* Enable the CPU Cache */

  /* Enable I-Cache---------------------------------------------------------*/
  SCB_EnableICache();

  /* Enable D-Cache---------------------------------------------------------*/
  SCB_EnableDCache();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART1_UART_Init();
  MX_FMC_Init();
  MX_DMA2D_Init();
  MX_LTDC_Init();
  MX_DCMI_Init();
  MX_SDMMC1_SD_Init();
  MX_FATFS_Init();
  /* USER CODE BEGIN 2 */
	LED_Init();					// Initialize LED
	SDRAM_Initialization_Sequence(&hsdram1);	// SDRAM initialization sequence and mode configuration	
	Buzzer_Init();
	delay_init(480); 
  MX_USART1_UART_Init();
	Sensors_GPIO_Init();
	Sensors_ADC_Init(); 
  Relay_GPIO_Init();
	Voice_Input_Init();
	if(DHT11_Init() != 0) {
			printf("DHT11 Check Failed!\r\n");
	} else {
			printf("DHT11 Check Success!\r\n");
	}

	/* 读取4个引脚的初始电平（上电瞬间的稳定状态） */
	HAL_Delay(200);  // 等待电平稳定
	last_time_val    = GPIO_PIN_RESET;
	last_weather_val = GPIO_PIN_RESET;
	last_opera_val   = GPIO_PIN_RESET;
	last_song_val    = GPIO_PIN_RESET;
  printf("=== Sensors & Voice GPIO Init OK ===\r\n");
	
  DS1302_Init();
	printf("=== DS1302 RTC Init OK ===\r\n"); 
	
	// Copy AI weights from Flash to SDRAM (must be done after SDRAM init)
//	AI_CopyWeightsToSDRAM();
  
	LCD_RGB_Init();		// Initialize LCD
	Touch_Init();				// Initialize touch screen		

	DCMI_OV5640_Init();   			 	// Initialize DCMI and OV5640 camera
//  OV5640_Set_Exposure(100);			// Increase exposure level (0-100) to improve low-light performance for AI recognition
//	OV5640_Set_Brightness(4);			// Increase brightness (-4 to +4), higher value = brighter - temporarily disabled
	
	OV5640_AF_Download_Firmware();	// Download autofocus firmware
	OV5640_AF_Trigger_Constant();		// Continuous autofocus - OV5640 keeps autofocusing until focusing is completed
//	OV5640_AF_Trigger_Single();		// Single autofocus trigger
	
//	Default lens for 120° and 160° wide-angle lenses uses autofocus lens. Users can adjust according to actual situation.
//	OV5640_Set_Vertical_Flip( OV5640_Disable );		// Disable vertical flip
//	OV5640_Set_Horizontal_Mirror( OV5640_Enable );	// Enable horizontal mirror
	
	OV5640_DMA_Transmit_Continuous(Camera_Buffer, 480*480*2/4);	
	
	// AI Network Initialization - use X-CUBE-AI application layer function
//	MX_X_CUBE_AI_Init();
	//LED2_On;	// AI initialization successful, turn on LED2

	BYTE work[4096];

	printf("\r\n===== FATFS File System Initialization =====\r\n");
	printf("[FATFS] SD path: %s\r\n", SDPath);

	MyFile_Res = f_mount(&SDFatFS, SDPath, 1);
	if (MyFile_Res == FR_OK)
	{
		printf("[FATFS] SD card mounted successfully!\r\n");

		DWORD free_clusters, free_sectors, total_sectors;
		FATFS *fs;

		if (f_getfree(SDPath, &free_clusters, &fs) == FR_OK)
		{
			total_sectors = (fs->n_fatent - 2) * fs->csize;
			free_sectors = free_clusters * fs->csize;
			printf("[FATFS] Total capacity: %lu KB\r\n", total_sectors / 2);
			printf("[FATFS] Free space: %lu KB\r\n", free_sectors / 2);
			printf("[FATFS] File system type: %s\r\n", fs->fs_type == FS_FAT12 ? "FAT12" : fs->fs_type == FS_FAT16 ? "FAT16" : "FAT32");
		}
	}
	else
	{
		printf("[FATFS] SD card mount failed! Error code: %d\r\n", MyFile_Res);
		printf("[FATFS] Starting auto-format SD card...\r\n");

		MyFile_Res = f_mkfs(SDPath, FM_FAT32, 0, work, sizeof(work));
		if (MyFile_Res == FR_OK)
		{
			printf("[FATFS] SD card formatted successfully!\r\n");

			MyFile_Res = f_mount(&SDFatFS, SDPath, 1);
			if (MyFile_Res == FR_OK)
			{
				printf("[FATFS] SD card mounted after format!\r\n");
			}
			else
			{
				printf("[FATFS] SD card mount failed after format! Error code: %d\r\n", MyFile_Res);
			}
		}
		else
		{
			printf("[FATFS] SD card format failed! Error code: %d\r\n", MyFile_Res);
			printf("[FATFS] Please check if SD card is damaged or not connected properly!\r\n");
		}
	}

	printf("[FATFS] File system initialization complete!\r\n");
	printf("============================================\r\n");
	
	printf("\r\n===== Audio Player Initialization =====\r\n");
	MX_I2C1_Init();
	printf("[AUDIO] I2C1 Init OK\r\n");
	if (WAV_Player_Init() == 0)
	{
			wav_player_ready = 1;
			printf("[AUDIO] WAV Player Init OK!\r\n");
	}
	else
	{
			wav_player_ready = 0;
			printf("[AUDIO] WAV Player Init Failed!\r\n");
	}
	printf("=======================================\r\n");	
	
  printf("\r\n===== Cloud Upload (L610 UART4) =====\r\n");
	MX_UART4_Init();
	printf("[CLOUD] UART4 Init OK\r\n");
	while (Cloud_Init() != 1)
	{
		printf("[CLOUD] Cloud Init Failed, retrying...\r\n");
		HAL_Delay(500);
	}
	printf("[CLOUD] Cloud Init Success!\r\n");
	printf("====================================\r\n");
	
	// Initialize right side UI panel (AI mode)
	JPEG_Encode_Init();
	PHOTO_CAPTURE_Init();
	GALLERY_Init();
	
	UI_DrawSwitchPanel();
	
	printf("[UI] Right side panel initialized!\r\n");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
//  USART3_Init();
//	K230_Init();
	
K230_Fall_GPIO_Init();
last_k230_fall_val = K230_FALL_READ(); 
	
  while (1)
		{
			Touch_Scan();
			
			if (touchInfo.flag && touchInfo.num > 0)
			{
				if (g_fall_alarm_active)
				{
						g_fall_alarm_active = 0;
						Buzzer_Off();
						printf("[SYS] Fall alarm stopped by touch\r\n");
				}
				if (g_key_alarm_active)
				{
						g_key_alarm_active = 0;
						HELP_flag = 0;
						Buzzer_Off();
						if (Cloud_IsCallActive() != 0U)
						{
							Cloud_HangupCall();
						}
						printf("[SYS] Key alarm stopped by touch, HELP_flag=0\r\n");
				}
				uint16_t touch_x = touchInfo.x[0];
				uint16_t touch_y = touchInfo.y[0];
				
				if (ui_mode == 0)
					{
						// AI mode - handle SHOT button
						if (touch_x >= 645 && touch_x <= 795 && touch_y >= 120 && touch_y <= 210)
						{
							if (touch_x != last_press_x && touch_y != last_press_y)
							{
								rec_press_count++;
								
								LCD_SetColor(LCD_BLACK);
								LCD_SetFont(&Font24);
								LCD_DisplayString(645, 250, "pressed", LCD_WHITE);
								LCD_DisplayString(645, 280, "Count:", LCD_WHITE);
								LCD_DisplayNumber(720, 280, rec_press_count, 3, LCD_WHITE);
								printf("[UI] SHOT button pressed! Count: %d, Touch: (%d, %d)\r\n", rec_press_count, touch_x, touch_y);
								
								last_press_x = touch_x;
								last_press_y = touch_y;
								
								// Switch to photo capture mode
								LCD_SetColor(LCD_BackColor);
								LCD_FillRect(0, 0, 640, 480);
								ui_mode = 1;
								UI_DrawRecordPanel();
								DCMI_SetMode_Video();
								printf("[UI] Switched to Photo Capture mode!\r\n");
							}
						}
						// AI mode - handle GALLERY button
						else if (touch_x >= 645 && touch_x <= 795 && touch_y >= 230 && touch_y <= 320)
						{
							if (touch_x != last_press_x && touch_y != last_press_y)
							{
								last_press_x = touch_x;
								last_press_y = touch_y;
								
								// Switch to gallery mode
								ui_mode = 2;
								GALLERY_Enter(0);
								printf("[UI] GALLERY button pressed! Touch: (%d, %d)\r\n", touch_x, touch_y);
								printf("[UI] Switched to Gallery mode!\r\n");
							}
						}
					}
					else if (ui_mode == 1)
					{
						// Photo capture mode - handle Capture/Back AI buttons
						if (touch_x >= 645 && touch_x <= 795)
						{
							if (touch_y >= 80 && touch_y <= 170)
							{
                                                                if (MAIN_RequestPhotoCapture("UI") != 0U) {
									printf("[UI] Capture button pressed! Touch: (%d, %d)\r\n", touch_x, touch_y);
                                                                } else {
                                                                        printf("[UI] Camera busy, cannot capture!\r\n");
								}
							}
							else if (touch_y >= 190 && touch_y <= 280)
							{
								// Gallery button pressed
								if (touch_x != last_press_x && touch_y != last_press_y)
								{
									last_press_x = touch_x;
									last_press_y = touch_y;
									
									// Switch to gallery mode
									ui_mode = 2;
									GALLERY_Enter(1);
									printf("[UI] GALLERY button pressed! Touch: (%d, %d)\r\n", touch_x, touch_y);
									printf("[UI] Switched to Gallery mode!\r\n");
								}
							}
							else if (touch_y >= 300 && touch_y <= 390)
							{
								// Back AI button pressed
								LCD_SetColor(LCD_BackColor);
								LCD_FillRect(0, 0, 640, 480);
								ui_mode = 0;
								UI_DrawSwitchPanel();
								DCMI_SetMode_AI();
								printf("[UI] Back AI button pressed! Touch: (%d, %d)\r\n", touch_x, touch_y);
								printf("[UI] Switched back to AI mode!\r\n");
								/* ===== 立即强制刷新一次全部主界面 ===== */
								force_refresh_main = 1;
								last_sec = 0xFF;
								last_day = 0xFF;
								last_flame = 0xFF;
								last_smoke = 0xFF;
								last_rain  = 0xFF;
								last_rtc_tick    = 0;   /* 让下次判断立即成立 */
								last_sensor_tick = 0;
								
							}
						}
					}
					else if (ui_mode == 2)
					{
						if (touch_x >= 640 && touch_x <= 800 && touch_y >= 400 && touch_y <= 480)
						{
							if (gallery_ctx.state == GALLERY_STATE_VIEWING) {
								HAL_JPEG_Abort(&jpeg_encode_ctx.hjpeg);
							}
							
							GALLERY_Exit();
							if (gallery_ctx.prev_mode == 0)
							{
								
								ui_mode = 0;
								LCD_SetColor(LCD_BackColor);
								LCD_FillRect(0, 0, 640, 480);
								UI_DrawSwitchPanel();
								DCMI_SetMode_AI();
								printf("[UI] Exited gallery, returned to AI mode!\r\n");
								/* ===== 立即强制刷新一次全部主界面 ===== */
								force_refresh_main = 1;
								last_sec = 0xFF;
								last_day = 0xFF;
								last_flame = 0xFF;
								last_smoke = 0xFF;
								last_rain  = 0xFF;
								last_rtc_tick    = 0;
								last_sensor_tick = 0;
							}
							else if (gallery_ctx.prev_mode == 1)
							{
								ui_mode = 1;
								LCD_SetColor(LCD_BackColor);
								LCD_FillRect(0, 0, 640, 480);
								UI_DrawRecordPanel();
								DCMI_SetMode_Video();
								printf("[UI] Exited gallery, returned to Photo mode!\r\n");
							}
						}
						else
						{
							GALLERY_HandleTouchEvent(touch_x, touch_y);
						}
					}
			}
			else
			{
				last_press_x = 0;
				last_press_y = 0;
				if (ui_mode == 2) {
					GALLERY_HandleTouchRelease();
				}
			}

                Cloud_Service();

		if (ui_mode == 0)
		{
			if (OV5640_FrameState == 1)
			{		
				SCB_InvalidateDCache();
				
//				Image_Downscale_480to96((uint16_t*)Camera_Buffer, (uint16_t*)AI_Input_Buffer);
//				
//				RGB565_to_Gray((uint16_t*)AI_Input_Buffer, (uint8_t*)AI_Gray_Buffer, 96, 96);
//				
//				UINT8_to_INT8((uint8_t*)AI_Gray_Buffer, (int8_t*)AI_Quant_Buffer, 96*96);
//				
//				int32_t gesture_result = AI_Run_Inference((int8_t*)AI_Quant_Buffer);

//				LCD_SetFont(&Font32);
//				LCD_SetColor(LIGHT_BLUE);
//				//LCD_DisplayString(95, 20, "AI MODE", LCD_BackColor);
//				LCD_SetFont(&Font32);
//				LCD_SetColor(LIGHT_BLUE);
//				LCD_DisplayString(65, 20, "AI", LCD_BackColor);
//				LCD_SetColor(LIGHT_BLUE);			
//				LCD_SetTextFont(&CH_Font32);
//				LCD_DisplayText(97, 20, "\xC4\xA3\xCA\xBD",LCD_BackColor);
				
				

//				LCD_CopyBuffer(70, 150, 96, 96, (uint16_t*)AI_Input_Buffer, 96);

//				LCD_SetFont(&Font24);
//				LCD_SetColor(LIGHT_GREEN);
//				LCD_DisplayString(70, 270, "FPS:", LCD_BackColor);
//				LCD_DisplayNumber(118, 270, OV5640_FPS, 2, LCD_BackColor);

//				LCD_SetColor(LIGHT_CYAN);
//				LCD_DisplayString(50, 320, "Gesture:", LCD_BackColor);
//				LCD_DisplayNumber(134, 320, gesture_result, 2, LCD_BackColor);
				
				OV5640_FrameState = 0;
				LED1_Toggle;	
			}
		}
		else if (ui_mode == 1)
		{
			if (OV5640_FrameState == 1 && !PHOTO_CAPTURE_IsBusy())
			{
				LCD_CopyBuffer(0, 0, 640, 480, (uint16_t*)Frame_Buffer, Display_Width);
				OV5640_FrameState = 0;
			}
			
			PHOTO_CAPTURE_Process();
			
			LED1_Toggle;
		}
		else if (ui_mode == 2)
		{
			GALLERY_Process();
			GALLERY_Render();
			LED1_Toggle;
		}

		/* cmd=1 时间 (PA11) */
		curr_val = HAL_GPIO_ReadPin(VOICE_IN_TIME_PORT, VOICE_IN_TIME_PIN);
		if (curr_val == GPIO_PIN_SET && last_time_val == GPIO_PIN_RESET) {
				cmd = 1;
				printf("cmd:%d\r\n", cmd);
		}
		last_time_val = curr_val;

		/* cmd=2 天气 (PA12) */
		curr_val = HAL_GPIO_ReadPin(VOICE_IN_WEATHER_PORT, VOICE_IN_WEATHER_PIN);
		if (curr_val == GPIO_PIN_SET && last_weather_val == GPIO_PIN_RESET) {
				cmd = 2;
				printf("cmd:%d\r\n", cmd);
		}
		last_weather_val = curr_val;

		/* cmd=3 戏曲 (PB10) */
		curr_val = HAL_GPIO_ReadPin(VOICE_IN_OPERA_PORT, VOICE_IN_OPERA_PIN);
		if (curr_val == GPIO_PIN_SET && last_opera_val == GPIO_PIN_RESET) {
				cmd = 3;
				printf("cmd:%d\r\n", cmd);
		}
		last_opera_val = curr_val;

		/* cmd=4 歌曲 (PB11) */
		curr_val = HAL_GPIO_ReadPin(VOICE_IN_SONG_PORT, VOICE_IN_SONG_PIN);
		if (curr_val == GPIO_PIN_SET && last_song_val == GPIO_PIN_RESET) {
				cmd = 4;
				printf("cmd:%d\r\n", cmd);
		}
		last_song_val = curr_val;

		/* cmd=5 停止 (PB15) */
		curr_val = HAL_GPIO_ReadPin(VOICE_IN_STOP_PORT, VOICE_IN_STOP_PIN);
		if (curr_val == GPIO_PIN_SET && last_stop_val == GPIO_PIN_RESET) {
				cmd = 5;
				printf("cmd:%d (STOP)\r\n", cmd);
		}
		last_stop_val = curr_val;

/* ============ 按键检测 (PF6) - 低电平触发蜂鸣器持续响 ============ */
		GPIO_PinState key_state = HAL_GPIO_ReadPin(GPIOF, GPIO_PIN_6);
		if ((key_state == GPIO_PIN_RESET) && (g_key_last_state == GPIO_PIN_SET))
		{
				/* 消抖：延时20ms后再次确认 */
				HAL_Delay(20);
				key_state = HAL_GPIO_ReadPin(GPIOF, GPIO_PIN_6);
				if (key_state == GPIO_PIN_RESET)
				{
						/* 激活按键报警，蜂鸣器持续响，HELP_flag置1 */
						g_key_alarm_active = 1;
						HELP_flag = 1;
					printf("Button pressed, making call...\r\n");
      
					if (Cloud_MakeCall(PHONE_NUMBER))
					{
						printf("Call initiated\r\n");
					}
					else
					{
						printf("Call failed\r\n");
					}
    
					
//						Buzzer_On();

						/* 串口发送HELP */
						printf("HELP\r\n");
						printf("[KEY] Emergency button pressed! HELP_flag=1, Buzzer ON\r\n");
				}
		}
		g_key_last_state = key_state;

/* ============ K230 跌倒检测（GPIO 电平监测） ============ */

    uint8_t k230_val = K230_FALL_READ();
    /* 上升沿：从 0 变 1 才触发一次，避免长期高电平重复触发 */
    if (k230_val == GPIO_PIN_SET && last_k230_fall_val == GPIO_PIN_RESET)
    {
        if (!g_fall_alarm_active)
        {
            g_fall_alarm_active = 1;
            alarm_beep_cnt = 0;
            printf("[ALARM] K230 Fall detected! GPIO HIGH -> Buzzer ON\r\n");
        }

        if (MAIN_RequestPhotoCapture("K230") != 0U) {
            printf("[K230] Rising edge auto capture triggered\r\n");
        } else {
            printf("[K230] Rising edge auto capture skipped\r\n");
        }
    }
    last_k230_fall_val = k230_val;
		
		/* ============ 处理音频播放指令 ============ */
		if (cmd != last_cmd)   /* cmd 变化时才执行，避免重复触发 */
		{
				if (wav_player_ready)
				{
						if (cmd == 1)
						{
								/* 下一首：仅在歌曲/京剧模式下有效 */
								if (play_mode == 1) {
										music_index++;
										if (music_index > MUSIC_COUNT) music_index = 1;
										printf("[AUDIO] Next music -> music%d.wav\r\n", music_index);
										Play_Current_Track();
								}
								else if (play_mode == 2) {
										jingju_index++;
										if (jingju_index > JINGJU_COUNT) jingju_index = 1;
										printf("[AUDIO] Next jingju -> jingju%d.wav\r\n", jingju_index);
										Play_Current_Track();
								}
								else {
										printf("[AUDIO] Next ignored: no active play mode\r\n");
								}
						}
						else if (cmd == 2)
						{
								/* 上一首：仅在歌曲/京剧模式下有效 */
								if (play_mode == 1) {
										if (music_index <= 1) music_index = MUSIC_COUNT;
										else music_index--;
										printf("[AUDIO] Prev music -> music%d.wav\r\n", music_index);
										Play_Current_Track();
								}
								else if (play_mode == 2) {
										if (jingju_index <= 1) jingju_index = JINGJU_COUNT;
										else jingju_index--;
										printf("[AUDIO] Prev jingju -> jingju%d.wav\r\n", jingju_index);
										Play_Current_Track();
								}
								else {
										printf("[AUDIO] Prev ignored: no active play mode\r\n");
								}
						}
						else if (cmd == 3)
						{
								/* 京剧模式：从当前索引开始播放 */
								play_mode = 2;
								printf("[AUDIO] Enter Jingju mode, index=%d\r\n", jingju_index);
								Play_Current_Track();
						}
						else if (cmd == 4)
						{
								/* 歌曲模式：从当前索引开始播放 */
								play_mode = 1;
								printf("[AUDIO] Enter Music mode, index=%d\r\n", music_index);
								Play_Current_Track();
						}
						else if (cmd == 5)
						{
								printf("[AUDIO] Stop playback\r\n");
								WAV_Player_Stop();
								play_mode = 0;   /* 退出播放模式 */
						}
				}
				last_cmd = cmd;
				cmd = 0;   /* 清零，允许相同命令再次触发 */
		}

		/* WAV 播放器周期性处理（必须放在主循环内高频调用） */
		if (wav_player_ready)
		{
				WAV_Player_Process();
		}		

if (wav_player_ready && play_mode != 0)
{
    PlayerStateTypeDef cur_state = WAV_Player_GetState();

    /* 检测跳变：PLAYING -> IDLE，说明一首歌自然播放结束 */
    if (last_player_state == PLAYER_PLAYING && cur_state == PLAYER_IDLE)
    {
        printf("[AUDIO] Track finished, auto next...\r\n");

        if (play_mode == 1)   /* 歌曲模式 */
        {
            music_index++;
            if (music_index > MUSIC_COUNT) music_index = 1;
        }
        else if (play_mode == 2)   /* 京剧模式 */
        {
            jingju_index++;
            if (jingju_index > JINGJU_COUNT) jingju_index = 1;
        }

        Play_Current_Track();
    }

    last_player_state = cur_state;
}		
		
		/* ============== 时间检测（200ms） ============== */

		if (HAL_GetTick() - last_rtc_tick >= 200)
		{
				last_rtc_tick = HAL_GetTick();
				DS1302_ReadTime();
				Parse_RTC_Time(&cur_year, &cur_mon, &cur_day,
											 &cur_hour, &cur_min, &cur_sec, &cur_week);
				if (cur_sec != last_sec)
				{
						last_sec = cur_sec;
//						snprintf(time_str, sizeof(time_str),
//										 "[TIME] %04d-%02d-%02d %02d:%02d:%02d Week-%d",
//										 cur_year, cur_mon, cur_day,
//										 cur_hour, cur_min, cur_sec, cur_week);
//						printf("%s\r\n", time_str);

						/* 仅在 AI 模式刷新 LCD 上的日期/时间 */
						if (ui_mode == 0) {
								UI_UpdateDateTime(cur_year, cur_mon, cur_day,
																	cur_hour, cur_min, cur_sec, cur_week);
						}
						if (cur_day != last_day)
						{
								last_day = cur_day;
								SolarToLunar(cur_year, cur_mon, cur_day, &ld);
								snprintf(lunar_str, sizeof(lunar_str),
												 "[LUNAR] %04d %s%s",
												 ld.l_year, ld.l_month_cn, ld.l_day_cn);
								printf("%s\r\n", lunar_str);
								if (ui_mode == 0) {
										UI_UpdateLunar(&ld);
								}
						}
				}
		}
//			K230_CheckTimeout();
//      if (g_k230_new_data)
//      {
//          printf("[K230] Received: %s\r\n", g_k230_result);
//          // 检查是否包含 FALL_ALARM
//          if (strstr(g_k230_result, "FALL_ALARM") != NULL)
//          {
//              g_fall_alarm_active = 1;
//              printf("[ALARM] Fall detected! Buzzer ON\r\n");
//          }
//          g_k230_new_data = 0; // 处理完清空标志
//          memset(g_k230_result, 0, K230_BUF_LEN); // 清空缓冲区
//          g_k230_buf_len = 0;
//      }
		
		/* ============== 传感器检测（100ms） ============== */
		if (HAL_GetTick() - last_sensor_tick >= 100)
		{
				last_sensor_tick = HAL_GetTick();
		  	sensor_cnt++;
				flame_flag = Flame_DO_Detect();
				smoke_flag = Smoke_DO_Detect();
				rain_flag  = Rain_Detect();
				if (flame_flag != last_flame) {
						if (flame_flag) printf("[ALARM] Fire!\r\n");
						else            printf("[INFO ] Flame Safe\r\n");
						last_flame = flame_flag;
				}
				if (smoke_flag != last_smoke) {
						if (smoke_flag) printf("[ALARM] Smoke!\r\n");
						else            printf("[INFO ] Smoke Safe\r\n");
				  	last_smoke = smoke_flag;
				}
				if (rain_flag != last_rain) {
						if (rain_flag) 
							printf("[ALARM] Water Leak Detected!\r\n");		
						else           
							printf("[INFO ] Dry / Safe\r\n");
						last_rain = rain_flag;
				}

if (smoke_flag) {
    smoke_clear_cnt = 0;
    if (smoke_stable_cnt < 5) smoke_stable_cnt++;
    if (smoke_stable_cnt >= 3 && !relay_state) {
        RELAY_ON();
        relay_state = 1;
        printf("[RELAY] Fan ON (smoke detected)\r\n");
    }
} else {
    smoke_stable_cnt = 0;
    if (smoke_clear_cnt < 30) smoke_clear_cnt++;
    // 烟雾消失后延迟3秒再关（100ms*30）
    if (smoke_clear_cnt >= 30 && relay_state) {
        RELAY_OFF();
        relay_state = 0;
        printf("[RELAY] Fan OFF (smoke cleared)\r\n");
    }
}			
				
				if ((sensor_cnt % 20) == 0)
				{
						adc_flame = Sensors_ADC_Read(ADC_CH_FLAME);
						adc_smoke = Sensors_ADC_Read(ADC_CH_SMOKE);
						DHT11_Read_Data(&temperature, &humidity);
//						printf("F:%d S:%d W:%d | T:%dC H:%d%% | AO_F:%d AO_S:%d\r\n",
//									 flame_flag, smoke_flag, rain_flag,
//									 temperature, humidity, adc_flame, adc_smoke);
						uint8_t fire_warn  = flame_flag || (adc_flame < FLAME_AO_THRESHOLD);
						uint8_t smoke_warn = smoke_flag || (adc_smoke > SMOKE_AO_THRESHOLD);
						uint8_t leak_warn  = rain_flag;

//						if (fire_warn || smoke_warn || leak_warn) {
//								printf("[SYS ] WARN:");
//								if (fire_warn)  printf(" FIRE");
//								if (smoke_warn) printf(" SMOKE");
//								if (leak_warn)  printf(" LEAK");
//								printf("\r\n");
//						} else {
//								printf("[SYS ] ALL SAFE\r\n");
//						}

						/* 刷新 LCD 温湿度 + 传感器状态（仅 AI 模式） */
						if (ui_mode == 0) {
								UI_UpdateTempHumi(temperature, humidity);
								UI_UpdateSensors(flame_flag, smoke_flag, rain_flag,
								 							 adc_flame, adc_smoke);
						}
						
						/* 上传传感器数据到华为云 IoT */
						Cloud_Upload(flame_flag, smoke_flag, rain_flag,
									 (float)temperature, (float)humidity,
                                                                         adc_flame, adc_smoke,
                                                                         (k230_val == GPIO_PIN_SET) ? 1 : 0,
									                                                       HELP_flag);
				}
				if (g_fall_alarm_active)
				{
						alarm_beep_cnt++;
						// 每 200ms 翻转一次状态 (100ms 响, 100ms 灭)
						if (alarm_beep_cnt % 2 == 0) {
								Buzzer_On();
						} else {
								Buzzer_Off();
						}
				}		

		}
		/* ============== 强制刷新主界面（切换回来时用） ============== */
		if (force_refresh_main && ui_mode == 0)
		{
				force_refresh_main = 0;

				/* 读一次 RTC 立刻显示时间 */
				DS1302_ReadTime();
				Parse_RTC_Time(&cur_year, &cur_mon, &cur_day,
											 &cur_hour, &cur_min, &cur_sec, &cur_week);
				last_sec = cur_sec;
				last_day = cur_day;

				/* 农历（每天一次即可，这里也强制算一次） */
				SolarToLunar(cur_year, cur_mon, cur_day, &ld);

				/* 读一次传感器立刻显示 */
				flame_flag = Flame_DO_Detect();
				smoke_flag = Smoke_DO_Detect();
				rain_flag  = Rain_Detect();
				adc_flame  = Sensors_ADC_Read(ADC_CH_FLAME);
				adc_smoke  = Sensors_ADC_Read(ADC_CH_SMOKE);
				DHT11_Read_Data(&temperature, &humidity);
				last_flame = flame_flag;
				last_smoke = smoke_flag;
				last_rain  = rain_flag;

				/* 一次性刷新所有 UI 区域 */
				UI_UpdateDateTime(cur_year, cur_mon, cur_day,
													cur_hour, cur_min, cur_sec, cur_week);
				UI_UpdateLunar(&ld);
				UI_UpdateTempHumi(temperature, humidity);
				UI_UpdateSensors(flame_flag, smoke_flag, rain_flag, adc_flame, adc_smoke);

				/* 重置计时基准 */
				last_rtc_tick    = HAL_GetTick();
				last_sensor_tick = HAL_GetTick();
				sensor_cnt       = 0;

				printf("[UI] Main page force refresh done!\r\n");
		}
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

  }
  /* USER CODE END 3 */
}


/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
	RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 5;
  RCC_OscInitStruct.PLL.PLLN = 192;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
	
	/* 让 PLL2 单独为 ADC 提供时钟 */
	PeriphClkInitStruct.PLL2.PLL2M = 5;
	PeriphClkInitStruct.PLL2.PLL2N = 40;    /* 5MHz * 40 = 200MHz VCO */
	PeriphClkInitStruct.PLL2.PLL2P = 4;     /* 200/4 = 50MHz -> ADC */
	PeriphClkInitStruct.PLL2.PLL2Q = 2;
	PeriphClkInitStruct.PLL2.PLL2R = 2;
	PeriphClkInitStruct.PLL2.PLL2RGE = RCC_PLL2VCIRANGE_2;
	PeriphClkInitStruct.PLL2.PLL2VCOSEL = RCC_PLL2VCOMEDIUM;
	PeriphClkInitStruct.PLL2.PLL2FRACN = 0;

	/* 同时保留 PLL3 (LCD 用) */
	PeriphClkInitStruct.PLL3.PLL3M = 25;
	PeriphClkInitStruct.PLL3.PLL3N = 330;
	PeriphClkInitStruct.PLL3.PLL3P = 2;
	PeriphClkInitStruct.PLL3.PLL3Q = 2;
	PeriphClkInitStruct.PLL3.PLL3R = 10;
	PeriphClkInitStruct.PLL3.PLL3RGE = RCC_PLL3VCIRANGE_0;
	PeriphClkInitStruct.PLL3.PLL3VCOSEL = RCC_PLL3VCOMEDIUM;
	PeriphClkInitStruct.PLL3.PLL3FRACN = 0;

	PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_ADC | RCC_PERIPHCLK_LTDC;
	PeriphClkInitStruct.AdcClockSelection = RCC_ADCCLKSOURCE_PLL2;    /* 关键 */

	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
	{
			Error_Handler();
	}	
	
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0xC0000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_32MB;
  MPU_InitStruct.SubRegionDisable = 0x0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Number = MPU_REGION_NUMBER1;
  MPU_InitStruct.BaseAddress = 0x24000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_512KB;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Number = MPU_REGION_NUMBER2;
  MPU_InitStruct.BaseAddress = 0x30000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_256KB;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
