#pragma once

#include <gctypes.h>

#define IPC_HEAP			 -1

#define IPC_OPEN_NONE		  0
#define IPC_OPEN_READ		  1
#define IPC_OPEN_WRITE		  2
#define IPC_OPEN_RW			  (IPC_OPEN_READ|IPC_OPEN_WRITE)

#define IPC_MAXPATH_LEN		 64

// ISFS modes are the same
#define ISFS_OPEN_READ		IPC_OPEN_READ
#define ISFS_OPEN_WRITE		IPC_OPEN_WRITE
#define ISFS_OPEN_RW		IPC_OPEN_RW
#define ISFS_MAXPATH_LEN	IPC_MAXPATH_LEN

#define IPC_OK				  0
#define IPC_EINVAL			 -4
#define IPC_ENOHEAP			 -5
#define IPC_ENOENT			 -6
#define IPC_EQUEUEFULL		 -8
#define IPC_ENOMEM			-22

#define IOS_OPEN           0x01
#define IOS_CLOSE          0x02
#define IOS_READ           0x03
#define IOS_WRITE          0x04
#define IOS_SEEK           0x05
#define IOS_IOCTL          0x06
#define IOS_IOCTLV         0x07
#define IOS_CALLBACK       0x08

#ifdef __cplusplus
	extern "C" {
#endif

typedef struct _ioctlv
{
	void *data;
	u32 len;
} ioctlv;


typedef s32 (*ipccallback)(s32 result,void *usrdata);

typedef struct ipcmessage
{
	u32 command;		// 0
	u32 result;			// 4
	u32 fd;				// 8
	union
	{
		struct
		{
			const char* device;	// 12
			u32 mode;		// 16
			u32 uid;	// 20
			u16 gid;
		} open;

		struct
		{
			void* data;
			u32 length;
		} read;

		struct
		{
			const void* data;
			u32 length;
		} write;

		struct
		{
			s32 offset;
			s32 origin;
		} seek;

		struct
		{
			u32 command;

			const void* buffer_in;
			u32 length_in;
			void* buffer_io;
			u32 length_io;
		} ioctl;

		struct
		{
			u32 command;

			u32 num_in;
			u32 num_io;
			ioctlv* vector;
		} ioctlv;

		void *cb_data;
	};
} __attribute__((packed)) ipcmessage;

#ifdef __cplusplus
	}
#endif
