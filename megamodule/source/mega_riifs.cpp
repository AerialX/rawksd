#include "mega_riifs.h"

#include <network.h>
#include <syscalls.h>
#include "print.h"

#define CONNECT_SLEEP_INTERVAL 100000 // 0.1 seconds

//#define RECEIVE_DEBUG

namespace ProxiIOS { namespace Debugger {
	int DebugHandler::Connect(const void* options, int length)
	{
		int ret;
		u32 sock_opt;
		while ((ret = net_init()) == -EAGAIN)
			usleep(10000);
		if (ret < 0)
			return Errors::NotConnected;
		memcpy(&Port, options, sizeof(int));
		const char* hoststr = (const char*)options + sizeof(int);
		strncpy(Host, hoststr, 0x30);
		struct sockaddr_in address;
		memset(&address, 0, sizeof(address));
		address.sin_family = PF_INET;
		if (!Host[0]) {
			// broadcast a ping to try and find a server
			int found = 0;
			sock_opt = 1;
			int locate_socket = net_socket(AF_INET, SOCK_DGRAM, 0);
			if (locate_socket<0)
				return Errors::NotConnected;
			// set to non-blocking
			ret = net_ioctl(locate_socket, FIONBIO, &sock_opt);
			if (ret < 0) {
				net_close(locate_socket);
				return Errors::NotConnected;
			}
			ret = RII_OPTION_PING;
			address.sin_port = htons(Port ? Port : 1137);
			address.sin_addr.s_addr = htonl(INADDR_BROADCAST);
			ret = net_sendto(locate_socket, &ret, sizeof(ret), 0, (struct sockaddr*)&address, 8);
			if (ret>=4) {
				// wait for up to half a second (5 * CONNECT_SLEEP_INTERVAL)
				for (int i=0; i < 5; i++) {
					sock_opt = 8;
					if (net_recvfrom(locate_socket, &ret, sizeof(ret), 0, (struct sockaddr*)&address, &sock_opt)==4 && sock_opt>=8) {
						Port = ntohs(ret);
						strcpy(Host, inet_ntoa(address.sin_addr));
						found = 1;
						break;
					}
					usleep(CONNECT_SLEEP_INTERVAL);
				}
			}
			net_close(locate_socket);
			if (!found)
				return Errors::NotConnected;
		}
		else if (!inet_aton(hoststr, &address.sin_addr)) {
			hostent* host = net_getnbhostbyname_async(hoststr, CONNECT_SLEEP_INTERVAL * 5); // 0.5 second timeout
			if (host==NULL)
				host = net_gethostbyname_async(hoststr, CONNECT_SLEEP_INTERVAL * 20); // 2 second timeout
			if (!host || host->h_length != sizeof(address.sin_addr) ||
				host->h_addrtype != PF_INET || host->h_addr_list==NULL ||
				host->h_addr_list[0]==NULL)
				return Errors::NotConnected;
			memcpy(&address.sin_addr, host->h_addr_list[0], host->h_length);
		}
		address.sin_port = htons(Port);
		Socket = net_socket(PF_INET, SOCK_STREAM, 0);
		// Non-blocking
		sock_opt = 1;
		if (net_ioctl(Socket, FIONBIO, &sock_opt) < 0) {
			net_close(Socket);
			return Errors::NotConnected;
		}
		for (u32 i = 0; i < 50; i++) { // 5 second timeout
			ret = net_connect(Socket, (struct sockaddr*)&address, sizeof(address));
			if (ret == -EINPROGRESS || ret == -EALREADY)
				usleep(CONNECT_SLEEP_INTERVAL);
			else
				break;
		}
		if (ret < 0 && ret != -EISCONN) {
			net_close(Socket);
			return Errors::NotConnected;
		}
		// Back to blocking
		sock_opt = 0;
		if (net_ioctl(Socket, FIONBIO, &sock_opt) < 0) {
			Disconnect();
			return Errors::NotConnected;
		}
		if (!SendCommand(RII_HANDSHAKE, (const u8*)RII_VERSION, strlen(RII_VERSION))) {
			Disconnect();
			return Errors::NotConnected;
		}
		ServerVersion = ReceiveCommand(RII_HANDSHAKE);
		if (ServerVersion < RII_VERSION_RET) {
			Disconnect();
			return Errors::NotConnected;
		}
		Host[0x30] = '\0';
		strcpy(MountPoint, "/mnt/net/");
		static int connection = 0;
		int id = ++connection;
		// find first non-zero digit (assume 100000>id>0)
		for (ret = 10000; (id / ret) == 0; ret /= 10)
			;
		while (id) {
			char digit[2] = {0};
			int value = id / ret;
			digit[0] = '0' + value;
			strcat(MountPoint, digit);
			id -= value * ret;
			ret /= 10;
		}
		return Errors::Success;
	}

