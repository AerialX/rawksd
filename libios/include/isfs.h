#pragma once

#include <gctypes.h>

#define ISFS_MAXPATH				IPC_MAXPATH_LEN

#define ISFS_OPEN_READ				0x01
#define ISFS_OPEN_WRITE				0x02
#define ISFS_OPEN_RW				(ISFS_OPEN_READ | ISFS_OPEN_WRITE)

#define ISFS_OK						   0
#define ISFS_ENOMEM					 -22
#define ISFS_EINVAL					-101

#ifdef __cplusplus
   extern "C" {
#endif

typedef s32 (*isfscallback)(s32 result,void *usrdata);

#define ISFS_Initialize(...) IPC_OK
#define ISFS_Deinitialize(...) IPC_OK

s32 ISFS_Open(const char* filepath, u8 mode);
s32 ISFS_Close(s32 fd);
s32 ISFS_Write(s32 fd, const void* buffer, u32 length);
s32 ISFS_Read(s32 fd, void* buffer, u32 length);
s32 ISFS_Seek(s32 fd, s32 where, s32 whence);

#ifdef __cplusplus
   }
#endif
