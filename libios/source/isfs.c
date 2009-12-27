#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <time.h>
#include <gcutil.h>
#include <ipc.h>

#include "isfs.h"

#include "syscalls.h"
#include "mem.h"

#define IOS_Ioctlv os_ioctlv
#define IOS_Ioctl os_ioctl
#define IOS_Open os_open
#define IOS_Close os_close
#define IOS_Read os_read
#define IOS_Write os_write
#define IOS_Seek os_seek

#define ISFS_STRUCTSIZE				(sizeof(struct isfs_cb))
#define ISFS_HEAPSIZE				(ISFS_STRUCTSIZE<<4)

#define ISFS_FUNCNULL				0
#define ISFS_FUNCGETSTAT			1
#define ISFS_FUNCREADDIR			2
#define ISFS_FUNCGETATTR			3
#define ISFS_FUNCGETUSAGE			4

#define ISFS_IOCTL_FORMAT			1
#define ISFS_IOCTL_GETSTATS			2
#define ISFS_IOCTL_CREATEDIR		3
#define ISFS_IOCTL_READDIR			4
#define ISFS_IOCTL_SETATTR			5
#define ISFS_IOCTL_GETATTR			6
#define ISFS_IOCTL_DELETE			7
#define ISFS_IOCTL_RENAME			8
#define ISFS_IOCTL_CREATEFILE		9
#define ISFS_IOCTL_SETFILEVERCTRL	10
#define ISFS_IOCTL_GETFILESTATS		11
#define ISFS_IOCTL_GETUSAGE			12
#define ISFS_IOCTL_SHUTDOWN			13

s32 ISFS_Open(const char* filepath, u8 mode)
{
	s32 ret;
	s32 len;

	len = strnlen(filepath, ISFS_MAXPATH);
	if (len >= ISFS_MAXPATH)
		return ISFS_EINVAL;

	os_sync_after_write(filepath, len + 1);
	ret = os_open(filepath, mode);

	return ret;
}

s32 ISFS_Write(s32 fd, const void* buffer, u32 length)
{
	if (length <= 0 || buffer == NULL || ((u32)buffer % 32) != 0)
		return ISFS_EINVAL;
	
	os_sync_after_write(buffer, length);
	return os_write(fd, buffer, length);
}

s32 ISFS_Read(s32 fd, void* buffer, u32 length)
{
	if (length <= 0 || buffer == NULL || ((u32)buffer % 32) != 0)
		return ISFS_EINVAL;
	
	int ret = os_read(fd, buffer, length);
	os_sync_before_read(buffer, length);
	return ret;
}

s32 ISFS_Seek(s32 fd, s32 where, s32 whence)
{
	return os_seek(fd, where, whence);
}

s32 ISFS_Close(s32 fd)
{
	if (fd < 0)
		return 0;

	return os_close(fd);
}