	bool DebugHandler::SendCommand(int type, const void* data, int size)
	{
#ifdef RIIFS_LOCAL_OPTIONS
		int value;
		if (size == 4 && type>0) {
			memcpy(&value, data, 4);
			if (OptionsInit[type - 1] && Options[type - 1] == value)
				return true;
		}
#endif
		bool fail = false;
		static u32 message[0x03] ATTRIBUTE_ALIGN(32);
		message[0] = RII_SEND;
		message[1] = type;
		message[2] = size;
		fail |= net_send(Socket, message, 12, 0) != 12;
		if (!fail && size && data) {
			fail |= net_send(Socket, data, size, 0) != size;
		} else if (!fail && size) {
			STACK_ALIGN(int, zero, 1, 32);
			*zero = *(u32*)0;
			fail |= net_send(Socket, zero, 4, 0) != 4;
			fail |= net_send(Socket, (u8*)data+4, size-4, 0) != size-4;
		}
#ifdef RIIFS_LOCAL_OPTIONS
		if (size == 4 && type>0 && !fail) {
			Options[type - 1] = value;
			OptionsInit[type - 1] = 1;
		}
#endif
		IdleCount = 0;
		return !fail;
	}

	static int netrecv(int socket, u8* data, int size, int opts)
	{
#ifdef RECEIVE_DEBUG
			STACK_ALIGN(u32, message, 2, 32);
			message[0] = RII_RECEIVE;
			message[1] = size+0x100;
			net_send(socket, message, 0x08, 0);
#endif
		int read = 0;
		while (read < size) {
#ifdef RECEIVE_DEBUG
			message[0] = RII_RECEIVE;
			message[1] = read+0x40;
			net_send(socket, message, 0x08, 0);
#endif
			// don't attempt to read more than 0x2000 in case a temp buffer is needed
			int ret = net_recv(socket, data + read, MIN(0x2000, size - read), opts);
			if (ret < 0)
				return ret;
			if (ret == 0)
				break;
			read += ret;
		}
		return read;
	}

	int DebugHandler::ReceiveCommand(int type, void* data, int size)
	{
		bool fail = false;
		STACK_ALIGN(u32, message, 2, 32);
		STACK_ALIGN(int, ret, 1, 32);
		message[0] = RII_RECEIVE;
		message[1] = type;
		fail |= net_send(Socket, message, 0x08, 0) != 8;
		*ret = 0;
		if (!fail && size) {
			if (data) {
#ifdef RECEIVE_DEBUG
				message[0] = RII_RECEIVE;
				message[1] = 30;
				net_send(Socket, message, 0x08, 0);
#endif
				fail |= netrecv(Socket, (u8*)data, size, 0) != size;
#ifdef RECEIVE_DEBUG
				message[0] = RII_RECEIVE;
				message[1] = 31;
				net_send(Socket, message, 0x08, 0);
#endif
			} else {
				void* temp = Memalign(32, size);
				if (temp) {
					fail |= netrecv(Socket, (u8*)temp, size, 0) != size;
					Dealloc(temp);
				} else
					fail = 1;
			}
		}
#ifdef RECEIVE_DEBUG
		message[0] = RII_RECEIVE;
		message[1] = 33;
		net_send(Socket, message, 0x08, 0);
#endif
		if (!fail) {
#ifdef RECEIVE_DEBUG
			message[0] = RII_RECEIVE;
			message[1] = 34;
			net_send(Socket, message, 0x08, 0);
#endif
			fail |= netrecv(Socket, (u8*)ret, 4, 0) != 4;
#ifdef RECEIVE_DEBUG
			message[0] = RII_RECEIVE;
			message[1] = 36;
			net_send(Socket, message, 0x08, 0);
#endif
		}
		IdleCount = 0;
		if (fail)
			return -1;
		return *ret;
	}

