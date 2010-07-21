#include "debugger.h"

#include "rtc.h"

#include "mega_riifs.h"

#include "print.h"

#define DEBUG_IOCTL_FD 0x456
#define IDLE_MSG 0x500
#define RTC_MSG  0x501

namespace ProxiIOS { namespace Debugger {
	Debugger::Debugger() : Module(MEGA_MODULE_NAME)
	{
		memset(Connection, null, sizeof(DebuggerHandler));

		Timer_Init();

		Idle_Timer = os_create_timer(IDLE_TICK, 0, queuehandle, IDLE_MSG);

		State = -1;
	}

	int Debugger::HandleIoctl(ipcmessage* message)
	{
		u32 *buffer_in = (u32*)message->ioctl.buffer_in;
		if (message->ioctl.length_in)
			os_sync_before_read(buffer_in, message->ioctl.length_in);

		switch (message->ioctl.command) {
			case Ioctl::Epoch: {
				time_t epoch;
				memcpy(&epoch, message->ioctl.buffer_in, sizeof(time_t));
				RTC_Init(epoch);
				// update (approximately) every 10 minutes
				os_create_timer(0, 600000000, queuehandle, RTC_MSG);
				return Errors::Success; }
			case Ioctl::Connect: {
				if(State >= 0)
					return Errors::AlreadyConnected;
				DebuggerHandler* system = null;
				system = new DebugHandler(this);

				if (!system)
					return Errors::Unrecognized;

				os_sync_before_read(message->ioctl.buffer_io, message->ioctl.length_io);
				int ret = system->Connect(message->ioctl.buffer_io, message->ioctl.length_io);
				if (ret < 0) {
					delete system;
					return ret;
				}

				Connection = system;
				State = 0;

				return State; }
			case Ioctl::Disconnect: {
				DebuggerHandler* system = Connection;
				if (!system)
					return Errors::NotConnected;

				int ret = system->Disconnect();

				if (ret >= 0) {
					delete system;
					Connection = null;
					State = -1;
				}
				return ret; }
			case Ioctl::SetFreezeLocation: {
				u32 loc = *(u32*)buffer_in;
				if (!Connection)
					return Errors::NotConnected;
				return Connection->SetFreezeLocation(loc); }
			case Ioctl::GetState: {
				return State; }
			case Ioctl::StartPolling: {
				if(State < 0)
					return Errors::NotConnected;
				State = 1;
				return 0; }
			case Ioctl::StopPolling: {
				if(State < 0)
					return Errors::NotConnected;
				State = 0;
				return 0; }
			case Ioctl::Poll: {
				if (!Connection)
					return Errors::NotConnected;
				int ret = Connection->Poll((u8*)message->ioctl.buffer_io, message->ioctl.length_io);
				os_sync_after_write(message->ioctl.buffer_io, message->ioctl.length_io);
				return ret; }
			case Ioctl::Log: {
				if (State >= 0 && Connection && message->ioctl.length_in)
					return Connection->Log(message->ioctl.buffer_in, message->ioctl.length_in);
				return Errors::Success; }
			case Ioctl::QuickLog: {
				if (State >= 0 && Connection && message->ioctl.length_in)
					return Connection->QuickLog(message->ioctl.buffer_in, message->ioctl.length_in);
				return Errors::Success; }
#if 1 // testing only
			case Ioctl::Context: {
				static const char *exception_name[15] = {
						"System Reset", "Machine Check", "DSI", "ISI",
						"Interrupt", "Alignment", "Program", "Floating Point",
						"Decrementer", "System Call", "Trace", "Performance",
						"IABR", "Reserved", "Thermal"};
				int i;
				if (buffer_in[36]<15)
					PrintLog("%s Exception!\n", exception_name[buffer_in[36]]);
				else
					PrintLog("Unrecoverable Exception!\n");
				PrintLog("-------------- Exception 0x%08x Context 0x%08x ---------------\n", buffer_in[36], (u32)buffer_in|0x80000000);
				for (i=0; i < 16; i++)
					PrintLog("r%2d  = 0x%08x (%14d)  r%2d  = 0x%08x (%14d)\n", i, buffer_in[i], buffer_in[i], i+16, buffer_in[i+16], buffer_in[i+16]);
				PrintLog("LR   = 0x%08x                   CR   = 0x%08x\n", buffer_in[33], buffer_in[32]);
				PrintLog("SRR0 = 0x%08x                   SRR1 = 0x%08x\n", buffer_in[102], buffer_in[103]);
				PrintLog("DSISR= 0x%08x                   DAR  = 0x%08x\n", buffer_in[34], buffer_in[35]);
				PrintLog("Address:      Back Chain    LR Save\n");
				u32* stack = (u32*)(buffer_in[1]&0x3FFFFFFF); // r1 = sp
				for (i=0; i<16 && stack && ((s32)((u32)stack|0x80000000)+0x10000)!=-1; i++, stack = (u32*)(stack[0]&0x3FFFFFFF)) {
					os_sync_before_read(stack, 8);
					PrintLog("0x%08x:   0x%08x    0x%08x\n", (u32)stack|0x80000000, stack[0], stack[1]);
				}
				return Errors::Success; }
#endif
			default:
				return -1;
		}
	}

	void Debugger::PrintLog(const char* fmt, ...)
	{
		static char buf[200] ATTRIBUTE_ALIGN(32);
		if (State<0 || !Connection)
			return;

		va_list arg;
		va_start(arg, fmt);
		int len = _vsprintf(buf, fmt, arg);
		va_end(arg);

		Connection->Log(buf, len);
	}

	int Debugger::HandleIoctlv(ipcmessage* message)
	{
		for (u32 i = 0; i < message->ioctlv.num_in; i++)
			os_sync_before_read(message->ioctlv.vector[i].data, message->ioctlv.vector[i].len);
		switch (message->ioctlv.command) {
			default:
				return -1;
		}
	}

	int Debugger::HandleOpen(ipcmessage* message)
	{
		if (!strcmp(message->open.device, MEGA_MODULE_NAME))
			return DEBUG_IOCTL_FD;

		return Errors::NotOpened;
	}

	int Debugger::HandleClose(ipcmessage* message)
	{
		if (message->fd == DEBUG_IOCTL_FD)
			return 1;

		return Errors::NotOpened;
	}

	int Debugger::HandleSeek(ipcmessage* message)
	{
		return -1;
	}

	int Debugger::HandleRead(ipcmessage* message)
	{
		return -1;
	}

	int Debugger::HandleWrite(ipcmessage* message)
	{
		return -1;
	}

	bool Debugger::HandleOther(u32 message, int &result, bool &ack)
	{
		if (message == IDLE_MSG)
		{
			os_stop_timer(Idle_Timer);
			bool poll = false;
			if(State == 1)
				poll = true;
			if (Connection)
				Connection->IdleTick(poll);
			os_restart_timer(Idle_Timer, IDLE_TICK, 0);
			ack = false;
			return true;
		} else if (message == RTC_MSG) {
			RTC_Update();
			ack = false;
			return true;
		}

		return false;
	}
} }
