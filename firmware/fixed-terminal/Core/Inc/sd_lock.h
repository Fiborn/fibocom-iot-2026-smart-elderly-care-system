#ifndef __SD_LOCK_H__
#define __SD_LOCK_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

typedef enum
{
    SD_LOCK_OWNER_NONE = 0,
    SD_LOCK_OWNER_GALLERY_SCAN,
    SD_LOCK_OWNER_GALLERY_VIEW,
    SD_LOCK_OWNER_PHOTO_WRITE,
    SD_LOCK_OWNER_HTTP_UPLOAD,
    SD_LOCK_OWNER_WAV_PLAYER
} SD_LockOwnerTypeDef;

uint8_t SD_Lock_TryAcquire(SD_LockOwnerTypeDef owner);
void SD_Lock_Release(SD_LockOwnerTypeDef owner);
uint8_t SD_Lock_IsOwnedBy(SD_LockOwnerTypeDef owner);
SD_LockOwnerTypeDef SD_Lock_GetOwner(void);
const char *SD_Lock_OwnerName(SD_LockOwnerTypeDef owner);

#ifdef __cplusplus
}
#endif

#endif
