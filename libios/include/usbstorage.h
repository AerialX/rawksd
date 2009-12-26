#pragma once

#include "disc_io.h"
#include "gctypes.h"

#ifdef __cplusplus
   extern "C" {
#endif

bool usbstorage_Init(void);
bool usbstorage_Shutdown(void);
bool usbstorage_IsInserted(void);
bool usbstorage_ReadSectors(u32 sector, u32 numSectors, void *buffer);
bool usbstorage_WriteSectors(u32 sector, u32 numSectors, void *buffer);
bool usbstorage_ClearStatus(void);

#ifdef __cplusplus
   }
#endif

extern const DISC_INTERFACE __io_usbstorage;
