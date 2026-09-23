#include "cloud_upload.h"
#include "usart.h"
#include "fatfs.h"
#include "dcmi.h"
#include "sd_lock.h"
#include "string.h"
#include "stdio.h"
#include "stdint.h"

extern UART_HandleTypeDef huart4;

#define RX_BUF_SIZE                     2048U
#define CLOUD_HTTP_BASE64_CHUNK_MAX    (8U * 1024U)
#define CLOUD_HTTP_RAW_CHUNK_MAX       ((CLOUD_HTTP_BASE64_CHUNK_MAX / 4U) * 3U)
#define CLOUD_HTTP_B64_BUFFER_SIZE     ((((CLOUD_HTTP_RAW_CHUNK_MAX + 2U) / 3U) * 4U) + 1U)
#define CLOUD_HTTP_JSON_BUFFER_SIZE    22000U
#define CLOUD_HTTP_CMD_BUFFER_SIZE     128U
#define CLOUD_HTTP_JSON_SETTLE_MS      100U
#define CLOUD_HTTP_RETRY_MAX           1U

typedef enum
{
  CLOUD_WAIT_IDLE = 0,
  CLOUD_WAIT_PENDING,
  CLOUD_WAIT_OK,
  CLOUD_WAIT_FAIL
} Cloud_WaitStateTypeDef;

typedef enum
{
  CLOUD_HTTP_IDLE = 0,
  CLOUD_HTTP_OPEN_FILE,
  CLOUD_HTTP_PREPARE_CHUNK,
  CLOUD_HTTP_SET_URL,
  CLOUD_HTTP_SET_CONTYPE,
  CLOUD_HTTP_SEND_DATA_CMD,
  CLOUD_HTTP_SEND_JSON,
  CLOUD_HTTP_ACT,
  CLOUD_HTTP_WAIT_RESULT,
  CLOUD_HTTP_FINISH,
  CLOUD_HTTP_ERROR
} Cloud_HttpStateTypeDef;

typedef struct
{
  Cloud_HttpStateTypeDef state;
  uint8_t started;
  uint8_t file_opened;
  uint8_t telemetry_prev_enabled;
  uint8_t retry_count;
  char filename[32];
  FIL file;
  uint32_t file_size;
  uint32_t chunk_index;
  uint32_t total_chunks;
  uint32_t bytes_consumed;
  uint32_t current_raw_len;
  uint32_t current_b64_len;
  uint32_t current_json_len;
  uint32_t json_sent_tick;
} Cloud_HttpContextTypeDef;

static volatile char g_rx_buffer[RX_BUF_SIZE];
static volatile uint16_t g_rx_len = 0;
uint8_t g_rx_byte;

static char temp[512];
static char json_raw[256];
static char json_escaped[512];
static uint8_t g_cloud_telemetry_enabled = 1;
static uint8_t g_cloud_rx_started = 0;
static uint8_t g_call_active = 0;
static uint8_t g_call_prev_telemetry_enabled = 1;
static volatile SD_LockOwnerTypeDef g_sd_lock_owner = SD_LOCK_OWNER_NONE;

static Cloud_HttpContextTypeDef g_http_ctx;
static char g_http_cmd[CLOUD_HTTP_CMD_BUFFER_SIZE];
static uint8_t *g_http_raw_chunk = (uint8_t *)CLOUD_HTTP_RAW_BUFFER_ADDR;
static char *g_http_b64_chunk = (char *)CLOUD_HTTP_B64_BUFFER_ADDR;
static char *g_http_json_chunk = (char *)CLOUD_HTTP_JSON_BUFFER_ADDR;

static const char *device_id = "6a1981b67f2e6c302f75b255";
static const char *device_name = "flame_flag";
static const char *http_url = "http://110.42.233.31:5000/upload_chunk";

static const char g_base64_table[] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static const char *g_wait_expect1 = NULL;
static const char *g_wait_expect2 = NULL;
static const char *g_wait_expect3 = NULL;
static uint32_t g_wait_start_tick = 0;
static uint32_t g_wait_timeout_ms = 0;

static uint8_t Cloud_BufferContains(const char *expected);

static void Cloud_InvalidateDCache(const void *addr, uint32_t size)
{
  uintptr_t start;
  uintptr_t end;
  uint32_t aligned_size;

  if ((addr == NULL) || (size == 0U))
  {
    return;
  }

  start = ((uintptr_t)addr) & ~(uintptr_t)31U;
  end = (((uintptr_t)addr) + (uintptr_t)size + 31U) & ~(uintptr_t)31U;
  aligned_size = (uint32_t)(end - start);

  SCB_InvalidateDCache_by_Addr((uint32_t *)start, (int32_t)aligned_size);
}

