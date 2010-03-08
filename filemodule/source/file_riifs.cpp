#include "file_riifs.h"

#include <network.h>

#include <syscalls.h>

namespace ProxiIOS { namespace Filesystem {	
	FilesystemInfo* RiiHandler::Mount(DISC_INTERFACE* disk)
	{
		if (!net_init())
			return NULL;

		socket = net_socket(PF_INET, SOCK_STREAM, 0);
		struct sockaddr_in address;
		memset(&address, 0, sizeof(address));
		address.sin_family = PF_INET;
		address.sin_port = htons(1137);
		if (inet_aton("192.168.1.8", &address.sin_addr) <= 0)
			return NULL;
		if (net_connect(socket, (struct sockaddr*)(const void *)&address, sizeof(address)) == -1)
			return NULL;
		
		if (!SendCommand(RII_HANDSHAKE, (const u8*)RII_VERSION, strlen(RII_VERSION)))
			return NULL;
		
		if (ReceiveCommand(RII_HANDSHAKE) != RII_VERSION_RET)
			return NULL;
		
		return new RiiFilesystemInfo(this);
	}
	
	bool RiiHandler::SendCommand(int type)
	{
		return SendCommand(type, NULL, 0);
	}
	
	bool RiiHandler::SendCommand(int type, const void* data, int size)
	{
		int sendcommand = RII_SEND;
		bool fail = false;
		fail |= net_send(socket, &sendcommand, 4, 0) < 0;
		fail |= net_send(socket, &type, 4, 0) < 0;
		fail |= net_send(socket, &size, 4, 0) < 0;
		if (size && data)
			fail |= net_send(socket, data, size, 0) < 0;
		
		return !fail;
	}
	
	int RiiHandler::ReceiveCommand(int type)
	{
		return ReceiveCommand(type, NULL, 0);
	}
	
	int netrecv(int socket, u8* data, int size, int opts)
	{
		int read = 0;
		while (size > 0) {
			int ret = net_recv(socket, data, size, opts);
			if (ret < 0)
				return ret;
			if (ret == 0)
				return read;
			
			read += ret;
			data += ret;
			size -= ret;
		}
		
		return read;
	}
	
	int RiiHandler::ReceiveCommand(int type, void* data, int size)
	{
		int sendcommand = RII_RECEIVE;
		bool fail = false;
		fail |= net_send(socket, &sendcommand, 4, 0) < 0;
		fail |= net_send(socket, &type, 4, 0) < 0;
		int ret = 0;
		fail |= netrecv(socket, (u8*)&ret, 4, 0) < 0;
		if (size) {
			if (data)
				fail |= netrecv(socket, (u8*)data, size, 0) < 0;
			else {
				void* temp = Alloc(size);
				netrecv(socket, (u8*)temp, size, 0);
				Dealloc(temp);
			}
		}
		
		if (fail)
			return -1;
		
		return ret;
	}
	
	int RiiHandler::Unmount(FilesystemInfo* filesystem)
	{
		ReceiveCommand(RII_GOODBYE);
		net_close(socket);
		return 0;
	}
	
	FileInfo* RiiHandler::Open(FilesystemInfo* filesystem, const char* path, u8 mode)
	{
		SendCommand(RII_OPTION_PATH, path, strlen(path));
		SendCommand(RII_OPTION_MODE, &mode, 1);
		
		int ret = ReceiveCommand(RII_FILE_OPEN);
		if (ret < 0)
			return NULL;
		RiiFileInfo* file = new RiiFileInfo();
		file->System = filesystem;
		file->File = ret;
		return file;
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
	
	int RiiHandler::Stat(const char* filename, Stats* st)
	{
		SendCommand(RII_OPTION_PATH, filename, strlen(filename));
		return ReceiveCommand(RII_FILE_STAT, st, sizeof(st));
	}
	
	int RiiHandler::CreateFile(const char* filename)
	{
		SendCommand(RII_OPTION_PATH, filename, strlen(filename));
		return ReceiveCommand(RII_FILE_CREATE);
	}
	
	int RiiHandler::Delete(const char* filename)
	{
		SendCommand(RII_OPTION_PATH, filename, strlen(filename));
		return ReceiveCommand(RII_FILE_DELETE);
	}
	
	int RiiHandler::Rename(const char* source, const char* destination)
	{
		SendCommand(RII_OPTION_RENAME_SOURCE, source, strlen(source));
		SendCommand(RII_OPTION_RENAME_DESTINATION, destination, strlen(destination));
		return ReceiveCommand(RII_FILE_RENAME);
	}
	
	int RiiHandler::CreateDir(const char* dirname)
	{
		SendCommand(RII_OPTION_PATH, dirname, strlen(dirname));
		return ReceiveCommand(RII_FILE_CREATEDIR);
	}
	
	int RiiHandler::OpenDir(const char* dirname)
	{
		SendCommand(RII_OPTION_PATH, dirname, strlen(dirname));
		return ReceiveCommand(RII_FILE_OPENDIR);
	}
	
	int RiiHandler::NextDir(int dir, char* filename, Stats* st)
	{
		SendCommand(RII_OPTION_FILE, &dir, 4);
		ReceiveCommand(RII_FILE_NEXTDIR_PATH, filename, MAXPATHLEN);
		return ReceiveCommand(RII_FILE_NEXTDIR_STAT, st, sizeof(Stats));
	}
	
	int RiiHandler::CloseDir(int dir)
	{
		SendCommand(RII_OPTION_FILE, &dir, 4);
		return ReceiveCommand(RII_FILE_CLOSEDIR);
	}
	
} }
