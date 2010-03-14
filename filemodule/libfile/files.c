#include <files.h>

#include <string.h>
#include <unistd.h>

#ifdef GEKKO
#include <stdlib.h>
#include <gccore.h>
#define os_open IOS_Open
#define os_close IOS_Close
#define os_read IOS_Read
#define os_write IOS_Write
#define os_ioctl IOS_Ioctl
#define os_ioctlv IOS_Ioctlv
#define os_seek IOS_Seek
#define os_sync_after_write DCFlushRange
#define os_sync_before_read DCInvalidateRange
#define Alloc malloc
#define Realloc(a, b, c) realloc(a, b)
#define Dealloc free
#else
#include <syscalls.h>
#include <mem.h>
#define MEM_VIRTUAL_TO_PHYSICAL(a) (a)
#endif

static u32 ioctlbuffer[0x20];
static ioctlv ioctlvbuffer[0x04];
static int file_fd = -1;

int File_Init()
{
	if (file_fd < 0)
		file_fd = os_open(FILE_MODULE_NAME, 0);

	return file_fd;
}

int File_Deinit()
{
	if (file_fd >= 0)
		os_close(file_fd);
	file_fd = -1;
	
	return 0;
}

static int FileMountDisk(disk_phys disk)
{
	ioctlbuffer[0] = disk;
	os_sync_after_write(ioctlbuffer, 0x04);
	return os_ioctl(file_fd, IOCTL_InitDisc, ioctlbuffer, sizeof(ioctlbuffer), NULL, 0);
}

int File_MountDisk(disk_fs filesystem, disk_phys disk)
{
	if (file_fd < 0)
		return ERROR_NOTOPENED;
	
	int ret = FileMountDisk(disk);
	if (ret < 0)
		return ret;
	
	return File_Mount(filesystem, &ret, sizeof(ret));
}

int File_Mount(disk_fs filesystem, const void* options, int optionslen)
{
	if (file_fd < 0)
		return ERROR_NOTOPENED;
	
	ioctlbuffer[0] = filesystem;
	os_sync_after_write(ioctlbuffer, 0x04);
	os_sync_after_write((void*)options, optionslen);
	return os_ioctl(file_fd, IOCTL_Mount, ioctlbuffer, sizeof(ioctlbuffer), (void*)options, optionslen);
}

int File_Unmount(int fs)
{
	ioctlbuffer[0] = fs;
	os_sync_after_write(ioctlbuffer, 0x04);
	return os_ioctl(file_fd, IOCTL_Unmount, ioctlbuffer, sizeof(ioctlbuffer), NULL, 0);
}

int File_SetDefault(int fs)
{
	ioctlbuffer[0] = fs;
	os_sync_after_write(ioctlbuffer, 0x04);
	return os_ioctl(file_fd, IOCTL_SetDefault, ioctlbuffer, sizeof(ioctlbuffer), NULL, 0);
}

int File_SetDefaultPath(const char* mountpoint)
{
	int size = strlen(mountpoint) + 1;
	os_sync_after_write((void*)mountpoint, size);
	return os_ioctl(file_fd, IOCTL_SetDefault, NULL, 0, (void*)mountpoint, size);
}

int File_GetMountPoint(int fs, char* mountpoint, int length)
{
	ioctlbuffer[0] = fs;
	os_sync_after_write(ioctlbuffer, 0x04);
	int ret = os_ioctl(file_fd, IOCTL_MountPoint, ioctlbuffer, sizeof(ioctlbuffer), mountpoint, length);
	os_sync_before_read(mountpoint, length);
	return ret;
}

int File_Stat(const char* path, Stats* st)
{
	if (file_fd < 0)
		return ERROR_NOTOPENED;
	
	int ret;
	int len = strlen(path) + 1;
	os_sync_after_write((void*)path, len);
	ret = os_ioctl(file_fd, IOCTL_Stat, (void*)path, len, st, sizeof(Stats));
	
	os_sync_before_read(st, sizeof(Stats));
	return ret;
}

int File_CreateFile(const char* path)
{
	if (file_fd < 0)
		return ERROR_NOTOPENED;
	
	int len = strlen(path) + 1;
	os_sync_after_write((void*)path, len);
	return os_ioctl(file_fd, IOCTL_CreateFile, (void*)path, len, NULL, 0);
}

int File_Delete(const char* path)
{
	if (file_fd < 0)
		return ERROR_NOTOPENED;
	
	int len = strlen(path) + 1;
	os_sync_after_write((void*)path, len);
	return os_ioctl(file_fd, IOCTL_Delete, (void*)path, len, NULL, 0);
}

