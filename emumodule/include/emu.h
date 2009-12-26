#pragma once

#include <syscalls.h>
#include <stdlib.h>

#include <string.h>

#include <mem.h>

#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <timer.h>
#include <usbstorage.h>

#include <wiisd_io.h>
#include <ipc.h>

#include "es.h"

#include <binfile.h>

#include <network.h>

#include <files.h>
#include <print.h>

#include "logging.h"

#define ERROR_OPEN_FAILURE -6

#define IOS_OPEN 0x01
#define IOS_CLOSE 0x02
#define IOS_READ 0x03
#define IOS_WRITE 0x04
#define IOS_SEEK 0x05
#define IOS_IOCTL 0x06
#define IOS_IOCTLV 0x07

#define IOCTL_MOUNT_SD			0x80
#define IOCTL_MOUNT_USB			0x81
#define IOCTL_UNMOUNT			0x82
#define IOCTL_GET_FS_HANDLER	0x100
#define IOCTL_PROXY_MESSAGE		0x120
#define IOCTL_LWP_INFO			0x122

#define FSIOCTL_GETSTATS		2
#define FSIOCTL_CREATEDIR		3
#define FSIOCTL_READDIR			4
#define FSIOCTL_SETATTR			5
#define FSIOCTL_GETATTR			6
#define FSIOCTL_DELETE			7
#define FSIOCTL_RENAME			8
#define FSIOCTL_CREATEFILE		9

#define ISFS_MAXPATH			64

#define MAX_FILES_OPEN 0x10

#define DEV_FS_FILESTRUCTS 0x2004EEE4
#define EMU_CATCH_WRITES		-200

typedef struct isfs_ioctl
{
	union {
		struct {
			char filepathOld[ISFS_MAXPATH];
			char filepathNew[ISFS_MAXPATH];
		} fsrename;
		struct {
			u32 owner_id;
			u16 group_id;
			char filepath[ISFS_MAXPATH];
			u8 ownerperm;
			u8 groupperm;
			u8 otherperm;
			u8 attributes;
			u8 pad0[2];
		} fsattr;
	};
} isfs_ioctl;
