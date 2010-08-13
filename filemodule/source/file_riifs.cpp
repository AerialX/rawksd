#include "file_riifs.h"

#include <network.h>
#include <syscalls.h>

#define CONNECT_SLEEP_INTERVAL 100000 // 0.1 seconds

#ifdef RIIFS_LOCAL_SEEKING
#define DIRTY_SEEK(info) { \
	if (info->SeekDirty) { \
		RiiSeek(info, info->Position, SEEK_SET); \
		info->SeekDirty = false; \
	} \
}
#else
#define DIRTY_SEEK(info)
#endif

namespace ProxiIOS { namespace Filesystem {
	int RiiHandler::Mount(const void* options, int length)
	{
		int ret;
		u32 sock_opt;
		while ((ret = net_init()) == -EAGAIN)
			usleep(10000);
		if (ret < 0)
			return Errors::DiskNotMounted;

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
				return Errors::DiskNotMounted;

			// set to non-blocking
			ret = net_ioctl(locate_socket, FIONBIO, &sock_opt);
			if (ret < 0) {
				net_close(locate_socket);
				return Errors::DiskNotMounted;
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
				return Errors::DiskNotMounted;
		}
		else if (!inet_aton(hoststr, &address.sin_addr)) {
			hostent* host = net_getnbhostbyname_async(hoststr, CONNECT_SLEEP_INTERVAL * 5); // 0.5 second timeout
			if (host==NULL)
				host = net_gethostbyname_async(hoststr, CONNECT_SLEEP_INTERVAL * 20); // 2 second timeout
			if (!host || host->h_length != sizeof(address.sin_addr) ||
				host->h_addrtype != PF_INET || host->h_addr_list==NULL ||
				host->h_addr_list[0]==NULL)
				return Errors::DiskNotMounted;
			memcpy(&address.sin_addr, host->h_addr_list[0], host->h_length);
		}
		address.sin_port = htons(Port);

		Socket = net_socket(PF_INET, SOCK_STREAM, 0);
		// Non-blocking
		sock_opt = 1;
		if (net_ioctl(Socket, FIONBIO, &sock_opt) < 0) {
			net_close(Socket);
			return Errors::DiskNotMounted;
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
			return Errors::DiskNotMounted;
		}

		// Back to blocking
		sock_opt = 0;
		if (net_ioctl(Socket, FIONBIO, &sock_opt) < 0) {
			Unmount();
			return Errors::DiskNotMounted;
		}

		if (!SendCommand(RII_HANDSHAKE, (const u8*)RII_VERSION, strlen(RII_VERSION))) {
			Unmount();
			return Errors::DiskNotMounted;
		}

		ServerVersion = ReceiveCommand(RII_HANDSHAKE);
		if (ServerVersion < RII_VERSION_RET) {
			Unmount();
			return Errors::DiskNotMounted;
		}

		Host[0x30] = '\0';

		strcpy(MountPoint, "/mnt/net/");
		static int connection = 0;
		int id = ++connection;

		// find first non-zero digit (assume 100000>id>0)
		for (ret = 10000; (id / ret) == 0; ret /= 10)
			;

		while (id) {
			char digit[2] = { 0, 0 };
			int value = id / ret;
			digit[0] = '0' + value;
			strcat(MountPoint, digit);
			id -= value * ret;
			ret /= 10;
		}

		return Errors::Success;
	}

	int RiiHandler::CheckPhysical()
	{
		return (SendCommand(RII_OPTION_PING)) ? 1 : -1;
	}

