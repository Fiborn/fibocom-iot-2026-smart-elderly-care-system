/**
 ******************************************************************************
 * @file    video_avi.c
 * @brief   AVI video file format implementation for STM32H7
 ******************************************************************************
 */

#include "video_avi.h"
#include <stdio.h>
#include <string.h>

/*============================================================================
 * Private Macros
 *==========================================================================*/

#define AVI_MIN(a, b) (((a) < (b)) ? (a) : (b))
#define AVI_MAX(a, b) (((a) > (b)) ? (a) : (b))

/* Write helper macros */
#define WRITE_CHUNK_ID(f, id) do { \
    uint32_t _id = id; \
    f_write(&(f), &(_id), 4, NULL); \
} while(0)

#define WRITE_UINT32_LE(f, val) do { \
    uint32_t _val = (val); \
    f_write(&(f), &(_val), 4, NULL); \
} while(0)

#define WRITE_UINT16_LE(f, val) do { \
    uint16_t _val = (val); \
    f_write(&(f), &(_val), 2, NULL); \
} while(0)

#define WRITE_INT32_LE(f, val) do { \
    int32_t _val = (val); \
    f_write(&(f), &(_val), 4, NULL); \
} while(0)

/*============================================================================
 * Private Function Prototypes
 *==========================================================================*/

static int8_t AVI_WriteHeader(avi_handle_t *handle);
static int8_t AVI_WriteFrameHeader(avi_handle_t *handle);
static int8_t AVI_UpdateHeader(avi_handle_t *handle);

/*============================================================================
 * AVI API Functions
 *==========================================================================*/

/**
 * @brief   Initialize AVI file for writing
 */
int8_t AVI_Init(avi_handle_t *handle, const char *filename, 
                 uint16_t width, uint16_t height, uint16_t fps,
                 uint32_t max_size)
{
    FRESULT res;
    
    /* Validate parameters */
    if (handle == NULL || filename == NULL) {
        return -1;
    }
    
    if (width == 0 || height == 0 || fps == 0) {
        return -1;
    }
    
    /* Initialize handle */
    memset(handle, 0, sizeof(avi_handle_t));
    handle->width = width;
    handle->height = height;
    handle->fps = fps;
    handle->frame_size = width * height * 2;  /* RGB565 = 2 bytes per pixel */
    handle->max_file_size = max_size;
    handle->frame_count = 0;
    handle->movi_offset = 0;
    handle->movi_size = 0;
    handle->file_size = 0;
    handle->idx_count = 0;
    handle->is_open = 0;
    
    /* Allocate index buffer */
    handle->idx_buf_size = VIDEO_MAX_INDEX_ENTRIES * sizeof(avi_index_entry_t);
    handle->idx_entries = (avi_index_entry_t *)malloc(handle->idx_buf_size);
    if (handle->idx_entries == NULL) {
        return -1;
    }
    
    /* Create file */
    res = f_open(&(handle->file), filename, FA_CREATE_ALWAYS | FA_WRITE);
    if (res != FR_OK) {
        free(handle->idx_entries);
        return -1;
    }
    
    handle->is_open = 1;
    
    /* Write AVI header */
    if (AVI_WriteHeader(handle) != 0) {
        f_close(&(handle->file));
        free(handle->idx_entries);
        handle->is_open = 0;
        return -1;
    }
    
    return 0;
}

/**
 * @brief   Write a video frame to AVI file
 */
