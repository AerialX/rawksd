#pragma once

#include <syscalls.h>
#include <stdlib.h>

#include <string.h>
#include "mem.h"

namespace ProxiIOS {
	namespace Errors {
		enum Enum {
			OpenFailure = -6,
			OpenProxyFailure = -5
		};
	}
	namespace Ios {
		enum Enum {
			Open = 0x01,
			Close = 0x02,
			Read = 0x03,
			Write = 0x04,
			Seek = 0x05,
			Ioctl = 0x06,
			Ioctlv = 0x07,
			Callback = 0x08
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
		Module(const char* device)
		{
			strncpy(Device, device, 0x20);
			Device[0x20 - 1] = '\0';
			Fd = -1;
			queuehandle = -1;
		}

		virtual int Loop();

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
