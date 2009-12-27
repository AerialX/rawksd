#include <files.h>

#include <syscalls.h>

#include <string.h>
#include <unistd.h>

#define IOCTL_InitDisc		0x30
#define IOCTL_Mount			0x31
#define IOCTL_Stat			0x40
#define IOCTL_CreateFile	0x41
#define IOCTL_Delete		0x42
#define IOCTL_Rename		0x43
#define SEEK_Tell			0x48
#define SEEK_Sync			0x49
#define IOCTL_CreateDir		0x50
#define IOCTL_OpenDir		0x51
#define IOCTL_NextDir		0x52
#define IOCTL_CloseDir		0x53

static u32 ioctlbuffer[0x20];
static int file_fd = -1;
static const char __fileName[] = "file";

#ifdef GEKKO
#include <gccore.h>
#define os_open IOS_Open
#define os_close IOS_Close
#define os_read IOS_Read
#define os_write IOS_Write
#define os_ioctl IOS_Ioctl
#define os_seek IOS_Seek
#define os_sync_after_write DCFlushRange
#define os_sync_before_read DCInvalidateRange
#define null NULL
#endif

int File_Init()
{
	file_fd = os_open(__fileName, 0);
	if (file_fd < 0)
		return file_fd;
	return 0;
}

int File_Deinit()
{
	if (file_fd >= 0)
		os_close(file_fd);
	
	return 0;
}

int File_Mount(int disk, int filesystem)
{
	ioctlbuffer[0] = disk;
	os_sync_after_write(ioctlbuffer, 0x04);
	int ret = os_ioctl(file_fd, IOCTL_InitDisc, ioctlbuffer, sizeof(ioctlbuffer), null, 0);
	if (ret < 0)
		return ret;
	ioctlbuffer[0] = filesystem;
	os_sync_after_write(ioctlbuffer, 0x04);
	return os_ioctl(file_fd, IOCTL_Mount, ioctlbuffer, sizeof(ioctlbuffer), null, 0);
}

int File_Stat(const char* path, Stats* st)
{
	int len = strlen(path) + 1;
	os_sync_after_write(path, len);
	int ret = os_ioctl(file_fd, IOCTL_Stat, path, len, st, sizeof(Stats));
	os_sync_before_read(st, sizeof(Stats));
	return ret;
}

int File_CreateFile(const char* path)
{
	int len = strlen(path) + 1;
	os_sync_after_write(path, len);
	return os_ioctl(file_fd, IOCTL_CreateFile, path, len, null, 0);
}

int File_Delete(const char* path)
{
	int len = strlen(path) + 1;
	os_sync_after_write(path, len);
	return os_ioctl(file_fd, IOCTL_Delete, path, len, null, 0);
}

int File_Rename(const char* source, const char* dest)
{
	#ifdef GEKKO
	source = MEM_VIRTUAL_TO_PHYSICAL(source);
	dest = MEM_VIRTUAL_TO_PHYSICAL(dest);
	#endif
	
	os_sync_after_write(source, strlen(source) + 1);
	os_sync_after_write(dest, strlen(dest) + 1);
	ioctlbuffer[0] = (int)source;
	ioctlbuffer[1] = (int)dest;
	os_sync_after_write(ioctlbuffer, 0x08);
	return os_ioctl(file_fd, IOCTL_Rename, ioctlbuffer, sizeof(ioctlbuffer), null, 0);
}

int File_CreateDir(const char* path)
{
	int len = strlen(path) + 1;
	os_sync_after_write(path, len);
	return os_ioctl(file_fd, IOCTL_CreateDir, path, len, null, 0);
}

int File_OpenDir(const char* path)
{
	int len = strlen(path) + 1;
	os_sync_after_write(path, len);
	return os_ioctl(file_fd, IOCTL_OpenDir, path, len, null, 0);
}

int File_NextDir(int dir, char* path, Stats* st)
{
	ioctlbuffer[0] = dir;
	#ifdef GEKKO
	ioctlbuffer[1] = (int)MEM_VIRTUAL_TO_PHYSICAL(st);
	#else
	ioctlbuffer[1] = (int)st;
	#endif
	os_sync_after_write(ioctlbuffer, 0x04*2);
	int ret = os_ioctl(file_fd, IOCTL_NextDir, ioctlbuffer, sizeof(ioctlbuffer), path, MAXPATHLEN);
	os_sync_before_read(path, MAXPATHLEN);
	os_sync_before_read(st, sizeof(Stats));
	return ret;
}

int File_CloseDir(int dir)
{
	ioctlbuffer[0] = dir;
	os_sync_after_write(ioctlbuffer, 0x04);
	return os_ioctl(file_fd, IOCTL_CloseDir, ioctlbuffer, sizeof(ioctlbuffer), null, 0);
}

int File_Open(const char* path, int mode)
{
	static char fpath[MAXPATHLEN];
	strcpy(fpath, __fileName);
	strcat(fpath, path);
	os_sync_after_write(path, strlen(fpath) + 1);
	return os_open(fpath, mode);
}

int File_Close(int fd)
{
	return os_close(fd);
}

int File_Read(int fd, u8* buffer, int length)
{
	int ret = os_read(fd, buffer, length);
	os_sync_before_read(buffer, length);
	return ret;
}

int File_Write(int fd, const u8* buffer, int length)
{
	os_sync_after_write(buffer, length);
	return os_write(fd, buffer, length);
}

int File_Seek(int fd, int where, int whence)
{
	return os_seek(fd, where, whence);
}

int File_Tell(int fd)
{
	return os_seek(fd, 0, SEEK_Tell);
}

int File_Sync(int fd)
{
	return os_seek(fd, 0, SEEK_Sync);
}
