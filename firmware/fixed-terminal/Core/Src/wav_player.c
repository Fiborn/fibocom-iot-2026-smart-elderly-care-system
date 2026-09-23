#include "wav_player.h"
#include "sai.h"
#include "tlv320aic3254.h"
#include "i2c.h"
#include "sd_lock.h"
#include <stdio.h>
#include <string.h>

#define AUDIO_BUFFER_SIZE   4096  /* 每半缓冲区的halfword数 */

/* 双缓冲区 - 放在D1 SRAM中确保DMA可访问 */
__attribute__((section(".dma_buffer"), aligned(32)))

#define AUDIO_BUFFER_HALFWORDS   (AUDIO_BUFFER_SIZE * 2)
#define AUDIO_BUFFER_BYTES       (AUDIO_BUFFER_HALFWORDS * sizeof(uint16_t))

static uint16_t *AudioBuffer = (uint16_t*)0x24000000;
static void FillTestTone(void)
{
    int16_t *p = (int16_t *)AudioBuffer;

    for (uint32_t i = 0; i < AUDIO_BUFFER_HALFWORDS; i += 2)
    {
        int16_t s = (((i / 2) % 44) < 22) ? 12000 : -12000;   // 约 1kHz 方波
        p[i]     = s;   // Left
        p[i + 1] = s;   // Right
    }

    SCB_CleanDCache_by_Addr((uint32_t *)AudioBuffer, AUDIO_BUFFER_BYTES);
}

void Audio_TestTone(void)
{
    /* 1. 彻底重置 */
    HAL_SAI_DMAStop(&hsai_BlockB1);
    HAL_SAI_DeInit(&hsai_BlockB1);
    HAL_Delay(10);

    /* 2. 初始化 SAI */
    MX_SAI1_Init(44100);

    /* 3. 先填测试音到 buffer（还没启动DMA） */
    FillTestTone();

    /* 4. 直接用测试音数据启动 SAI DMA */
    HAL_StatusTypeDef st = HAL_SAI_Transmit_DMA(&hsai_BlockB1, (uint8_t*)AudioBuffer, AUDIO_BUFFER_HALFWORDS);
    printf("DMA start: %d\r\n", st);

    /* 5. BCLK 现在在跑了，初始化 TLV320 */
    HAL_Delay(20);
    TLV320AIC3254_Init();
    HAL_Delay(200);  // 给PLL充足时间锁定

    /* 6. 验证 */
    uint8_t flag = TLV320AIC3254_ReadRegister(0, 0x25);
    printf("DAC Flag = 0x%02X\r\n", flag);

    /* 7. 不要停SAI！直接循环 */
    while(1)
    {
        HAL_Delay(1000);
        flag = TLV320AIC3254_ReadRegister(0, 0x25);
        printf("Playing... SR=0x%08lX  DAC=0x%02X\r\n", SAI1_Block_B->SR, flag);
    }
}

static PlayerStateTypeDef playerState = PLAYER_IDLE;
static FIL wavFile;
WAV_HeaderTypeDef wavHeader;
static uint32_t audioDataRemaining = 0;
static volatile uint8_t bufferHalfComplete = 0;
static volatile uint8_t bufferFullComplete = 0;

extern void MX_I2C1_Init(void);

/**
  * @brief  初始化WAV播放器
  */
static const char *HAL_StatusStr(HAL_StatusTypeDef st)
{
    switch (st)
    {
    case HAL_OK: return "HAL_OK";
    case HAL_ERROR: return "HAL_ERROR";
    case HAL_BUSY: return "HAL_BUSY";
    case HAL_TIMEOUT: return "HAL_TIMEOUT";
    default: return "UNKNOWN";
    }
}	
	