static const char *Cloud_HttpStateToString(Cloud_HttpStateTypeDef state)
{
  switch (state)
  {
    case CLOUD_HTTP_IDLE: return "IDLE";
    case CLOUD_HTTP_OPEN_FILE: return "OPEN_FILE";
    case CLOUD_HTTP_PREPARE_CHUNK: return "PREPARE_CHUNK";
    case CLOUD_HTTP_SET_URL: return "SET_URL";
    case CLOUD_HTTP_SET_CONTYPE: return "SET_CONTYPE";
    case CLOUD_HTTP_SEND_DATA_CMD: return "SEND_DATA_CMD";
    case CLOUD_HTTP_SEND_JSON: return "SEND_JSON";
    case CLOUD_HTTP_ACT: return "ACT";
    case CLOUD_HTTP_WAIT_RESULT: return "WAIT_RESULT";
    case CLOUD_HTTP_FINISH: return "FINISH";
    case CLOUD_HTTP_ERROR: return "ERROR";
    default: return "UNKNOWN";
  }
}

const char *SD_Lock_OwnerName(SD_LockOwnerTypeDef owner)
{
  switch (owner)
  {
    case SD_LOCK_OWNER_NONE: return "NONE";
    case SD_LOCK_OWNER_GALLERY_SCAN: return "GALLERY_SCAN";
    case SD_LOCK_OWNER_GALLERY_VIEW: return "GALLERY_VIEW";
    case SD_LOCK_OWNER_PHOTO_WRITE: return "PHOTO_WRITE";
    case SD_LOCK_OWNER_HTTP_UPLOAD: return "HTTP_UPLOAD";
    case SD_LOCK_OWNER_WAV_PLAYER: return "WAV_PLAYER";
    default: return "UNKNOWN";
  }
}

uint8_t SD_Lock_TryAcquire(SD_LockOwnerTypeDef owner)
{
  if ((g_sd_lock_owner == SD_LOCK_OWNER_NONE) || (g_sd_lock_owner == owner))
  {
    g_sd_lock_owner = owner;
    return 1U;
  }

  return 0U;
}

void SD_Lock_Release(SD_LockOwnerTypeDef owner)
{
  if (g_sd_lock_owner == owner)
  {
    g_sd_lock_owner = SD_LOCK_OWNER_NONE;
  }
}

uint8_t SD_Lock_IsOwnedBy(SD_LockOwnerTypeDef owner)
{
  return (g_sd_lock_owner == owner) ? 1U : 0U;
}

SD_LockOwnerTypeDef SD_Lock_GetOwner(void)
{
  return g_sd_lock_owner;
}

static void Cloud_PrintRxSnapshot(const char *tag)
{
  char preview[161];
  uint16_t copy_len = g_rx_len;

  if (copy_len > 160U)
  {
    copy_len = 160U;
  }

  memset(preview, 0, sizeof(preview));
  if (copy_len > 0U)
  {
    memcpy(preview, (const void *)g_rx_buffer, copy_len);
  }
  preview[copy_len] = '\0';

  printf("[HTTP][RX] %s len=%u data=%s\r\n",
         tag,
         (unsigned int)g_rx_len,
         (copy_len > 0U) ? preview : "<empty>");
}

static void Cloud_ClearRxBuffer(void)
{
  memset((void *)g_rx_buffer, 0, RX_BUF_SIZE);
  g_rx_len = 0;
}

static void Cloud_SetCallActive(uint8_t active)
{
  if (active != 0U)
  {
    if (g_call_active == 0U)
    {
      g_call_prev_telemetry_enabled = g_cloud_telemetry_enabled;
      g_cloud_telemetry_enabled = 0U;
    }

    g_call_active = 1U;
  }
  else
  {
    g_call_active = 0U;
    g_cloud_telemetry_enabled = g_call_prev_telemetry_enabled;
  }
}

static uint8_t Cloud_CallBufferHasFailure(void)
{
  return (Cloud_BufferContains("NO CARRIER") ||
          Cloud_BufferContains("BUSY") ||
          Cloud_BufferContains("NO ANSWER") ||
          Cloud_BufferContains("NO DIALTONE")) ? 1U : 0U;
}

static int8_t Cloud_ParseClccState(void)
{
  const char *clcc = strstr((char *)g_rx_buffer, "+CLCC:");
  int idx;
  int dir;
  int stat;
  int mode;
  int mpty;
  int type;
  char number[32];

  if (clcc == NULL)
  {
    return -1;
  }

  memset(number, 0, sizeof(number));

  if (sscanf(clcc,
             "+CLCC: %d,%d,%d,%d,%d,\"%31[^\"]\",%d",
             &idx,
             &dir,
             &stat,
             &mode,
             &mpty,
             number,
             &type) >= 5)
  {
    return (int8_t)stat;
  }

  if (sscanf(clcc, "+CLCC: %d,%d,%d,%d,%d", &idx, &dir, &stat, &mode, &mpty) == 5)
  {
    return (int8_t)stat;
  }

  return -1;
}