	int DebugHandler::Disconnect()
	{
		if (Socket >= 0) {
			ReceiveCommand(RII_GOODBYE);
			net_close(Socket);
			IdleCount = -1;
			Socket = -1;
		}
		Dealloc(LogBuffer);
		LogBuffer = NULL;
		LogSize = 0;
		return 0;
	}

	int DebugHandler::SetFreezeLocation(u32 loc)
	{
		Print("SetFreezeLocation(0x%08x);\n", loc);
		if((loc < 0x80001800) || (loc > 0x80003f00))
			return -1;
		if(loc % 4)
			return -2;
		loc &= 0x7fffffff;
		freeze_location = loc;
		return 0;
	}

	int DebugHandler::Poll(u8* buffer, int length)
	{
		SendCommand(RII_OPTION_LENGTH, &length, 4);
		// Looking for 16byte buffer
		int ret = ReceiveCommand(RII_POLL, buffer, length);
		if(ret > 0)
		{
			// MAYBE HANDLE SPECIAL COMMANDS HERE
		}
		return ret;
	}

	int DebugHandler::IdleTick(bool poll)
	{
		if (LogBuffer && LogSize>0)
		{
			SendCommand(RII_OPTION_DATA, LogBuffer, LogSize);
			ReceiveCommand(RII_LOG);
			// prevent the buffer from staying too big
			if (LogSize > 2048)
			{
				Dealloc(LogBuffer);
				LogBuffer = NULL;
			}
			LogSize = 0;
		}
		if(poll) {
			u8 buffer[0x20] ATTRIBUTE_ALIGN(32);
			int blah = Poll(buffer, 0x20);
			blah = HandleArmPoll(buffer);
		}

		if (ServerVersion < 0x03 || IdleCount < 0)
			return -1;
		if (IdleCount++ > (RII_IDLE_TIME/IDLE_TICK))
			return SendCommand(RII_OPTION_PING);
		return 0;
	}

	int DebugHandler::Log(const void* buffer, int length)
	{
		if (ServerVersion >= 0x04)
		{
			if (LogSize > 0x2000) // At this point, just flush the damn thing
				IdleTick(false);
			u8 *NewBuffer = (u8*)Realloc(LogBuffer, length+LogSize, LogSize);
			if (NewBuffer) { // if Realloc failed LogBuffer should be left untouched
				LogBuffer = NewBuffer;
				memcpy(LogBuffer+LogSize, buffer, length);
				LogSize += length;
			}
		}
		return length;
	}

	int DebugHandler::QuickLog(const void* buffer, int length)
	{
		if (ServerVersion >= 0x04)
		{
			SendCommand(RII_OPTION_DATA, buffer, length);
			ReceiveCommand(RII_LOG);
		}
		return length;
	}

	void DebugHandler::Print(const char* fmt, ...)
	{
		static char buf[200] ATTRIBUTE_ALIGN(32);

		va_list arg;
		va_start(arg, fmt);
		int len = _vsprintf(buf, fmt, arg);
		va_end(arg);

		QuickLog(buf, len);
	}