uint8_t WAV_Player_Init(void)
{
    uint8_t regVal;
    HAL_StatusTypeDef st;

    printf("WAV Player Initializing...\r\n");

    MX_I2C1_Init();
    printf("I2C1 Init OK\r\n");

    MX_SAI1_Init(44100);

    memset(AudioBuffer, 0, AUDIO_BUFFER_BYTES);
    SCB_CleanDCache_by_Addr((uint32_t*)AudioBuffer, AUDIO_BUFFER_BYTES);

    printf("AudioBuffer addr = 0x%08lX\r\n", (uint32_t)AudioBuffer);

    st = HAL_SAI_Transmit_DMA(&hsai_BlockB1, (uint8_t*)AudioBuffer, AUDIO_BUFFER_SIZE * 2);
    printf("Silence DMA start status = %d\r\n", st);

    if (st != HAL_OK)
    {
        printf("SAI start failed, hsai err=0x%08lX dma err=0x%08lX sr=0x%08lX\r\n",
               hsai_BlockB1.ErrorCode,
               hsai_BlockB1.hdmatx ? hsai_BlockB1.hdmatx->ErrorCode : 0,
               hsai_BlockB1.Instance->SR);
        return 1;
    }

    HAL_Delay(20);

    if(TLV320AIC3254_Init() != HAL_OK)
    {
        printf("TLV320AIC3254 Init Failed!\r\n");
        HAL_SAI_DMAStop(&hsai_BlockB1);
        return 1;
    }

    HAL_Delay(100);

    regVal = TLV320AIC3254_ReadRegister(0, 0x25);
    printf("DAC Flag after PLL lock: 0x%02X\r\n", regVal);

    HAL_SAI_DMAStop(&hsai_BlockB1);
    HAL_SAI_DeInit(&hsai_BlockB1);
    HAL_Delay(10);
    MX_SAI1_Init(44100);

    TLV320AIC3254_SetVolume(0x00);
    playerState = PLAYER_IDLE;
    printf("WAV Player Init OK\r\n\r\n");
    printf("Silence DMA start status = %s(%d)\r\n", HAL_StatusStr(st), st);
    return 0;
}

/**
  * @brief  解析WAV文件头 - 正确处理各种chunk
  */
uint8_t WAV_ParseHeader(FIL *file, WAV_HeaderTypeDef *header)
{
    UINT bytesRead;
    uint8_t buf[12];
    uint32_t chunkID, chunkSize;
    
    /* 读RIFF头 */
    if(f_read(file, buf, 12, &bytesRead) != FR_OK || bytesRead != 12)
        return 1;
    
    if(*(uint32_t*)buf != 0x46464952)  /* "RIFF" */
        return 2;
    if(*(uint32_t*)(buf+8) != 0x45564157)  /* "WAVE" */
        return 3;
    
    printf("RIFF Header:\n  ChunkID: RIFF\n  ChunkSize: %lu\n  Format: WAVE\r\n",
           *(uint32_t*)(buf+4));
    
    /* 搜索fmt和data chunk */
    uint8_t fmtFound = 0, dataFound = 0;
    
    while(!dataFound)
    {
        if(f_read(file, buf, 8, &bytesRead) != FR_OK || bytesRead != 8)
            return 4;
        
        chunkID = *(uint32_t*)buf;
        chunkSize = *(uint32_t*)(buf+4);
        
        printf("  Chunk: %c%c%c%c, Size: %lu\r\n",
               buf[0], buf[1], buf[2], buf[3], chunkSize);
        
        if(chunkID == 0x20746D66)  /* "fmt " */
        {
            uint8_t fmtBuf[40];
            uint32_t toRead = (chunkSize > 40) ? 40 : chunkSize;
            
            if(f_read(file, fmtBuf, toRead, &bytesRead) != FR_OK)
                return 5;
            
            header->AudioFormat = *(uint16_t*)fmtBuf;
            header->NumChannels = *(uint16_t*)(fmtBuf+2);
            header->SampleRate = *(uint32_t*)(fmtBuf+4);
            header->ByteRate = *(uint32_t*)(fmtBuf+8);
            header->BlockAlign = *(uint16_t*)(fmtBuf+12);
            header->BitsPerSample = *(uint16_t*)(fmtBuf+14);
            
            /* 跳过多余的fmt数据 */
            if(chunkSize > toRead)
                f_lseek(file, f_tell(file) + (chunkSize - toRead));
            
            fmtFound = 1;
        }
        else if(chunkID == 0x61746164)  /* "data" */
        {
            header->Subchunk2Size = chunkSize;
            dataFound = 1;
        }
        else
        {
            /* 跳过未知chunk */
            f_lseek(file, f_tell(file) + chunkSize);
            /* 如果chunkSize为奇数，跳过padding字节 */
            if(chunkSize & 1)
                f_lseek(file, f_tell(file) + 1);
        }
        
        /* 防死循环 */
        if(f_tell(file) > 100000)
            return 6;
    }
    
    if(!fmtFound) return 7;
    
    printf("\r\n=== WAV File Info ===\r\n");
    printf("  Audio Format: %d (1=PCM)\r\n", header->AudioFormat);
    printf("  Channels: %d\r\n", header->NumChannels);
    printf("  Sample Rate: %lu Hz\r\n", header->SampleRate);
    printf("  Byte Rate: %lu bytes/sec\r\n", header->ByteRate);
    printf("  Block Align: %d\r\n", header->BlockAlign);
    printf("  Bits Per Sample: %d\r\n", header->BitsPerSample);
    printf("  Data Size: %lu bytes\r\n", header->Subchunk2Size);
    printf("  Duration: %.2f seconds\r\n",
           (float)header->Subchunk2Size / header->ByteRate);
    printf("  Data Start Position: %lu\r\n", (uint32_t)f_tell(file));
    printf("=====================\r\n\r\n");
    
    return 0;
}