static const char *Cloud_CallStateToString(int8_t state)
{
  switch (state)
  {
    case 0: return "ACTIVE";
    case 1: return "HELD";
    case 2: return "DIALING";
    case 3: return "ALERTING";
    case 4: return "INCOMING";
    case 5: return "WAITING";
    default: return "UNKNOWN";
  }
}

static uint8_t Cloud_PrepareVoiceCall(void)
{
  if (!Cloud_SendCommandAndWait("AT+CAVIMS=1\r\n", "OK", NULL, NULL, 3000))
  {
    printf("[CALL] warning: AT+CAVIMS=1 unsupported or failed, continue\r\n");
  }

  if (!Cloud_SendCommandAndWait("AT+GTFIRST=0\r\n", "OK", NULL, NULL, 3000))
  {
    printf("[CALL] warning: AT+GTFIRST=0 unsupported or failed, continue\r\n");
  }

  if (!Cloud_SendCommandAndWait("AT+CALLBREAK=1\r\n", "OK", NULL, NULL, 3000))
  {
    printf("[CALL] warning: AT+CALLBREAK=1 unsupported or failed, continue\r\n");
  }

  if (!Cloud_SendCommandAndWait("AT+CLIP=1\r\n", "OK", NULL, NULL, 3000))
  {
    printf("[CALL] warning: AT+CLIP=1 unsupported or failed, continue\r\n");
  }

  if (!Cloud_SendCommandAndWait("AT+CLVL=5\r\n", "OK", NULL, NULL, 3000))
  {
    printf("[CALL] warning: AT+CLVL=5 unsupported or failed, continue\r\n");
  }

  return 1U;
}

static uint8_t Cloud_WaitForCallProgress(uint32_t timeout_ms)
{
  uint32_t start_tick = HAL_GetTick();

  while ((HAL_GetTick() - start_tick) < timeout_ms)
  {
    if (!Cloud_SendCommandAndWait("AT+CLCC\r\n", "OK", NULL, NULL, 2000))
    {
      if (Cloud_CallBufferHasFailure() != 0U)
      {
        Cloud_PrintRxSnapshot("call progress failed");
        return 0U;
      }

      HAL_Delay(200);
      continue;
    }

    if (Cloud_CallBufferHasFailure() != 0U)
    {
      Cloud_PrintRxSnapshot("call progress failed");
      return 0U;
    }

    if (Cloud_BufferContains("+CLCC:"))
    {
      int8_t state = Cloud_ParseClccState();
      printf("[CALL] CLCC state=%d(%s)\r\n", state, Cloud_CallStateToString(state));

      if ((state == 0) || (state == 2) || (state == 3))
      {
        return 1U;
      }
    }

    HAL_Delay(500);
  }

  Cloud_PrintRxSnapshot("call progress timeout");
  return 0U;
}

static void Cloud_SendString(const char *data)
{
  uint32_t data_len = (uint32_t)strlen(data);
  if (data_len > 120U)
  {
    printf("[HTTP][TX] len=%lu preview=%.120s...\r\n",
           (unsigned long)data_len,
           data);
  }
  else
  {
    printf("[HTTP][TX] %s\r\n", data);
  }
  HAL_UART_Transmit(&huart4, (uint8_t *)data, (uint16_t)strlen(data), 5000);
}

static void Escape_JSON_To_AT(const char *src, char *dest, uint16_t dest_size)
{
  uint16_t i = 0;
  uint16_t j = 0;

  while (src[i] != '\0' && j < dest_size - 3U)
  {
    if (src[i] == '"')
    {
      dest[j++] = '\\';
      dest[j++] = '"';
    }
    else
    {
      dest[j++] = src[i];
    }
    i++;
  }

  dest[j] = '\0';
}

static uint8_t Cloud_BufferContains(const char *expected)
{
  if (expected == NULL)
  {
    return 0;
  }

  return (strstr((char *)g_rx_buffer, expected) != NULL) ? 1U : 0U;
}

static void Cloud_WaitStart(const char *expect1, const char *expect2, const char *expect3, uint32_t timeout_ms)
{
  g_wait_expect1 = expect1;
  g_wait_expect2 = expect2;
  g_wait_expect3 = expect3;
  g_wait_timeout_ms = timeout_ms;
  g_wait_start_tick = HAL_GetTick();

  printf("[HTTP][WAIT] state=%s expect1=%s expect2=%s expect3=%s timeout=%lu\r\n",
         Cloud_HttpStateToString(g_http_ctx.state),
         (expect1 != NULL) ? expect1 : "<null>",
         (expect2 != NULL) ? expect2 : "<null>",
         (expect3 != NULL) ? expect3 : "<null>",
         (unsigned long)timeout_ms);
}