int File_Rename(const char* source, const char* dest)
{
	if (file_fd < 0)
		return ERROR_NOTOPENED;
	
	os_sync_after_write((void*)source, strlen(source) + 1);
	os_sync_after_write((void*)dest, strlen(dest) + 1);
	ioctlbuffer[0] = (int)MEM_VIRTUAL_TO_PHYSICAL(source);
	ioctlbuffer[1] = (int)MEM_VIRTUAL_TO_PHYSICAL(dest);
	os_sync_after_write(ioctlbuffer, 0x08);
	return os_ioctl(file_fd, IOCTL_Rename, ioctlbuffer, sizeof(ioctlbuffer), NULL, 0);
}

int File_CreateDir(const char* path)
{
	if (file_fd < 0)
		return ERROR_NOTOPENED;
	
	int len = strlen(path) + 1;
	os_sync_after_write((void*)path, len);
	return os_ioctl(file_fd, IOCTL_CreateDir, (void*)path, len, NULL, 0);
}

int File_OpenDir(const char* path)
{
	if (file_fd < 0)
		return ERROR_NOTOPENED;
	
	int len = strlen(path) + 1;
	os_sync_after_write((void*)path, len);
	return os_ioctl(file_fd, IOCTL_OpenDir, (void*)path, len, NULL, 0);
}

int File_NextDir(int dir, char* path, Stats* st)
{
	if (file_fd < 0)
		return ERROR_NOTOPENED;
	
	int ret;
	ioctlbuffer[0] = dir;
	os_sync_after_write(ioctlbuffer, 0x04);

	ioctlvbuffer[0].data = ioctlbuffer;
	ioctlvbuffer[0].len = 0x04;
	ioctlvbuffer[1].data = path;
	ioctlvbuffer[1].len = MAXPATHLEN;
	ioctlvbuffer[2].data = st;
	ioctlvbuffer[2].len = sizeof(Stats);
	os_sync_after_write(ioctlvbuffer, sizeof(ioctlvbuffer));
	ret = os_ioctlv(file_fd, IOCTL_NextDir, 1, 2, ioctlvbuffer);
	os_sync_before_read(path, MAXPATHLEN);
	os_sync_before_read(st, sizeof(Stats));

	return ret;
}

int File_CloseDir(int dir)
{
	if (file_fd < 0)
		return ERROR_NOTOPENED;
	
	ioctlbuffer[0] = dir;
	os_sync_after_write(ioctlbuffer, 0x04);
	return os_ioctl(file_fd, IOCTL_CloseDir, ioctlbuffer, sizeof(ioctlbuffer), NULL, 0);
}

int File_Open(const char* path, int mode)
{
	char fpath[MAXPATHLEN];
	strcpy(fpath, FILE_MODULE_NAME);
	strcat(fpath, path);
	os_sync_after_write(fpath, strlen(fpath) + 1);
	return os_open(fpath, mode);
}

int File_Close(int fd)
{
	return os_close(fd);
}

int File_Read(int fd, void* buffer, int length)
{
	int ret = os_read(fd, buffer, length);
	os_sync_before_read(buffer, length);
	return ret;
}

int File_Write(int fd, const void* buffer, int length)
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

static const char HEX_CHARS[] = "0123456789abcdef";
static void IntToHex(char* dest, u64 num, int length)
{
	for (int i = length - 1; i >= 0; i--)
		*(dest++) = HEX_CHARS[(num >> (i * 4)) & 0x0F];
}

int File_Open_ID(u64 id, int mode)
{
	char path[0x40];
	strcpy(path, FILE_ID_PATH);
	IntToHex(path + FILE_ID_PATH_LEN, id, 0x10);
	path[FILE_ID_PATH_LEN + 0x10] = '\0';
	return File_Open(path, mode);
}

int File_RiiFS_Mount(const char* ip, int port)
{
	if (strlen(ip) >= 0x10)
		return ERROR_UNRECOGNIZED;

	char data[0x20];
	strcpy(data, ip);
	*(int*)(data + 0x10) = port;
	return File_Mount(FS_RIIFS, data, 0x14);
}

int File_Fat_Mount(disk_phys disk, const char* name)
{
	int ret = FileMountDisk(disk);
	if (ret < 0)
		return ret;

	char options[0x40];
	*(u32*)options = ret;
	strncpy(options + 4, name, 0x40 - 4);
	return File_Mount(FS_FAT, options, 4 + strlen(name) + 1);
}
