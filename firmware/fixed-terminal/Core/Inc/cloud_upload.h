#ifndef __CLOUD_UPLOAD_H__
#define __CLOUD_UPLOAD_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

extern uint8_t g_rx_byte;

uint8_t Cloud_Init(void);
void Cloud_Service(void);
void Cloud_SetTelemetryEnabled(uint8_t enabled);
uint8_t Cloud_IsBusy(void);
uint8_t Cloud_RequestLatestPhotoUpload(const char *filename);
void Cloud_Upload(int flame_flag, int smoke_flag, int rain_flag,
                  float temperature, float humidity,
                  int adc_flame, int adc_smoke, int k230,int help_flag);
void Cloud_UART_RxCallback(UART_HandleTypeDef *huart);
uint8_t Cloud_SendCommandAndWait(const char *cmd,
											const char *expect1,
											const char *expect2,
											const char *expect3,
											uint32_t timeout_ms);
uint8_t Cloud_IsCallActive(void);
uint8_t Cloud_MakeCall(const char *phone_number);
void Cloud_HangupCall(void);

#ifdef __cplusplus
}
#endif

#endif /* __CLOUD_UPLOAD_H__ */
