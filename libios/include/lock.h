#pragma once

#include "gctypes.h"

typedef osqueue_t lock_t;

#ifdef __cplusplus
{
#endif

lock_t InitializeLock(void *ptr, u32 initial, u32 max);
void GetLock(lock_t);
void ReleaseLock(lock_t);
void DestroyLock(lock_t);

#ifdef __cplusplus
}
#endif