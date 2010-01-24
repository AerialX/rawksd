#pragma once

#include "disc_io.h"
#include "gctypes.h"

#ifdef __cplusplus
   extern "C" {
#endif

bool usb2storage_Init(void);
bool usb2storage_Shutdown(void);
bool usb2storage_IsInserted(void);
bool usb2storage_ReadSectors(u32 sector, u32 numSectors, void *buffer);
bool usb2storage_WriteSectors(u32 sector, u32 numSectors, void *buffer);
bool usb2storage_ClearStatus(void);

#ifdef __cplusplus
   }
#endif

extern const DISC_INTERFACE __io_usb2storage;
