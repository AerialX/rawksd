#pragma once

#include <string.h>

#include "syscalls.h"
#include "mem.h"
#include "ipc.h"

namespace ProxiIOS {
	namespace Errors {
		enum Enum {
			Success = 0,
			OpenFailure = IPC_ENOENT,
			OpenProxyFailure = -5
		};
	}
	namespace Ios {
		enum Enum {
			Open     = IOS_OPEN,
			Close    = IOS_CLOSE,
			Read     = IOS_READ,
			Write    = IOS_WRITE,
			Seek     = IOS_SEEK,
			Ioctl    = IOS_IOCTL,
			Ioctlv   = IOS_IOCTLV,
			Callback = IOS_CALLBACK
		};
	}

	namespace ISFS {
		enum Enum {
			Format = 0x01,
			GetStats,
			CreateDir,
			ReadDir,
			SetAttrib,
			GetAttrib,
			Delete,
			Rename,
			CreateFile,
			SetFileVerCtrl,
			GetFileStats,
			GetUsage,
			Shutdown
		};

		struct Stats {
			s32 Length;
			s32 Pos;
		};

		struct FSattr {
			u32 owner_id;
			u16 group_id;
			char path[ISFS_MAXPATH_LEN];
			u8 ownerperm;
			u8 groupperm;
			u8 otherperm;
			u8 attributes;
			u8 pad[2];
		};
	}

	class Module
	{
	protected:
		u32 queue[8];
		char Device[0x20];
		int Fd;
		osqueue_t queuehandle;

	public:
		Module(const char* device);

		int Loop();

		virtual int HandleOpen(ipcmessage* message);

		virtual int HandleClose(ipcmessage* message)
		{
			Fd = -1;
			return 0;
		}
		virtual int HandleIoctl(ipcmessage* message)
		{
			return -1;
		}
		virtual int HandleIoctlv(ipcmessage* message)
		{
			return -1;
		}
		virtual int HandleRead(ipcmessage* message)
		{
			return -1;
		}
		virtual int HandleWrite(ipcmessage* message)
		{
			return -1;
		}
		virtual int HandleSeek(ipcmessage* message)
		{
			return -1;
		}
		virtual int HandleCallback(ipcmessage* message) // eat unknown callback messages
		{
			return 0;
		}
		virtual bool HandleOther(u32 message, int &result, bool &ack) // for timers and other static messages
		{
			if (message==0) // do not dereference null pointer
			{
				ack = false;
				return true;
			}
			return false;
		}

	};

	class ProxyModule : public Module
	{
	protected:
		char ProxyDevice[0x20];
		int ProxyHandle;
	public:
		ProxyModule(const char* device, const char* proxydevice) : Module(device)
		{
			strncpy(ProxyDevice, proxydevice, 0x20);
			ProxyDevice[0x20 - 1] = '\0';
			ProxyHandle = -1;
		}

		virtual int ForwardIoctl(ipcmessage* message);
		virtual int ForwardIoctlv(ipcmessage* message);
		virtual int ForwardRead(ipcmessage* message);
		virtual int ForwardWrite(ipcmessage* message);
		virtual int ForwardSeek(ipcmessage* message);
		virtual int ForwardOpen(ipcmessage* message);

		virtual int HandleOpen(ipcmessage* message);
		virtual int HandleClose(ipcmessage* message);

		virtual int HandleIoctl(ipcmessage* message)
		{
			return ForwardIoctl(message);
		}
		virtual int HandleIoctlv(ipcmessage* message)
		{
			return ForwardIoctlv(message);
		}
		virtual int HandleRead(ipcmessage* message)
		{
			return ForwardRead(message);
		}
		virtual int HandleWrite(ipcmessage* message)
		{
			return ForwardWrite(message);
		}
		virtual int HandleSeek(ipcmessage* message)
		{
			return ForwardSeek(message);
		}
	};
}
