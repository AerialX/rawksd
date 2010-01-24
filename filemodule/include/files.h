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

typedef enum {
	DISK_SD,
	DISK_USB,
	DISK_USB2
} disk_phys;

typedef enum {
	FS_FAT,
	FS_ELM,
	FS_NTFS,
	FS_EXT2,
	FS_SMB,
	FS_ISFS
} disk_fs;

typedef enum {
	IOCTL_InitDisc =	0x30,
	IOCTL_Mount	=		0x31,
	IOCTL_Stat =		0x40,
	IOCTL_CreateFile =	0x41,
	IOCTL_Delete =		0x42,
	IOCTL_Rename =		0x43,
	SEEK_Tell =			0x48,
	SEEK_Sync =			0x49,
	IOCTL_CreateDir =	0x50,
	IOCTL_OpenDir =		0x51,
	IOCTL_NextDir =		0x52,
	IOCTL_CloseDir =	0x53
} file_ioctl;

int File_Init();
int File_Deinit();

int File_Mount(disk_phys disk, disk_fs filesystem);

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