/**
  * @brief  填充半缓冲区
  */
static uint8_t FillBuffer(uint32_t offset)
{
    UINT bytesRead;
    uint32_t bytesToRead = AUDIO_BUFFER_SIZE * 2;  /* halfword = 2 bytes */
    
    if(audioDataRemaining == 0)
        return 1;
    
    if(bytesToRead > audioDataRemaining)
        bytesToRead = audioDataRemaining;
    
    if(f_read(&wavFile, &AudioBuffer[offset], bytesToRead, &bytesRead) != FR_OK)
        return 2;
    
    audioDataRemaining -= bytesRead;
    
    /* 不足部分填静音 */
    if(bytesRead < AUDIO_BUFFER_SIZE * 2)
    {
        memset((uint8_t*)&AudioBuffer[offset] + bytesRead, 0, 
               AUDIO_BUFFER_SIZE * 2 - bytesRead);
    }
    
    /* 清DCache */
    SCB_CleanDCache_by_Addr((uint32_t*)&AudioBuffer[offset], AUDIO_BUFFER_SIZE * 2);
    
    return 0;
}

uint8_t WAV_Player_Start(const char *filename)
{
    FRESULT res;
    uint8_t regVal;
    
    if(playerState == PLAYER_PLAYING)
    {
        WAV_Player_Stop();
        HAL_Delay(10);
    }
    
    printf("\r\n=============================\r\n");
    printf("Opening file: %s\r\n", filename);

    if (SD_Lock_TryAcquire(SD_LOCK_OWNER_WAV_PLAYER) == 0U)
    {
        printf("Open file blocked, SD lock owner=%s\r\n",
               SD_Lock_OwnerName(SD_Lock_GetOwner()));
        return 1;
    }
    
    res = f_open(&wavFile, filename, FA_READ);
    if(res != FR_OK)
    {
        printf("Open file failed! Error: %d\r\n", res);
        SD_Lock_Release(SD_LOCK_OWNER_WAV_PLAYER);
        return 1;
    }
    
    printf("File size: %lu bytes\r\n", (uint32_t)f_size(&wavFile));
    
    if(WAV_ParseHeader(&wavFile, &wavHeader) != 0)
    {
        printf("Parse WAV header failed!\r\n");
        f_close(&wavFile);
        SD_Lock_Release(SD_LOCK_OWNER_WAV_PLAYER);
        return 2;
    }
    
    if(wavHeader.AudioFormat != 1)
    {
        printf("Only PCM supported!\r\n");
        f_close(&wavFile);
        SD_Lock_Release(SD_LOCK_OWNER_WAV_PLAYER);
        return 3;
    }
    
    if(wavHeader.BitsPerSample != 16)
    {
        printf("Only 16-bit supported!\r\n");
        f_close(&wavFile);
        SD_Lock_Release(SD_LOCK_OWNER_WAV_PLAYER);
        return 4;
    }
    
    /* ===== 关键修复：完全重新初始化 SAI ===== */
    HAL_SAI_DMAStop(&hsai_BlockB1);
    HAL_SAI_DeInit(&hsai_BlockB1);
    HAL_Delay(5);
    
    MX_SAI1_Init(wavHeader.SampleRate);
    
    /* 用静音启动 BCLK，让 TLV320 PLL 锁定 */
    memset(AudioBuffer, 0, AUDIO_BUFFER_BYTES);
    SCB_CleanDCache_by_Addr((uint32_t*)AudioBuffer, AUDIO_BUFFER_BYTES);
    
    HAL_StatusTypeDef dmaStatus;
    dmaStatus = HAL_SAI_Transmit_DMA(&hsai_BlockB1, (uint8_t*)AudioBuffer, AUDIO_BUFFER_SIZE * 2);
    printf("Silence DMA start status: %d\r\n", dmaStatus);
    
    HAL_Delay(100);
    
    regVal = TLV320AIC3254_ReadRegister(0, 0x25);
    printf("DAC Flag: 0x%02X\r\n", regVal);
    
    HAL_SAI_DMAStop(&hsai_BlockB1);
    HAL_SAI_DeInit(&hsai_BlockB1);
    HAL_Delay(5);
    
    MX_SAI1_Init(wavHeader.SampleRate);
    
    audioDataRemaining = wavHeader.Subchunk2Size;
    bufferHalfComplete = 0;
    bufferFullComplete = 0;
    
    printf("Filling audio buffer...\r\n");
    FillBuffer(0);
    FillBuffer(AUDIO_BUFFER_SIZE);
    printf("Data remaining after initial fill: %lu bytes\r\n", audioDataRemaining);
    
    playerState = PLAYER_PLAYING;
    SAI_Play(AudioBuffer, AUDIO_BUFFER_SIZE * 2);
    
    printf("Playback started!\r\n");
    printf("=============================\r\n\r\n");
    
    return 0;
}

