#pragma once

#include <sys/stat.h>
#include <gctypes.h>

#ifdef __cplusplus
	extern "C" {
#endif

typedef struct _stats
{
	s32 Device;
	u64 Size;
	s32 Mode;
	u64 Identifier; // st_ino; on fat, cluster number.
} Stats;

int File_Init();
int File_Deinit();

int File_Mount(int disk, int filesystem);

int File_Stat(const char* path, Stats* st);
int File_CreateFile(const char* path);
int File_Delete(const char* path);
int File_Rename(const char* source, const char* dest);

int File_CreateDir(const char* path);
int File_OpenDir(const char* path);
int File_NextDir(int dir, char* path, Stats* st);
int File_CloseDir(int dir);

int File_Open(const char* path, int mode);
int File_Close(int fd);
int File_Read(int fd, u8* buffer, int length);
int File_Write(int fd, const u8* buffer, int length);
int File_Seek(int fd, int whence, int where);
int File_Tell(int fd);
int File_Sync(int fd);

#ifdef __cplusplus
	}
#endif

#define DISK_SD				0x01
#define DISK_USB			0x02

#define FILE_FAT			0x01
#define FILE_ELM			0x02
#define FILE_NTFS			0x03
#define FILE_EXT2			0x04
#define FILE_SMB			0x05
