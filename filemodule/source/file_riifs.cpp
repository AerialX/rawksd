#include "file_riifs.h"

#include <network.h>

#include <syscalls.h>

#include <print.h>

namespace ProxiIOS { namespace Filesystem {
	int RiiHandler::Mount(const void* options, int length)
	{
		if (length != 0x14)
			return Errors::Unrecognized;

		int ret;
		while ((ret = net_init()) == -EAGAIN)
			usleep(10000);
		if (ret < 0)
			return NULL;

		strcpy(IP, (const char*)options);
		Port = *(int*)((u8*)options + 0x10);

		Socket = net_socket(PF_INET, SOCK_STREAM, 0);
		struct sockaddr_in address;
		memset(&address, 0, sizeof(address));
		address.sin_family = PF_INET;
		address.sin_port = htons(Port);
		if (inet_aton(IP, &address.sin_addr) < 0)
			return Errors::DiskNotMounted;
		if (net_connect(Socket, (struct sockaddr*)(const void *)&address, sizeof(address)) < 0)
			return Errors::DiskNotMounted;
		
		if (!SendCommand(RII_HANDSHAKE, (const u8*)RII_VERSION, strlen(RII_VERSION))) {
			Unmount();
			return Errors::DiskNotMounted;
		}
		
		ServerVersion = ReceiveCommand(RII_HANDSHAKE);
		if (ServerVersion > RII_VERSION_RET) {
			Unmount();
			return Errors::DiskNotMounted;
		}
		
		_sprintf(MountPoint, "/mnt/net/%s/%d", IP, Port);

		// TODO: Add a timer for pinging the server every 30 seconds; don't wait for a response

		return Errors::Success;
	}
	
	bool RiiHandler::SendCommand(int type)
	{
		return SendCommand(type, NULL, 0);
	}
	
	bool RiiHandler::SendCommand(int type, const void* data, int size)
	{
#ifdef RIIFS_LOCAL_OPTIONS
		int value = 0;
		if (size == 4) {
			value = *(int*)data;
			if (OptionsInit[type - 1] && Options[type - 1] == value)
				return true;
		}
#endif
		int sendcommand = RII_SEND;
		bool fail = false;
		fail |= net_send(Socket, &sendcommand, 4, 0) < 0;
		fail |= net_send(Socket, &type, 4, 0) < 0;
		fail |= net_send(Socket, &size, 4, 0) < 0;
		if (size && data)
			fail |= net_send(Socket, data, size, 0) < 0;
#ifdef RIIFS_LOCAL_OPTIONS
		if (size == 4 && !fail) {
			Options[type - 1] = value;
			OptionsInit[type - 1] = 1;
		}
#endif
		return !fail;
	}
	
	static int netrecv(int socket, u8* data, int size, int opts)
	{
		int read = 0;
		static u8 buffer[0x1000] ATTRIBUTE_ALIGN(32);
		bool misaligned = (u32)data & 0x1F;
		while (read < size) {
			int ret;
			if (misaligned)
				ret = net_recv(socket, buffer, MIN(0x1000, size - read), opts);
			else
				ret = net_recv(socket, data + read, MIN(0x2000, size - read), opts);

			if (ret < 0)
				return ret;
			if (ret == 0)
				return read;

			if (misaligned)
				memcpy(data + read, buffer, ret);

			read += ret;
		}

		return read;
	}
	
	int RiiHandler::ReceiveCommand(int type)
	{
		return ReceiveCommand(type, NULL, 0);
	}

	int RiiHandler::ReceiveCommand(int type, void* data, int size)
	{
		int sendcommand = RII_RECEIVE;
		bool fail = false;
		fail |= net_send(Socket, &sendcommand, 4, 0) < 0;
		fail |= net_send(Socket, &type, 4, 0) < 0;
		int ret = 0;
		if (size) {
			if (data)
				fail |= netrecv(Socket, (u8*)data, size, 0) < 0;
			else {
				void* temp = Alloc(size);
				netrecv(Socket, (u8*)temp, size, 0);
				Dealloc(temp);
			}
		}
		fail |= netrecv(Socket, (u8*)&ret, 4, 0) < 0;
		
		if (fail)
			return -1;
		
		return ret;
	}
	
	int RiiHandler::Unmount()
	{
		ReceiveCommand(RII_GOODBYE);
		net_close(Socket);
		return 0;
	}
	
	FileInfo* RiiHandler::Open(const char* path, int mode)
	{
		SendCommand(RII_OPTION_PATH, path, strlen(path));
		SendCommand(RII_OPTION_MODE, &mode, 4);
		
		int ret = ReceiveCommand(RII_FILE_OPEN);
		if (ret < 0)
			return NULL;
		return new RiiFileInfo(this, ret);
	}
	
	int RiiHandler::Read(FileInfo* file, u8* buffer, int length)
	{
		RiiFileInfo* info = (RiiFileInfo*)file;

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
		RiiFileInfo* info = (RiiFileInfo*)file;
#ifdef RIIFS_LOCAL_SEEKING
		if (whence == SEEK_SET && (u32)where == info->Position)
			return 0; // Ignore excessive seeking
#endif
		SendCommand(RII_OPTION_FILE, &info->File, 4);
		SendCommand(RII_OPTION_SEEK_WHERE, &where, 4);
		SendCommand(RII_OPTION_SEEK_WHENCE, &whence, 4);
		int ret = ReceiveCommand(RII_FILE_SEEK);
#ifdef RIIFS_LOCAL_SEEKING
		if (!ret) {
			if (whence == SEEK_CUR)
				info->Position += where;
			else
				info->Position = ReceiveCommand(RII_FILE_TELL);
		}
#endif
		return ret;
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
		return ReceiveCommand(RII_FILE_NEXTDIR_STAT, st, sizeof(Stats));
	}
	
	int RiiHandler::CloseDir(FileInfo* dir)
	{
		RiiFileInfo* info = (RiiFileInfo*)dir;
		SendCommand(RII_OPTION_FILE, &info->File, 4);
#ifdef RIIFS_LOCAL_DIRNEXT
		if (info->DirCache)
			Dealloc(info->DirCache);
#endif
		delete dir;
		return ReceiveCommand(RII_FILE_CLOSEDIR);
	}
} }
