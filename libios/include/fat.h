#pragma once

#include <sys/stat.h>
#include <fcntl.h>

#include "gctypes.h"

#ifdef __cplusplus
	extern "C" {
#endif

#include <stdint.h>
#include <disc_io.h>

/*
Initialise any inserted block-devices.
Add the fat device driver to the devoptab, making it available for standard file functions.
cacheSize: The number of pages to allocate for each inserted block-device
setAsDefaultDevice: if true, make this the default device driver for file operations
*/
extern bool fatInit (uint32_t cacheSize, bool setAsDefaultDevice);

/*
Calls fatInit with setAsDefaultDevice = true and cacheSize optimised for the host system.
*/
extern bool fatInitDefault (void);

/*
Mount the device pointed to by interface, and set up a devoptab entry for it as "name:".
You can then access the filesystem using "name:/".
This will mount the active partition or the first valid partition on the disc, 
and will use a cache size optimized for the host system.
*/
extern bool fatMountSimple (const char* name, const DISC_INTERFACE* interface);

/*
Mount the device pointed to by interface, and set up a devoptab entry for it as "name:".
You can then access the filesystem using "name:/".
If startSector = 0, it will mount the active partition of the first valid partition on
the disc. Otherwise it will try to mount the partition starting at startSector.
cacheSize specifies the number of pages to allocate for the cache.
This will not startup the disc, so you need to call interface->startup(); first.
*/
extern bool fatMount (const char* name, const DISC_INTERFACE* interface, sec_t startSector, uint32_t cacheSize, uint32_t SectorsPerPage);
/*
Unmount the partition specified by name.
If there are open files, it will attempt to synchronise them to disc.
*/
extern void fatUnmount (const char* name);

s32 FAT_Open(const char *path, u32 mode);
//s32 FAT_Open_Prealloc(const char *path, u32 mode, FILE_STRUCT* fs);
s32 FAT_Close(s32 fd);
//s32 FAT_Close_Prealloc(s32 fd);
s32 FAT_Read(s32 fd, void *buffer, u32 len);
s32 FAT_Write(s32 fd, void *buffer, u32 len);
s32 FAT_Seek(s32 fd, u32 where, u32 whence);
s32 FAT_Tell(s32 fd);
s32 FAT_CreateDir(const char *dirpath);
s32 FAT_CreateFile(const char *filepath);
s32 FAT_ReadDir(const char *dirpath, char *outbuf, u32 *outlen, u32 maxlen);
s32 FAT_Delete(const char *path);
s32 FAT_DeleteDir(const char *dirpath);
s32 FAT_Rename(const char *oldname, const char *newname);
s32 FAT_Stat(const char *path, struct stat *stats);
s32 FAT_GetVfsStats(const char *path, void *stats);
//s32 FAT_GetFileStats(s32 fd, fstats *stats);
s32 FAT_GetUsage(const char *dirpath, u64 *size, u32 *files);
s32 FAT_Sync(s32 fd);
s32 FAT_OpenDir(const char* path);
s32 FAT_NextDir(s32 state, char* filename, struct stat* st);
s32 FAT_CloseDir(s32 dir);

#ifdef __cplusplus
	}
#endif
