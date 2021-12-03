#include <mega.h>

#include <string.h>
#include <unistd.h>
#include <sys/param.h>

#ifdef GEKKO
#include <stdlib.h>
#include <gccore.h>
#define os_open IOS_Open
#define os_close IOS_Close
#define os_ioctl IOS_Ioctl
#define os_ioctlv IOS_Ioctlv
// libogc will take care of syncing
#define os_sync_after_write(a,b)
#define os_sync_before_read(a,b)
#define Alloc malloc
#define Realloc(a, b, c) realloc(a, b)
#define Dealloc free
#else
#include <syscalls.h>
#include <mem.h>
#define MEM_VIRTUAL_TO_PHYSICAL(a) (a)
#endif

static u32 ioctlbuffer[0x20];
static u32 ioctlbufferout[0x0C] __attribute__((aligned(32)));
static u32 ioctlbufferout_size = 0xC;
//static ioctlv ioctlvbuffer[0x04];
static int mega_fd = -1;

int Mega_Init()
{
	if (mega_fd < 0) {
		mega_fd = os_open(MEGA_MODULE_NAME, 0);
#ifdef GEKKO
		if (mega_fd >= 0) {
			time_t epoch = time(NULL);
			memcpy(ioctlbuffer, &epoch, sizeof(time_t));
			//os_ioctl(mega_fd, IOCTL_Epoch, ioctlbuffer, sizeof(time_t), NULL, 0);
			os_ioctl(mega_fd, 0x37, ioctlbuffer, sizeof(time_t), NULL, 0);
		}
#endif
	}

	return mega_fd;
}

int Mega_Deinit()
{
	if (mega_fd >= 0)
		os_close(mega_fd);
	mega_fd = -1;

	return 0;
}

int Mega_Connect(const void* options, int optionslen)
{
	if (mega_fd < 0)
		return -0x40; //return ERROR_NOTOPENED;

	ioctlbuffer[0] = 0;
	os_sync_after_write(ioctlbuffer, 0x04);
	os_sync_after_write((void*)options, optionslen);
	//return os_ioctl(mega_fd, IOCTL_Connect, ioctlbuffer, sizeof(ioctlbuffer), (void*)options, optionslen);
	return os_ioctl(mega_fd, 0x31, ioctlbuffer, sizeof(ioctlbuffer), (void*)options, optionslen);
}


int Mega_Disconnect(int fs)
{
	ioctlbuffer[0] = fs;
	os_sync_after_write(ioctlbuffer, 0x04);
	//return os_ioctl(mega_fd, IOCTL_Disconnect, ioctlbuffer, sizeof(ioctlbuffer), NULL, 0);
	return os_ioctl(mega_fd, 0x32, ioctlbuffer, sizeof(ioctlbuffer), NULL, 0);
}

int Mega_GetState()
{
	if (mega_fd >= 0) {
		//return os_ioctl(mega_fd, IOCTL_GetState, NULL, 0, NULL, 0);
		return os_ioctl(mega_fd, 0x36, NULL, 0, NULL, 0);
	}else{
		return -1;
	}
}

int Mega_StartPolling()
{
	if (mega_fd >= 0) {
		//return os_ioctl(mega_fd, IOCTL_StartPolling, NULL, 0, NULL, 0);
		return os_ioctl(mega_fd, 0x38, NULL, 0, NULL, 0);
	}else{
		return -1;
	}
}

int Mega_StopPolling()
{
	if (mega_fd >= 0) {
		//return os_ioctl(mega_fd, IOCTL_StopPolling, NULL, 0, NULL, 0);
		return os_ioctl(mega_fd, 0x39, NULL, 0, NULL, 0);
	}else{
		return -1;
	}
}

int Mega_SetFreezeLocation(u32 location)
{
	ioctlbuffer[0] = location;
	os_sync_after_write(ioctlbuffer, 0x04);
	if (mega_fd >= 0) {
		//return os_ioctl(mega_fd, IOCTL_SetFreezeLocation, NULL, 0, NULL, 0);
		return os_ioctl(mega_fd, 0x35, ioctlbuffer, sizeof(ioctlbuffer), NULL, 0);
	}else{
		return -1;
	}
}

int Mega_Log(const void* buffer, int length)
{
	if (mega_fd >= 0 && length>0)
	{
		os_sync_after_write(buffer, length);
		//return os_ioctl(mega_fd, IOCTL_Log, (void*)buffer, length, NULL, 0);
		return os_ioctl(mega_fd, 0x61, (void*)buffer, length, NULL, 0);
	}
	return 0;
}

int Mega_Poll(void)
{
	int ret = os_ioctl(mega_fd, 0x69, ioctlbuffer, sizeof(ioctlbuffer), ioctlbufferout, ioctlbufferout_size);
	os_sync_before_read(ioctlbufferout, ioctlbufferout_size);
	return ret;
}

int Mega_Debugger_Connect(const char* host, int port)
{
	char data[MAXPATHLEN];

	memcpy(data, &port, sizeof(int));
	strcpy(data + sizeof(int), host);
	return Mega_Connect(data, sizeof(int) + strlen(host) + 1);
}