static Cloud_WaitStateTypeDef Cloud_WaitPoll(void)
{
  if (Cloud_BufferContains(g_wait_expect1) ||
      Cloud_BufferContains(g_wait_expect2) ||
      Cloud_BufferContains(g_wait_expect3))
  {
    return CLOUD_WAIT_OK;
  }

  if (Cloud_BufferContains("ERROR"))
  {
    Cloud_PrintRxSnapshot("matched ERROR");
    return CLOUD_WAIT_FAIL;
  }

  if (Cloud_BufferContains("+HTTPRES:") && !Cloud_BufferContains("+HTTPRES: 1,200,"))
  {
    Cloud_PrintRxSnapshot("matched non-200 HTTPRES");
    return CLOUD_WAIT_FAIL;
  }

  if ((HAL_GetTick() - g_wait_start_tick) >= g_wait_timeout_ms)
  {
    printf("[HTTP][WAIT] timeout state=%s after=%lu ms\r\n",
           Cloud_HttpStateToString(g_http_ctx.state),
           (unsigned long)(HAL_GetTick() - g_wait_start_tick));
    Cloud_PrintRxSnapshot("timeout");
    return CLOUD_WAIT_FAIL;
  }

  return CLOUD_WAIT_PENDING;
}

uint8_t Cloud_SendCommandAndWait(const char *cmd,
                                        const char *expect1,
                                        const char *expect2,
                                        const char *expect3,
                                        uint32_t timeout_ms)
{
  Cloud_ClearRxBuffer();
  Cloud_SendString(cmd);
  Cloud_WaitStart(expect1, expect2, expect3, timeout_ms);

  while (1)
  {
    Cloud_WaitStateTypeDef wait_state = Cloud_WaitPoll();

    if (wait_state == CLOUD_WAIT_OK)
    {
      return 1;
    }

    if (wait_state == CLOUD_WAIT_FAIL)
    {
      return 0;
    }

    HAL_Delay(10);
  }
}

static uint32_t Cloud_Base64Encode(const uint8_t *src, uint32_t src_len, char *dest, uint32_t dest_size)
{
  uint32_t i = 0;
  uint32_t out_len = 0;

  while (i < src_len)
  {
    uint32_t remain = src_len - i;
    uint8_t octet_a = src[i++];
    uint8_t octet_b = (remain > 1U) ? src[i++] : 0U;
    uint8_t octet_c = (remain > 2U) ? src[i++] : 0U;
    uint32_t triple = ((uint32_t)octet_a << 16) | ((uint32_t)octet_b << 8) | octet_c;

    if ((out_len + 4U) >= dest_size)
    {
      break;
    }

    dest[out_len++] = g_base64_table[(triple >> 18) & 0x3FU];
    dest[out_len++] = g_base64_table[(triple >> 12) & 0x3FU];
    dest[out_len++] = (remain > 1U) ? g_base64_table[(triple >> 6) & 0x3FU] : '=';
    dest[out_len++] = (remain > 2U) ? g_base64_table[triple & 0x3FU] : '=';
  }

  if (out_len < dest_size)
  {
    dest[out_len] = '\0';
  }
  else
  {
    dest[dest_size - 1U] = '\0';
  }

  return out_len;
}

static void Cloud_CloseUploadFile(void)
{
  if (g_http_ctx.file_opened != 0U)
  {
    f_close(&g_http_ctx.file);
    g_http_ctx.file_opened = 0U;
  }

  SD_Lock_Release(SD_LOCK_OWNER_HTTP_UPLOAD);
}

static void Cloud_EnsureRxStarted(void)
{
  if (g_cloud_rx_started == 0U)
  {
    if (HAL_UART_Receive_IT(&huart4, &g_rx_byte, 1) == HAL_OK)
    {
      g_cloud_rx_started = 1U;
    }
  }
}

static void Cloud_RestoreTelemetry(void)
{
  g_cloud_telemetry_enabled = g_http_ctx.telemetry_prev_enabled;
}

static void Cloud_ResetHttpContext(void)
{
  memset(&g_http_ctx, 0, sizeof(g_http_ctx));
  memset(g_http_raw_chunk, 0, CLOUD_HTTP_RAW_CHUNK_MAX);
  memset(g_http_b64_chunk, 0, CLOUD_HTTP_B64_BUFFER_SIZE);
  memset(g_http_json_chunk, 0, CLOUD_HTTP_JSON_BUFFER_SIZE);
  memset(g_http_cmd, 0, sizeof(g_http_cmd));
}

static void Cloud_FailCurrentUpload(const char *reason)
{
  printf("[HTTP] upload failed: %s\r\n", reason);
  printf("[HTTP] fail ctx: file=%s state=%s chunk=%lu/%lu retry=%u\r\n",
         g_http_ctx.filename,
         Cloud_HttpStateToString(g_http_ctx.state),
         (unsigned long)(g_http_ctx.chunk_index + 1U),
         (unsigned long)g_http_ctx.total_chunks,
         (unsigned int)g_http_ctx.retry_count);
  g_http_ctx.state = CLOUD_HTTP_ERROR;
  g_http_ctx.started = 0U;
}