int8_t AVI_WriteFrame(avi_handle_t *handle, const uint8_t *frame_data)
{
    FRESULT res;
    uint32_t bytes_written;
    uint32_t frame_offset;
    uint32_t frame_size;
    avi_index_entry_t *idx_entry;
    
    if (handle == NULL || frame_data == NULL) {
        return -1;
    }
    
    if (!handle->is_open) {
        return -1;
    }
    
    /* Check if max size will be exceeded */
    /* Need space for: '00dc' chunk header (8) + frame data + current file size */
    frame_size = handle->frame_size;
    if (handle->file_size + 8 + frame_size > handle->max_file_size) {
        return 1;  /* Max size reached, auto-stop */
    }
    
    /* Write frame header ('00dc' chunk) */
    if (AVI_WriteFrameHeader(handle) != 0) {
        return -1;
    }
    
    /* Record offset for index (relative to 'movi' chunk start) */
    frame_offset = handle->movi_size;
    
    /* Write frame data */
    res = f_write(&(handle->file), frame_data, frame_size, &bytes_written);
    if (res != FR_OK || bytes_written != frame_size) {
        return -1;
    }
    
    /* Pad to word boundary if needed */
    if (frame_size % 2) {
        uint8_t pad = 0;
        f_write(&(handle->file), &pad, 1, NULL);
        frame_size++;  /* Include padding in size */
    }
    
    /* Update statistics */
    handle->frame_count++;
    handle->movi_size += 8 + frame_size;  /* '00dc' header + frame data */
    handle->file_size += 8 + frame_size;   /* Include header in file size */
    
    /* Add index entry */
    if (handle->idx_count < VIDEO_MAX_INDEX_ENTRIES) {
        idx_entry = &(handle->idx_entries[handle->idx_count]);
        idx_entry->chunk_id = AVI_FOURCC_00DC;
        idx_entry->flags = 0x10;  /* AVIIF_KEYFRAME - all frames are keyframes */
        idx_entry->offset = frame_offset;
        idx_entry->size = frame_size;
        handle->idx_count++;
    }
    
    return 0;
}

/**
 * @brief   Close AVI file and finalize
 */
int8_t AVI_Close(avi_handle_t *handle)
{
    FRESULT res;
    
    if (handle == NULL) {
        return -1;
    }
    
    if (!handle->is_open) {
        return 0;
    }
    
    /* Write index chunk */
    if (handle->idx_count > 0) {
        uint32_t idx1_size = 8 + handle->idx_count * sizeof(avi_index_entry_t);
        
        /* Write 'idx1' chunk header */
        WRITE_CHUNK_ID(handle->file, AVI_FOURCC_IDX1);
        WRITE_UINT32_LE(handle->file, handle->idx_count * sizeof(avi_index_entry_t));
        
        /* Write index entries */
        res = f_write(&(handle->file), handle->idx_entries, 
                      handle->idx_count * sizeof(avi_index_entry_t), NULL);
        if (res != FR_OK) {
            goto error;
        }
        
        /* Pad to word boundary */
        if (idx1_size % 2) {
            uint8_t pad = 0;
            f_write(&(handle->file), &pad, 1, NULL);
        }
    }
    
    /* Update header with final values */
    AVI_UpdateHeader(handle);
    
    /* Close file */
    res = f_close(&(handle->file));
    if (res != FR_OK) {
        goto error;
    }
    
    /* Free index buffer */
    if (handle->idx_entries) {
        free(handle->idx_entries);
        handle->idx_entries = NULL;
    }
    
    handle->is_open = 0;
    return 0;

error:
    f_close(&(handle->file));
    if (handle->idx_entries) {
        free(handle->idx_entries);
        handle->idx_entries = NULL;
    }
    handle->is_open = 0;
    return -1;
}

/**
 * @brief   Get AVI recording status
 */
uint32_t AVI_GetFrameCount(avi_handle_t *handle)
{
    if (handle == NULL) {
        return 0;
    }
    return handle->frame_count;
}

/**
 * @brief   Get current file size
 */
uint32_t AVI_GetFileSize(avi_handle_t *handle)
{
    if (handle == NULL) {
        return 0;
    }
    return handle->file_size;
}

/**
 * @brief   Check if max file size reached
 */
uint8_t AVI_IsMaxSizeReached(avi_handle_t *handle)
{
    if (handle == NULL) {
        return 0;
    }
    return (handle->file_size >= handle->max_file_size) ? 1 : 0;
}

/*============================================================================
 * Private Functions
 *==========================================================================*/

/**
 * @brief   Write AVI file header
 */