	int DebugHandler::HandleArmPoll(u8* buffer)
	{
		int ret = 0;
		commands *cmd = (commands*)buffer;
		if(cmd->command == 0x80000001) {
			ret = Poke(cmd->data1, cmd->data2, 1);
		} else if(cmd->command == 0x80000002) {
			ret = Poke(cmd->data1, cmd->data2, 2);
		} else if(cmd->command == 0x80000003) {
			ret = Poke(cmd->data1, cmd->data2, 4);
		} else if(cmd->command == 0x80000004) {
			ret = Peek(cmd->data1);
		} else if(cmd->command == 0x80000006) {
			ret = Freeze();
		} else if(cmd->command == 0x80000007) {
			ret = UnFreeze();
		} else if(cmd->command == 0x80000009) {
			ret = Dump(cmd->data1, cmd->data2);
		} else if(cmd->command == 0x80000099) {
			Print("Module Version: %s\n", RII_VERSION);
			ret = 0;
		} else {
			ret = -0x66;
		}
		return ret;
	}

	int DebugHandler::Dump(u32 address, u32 length)
	{
		if((address < 0x80000000) || (address > 0x933fffff))
			return -1;
		if(length % 0x20)
			return -2;
		address &= 0x7fffffff;

		Print("Dump(0x%08x, 0x%08x);\n",address,length);
		int fd = ReceiveCommand(RII_OPEN);
		u32 start = 0;
		if(!address) {
			STACK_ALIGN(int, data, 8, 32);
			*(data+0) = *(u32*)(address+0x00);
			*(data+1) = *(u32*)(address+0x04);
			*(data+2) = *(u32*)(address+0x08);
			*(data+3) = *(u32*)(address+0x0C);
			*(data+4) = *(u32*)(address+0x10);
			*(data+5) = *(u32*)(address+0x14);
			*(data+6) = *(u32*)(address+0x18);
			*(data+7) = *(u32*)(address+0x1C);
			SendCommand(RII_OPTION_FILE, &fd, 4);
			SendCommand(RII_OPTION_DATA, data, 0x20);
			if(!ReceiveCommand(RII_WRITE))
				return ReceiveCommand(RII_CLOSE);
			start += 0x20;
		}
#if 1
#define DLTA 0x500			// 0x400 is stable
		if(length > DLTA) {
			for(u32 ii=start; ii<length-DLTA; ii+=DLTA) {
				u32 addy = address+ii;
				SendCommand(RII_OPTION_FILE, &fd, 4);
				SendCommand(RII_OPTION_DATA,(u8*)addy,DLTA);
				if(!ReceiveCommand(RII_WRITE))
					return ReceiveCommand(RII_CLOSE);
				start += DLTA;
			}
		}
#endif
		for(u32 ii=start; ii<length; ii+=0x20) {
			u32 addy = address+ii;
			SendCommand(RII_OPTION_FILE, &fd, 4);
			SendCommand(RII_OPTION_DATA,(u8*)addy,0x20);
			if(!ReceiveCommand(RII_WRITE))
				return ReceiveCommand(RII_CLOSE);
		}
		SendCommand(RII_OPTION_FILE, &fd, 4);
		return ReceiveCommand(RII_CLOSE);
	}

	int DebugHandler::Peek(u32 address)
	{
		if((address < 0x80000000) || (address > 0x933fffff))
			return -1;
		address &= 0x7fffffff;
		u32 value = *(u32*)address;
		Print("Peek - 0x%08x : 0x%08x\n", address, value);
		return 0;
	}

	int DebugHandler::Poke(u32 address, u32 value, u32 type)
	{
		if((address > 0x81ffffff) && (address < 0x90000000))
			return -1;
		if((address < 0x80000000) || (address > 0x933fffff))
			return -1;
		address &= 0x7fffffff;
		if(type == 1) {
			*(u8*)address = (u8)value;
		} else if(type == 2) {
			*(u16*)address = (u16)value;
		} else if(type == 4) {
			*(u32*)address = value;
		} else {
			return -2;
		}
		Print("Poke(0x%08x, 0x%08x, 0x%08x);\n",
				address, value, type);
		return 0;
	}

	int DebugHandler::Freeze(void)
	{
		if(!freeze_location)
			return -1;
		*(u32*)(freeze_location) = 1;
		Print("Frozen\n");
		return 0;
	}

	int DebugHandler::UnFreeze(void)
	{
		if(!freeze_location)
			return -1;
		*(u32*)(freeze_location) = 0;
		Print("UnFrozen\n");
		return 0;
	}

} }