static void Cloud_BeginStep(void)
{
  g_http_ctx.started = 1U;
}

static void Cloud_EndStep(void)
{
  g_http_ctx.started = 0U;
}

static void Cloud_HandleChunkFailure(const char *reason)
{
  printf("[HTTP] chunk %lu/%lu failed: %s\r\n",
         (unsigned long)(g_http_ctx.chunk_index + 1U),
         (unsigned long)g_http_ctx.total_chunks,
         reason);
  printf("[HTTP] state=%s file=%s retry=%u\r\n",
         Cloud_HttpStateToString(g_http_ctx.state),
         g_http_ctx.filename,
         (unsigned int)g_http_ctx.retry_count);
  Cloud_PrintRxSnapshot(reason);

  if (g_http_ctx.retry_count < CLOUD_HTTP_RETRY_MAX)
  {
    g_http_ctx.retry_count++;
    g_http_ctx.state = CLOUD_HTTP_SET_URL;
    g_http_ctx.started = 0U;
    printf("[HTTP] chunk %lu/%lu retry %u\r\n",
           (unsigned long)(g_http_ctx.chunk_index + 1U),
           (unsigned long)g_http_ctx.total_chunks,
           (unsigned int)g_http_ctx.retry_count);
  }
  else
  {
    Cloud_FailCurrentUpload(reason);
  }
}

void Cloud_UART_RxCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == UART4)
  {
    g_cloud_rx_started = 1U;

    if (g_rx_len < (RX_BUF_SIZE - 1U))
    {
      g_rx_buffer[g_rx_len++] = (char)g_rx_byte;
      g_rx_buffer[g_rx_len] = '\0';
    }
    else
    {
      g_rx_len = 0;
      g_rx_buffer[g_rx_len] = '\0';
    }

    HAL_UART_Receive_IT(&huart4, &g_rx_byte, 1);
  }
}

uint8_t Cloud_Init(void)
{
  Cloud_EnsureRxStarted();

  if (!Cloud_SendCommandAndWait("AT\r\n", "OK", NULL, NULL, 1000))
  {
    return 0;
  }

  Cloud_SendCommandAndWait("AT+CGMR?\r\n", "OK", NULL, NULL, 1000);

  if (!Cloud_SendCommandAndWait("AT+CPIN?\r\n", "+CPIN: READY", NULL, NULL, 2000))
  {
    return 0;
  }

  if (!Cloud_SendCommandAndWait("AT+CSQ?\r\n", "+CSQ:", NULL, NULL, 2000))
  {
    return 0;
  }

  if (!Cloud_SendCommandAndWait("AT+CGREG?\r\n", "+CGREG: 0,1", "+CGREG: 0,5", NULL, 3000))
  {
    return 0;
  }

  if (!Cloud_PrepareVoiceCall())
  {
    return 0;
  }

  Cloud_SendCommandAndWait("AT+COPS?\r\n", "OK", NULL, NULL, 2000);

  if (!Cloud_SendCommandAndWait("AT+MIPCALL?\r\n", "+MIPCALL: 1,", NULL, NULL, 2000))
  {
    if (!Cloud_SendCommandAndWait("AT+MIPCALL=1\r\n", "OK", NULL, NULL, 3000))
    {
      return 0;
    }

    if (!Cloud_SendCommandAndWait("AT+MIPCALL?\r\n", "+MIPCALL: 1,", NULL, NULL, 10000))
    {
      return 0;
    }
  }

  if (!Cloud_SendCommandAndWait(
        "AT+HMCON=0,60,\"e0442c51e1.st1.iotda-device.cn-north-4.myhuaweicloud.com\",\"1883\",\"6a1981b67f2e6c302f75b255_flame_flag\",\"Flame_flag\",0\r\n",
        "+HMCON OK",
        NULL,
        NULL,
        15000))
  {
    return 0;
  }

  if (!Cloud_SendCommandAndWait(
        "AT+HMSUB=0,\"$oc/devices/6a1981b67f2e6c302f75b255_flame_flag/sys/properties/report\"\r\n",
        "+HMSUB OK",
        "ERR:8",
        NULL,
        5000))
  {
    return 0;
  }

  return 1;
}

void Cloud_SetTelemetryEnabled(uint8_t enabled)
{
  g_cloud_telemetry_enabled = (enabled != 0U) ? 1U : 0U;
}

uint8_t Cloud_IsBusy(void)
{
  return (g_http_ctx.state != CLOUD_HTTP_IDLE) ? 1U : 0U;
}

uint8_t Cloud_IsCallActive(void)
{
  return g_call_active;
}

