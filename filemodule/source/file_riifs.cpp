#include "file_riifs.h"

#include <network.h>

#include <syscalls.h>

#include <print.h>

namespace ProxiIOS { namespace Filesystem {
	int RiiHandler::Mount(const void* options, int length)
	{
		if (length != 0x14)
			return Errors::Unrecognized;

		while (net_init() < 0)
			;

		/*
		if (!net_init())
			return NULL;
		 */
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
		
		if (ReceiveCommand(RII_HANDSHAKE) != RII_VERSION_RET) {
			Unmount();
			return Errors::DiskNotMounted;
		}
		
		_sprintf(MountPoint, "/mnt/net/%s/%d", IP, Port);

		return Errors::Success;
	}
	
	bool RiiHandler::SendCommand(int type)
	{
		return SendCommand(type, NULL, 0);
	}
	
	bool RiiHandler::SendCommand(int type, const void* data, int size)
	{
		int sendcommand = RII_SEND;
		bool fail = false;
		fail |= net_send(Socket, &sendcommand, 4, 0) < 0;
		fail |= net_send(Socket, &type, 4, 0) < 0;
		fail |= net_send(Socket, &size, 4, 0) < 0;
		if (size && data)
			fail |= net_send(Socket, data, size, 0) < 0;
		
		return !fail;
	}
	
	int RiiHandler::ReceiveCommand(int type)
	{
		return ReceiveCommand(type, NULL, 0);
	}
	
	static int netrecv(int socket, u8* data, int size, int opts)
	{
		int read = 0;
		while (read < size) {
			int ret = net_recv(socket, data + read, size - read, opts);
			if (ret < 0)
				return ret;
			if (ret == 0)
				return read;

			read += ret;
		}

		return read;
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
		SendCommand(RII_OPTION_FILE, &((RiiFileInfo*)file)->File, 4);
		SendCommand(RII_OPTION_LENGTH, &length, 4);
		return ReceiveCommand(RII_FILE_READ, buffer, length);
	}
	
	int RiiHandler::Write(FileInfo* file, const u8* buffer, int length)
	{
		SendCommand(RII_OPTION_FILE, &((RiiFileInfo*)file)->File, 4);
		SendCommand(RII_OPTION_DATA, buffer, length);
		return ReceiveCommand(RII_FILE_WRITE);
	}
	
	int RiiHandler::Seek(FileInfo* file, int where, int whence)
	{
		SendCommand(RII_OPTION_FILE, &((RiiFileInfo*)file)->File, 4);
		SendCommand(RII_OPTION_SEEK_WHERE, &where, 4);
		SendCommand(RII_OPTION_SEEK_WHENCE, &whence, 4);
		return ReceiveCommand(RII_FILE_SEEK);
	}
	
	int RiiHandler::Tell(FileInfo* file)
	{
		SendCommand(RII_OPTION_FILE, &((RiiFileInfo*)file)->File, 4);
		return ReceiveCommand(RII_FILE_TELL);
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
		return new RiiFileInfo(this, file);
	}
	
	int RiiHandler::NextDir(FileInfo* dir, char* filename, Stats* st)
	{
		SendCommand(RII_OPTION_FILE, &((RiiFileInfo*)dir)->File, 4);
		int len = ReceiveCommand(RII_FILE_NEXTDIR_PATH, filename, MAXPATHLEN);
		os_sync_after_write(filename, MAXPATHLEN);
		if (len < 0)
			return len;
		return ReceiveCommand(RII_FILE_NEXTDIR_STAT, st, sizeof(Stats));
	}
	
	int RiiHandler::CloseDir(FileInfo* dir)
	{
		SendCommand(RII_OPTION_FILE, &((RiiFileInfo*)dir)->File, 4);
		delete dir;
		return ReceiveCommand(RII_FILE_CLOSEDIR);
	}
} }
