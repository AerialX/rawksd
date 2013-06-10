#pragma once

#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#include <sys/iosupport.h>

#include "gctypes.h"
#include "proxiios.h"
#include "ipc.h"
#include "mega.h"
#include "mem.h"
#include "timer.h"

#define IDLE_TICK		1000000 // 1s, minimum DebuggerHandler Idle() tick

namespace ProxiIOS { namespace Debugger {
	namespace Ioctl {
		enum Enum {
			// Connection stuff
			Connect			= 0x31,//IOCTL_Connect,
			Disconnect		= 0x32,//IOCTL_Disconnect,
			// Emit a log buffer
			SetFreezeLocation = 0x35,//IOCTL_SetFreezeLocation,
			GetState		= 0x36,//IOCTL_GetState,
			StartPolling	= 0x38,//IOCTL_StartPolling,
			StopPolling		= 0x39,//IOCTL_StopPolling,
			Log				= 0x61,//IOCTL_Log,
			QuickLog		= 0x63,//IOCTL_Log,
			Poll			= 0x69,//IOCTL_Poll,
			// Set RTC epoch
			Epoch			= 0x37,//IOCTL_Epoch,
			// Hacky context dumper
			Context         = 0x62//IOCTL_Context
		};
	}

	namespace Errors {
		enum Enum {
			Success			= 0,//ERROR_SUCCESS,
			Unrecognized	= -0x80,//ERROR_UNRECOGNIZED,
			NotConnected	= -0x81,//ERROR_NOTCONNECTED,
			AlreadyConnected= -0x82,//ERROR_ALREADYCONNECTED,

			OutOfMemory		= -0x41,//ERROR_OUTOFMEMORY,

			NotOpened		= -0x40//ERROR_NOTOPENED
		};
	}

	class DebuggerHandler;

	class Debugger;
	class DebuggerHandler
	{
		public:
			Debugger* Module;
			char MountPoint[0x40];

			DebuggerHandler(Debugger* fs) { Module = fs; memset(MountPoint, 0, 0x40); }
			virtual ~DebuggerHandler() { }

			virtual int Connect(const void* options, int length) { return -1; };
			virtual int Disconnect() { return -1; };

			virtual int IdleTick(bool poll) { return -1; };
			virtual int SetFreezeLocation(u32 loc) {return -1;}
			virtual int Log(const void* buffer, int length) { return 0; };
			virtual int QuickLog(const void* buffer, int length) { return 0; };
			virtual int Poll(u8* buffer, int length) { return -2; };
	};

	class Debugger : public ProxiIOS::Module
	{
	private:
		void PrintLog(const char* fmt, ...);
	public:
		DebuggerHandler* Connection;
		ostimer_t Idle_Timer;
		int State;

		Debugger();

		int HandleOpen(ipcmessage* message);
		int HandleClose(ipcmessage* message);
		int HandleIoctl(ipcmessage* message);
		int HandleIoctlv(ipcmessage* message);
		int HandleRead(ipcmessage* message);
		int HandleWrite(ipcmessage* message);
		int HandleSeek(ipcmessage* message);
		bool HandleOther(u32 message, int &result, bool &ack);
	};
} }