static int8_t AVI_WriteHeader(avi_handle_t *handle)
{
    uint32_t hdrl_size;
    uint32_t temp_pos;
    
    hdrl_size = 4 + sizeof(avi_main_header_t) + 4 + 4 + sizeof(avi_stream_header_t) + 4 + 4 + sizeof(avi_bitmap_info_header_t);
    
    handle->movi_offset = 12 + hdrl_size + 8;
    
    WRITE_CHUNK_ID(handle->file, AVI_FOURCC_RIFF);
    
    temp_pos = f_tell(&(handle->file));
    WRITE_UINT32_LE(handle->file, 0);
    
    WRITE_CHUNK_ID(handle->file, AVI_FOURCC_AVI);
    
    WRITE_CHUNK_ID(handle->file, AVI_FOURCC_LIST);
    WRITE_UINT32_LE(handle->file, hdrl_size);
    WRITE_CHUNK_ID(handle->file, AVI_FOURCC_HDRL);
    
    {
        uint32_t avih_us_per_frame = 1000000 / handle->fps;
        uint32_t avih_max_bytes = handle->frame_size * handle->fps;
        
        WRITE_CHUNK_ID(handle->file, AVI_FOURCC_AVIH);
        WRITE_UINT32_LE(handle->file, sizeof(avi_main_header_t));
        
        WRITE_UINT32_LE(handle->file, avih_us_per_frame);
        WRITE_UINT32_LE(handle->file, avih_max_bytes);
        WRITE_UINT32_LE(handle->file, 2048);
        WRITE_UINT32_LE(handle->file, AVIF_HASINDEX | AVIF_ISINTERLEAVED);
        WRITE_UINT32_LE(handle->file, 0);
        WRITE_UINT32_LE(handle->file, 0);                    /* initial_frames */
        WRITE_UINT32_LE(handle->file, 1);                    /* streams */
        WRITE_UINT32_LE(handle->file, handle->frame_size);  /* suggested_buffer_size */
        WRITE_UINT32_LE(handle->file, handle->width);        /* width */
        WRITE_UINT32_LE(handle->file, handle->height);       /* height */
        WRITE_UINT32_LE(handle->file, 0);                    /* scale */
        WRITE_UINT32_LE(handle->file, 0);                    /* rate */
        WRITE_UINT32_LE(handle->file, 0);                    /* start */
        WRITE_UINT32_LE(handle->file, 0);                    /* length */
    }
    
    /* Write 'strl' LIST */
    WRITE_CHUNK_ID(handle->file, AVI_FOURCC_LIST);
    WRITE_UINT32_LE(handle->file, 4 + sizeof(avi_stream_header_t) + 4 + sizeof(avi_bitmap_info_header_t));
    WRITE_CHUNK_ID(handle->file, AVI_FOURCC_STRL);
    
    /* Write 'strh' chunk */
    WRITE_CHUNK_ID(handle->file, AVI_FOURCC_STRH);
    WRITE_UINT32_LE(handle->file, sizeof(avi_stream_header_t));
    WRITE_UINT32_LE(handle->file, AVI_FOURCC_VIDS);  /* fccType = 'vids' */
    WRITE_UINT32_LE(handle->file, AVI_FOURCC_DIB);   /* fccHandler = 'DIB ' */
    WRITE_UINT32_LE(handle->file, 0);                 /* flags */
    WRITE_UINT16_LE(handle->file, 0);                /* priority */
    WRITE_UINT16_LE(handle->file, 0);                /* language */
    WRITE_UINT32_LE(handle->file, 0);                /* initial_frames */
    WRITE_UINT32_LE(handle->file, 1);                /* scale */
    WRITE_UINT32_LE(handle->file, handle->fps);      /* rate = fps */
    WRITE_UINT32_LE(handle->file, 0);                /* start */
    WRITE_UINT32_LE(handle->file, 0);                /* length (update later) */
    WRITE_UINT32_LE(handle->file, handle->frame_size);  /* suggested_buffer_size */
    WRITE_UINT32_LE(handle->file, 10000);            /* quality */
    WRITE_UINT32_LE(handle->file, 0);                /* sample_size (0 = variable) */
    WRITE_INT32_LE(handle->file, 0);                 /* left */
    WRITE_INT32_LE(handle->file, 0);                 /* top */
    WRITE_INT32_LE(handle->file, handle->width);     /* right */
    WRITE_INT32_LE(handle->file, handle->height);    /* bottom */
    
    /* Write 'strf' chunk (BITMAPINFOHEADER) */
    WRITE_CHUNK_ID(handle->file, AVI_FOURCC_STRF);
    WRITE_UINT32_LE(handle->file, sizeof(avi_bitmap_info_header_t));
    WRITE_UINT32_LE(handle->file, sizeof(avi_bitmap_info_header_t));  /* bi_size */
    WRITE_INT32_LE(handle->file, handle->width);     /* bi_width */
    WRITE_INT32_LE(handle->file, handle->height);    /* bi_height (positive = bottom-up) */
    WRITE_UINT16_LE(handle->file, 1);                /* bi_planes */
    WRITE_UINT16_LE(handle->file, 16);               /* bi_bit_count (RGB565 = 16bit) */
    WRITE_UINT32_LE(handle->file, AVI_FOURCC_DIB);   /* bi_compression = 'DIB ' */
    WRITE_UINT32_LE(handle->file, handle->frame_size);  /* bi_size_image */
    WRITE_INT32_LE(handle->file, 0);                  /* bi_x_pels_per_meter */
    WRITE_INT32_LE(handle->file, 0);                  /* bi_y_pels_per_meter */
    WRITE_UINT32_LE(handle->file, 0);                /* bi_clr_used */
    WRITE_UINT32_LE(handle->file, 0);                /* bi_clr_important */
    
    /* Write 'movi' LIST */
    WRITE_CHUNK_ID(handle->file, AVI_FOURCC_LIST);
    WRITE_UINT32_LE(handle->file, 0);  /* Placeholder for 'movi' size */
    WRITE_CHUNK_ID(handle->file, AVI_FOURCC_MOVI);
    
    /* Update 'movi' size offset (later) */
    /* movi_size position = handle->movi_offset + 4 */
    
    /* Update RIFF file size */
    temp_pos = f_tell(&(handle->file));
    handle->file_size = temp_pos;
    
    return 0;
}

