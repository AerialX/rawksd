#pragma once

#include "debugger.h"

// Actions
#define RII_SEND 0x01
#define RII_RECEIVE 0x02

// Commands
#define RII_HANDSHAKE			0x00
#define RII_GOODBYE				0x01
#define RII_LOG					0x02
#define RII_POLL				0x03
#define RII_DUMP				0x04
#define RII_WRITE				0x05
#define RII_OPEN				0x06
#define RII_CLOSE				0x07

// Options
#define RII_OPTION_FILE					0x01
#define RII_OPTION_PATH					0x02
#define RII_OPTION_MODE					0x03
#define RII_OPTION_LENGTH				0x04
#define RII_OPTION_DATA					0x05
#define RII_OPTION_SEEK_WHERE			0x06
#define RII_OPTION_SEEK_WHENCE			0x07
#define RII_OPTION_RENAME_SOURCE		0x08
#define RII_OPTION_RENAME_DESTINATION	0x09
#define RII_OPTION_PING					0x10

#define RII_IDLE_TIME 30*1000*1000

#define RIIFS_LOCAL_OPTIONS

#define RII_VERSION 		"1.03"

#define RII_VERSION_RET		0x03

namespace ProxiIOS { namespace Debugger {
	class DebugHandler : public DebuggerHandler
	{
		protected:
			char Host[0x40];
			int Port;
			int Socket;
			int ServerVersion;
			int IdleCount;
			u8 *LogBuffer;
			int LogSize;
			u32 freeze_location;

#ifdef RIIFS_LOCAL_OPTIONS
			int Options[RII_OPTION_RENAME_DESTINATION];
			u8 OptionsInit[RII_OPTION_RENAME_DESTINATION];
#endif

			bool SendCommand(int type, const void* data=NULL, int size=0);
			int ReceiveCommand(int type, void* data=NULL, int size=0);

			int HandleArmPoll(u8* buffer);
			int Peek(u32 address);
			int Poke(u32 address, u32 value, u32 type);
			int Freeze(void);
			int UnFreeze(void);
			int Dump(u32 address, u32 range);
			void Print(const char* fmt, ...);

		public:
			DebugHandler(Debugger* fs) : DebuggerHandler(fs) {
#ifdef RIIFS_LOCAL_OPTIONS
				memset(Options, 0, sizeof(Options));
				memset(OptionsInit, 0, sizeof(OptionsInit));
#endif
				Socket = -1;
				IdleCount = -1;
				LogBuffer = NULL;
				LogSize = 0;
				freeze_location = 0;
			}

			~DebugHandler() {
				Disconnect();
			}

			int Connect(const void* options, int length);
			int Disconnect();

			int IdleTick(bool poll);
			int SetFreezeLocation(u32 loc);
			int Log(const void* buffer, int length);
			int QuickLog(const void* buffer, int length);
			int Poll(u8* buffer, int length);

	};
} }
