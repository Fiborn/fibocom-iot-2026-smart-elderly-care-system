#ifndef __WAV_PLAYER_H
#define __WAV_PLAYER_H

#include "stm32h7xx_hal.h"
#include "ff.h"

typedef enum
{
    PLAYER_IDLE = 0,
    PLAYER_PLAYING,
    PLAYER_PAUSED,
    PLAYER_STOPPED
} PlayerStateTypeDef;

typedef struct
{
    uint32_t ChunkID;
    uint32_t ChunkSize;
    uint32_t Format;
    uint16_t AudioFormat;
    uint16_t NumChannels;
    uint32_t SampleRate;
    uint32_t ByteRate;
    uint16_t BlockAlign;
    uint16_t BitsPerSample;
    uint32_t Subchunk2Size;
} WAV_HeaderTypeDef;
void Audio_TestTone(void);
uint8_t WAV_Player_Init(void);
uint8_t WAV_Player_Start(const char *filename);
void WAV_Player_Stop(void);
void WAV_Player_Pause(void);
void WAV_Player_Resume(void);
void WAV_Player_Process(void);
PlayerStateTypeDef WAV_Player_GetState(void);

#endif