/**
 * @brief   Write frame chunk header ('00dc')
 */
static int8_t AVI_WriteFrameHeader(avi_handle_t *handle)
{
    /* Write '00dc' chunk header */
    WRITE_CHUNK_ID(handle->file, AVI_FOURCC_00DC);
    WRITE_UINT32_LE(handle->file, handle->frame_size);
    
    return 0;
}

/**
 * @brief   Update AVI header with final values (called at close)
 */
static int8_t AVI_UpdateHeader(avi_handle_t *handle)
{
    uint32_t total_size;
    uint32_t hdrl_size;
    uint32_t movi_size;
    uint32_t idx_size;
    
    if (!handle->is_open) {
        return -1;
    }
    
    /* Calculate sizes */
    hdrl_size = 12 + sizeof(avi_main_header_t) + 12 + 4 + sizeof(avi_stream_header_t) + 12 + sizeof(avi_bitmap_info_header_t);
    movi_size = handle->movi_size;
    idx_size = handle->idx_count * sizeof(avi_index_entry_t);
    if (idx_size % 2) idx_size++;  /* Pad to word boundary */
    
    total_size = 4 + hdrl_size + 8 + movi_size + 8 + idx_size;  /* 4 = 'AVI ' */
    
    /* Update RIFF size at beginning of file */
    f_lseek(&(handle->file), 4);
    WRITE_UINT32_LE(handle->file, total_size);
    
    /* Update 'avih' total_frames at offset within 'hdrl' */
    /* 'RIFF' + 'AVI ' + 'LIST' + 'hdrl' + 'avih' + size */
    f_lseek(&(handle->file), 12 + 4 + 8);  /* Position at 'avih' total_frames */
    WRITE_UINT32_LE(handle->file, handle->frame_count);  /* total_frames */
    
    /* Update 'avih' length */
    WRITE_UINT32_LE(handle->file, handle->frame_count);  /* length = frame_count */
    
    /* Update 'strh' stream length */
    f_lseek(&(handle->file), 12 + 4 + 8 + 8 + sizeof(avi_main_header_t) + 12 + 4 + 40);  /* Approximate position */
    WRITE_UINT32_LE(handle->file, handle->frame_count);  /* length */
    
    /* Update 'movi' LIST size */
    f_lseek(&(handle->file), handle->movi_offset);
    WRITE_UINT32_LE(handle->file, movi_size);
    
    /* Update file_size in handle */
    handle->file_size = 8 + total_size;  /* 'RIFF' + size */
    
    return 0;
}