uint8_t Cloud_RequestLatestPhotoUpload(const char *filename)
{
  size_t name_len;

  if (g_call_active != 0U)
  {
    printf("[HTTP] call active, skip upload request: %s\r\n", filename);
    return 0;
  }

  if (filename == NULL || filename[0] == '\0')
  {
    return 0;
  }

  if (Cloud_IsBusy() != 0U)
  {
    printf("[HTTP] busy, skip upload request: %s active_file=%s state=%s chunk=%lu/%lu\r\n",
           filename,
           g_http_ctx.filename,
           Cloud_HttpStateToString(g_http_ctx.state),
           (unsigned long)(g_http_ctx.chunk_index + 1U),
           (unsigned long)g_http_ctx.total_chunks);
    return 0;
  }

  name_len = strlen(filename);
  if (name_len >= sizeof(g_http_ctx.filename))
  {
    return 0;
  }

  Cloud_EnsureRxStarted();
  Cloud_ResetHttpContext();
  strcpy(g_http_ctx.filename, filename);
  g_http_ctx.state = CLOUD_HTTP_OPEN_FILE;
  g_http_ctx.telemetry_prev_enabled = g_cloud_telemetry_enabled;
  g_cloud_telemetry_enabled = 0U;

  printf("[HTTP] start upload: %s\r\n", g_http_ctx.filename);
  return 1;
}