	bool RiiHandler::SendCommand(int type, const void* data, int size)
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
		if (!fail && size && data)
			fail |= net_send(Socket, data, size, 0) != size;
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
		int read = 0;
		while (read < size) {
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

	int RiiHandler::ReceiveCommand(int type, void* data, int size)
	{
		bool fail = false;
		STACK_ALIGN(u32, message, 2, 32);
		STACK_ALIGN(int, ret, 1, 32);
		message[0] = RII_RECEIVE;
		message[1] = type;
		fail |= net_send(Socket, message, 0x08, 0) != 8;
		*ret = 0;
		if (!fail && size) {
			if (data)
				fail |= netrecv(Socket, (u8*)data, size, 0) != size;
			else {
				void* temp = Memalign(32, size);
				if (temp) {
					fail |= netrecv(Socket, (u8*)temp, size, 0) != size;
					Dealloc(temp);
				} else
					fail = 1;
			}
		}
		if (!fail)
			fail |= netrecv(Socket, (u8*)ret, 4, 0) != 4;

		IdleCount = 0;
		if (fail)
			return -1;

		return *ret;
	}

	int RiiHandler::Unmount()
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

	FileInfo* RiiHandler::Open(const char* path, int mode)
	{
		RiiFileInfo* x;
		SendCommand(RII_OPTION_PATH, path, strlen(path));
		SendCommand(RII_OPTION_MODE, &mode, 4);

		int ret = ReceiveCommand(RII_FILE_OPEN);
		if (ret < 0)
			return NULL;
		x = new RiiFileInfo(this, ret);
		return x;
	}

	int RiiHandler::Read(FileInfo* file, u8* buffer, int length)
	{
		RiiFileInfo* info = (RiiFileInfo*)file;

		DIRTY_SEEK(info);

		SendCommand(RII_OPTION_FILE, &info->File, 4);
		SendCommand(RII_OPTION_LENGTH, &length, 4);
		int ret = ReceiveCommand(RII_FILE_READ, buffer, length);
#ifdef RIIFS_LOCAL_SEEKING
		if (ret > 0)
			info->Position += ret;
#endif
		return ret;
	}

	int RiiHandler::Write(FileInfo* file, const u8* buffer, int length)
	{
		RiiFileInfo* info = (RiiFileInfo*)file;

		DIRTY_SEEK(info);

		SendCommand(RII_OPTION_FILE, &info->File, 4);
		SendCommand(RII_OPTION_DATA, buffer, length);
		int ret = ReceiveCommand(RII_FILE_WRITE);
#ifdef RIIFS_LOCAL_SEEKING
		if (ret > 0)
			info->Position += ret;
#endif
		return ret;
	}

	int RiiHandler::Seek(FileInfo* file, int where, int whence)
	{
		// FIXME: This function doesn't return a proper result without LOCAL_SEEKING
		// It shouldn't allow seeking beyond end of file either
		RiiFileInfo* info = (RiiFileInfo*)file;
#ifdef RIIFS_LOCAL_SEEKING
		if (whence == SEEK_END) {
			int ret = RiiSeek(info, where, whence);
			if (!ret)
				info->Position = ReceiveCommand(RII_FILE_TELL);
			return info->Position;
		}
		if ((whence == SEEK_SET && (u32)where == info->Position) ||
			(whence == SEEK_CUR && where == 0))
			return info->Position; // Ignore excessive seeking

		info->SeekDirty = true;
		if (whence == SEEK_CUR)
			info->Position += where;
		else // SEEK_SET
			info->Position = where;
		return info->Position;
#else
		return RiiSeek(info, where, whence);
#endif
	}

	int RiiHandler::RiiSeek(RiiFileInfo* info, int where, int whence)
	{
		SendCommand(RII_OPTION_FILE, &info->File, 4);
		SendCommand(RII_OPTION_SEEK_WHENCE, &whence, 4);
		SendCommand(RII_OPTION_SEEK_WHERE, &where, 4);
		return ReceiveCommand(RII_FILE_SEEK);
	}

	int RiiHandler::Tell(FileInfo* file)
	{
#ifdef RIIFS_LOCAL_SEEKING
		return (int)((RiiFileInfo*)file)->Position;
#else
		SendCommand(RII_OPTION_FILE, &((RiiFileInfo*)file)->File, 4);
		return ReceiveCommand(RII_FILE_TELL);
#endif
	}

	int RiiHandler::Sync(FileInfo* file)
	{
		SendCommand(RII_OPTION_FILE, &((RiiFileInfo*)file)->File, 4);
#ifdef RIIFS_LOCAL_SEEKING
		((RiiFileInfo*)file)->Position = ReceiveCommand(RII_FILE_TELL);
#endif
		return ReceiveCommand(RII_FILE_SYNC);
	}

	int RiiHandler::Close(FileInfo* file)
	{
		SendCommand(RII_OPTION_FILE, &((RiiFileInfo*)file)->File, 4);
		int ret = ReceiveCommand(RII_FILE_CLOSE);
		delete file;
		return ret;
	}

	int RiiHandler::Stat(const char* path, Stats* st)
	{
		SendCommand(RII_OPTION_PATH, path, strlen(path));
		return ReceiveCommand(RII_FILE_STAT, st, sizeof(Stats));
	}

	int RiiHandler::CreateFile(const char* path)
	{
		SendCommand(RII_OPTION_PATH, path, strlen(path));
		return ReceiveCommand(RII_FILE_CREATE);
	}

	int RiiHandler::Delete(const char* path)
	{
		SendCommand(RII_OPTION_PATH, path, strlen(path));
		return ReceiveCommand(RII_FILE_DELETE);
	}

	int RiiHandler::Rename(const char* source, const char* dest)
	{
		SendCommand(RII_OPTION_RENAME_SOURCE, source, strlen(source));
		SendCommand(RII_OPTION_RENAME_DESTINATION, dest, strlen(dest));
		return ReceiveCommand(RII_FILE_RENAME);
	}

	int RiiHandler::CreateDir(const char* path)
	{
		SendCommand(RII_OPTION_PATH, path, strlen(path));
		return ReceiveCommand(RII_FILE_CREATEDIR);
	}

	FileInfo* RiiHandler::OpenDir(const char* path)
	{
		SendCommand(RII_OPTION_PATH, path, strlen(path));
		int file = ReceiveCommand(RII_FILE_OPENDIR);
		if (file < 0)
			return null;
		RiiFileInfo* dir = new RiiFileInfo(this, file);
#ifdef RIIFS_LOCAL_DIRNEXT
		if (dir && ServerVersion >= 0x02) {
			dir->DirCache = Memalign(32, RIIFS_LOCAL_DIRNEXT_SIZE);
			if (dir->DirCache)
				memset(dir->DirCache, 0, RIIFS_LOCAL_DIRNEXT_SIZE);
		}
#endif
		return dir;
	}

#ifdef RIIFS_LOCAL_DIRNEXT
	int RiiHandler::NextDirCache(RiiFileInfo* dir, char* filename, Stats* st)
	{
		if (dir->DirCache) {
			int* entries = (int*)dir->DirCache;
			if (!entries[0] || dir->Position >= (u32)entries[0]) {
				SendCommand(RII_OPTION_FILE, &dir->File, 4);
				int ret = ReceiveCommand(RII_FILE_NEXTDIR_CACHE, dir->DirCache, RIIFS_LOCAL_DIRNEXT_SIZE);
				if (ret < 0) {
					memset(dir->DirCache, 0, RIIFS_LOCAL_DIRNEXT_SIZE);
					return -2;
				}
				dir->Position = 0;
			}

			int* offsettable = entries + 1;
			Stats* stattable = (Stats*)(entries + 1 + entries[0]);
			char* nametable = (char*)(entries + 1 + entries[0] * (1 + 6));
			if (!entries[0] || offsettable[dir->Position] < 0)
				return -1;
			strcpy(filename, nametable + offsettable[dir->Position]);
			memcpy(st, stattable + dir->Position, sizeof(Stats));
			dir->Position++;
			return 0;
		}

		return -2;
	}
#endif

	int RiiHandler::NextDir(FileInfo* dir, char* filename, Stats* st)
	{
		RiiFileInfo* info = (RiiFileInfo*)dir;
#ifdef RIIFS_LOCAL_DIRNEXT
		int ret = NextDirCache(info, filename, st);
		if (ret == 0 || ret == -1)
			return ret;
#endif
		SendCommand(RII_OPTION_FILE, &info->File, 4);
		int len = ReceiveCommand(RII_FILE_NEXTDIR_PATH, filename, MAXPATHLEN);
		os_sync_after_write(filename, MAXPATHLEN);
		if (len < 0)
			return len;
		if (st==NULL)
			return 0;
		return ReceiveCommand(RII_FILE_NEXTDIR_STAT, st, sizeof(Stats));
	}

	int RiiHandler::CloseDir(FileInfo* dir)
	{
		RiiFileInfo* info = (RiiFileInfo*)dir;
		SendCommand(RII_OPTION_FILE, &info->File, 4);
#ifdef RIIFS_LOCAL_DIRNEXT
		Dealloc(info->DirCache);
#endif
		delete dir;
		return ReceiveCommand(RII_FILE_CLOSEDIR);
	}

	int RiiHandler::IdleTick()
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

		if (ServerVersion < 3 || IdleCount < 0)
			return -1;

		if (IdleCount++ > (RII_IDLE_TIME/FSIDLE_TICK))
			return (SendCommand(RII_OPTION_PING)) ? 1 : -1;
		return 0;
	}

	int RiiHandler::GetFreeSpace(u64 *free_bytes)
	{
		// unimplemented, just return max value
		*free_bytes = 0x7FFFFFFFFFFFFFFFllu;
		return 0;
	}

	int RiiHandler::Log(const void* buffer, int length)
	{
		if (ServerVersion >= 4)
		{
			if (LogSize > 0x2000) // At this point, just flush the damn thing
				IdleTick();
			u8 *NewBuffer = (u8*)Realloc(LogBuffer, length+LogSize, LogSize);
			if (NewBuffer) { // if Realloc failed LogBuffer should be left untouched
				LogBuffer = NewBuffer;
				memcpy(LogBuffer+LogSize, buffer, length);
				LogSize += length;
			}
		}
		return length;
	}
} }
