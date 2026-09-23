/**
 ******************************************************************************
 * @file    video_avi.h
 * @brief   AVI video file format implementation for STM32H7
 ******************************************************************************
 */

#ifndef __VIDEO_AVI_H__
#define __VIDEO_AVI_H__

#include <stdint.h>
#include "ff.h"

/*============================================================================
 * AVI Format Constants
 *==========================================================================*/

/* RIFF FourCC Codes */
#define AVI_FOURCC_RIFF     0x46464952  /* 'RIFF' */
#define AVI_FOURCC_AVI      0x20495641  /* 'AVI ' */
#define AVI_FOURCC_LIST     0x5453494C  /* 'LIST' */
#define AVI_FOURCC_HDRL     0x6C726468  /* 'hdrl' */
#define AVI_FOURCC_AVIH     0x68697661  /* 'avih' */
#define AVI_FOURCC_STRL     0x6C727473  /* 'strl' */
#define AVI_FOURCC_STRH     0x68727473  /* 'strh' */
#define AVI_FOURCC_STRF     0x66727473  /* 'strf' */
#define AVI_FOURCC_VIDS     0x73646976  /* 'vids' */
#define AVI_FOURCC_DIB      0x20424944  /* 'DIB ' (RGB565 Uncompressed) */
#define AVI_FOURCC_MOVI     0x69766F6D  /* 'movi' */
#define AVI_FOURCC_00DC     0x63643030  /* '00dc' (Uncompressed Video Frame) */
#define AVI_FOURCC_IDX1     0x31786469  /* 'idx1' (Index Chunk) */

/* AVI Flags */
#define AVIF_HASINDEX       0x00000010  /* Index at end of file */
#define AVIF_ISINTERLEAVED   0x00000100  /* Stream is interleaved */
#define AVIF_WASCAPTUREFILE 0x00010000  /* File is a capture file */
#define AVIF_MUSTUSEINDEX   0x00000020  /* Must use index */

/*============================================================================
 * AVI Data Structures
 *==========================================================================*/

/* Main AVI Header Chunk (56 bytes) */
typedef struct {
    uint32_t us_per_frame;       /* Microseconds per frame (33333 for 30fps) */
    uint32_t max_bytes_per_sec;  /* Max bytes per second */
    uint32_t padding_granularity;/* Pad to multiples of this size (2048) */
    uint32_t flags;              /* Flags (AVIF_HASINDEX | AVIF_ISINTERLEAVED) */
    uint32_t total_frames;       /* Total number of frames */
    uint32_t initial_frames;     /* Initial frames for audio sync */
    uint32_t streams;            /* Number of streams */
    uint32_t suggested_buffer_size; /* Suggested buffer size */
    uint32_t width;              /* Video width in pixels */
    uint32_t height;             /* Video height in pixels */
    uint32_t scale;              /* Time scale */
    uint32_t rate;               /* Frame rate */
    uint32_t start;              /* Start time */
    uint32_t length;             /* Duration */
} __attribute__((packed)) avi_main_header_t;

/* Stream Header Chunk (56 bytes) */
typedef struct {
    uint32_t fcc_type;           /* Stream type ('vids' for video) */
    uint32_t fcc_handler;        /* Codec handler ('DIB ' for RGB565) */
    uint32_t flags;              /* Stream flags */
    uint16_t priority;           /* Stream priority */
    uint16_t language;          /* Language */
    uint32_t initial_frames;     /* Initial frames */
    uint32_t scale;             /* Scale */
    uint32_t rate;               /* Rate (scale/rate = fps) */
    uint32_t start;              /* Start time */
    uint32_t length;            /* Stream length in frames */
    uint32_t suggested_buffer_size; /* Suggested buffer size */
    uint32_t quality;            /* Quality (10000 = default) */
    uint32_t sample_size;        /* Sample size (0 = variable) */
    int16_t  left;              /* Destination rectangle left */
    int16_t  top;               /* Destination rectangle top */
    int16_t  right;             /* Destination rectangle right */
    int16_t  bottom;            /* Destination rectangle bottom */
} __attribute__((packed)) avi_stream_header_t;

/* Bitmap Info Header (40 bytes) - BITMAPINFOHEADER */
typedef struct {
    uint32_t bi_size;           /* Size of this header (40) */
    int32_t  bi_width;          /* Width in pixels */
    int32_t  bi_height;         /* Height in pixels (positive = bottom-up) */
    uint16_t bi_planes;         /* Number of planes (must be 1) */
    uint16_t bi_bit_count;      /* Bits per pixel (16 for RGB565) */
    uint32_t bi_compression;     /* Compression type ('DIB ') */
    uint32_t bi_size_image;      /* Image size in bytes */
    int32_t  bi_x_pels_per_meter; /* Horizontal resolution */
    int32_t  bi_y_pels_per_meter; /* Vertical resolution */
    uint32_t bi_clr_used;       /* Colors used (0 = all) */
    uint32_t bi_clr_important;  /* Important colors (0 = all) */
} __attribute__((packed)) avi_bitmap_info_header_t;

