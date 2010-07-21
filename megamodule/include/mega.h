#pragma once

#include <sys/stat.h>
#include <gctypes.h>
#include <fcntl.h>
#include <stdio.h>

#ifdef __cplusplus
	extern "C" {
#endif

#define MEGA_MODULE_NAME "mega"
#define MEGA_MODULE_NAME_LENGTH 4

#if 0
typedef enum {
	IOCTL_Connect =			0x31,
	IOCTL_Disconnect =		0x32,
	IOCTL_GetState =		0x36,
	IOCTL_Epoch =			0x37,
	IOCTL_StartPolling =	0x38,
	IOCTL_StopPolling =		0x39,
	IOCTL_Log =				0x61,
	IOCTL_Context =			0x62,
	IOCTL_Poll =			0x69
} mega_ioctl;

typedef enum {
	ERROR_SUCCESS =				0,
	ERROR_NOTOPENED =			-0x40,
	ERROR_OUTOFMEMORY =			-0x41,
	ERROR_UNRECOGNIZED =		-0x80,
	ERROR_NOTCONNECTED =		-0x81,
	ERROR_ALREADYCONNECTED =	-0x82,
} mega_error;
#endif

typedef struct {
	u32 command;
	u32 data1;
	u32 data2;
	u32 data3;
} commands;

int Mega_Init();
int Mega_Deinit();

int Mega_Connect(const void* options, int length);
int Mega_Disconnect(int fs);
int Mega_GetState(void);
int Mega_StartPolling(void);
int Mega_StopPolling(void);

int Mega_SetFreezeLocation(u32 location);
int Mega_Log(const void* buffer, int length);
int Mega_Poll(void);

int Mega_Debugger_Connect(const char* host, int port);

#ifdef __cplusplus
	}
#endif
