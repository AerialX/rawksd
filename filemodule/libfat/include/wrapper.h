#ifndef _FAT_WRAPPER_H_
#define _FAT_WRAPPER_H_

#include <sys/stat.h>
#include <sys/statvfs.h>
#include "gctypes.h"
#include <fcntl.h>
#include "fat.h"
#include "fat/fatfile.h"

/* Filestats structure */
typedef struct _fstats {
	u32 file_length;
	u32 file_pos;
} fstats;

#ifdef __cplusplus
   extern "C" {
#endif /* __cplusplus */

/* Prototypes */
s32 FAT_Open(const char *path, u32 mode);
s32 FAT_Open_Prealloc(const char *path, u32 mode, FILE_STRUCT* fs);
s32 FAT_Close(s32 fd);
s32 FAT_Close_Prealloc(s32 fd);
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
s32 FAT_GetVfsStats(const char *path, struct statvfs *stats);
s32 FAT_GetFileStats(s32 fd, fstats *stats);
s32 FAT_GetUsage(const char *dirpath, u64 *size, u32 *files);
s32 FAT_Flush(s32 fd);
s32 FAT_OpenDir(const char* path);
s32 FAT_NextDir(s32 state, char* filename, struct stat* st);
s32 FAT_CloseDir(s32 dir);

#ifdef __cplusplus
   }
#endif /* __cplusplus */

#endif