void Cloud_Service(void)
{
  UINT bytes_read;
  FRESULT fres;
  Cloud_WaitStateTypeDef wait_state;

  if ((g_call_active != 0U) && (Cloud_CallBufferHasFailure() != 0U))
  {
    Cloud_PrintRxSnapshot("call released");
    Cloud_SetCallActive(0U);
    Cloud_ClearRxBuffer();
  }

  switch (g_http_ctx.state)
  {
    case CLOUD_HTTP_IDLE:
      break;

    case CLOUD_HTTP_OPEN_FILE:
      if (SD_Lock_TryAcquire(SD_LOCK_OWNER_HTTP_UPLOAD) == 0U)
      {
        if (g_http_ctx.started == 0U)
        {
          printf("[HTTP] waiting for SD lock owner=%s file=%s\r\n",
                 SD_Lock_OwnerName(SD_Lock_GetOwner()),
                 g_http_ctx.filename);
          g_http_ctx.started = 1U;
        }
        break;
      }

      g_http_ctx.started = 0U;
      fres = f_open(&g_http_ctx.file, g_http_ctx.filename, FA_READ);
      if (fres != FR_OK)
      {
        printf("[HTTP] open file failed: %s, err=%d\r\n", g_http_ctx.filename, fres);
        Cloud_FailCurrentUpload("f_open");
        break;
      }

      g_http_ctx.file_opened = 1U;
      g_http_ctx.file_size = (uint32_t)f_size(&g_http_ctx.file);
      g_http_ctx.total_chunks = (g_http_ctx.file_size + CLOUD_HTTP_RAW_CHUNK_MAX - 1U) / CLOUD_HTTP_RAW_CHUNK_MAX;
      g_http_ctx.chunk_index = 0U;
      g_http_ctx.bytes_consumed = 0U;
      g_http_ctx.retry_count = 0U;
      printf("[HTTP] open ok: %s size=%lu total_chunks=%lu\r\n",
             g_http_ctx.filename,
             (unsigned long)g_http_ctx.file_size,
             (unsigned long)g_http_ctx.total_chunks);
      g_http_ctx.state = CLOUD_HTTP_PREPARE_CHUNK;
      break;

    case CLOUD_HTTP_PREPARE_CHUNK:
      if (g_http_ctx.chunk_index >= g_http_ctx.total_chunks)
      {
        g_http_ctx.state = CLOUD_HTTP_FINISH;
        break;
      }

      memset(g_http_b64_chunk, 0, CLOUD_HTTP_B64_BUFFER_SIZE);
      memset(g_http_json_chunk, 0, CLOUD_HTTP_JSON_BUFFER_SIZE);

      fres = f_read(&g_http_ctx.file, g_http_raw_chunk, CLOUD_HTTP_RAW_CHUNK_MAX, &bytes_read);
      if (fres != FR_OK)
      {
        printf("[HTTP] read chunk failed: err=%d\r\n", fres);
        Cloud_FailCurrentUpload("f_read");
        break;
      }

      if (bytes_read == 0U)
      {
        g_http_ctx.state = CLOUD_HTTP_FINISH;
        break;
      }

      Cloud_InvalidateDCache(g_http_raw_chunk, bytes_read);

      g_http_ctx.current_raw_len = (uint32_t)bytes_read;
      g_http_ctx.current_b64_len = Cloud_Base64Encode(g_http_raw_chunk,
                                                      g_http_ctx.current_raw_len,
                                                      g_http_b64_chunk,
                                                      CLOUD_HTTP_B64_BUFFER_SIZE);
      g_http_ctx.current_json_len = (uint32_t)snprintf(
        g_http_json_chunk,
        CLOUD_HTTP_JSON_BUFFER_SIZE,
        "{\"filename\":\"%s\",\"chunk_index\":%lu,\"total_chunks\":%lu,\"image_data\":\"%s\"}",
        g_http_ctx.filename,
        (unsigned long)g_http_ctx.chunk_index,
        (unsigned long)g_http_ctx.total_chunks,
        g_http_b64_chunk);

      if (g_http_ctx.current_json_len >= CLOUD_HTTP_JSON_BUFFER_SIZE)
      {
        Cloud_FailCurrentUpload("json too long");
        break;
      }

      printf("[HTTP] chunk %lu/%lu raw=%lu b64=%lu json=%lu\r\n",
             (unsigned long)(g_http_ctx.chunk_index + 1U),
             (unsigned long)g_http_ctx.total_chunks,
             (unsigned long)g_http_ctx.current_raw_len,
             (unsigned long)g_http_ctx.current_b64_len,
             (unsigned long)g_http_ctx.current_json_len);

      g_http_ctx.state = CLOUD_HTTP_SET_URL;
      g_http_ctx.started = 0U;
      break;

    case CLOUD_HTTP_SET_URL:
      if (g_http_ctx.started == 0U)
      {
        snprintf(g_http_cmd,
                 sizeof(g_http_cmd),
                 "AT+HTTPSET=\"URL\",\"%s\"\r\n",
                 http_url);
        Cloud_ClearRxBuffer();
        Cloud_SendString(g_http_cmd);
        Cloud_WaitStart("OK", NULL, NULL, 4000);
        Cloud_BeginStep();
      }

      wait_state = Cloud_WaitPoll();
      if (wait_state == CLOUD_WAIT_OK)
      {
        Cloud_EndStep();
        g_http_ctx.state = CLOUD_HTTP_SET_CONTYPE;
      }
      else if (wait_state == CLOUD_WAIT_FAIL)
      {
        Cloud_HandleChunkFailure("HTTPSET URL");
      }
      break;

    case CLOUD_HTTP_SET_CONTYPE:
      if (g_http_ctx.started == 0U)
      {
        Cloud_ClearRxBuffer();
        Cloud_SendString("AT+HTTPSET=\"CONTYPE\",\"application/json\"\r\n");
        Cloud_WaitStart("OK", NULL, NULL, 4000);
        Cloud_BeginStep();
      }

      wait_state = Cloud_WaitPoll();
      if (wait_state == CLOUD_WAIT_OK)
      {
        Cloud_EndStep();
        g_http_ctx.state = CLOUD_HTTP_SEND_DATA_CMD;
      }
      else if (wait_state == CLOUD_WAIT_FAIL)
      {
        Cloud_HandleChunkFailure("HTTPSET CONTYPE");
      }
      break;

    case CLOUD_HTTP_SEND_DATA_CMD:
      if (g_http_ctx.started == 0U)
      {
        snprintf(g_http_cmd,
                 sizeof(g_http_cmd),
                 "AT+HTTPDATA=%lu\r\n",
                 (unsigned long)g_http_ctx.current_json_len);
        Cloud_ClearRxBuffer();
        Cloud_SendString(g_http_cmd);
        Cloud_WaitStart(">", NULL, NULL, 5000);
        Cloud_BeginStep();
      }

      wait_state = Cloud_WaitPoll();
      if (wait_state == CLOUD_WAIT_OK)
      {
        Cloud_EndStep();
        g_http_ctx.state = CLOUD_HTTP_SEND_JSON;
      }
      else if (wait_state == CLOUD_WAIT_FAIL)
      {
        Cloud_HandleChunkFailure("HTTPDATA");
      }
      break;

    case CLOUD_HTTP_SEND_JSON:
      if (g_http_ctx.started == 0U)
      {
        Cloud_SendString(g_http_json_chunk);
        g_http_ctx.json_sent_tick = HAL_GetTick();
        Cloud_BeginStep();
      }

      if ((HAL_GetTick() - g_http_ctx.json_sent_tick) >= CLOUD_HTTP_JSON_SETTLE_MS)
      {
        Cloud_EndStep();
        g_http_ctx.state = CLOUD_HTTP_ACT;
      }
      break;

    case CLOUD_HTTP_ACT:
      if (g_http_ctx.started == 0U)
      {
        Cloud_ClearRxBuffer();
        Cloud_SendString("AT+HTTPACT=1,30\r\n");
        Cloud_WaitStart("+HTTPRES: 1,200,", NULL, NULL, 30000);
        Cloud_BeginStep();
        g_http_ctx.state = CLOUD_HTTP_WAIT_RESULT;
      }
      break;

    case CLOUD_HTTP_WAIT_RESULT:
      wait_state = Cloud_WaitPoll();
      if (wait_state == CLOUD_WAIT_OK)
      {
        Cloud_EndStep();
        g_http_ctx.bytes_consumed += g_http_ctx.current_raw_len;
        printf("[HTTP] chunk %lu/%lu success\r\n",
               (unsigned long)(g_http_ctx.chunk_index + 1U),
               (unsigned long)g_http_ctx.total_chunks);
        g_http_ctx.chunk_index++;
        g_http_ctx.retry_count = 0U;
        g_http_ctx.state = CLOUD_HTTP_PREPARE_CHUNK;
      }
      else if (wait_state == CLOUD_WAIT_FAIL)
      {
        Cloud_HandleChunkFailure("HTTPACT");
      }
      break;

    case CLOUD_HTTP_FINISH:
      Cloud_CloseUploadFile();
      printf("[HTTP] upload done: %s\r\n", g_http_ctx.filename);
      Cloud_RestoreTelemetry();
      Cloud_ResetHttpContext();
      break;

    case CLOUD_HTTP_ERROR:
      Cloud_CloseUploadFile();
      Cloud_RestoreTelemetry();
      Cloud_ResetHttpContext();
      break;

    default:
      Cloud_FailCurrentUpload("unknown state");
      break;
  }
}