void WAV_Player_Stop(void)
{
    if(playerState != PLAYER_IDLE)
    {
        SAI_Stop();
        f_close(&wavFile);
        SD_Lock_Release(SD_LOCK_OWNER_WAV_PLAYER);
        playerState = PLAYER_STOPPED;
        printf("Playback stopped.\r\n");
    }
}

void WAV_Player_Pause(void)
{
    if(playerState == PLAYER_PLAYING)
    {
        SAI_Pause();
        playerState = PLAYER_PAUSED;
    }
}

void WAV_Player_Resume(void)
{
    if(playerState == PLAYER_PAUSED)
    {
        SAI_Resume();
        playerState = PLAYER_PLAYING;
    }
}

PlayerStateTypeDef WAV_Player_GetState(void)
{
    return playerState;
}

void WAV_Player_Process(void)
{
    if(playerState != PLAYER_PLAYING)
        return;
    
    if(bufferHalfComplete)
    {
        bufferHalfComplete = 0;
        if(audioDataRemaining > 0)
        {
            FillBuffer(0);
        }
        else
        {
            memset(AudioBuffer, 0, AUDIO_BUFFER_SIZE * 2);
            SCB_CleanDCache_by_Addr((uint32_t*)AudioBuffer, AUDIO_BUFFER_SIZE * 2);
        }
    }
    
    if(bufferFullComplete)
    {
        bufferFullComplete = 0;
        if(audioDataRemaining > 0)
        {
            FillBuffer(AUDIO_BUFFER_SIZE);
        }
        else
        {
            SAI_Stop();
            f_close(&wavFile);
            SD_Lock_Release(SD_LOCK_OWNER_WAV_PLAYER);
            playerState = PLAYER_IDLE;
            printf("\r\nPlayback finished.\r\n");
        }
    }
}

/* DMA回调 */
void HAL_SAI_TxHalfCpltCallback(SAI_HandleTypeDef *hsai)
{
    if(hsai->Instance == SAI1_Block_B)
        bufferHalfComplete = 1;
}

void HAL_SAI_TxCpltCallback(SAI_HandleTypeDef *hsai)
{
    if(hsai->Instance == SAI1_Block_B)
        bufferFullComplete = 1;
}

void HAL_SAI_ErrorCallback(SAI_HandleTypeDef *hsai)
{
    printf("SAI Error: 0x%lX\r\n", hsai->ErrorCode);
}