/* Index Entry (16 bytes per entry) */
typedef struct {
    uint32_t chunk_id;          /* '00dc' for video frame */
    uint32_t flags;             /* Flags (AVIIF_KEYFRAME = 0x10) */
    uint32_t offset;            /* Offset from 'movi' chunk start */
    uint32_t size;              /* Size of frame data */
} __attribute__((packed)) avi_index_entry_t;

/* AVI File Handle */
typedef struct {
    FIL             file;           /* FatFS file handle */
    uint16_t        width;          /* Video width */
    uint16_t        height;         /* Video height */
    uint16_t        fps;            /* Frames per second */
    uint16_t        frame_count;    /* Total frames recorded */
    uint32_t        frame_size;     /* Size of each frame in bytes */
    uint32_t        movi_offset;    /* Offset to 'movi' chunk */
    uint32_t        movi_size;      /* Current size of 'movi' chunk */
    uint32_t        file_size;      /* Current file size */
    uint32_t        max_file_size;  /* Maximum file size in bytes */
    uint32_t        idx_count;      /* Number of index entries */
    avi_index_entry_t *idx_entries; /* Index buffer (malloc'd) */
    uint32_t        idx_buf_size;   /* Index buffer allocated size */
    uint8_t         is_open;        /* File is open flag */
} avi_handle_t;

/*============================================================================
 * AVI API Functions
 *==========================================================================*/

/**
 * @brief   Initialize AVI file for writing
 * @param   handle: Pointer to AVI handle structure
 * @param   filename: Name of file to create (e.g., "0:/VIDEO/VID_20240101_120000.avi")
 * @param   width: Video width in pixels
 * @param   height: Video height in pixels
 * @param   fps: Frames per second
 * @param   max_size: Maximum file size in bytes (auto-stop when reached)
 * @retval  0: Success, -1: Error
 */
int8_t AVI_Init(avi_handle_t *handle, const char *filename, 
                 uint16_t width, uint16_t height, uint16_t fps,
                 uint32_t max_size);

/**
 * @brief   Write a video frame to AVI file
 * @param   handle: Pointer to AVI handle
 * @param   frame_data: Pointer to frame data (RGB565 format)
 * @retval  0: Success, -1: Error, 1: Max size reached (auto-stop)
 */
int8_t AVI_WriteFrame(avi_handle_t *handle, const uint8_t *frame_data);

/**
 * @brief   Close AVI file and finalize
 * @param   handle: Pointer to AVI handle
 * @retval  0: Success, -1: Error
 */
int8_t AVI_Close(avi_handle_t *handle);

/**
 * @brief   Get AVI recording status
 * @param   handle: Pointer to AVI handle
 * @retval  Current frame count
 */
uint32_t AVI_GetFrameCount(avi_handle_t *handle);

/**
 * @brief   Get current file size
 * @param   handle: Pointer to AVI handle
 * @retval  Current file size in bytes
 */
uint32_t AVI_GetFileSize(avi_handle_t *handle);

/**
 * @brief   Check if max file size reached
 * @param   handle: Pointer to AVI handle
 * @retval  1: Max size reached, 0: Not reached
 */
uint8_t AVI_IsMaxSizeReached(avi_handle_t *handle);

/*============================================================================
 * Video Recording Configuration
 *==========================================================================*/

/* Video Recording Parameters */
#define VIDEO_WIDTH_AI      480     /* AI mode width */
#define VIDEO_HEIGHT_AI     480     /* AI mode height */
#define VIDEO_WIDTH_REC     640     /* Recording mode width */
#define VIDEO_HEIGHT_REC    480     /* Recording mode height */
#define VIDEO_FPS           30      /* Frames per second */
#define VIDEO_FRAME_SIZE_AI     (VIDEO_WIDTH_AI * VIDEO_HEIGHT_AI * 2)  /* 460800 bytes */
#define VIDEO_FRAME_SIZE_REC    (VIDEO_WIDTH_REC * VIDEO_HEIGHT_REC * 2) /* 614400 bytes */
#define VIDEO_MAX_FILE_SIZE (200 * 1024 * 1024)  /* 200 MB per file */
#define VIDEO_MAX_INDEX_ENTRIES 10000 /* Max frames per file (memory limit) */

#endif /* __VIDEO_AVI_H__ */