void Cloud_Upload(int flame_flag, int smoke_flag, int rain_flag,
                  float temperature, float humidity,
                  int adc_flame, int adc_smoke, int k230,int help_flag)
{
  if ((g_cloud_telemetry_enabled == 0U) || (Cloud_IsBusy() != 0U))
  {
    return;
  }

  snprintf(json_raw, sizeof(json_raw),
           "{\"services\":[{\"service_id\":\"sensor_data\",\"properties\":{\"flame_flag\":%d,\"smoke_flag\":%d,\"rain_flag\":%d,\"k230\":%d,\"temperature\":%.2f,\"humidity\":%.2f,\"adc_flame\":%d,\"adc_smoke\":%d,\"help_flag\":%d}}]}",
           flame_flag, smoke_flag, rain_flag,
           k230,
           temperature, humidity,
           adc_flame, adc_smoke,
	         help_flag);

  Escape_JSON_To_AT(json_raw, json_escaped, sizeof(json_escaped));

  snprintf(temp, sizeof(temp),
           "AT+HMPUB=1,\"$oc/devices/%s_%s/sys/properties/report\",%d,\"%s\"\r\n",
           device_id, device_name,
           (int)strlen(json_raw), json_escaped);

  HAL_UART_Transmit(&huart4, (uint8_t *)temp, (uint16_t)strlen(temp), 100);
}

/**
 * @brief  ��������ͨ��
 * @param  phone_number: �绰�����ַ��������� "10086" �� "+8613800138000"
 * @retval 1=�ɹ�, 0=ʧ��
 */
uint8_t Cloud_MakeCall(const char *phone_number)
{
  char dial_cmd[64];
  
  if (phone_number == NULL || phone_number[0] == '\0')
  {
    printf("[CALL] invalid phone number\r\n");
    return 0;
  }

  if (g_call_active != 0U)
  {
    printf("[CALL] call already active\r\n");
    return 0;
  }
  
  // ����Ƿ������ϴ��ļ�����ѡ�������ͻ��
  if (Cloud_IsBusy() != 0U)
  {
    printf("[CALL] upload busy, cannot make call now\r\n");
    return 0;
  }
  
  Cloud_EnsureRxStarted();

  if (!Cloud_PrepareVoiceCall())
  {
    return 0;
  }
  
  // ������������ ATD<number>;
  // ע�⣺�ֺű�ʾ�������У����ӷֺ������ݺ���
  snprintf(dial_cmd, sizeof(dial_cmd), "ATD%s;\r\n", phone_number);
  
  printf("[CALL] dialing: %s\r\n", phone_number);
  
  // ���Ͳ�������ȴ�OK��Ӧ
  // ��ʱʱ������Ϊ5��
  if (!Cloud_SendCommandAndWait(dial_cmd, "OK", NULL, NULL, 5000))
  {
    printf("[CALL] dial command failed\r\n");
    return 0;
  }

  if (!Cloud_WaitForCallProgress(10000))
  {
    printf("[CALL] no CLCC progress after dial command\r\n");
    return 0;
  }

  Cloud_SetCallActive(1U);
  printf("[CALL] call initiated successfully\r\n");
  return 1;
}

/**
 * @brief  �Ҷϵ�ǰͨ��
 * @retval 1=�ɹ�, 0=ʧ��
 */
void Cloud_HangupCall(void)
{
  if (g_call_active == 0U)
  {
    return;
  }

  printf("[CALL] hanging up\r\n");
  Cloud_EnsureRxStarted();
  
  // ATH�������ڹҶϵ绰
  Cloud_SendCommandAndWait("ATH\r\n", "OK", NULL, NULL, 3000);
  Cloud_SetCallActive(0U);
  
  // ����ʹ�� AT+CHUP (��Щģ��֧��)
  // Cloud_SendCommandAndWait("AT+CHUP\r\n", "OK", NULL, NULL, 3000);
}
