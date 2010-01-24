#include <files.h>

#include <string.h>
#include <unistd.h>

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
#else
#include <syscalls.h>
#define MEM_VIRTUAL_TO_PHYSICAL(a) (a)
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

int File_Mount(disk_phys disk, disk_fs filesystem)
{
	int ret;

	ioctlbuffer[0] = disk;
	os_sync_after_write(ioctlbuffer, 0x04);

	ret = os_ioctl(file_fd, IOCTL_InitDisc, ioctlbuffer, sizeof(ioctlbuffer), NULL, 0);
	if (ret < 0)
		return ret;

	ioctlbuffer[0] = filesystem;
	os_sync_after_write(ioctlbuffer, 0x04);
	return os_ioctl(file_fd, IOCTL_Mount, ioctlbuffer, sizeof(ioctlbuffer), NULL, 0);
}

int File_Stat(const char* path, Stats* st)
{
	int ret, len = strlen(path) + 1;
	os_sync_after_write((void*)path, len);
	ret = os_ioctl(file_fd, IOCTL_Stat, (void*)path, len, st, sizeof(Stats));

	os_sync_before_read(st, sizeof(Stats));
	return ret;
}

int File_CreateFile(const char* path)
{
	int len = strlen(path) + 1;
	os_sync_after_write((void*)path, len);
	return os_ioctl(file_fd, IOCTL_CreateFile, (void*)path, len, NULL, 0);
}

int File_Delete(const char* path)
{
	int len = strlen(path) + 1;
	os_sync_after_write((void*)path, len);
	return os_ioctl(file_fd, IOCTL_Delete, (void*)path, len, NULL, 0);
}

int File_Rename(const char* source, const char* dest)
{
	os_sync_after_write((void*)source, strlen(source) + 1);
	os_sync_after_write((void*)dest, strlen(dest) + 1);
	ioctlbuffer[0] = (int)MEM_VIRTUAL_TO_PHYSICAL(source);
	ioctlbuffer[1] = (int)MEM_VIRTUAL_TO_PHYSICAL(dest);
	os_sync_after_write(ioctlbuffer, 0x08);
	return os_ioctl(file_fd, IOCTL_Rename, ioctlbuffer, sizeof(ioctlbuffer), NULL, 0);
}

int File_CreateDir(const char* path)
{
	int len = strlen(path) + 1;
	os_sync_after_write((void*)path, len);
	return os_ioctl(file_fd, IOCTL_CreateDir, (void*)path, len, NULL, 0);
}

int File_OpenDir(const char* path)
{
	int len = strlen(path) + 1;
	os_sync_after_write((void*)path, len);
	return os_ioctl(file_fd, IOCTL_OpenDir, (void*)path, len, NULL, 0);
}

int File_NextDir(int dir, char* path, Stats* st)
{
	int ret;
	ioctlbuffer[0] = dir;
	ioctlbuffer[1] = (int)MEM_VIRTUAL_TO_PHYSICAL(st);
	os_sync_after_write(ioctlbuffer, 0x04*2);
	ret = os_ioctl(file_fd, IOCTL_NextDir, ioctlbuffer, sizeof(ioctlbuffer), path, MAXPATHLEN);

	os_sync_before_read(path, MAXPATHLEN);
	os_sync_before_read(st, sizeof(Stats));
	return ret;
}

int File_CloseDir(int dir)
{
	ioctlbuffer[0] = dir;
	os_sync_after_write(ioctlbuffer, 0x04);
	return os_ioctl(file_fd, IOCTL_CloseDir, ioctlbuffer, sizeof(ioctlbuffer), NULL, 0);
}

int File_Open(const char* path, int mode)
{
	char fpath[MAXPATHLEN];
	strcpy(fpath, __fileName);
	strcat(fpath, path);
	os_sync_after_write(fpath, strlen(fpath) + 1);
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
	os_sync_after_write((void*)buffer, length);
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
