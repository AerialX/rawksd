#include "dip.h"
#include "logging.h"

#include "print.h"

//#define LOGGING_WIFI
//#define LOGGING_FILE

//#define SYNC_FREQUENCY 20
#define SYNC_FREQUENCY 1

#ifdef LOGGING_WIFI
	#include <network.h>
	static int LogSocket = -1;
	#define DEBUG_PORT 1100
//	#define DEBUG_IPADDRESS "192.168.1.8"
	#define DEBUG_IPADDRESS "192.168.1.100" // Tempus
#endif
#ifdef LOGGING_FILE
	static int LogFile = -1;
#endif

void LogInit()
{
#ifdef LOGGING_WIFI
	int init;
	while ((init = net_init()) < 0)
		;

	LogSocket = net_socket(PF_INET, SOCK_STREAM, 0);
	struct sockaddr_in address;
	memset(&address, 0, sizeof(address));
	address.sin_family = PF_INET;
	address.sin_port = htons(DEBUG_PORT);
	int ret = inet_aton(DEBUG_IPADDRESS, &address.sin_addr);
	if (ret <= 0)
		return;
	if (net_connect(LogSocket, (struct sockaddr*)(const void *)&address, sizeof(address)) == -1)
		return;
	net_send(LogSocket, "y halo thar mr network\n", 23, 0);
#endif
#ifdef LOGGING_FILE
	LogFile = File_Open("/log.txt", O_CREAT | O_WRONLY | O_TRUNC);
	LogPrintf("Super sekrit log file commence!\n");
#endif
}

#ifdef LOGGING_FILE
static int synccounter = 0;
#endif

void LogPrintf(const char *fmt, ...)
{
#if defined(LOGGING_FILE) || defined(LOGGING_WIFI)
	static char str[1024] __attribute__ ((aligned (32)));

	va_list arg;
	va_start(arg, fmt);
	int len = _vsprintf(str, fmt, arg);
	va_end(arg);
#ifdef LOGGING_FILE
	if (LogFile >= 0) {
		File_Write(LogFile, (const u8*)str, len);
		synccounter++;
		if (synccounter % SYNC_FREQUENCY == 0)
			File_Sync(LogFile);
	} else
		exit(0);
#endif
#ifdef LOGGING_WIFI
	if (LogSocket != -1) {
		net_send(LogSocket, str, len, 0);
	}
#endif
#endif
}

void LogTimestamp()
{
	int heapfree = 0;
	//heapfree = HeapInfo();
	LogPrintf("%u (%X): ", *(u32*)0x0D800010, heapfree);
}

void LogDeinit()
{
#ifdef LOGGING_FILE
	if (LogFile > 0)
		File_Close(LogFile);
	LogFile = -1;
#endif
#ifdef LOGGING_WIFI
	if (LogSocket != -1) {
		net_close(LogSocket);
	}
#endif
}
#if 0
void LogMessage(ipcmessage* message)
{
	LogTimestamp();
	switch (message->command) {
		case IOS_OPEN:
			if (!strstr(message->open.device, "/dev/net/ip/top")) // It's fucking annoying
				LogPrintf("IOS_Open: \"%s\" - 0x%X - 0x%X\n", message->open.device, message->open.mode, message->open.resultfd);
			break;
		case IOS_CLOSE:
			LogPrintf("IOS_Close: 0x%X\n", message->fd);
			break;
		case IOS_READ:
			LogPrintf("IOS_Read: 0x%X\n", message->read.length);
			break;
		case IOS_WRITE:
			LogPrintf("IOS_Write: 0x%X\n", message->write.length);
			break;
		case IOS_SEEK:
			LogPrintf("IOS_Seek: 0x%X - 0x%X\n", message->seek.offset, message->seek.origin);
			break;
		case IOS_IOCTL: {
			LogPrintf("IOS_Ioctl: 0x%X\n", message->ioctl.command);
			isfs_ioctl *data = (isfs_ioctl*)message->ioctl.buffer_in;
			switch (message->ioctl.command) {
				case FSIOCTL_CREATEDIR:
					LogPrintf("\tCREATEDIR: %s\n", data->fsattr.filepath);
					break;
				case FSIOCTL_SETATTR:
					LogPrintf("\tSETATTR: %s\n", data->fsattr.filepath);
					break;
				case FSIOCTL_DELETE:
					LogPrintf("\tDELETE: %s\n", (char*)message->ioctl.buffer_in);
					break;
				case FSIOCTL_RENAME:
					LogPrintf("\tRENAME: %s -> %s\n", data->fsrename.filepathOld, data->fsrename.filepathNew);
				case FSIOCTL_CREATEFILE:
					LogPrintf("\tCREATEFILE: %s\n", data->fsattr.filepath);
			}
			break; }
		case IOS_IOCTLV:
			LogPrintf("IOS_Ioctlv: 0x%X\n", message->ioctlv.command);
			break;
	}
}
#endif
