#pragma once

#include "module.h"

#define RII_VERSION 		"1.0"
#define RII_VERSION_RET		0x01

// Actions
#define RII_SEND 0x01
#define RII_RECEIVE 0x02

// Commands
#define RII_HANDSHAKE			0x00
#define RII_GOODBYE				0x01
#define RII_FILE_OPEN			0x10
#define RII_FILE_READ			0x11
#define RII_FILE_WRITE			0x12
#define RII_FILE_SEEK			0x13
#define RII_FILE_TELL			0x14
#define RII_FILE_SYNC			0x15
#define RII_FILE_CLOSE			0x16
#define RII_FILE_STAT			0x17
#define RII_FILE_CREATE			0x18
#define RII_FILE_DELETE			0x19
#define RII_FILE_RENAME			0x1A
#define RII_FILE_CREATEDIR		0x20
#define RII_FILE_OPENDIR		0x21
#define RII_FILE_CLOSEDIR		0x22
#define RII_FILE_NEXTDIR_PATH 	0x23
#define RII_FILE_NEXTDIR_STAT 	0x24

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

namespace ProxiIOS { namespace Filesystem {
	
	struct RiiFilesystemInfo : public FilesystemInfo
	{
		RiiFilesystemInfo(FilesystemHandler* handler) :
			FilesystemInfo(Filesystems::RiiFS, handler) { }
	};
	
	struct RiiFileInfo : public FileInfo
	{
		int File;
	};
	
	class RiiHandler : public FilesystemHandler
	{
		protected:
			int socket;
		
			bool SendCommand(int type, const void* data, int size);
			int ReceiveCommand(int type, void* data, int size);
			bool SendCommand(int type);
			int ReceiveCommand(int type);
		
		public:
			FilesystemInfo* Mount(DISC_INTERFACE* disk);
			int Unmount(FilesystemInfo* filesystem);
			
			FileInfo* Open(FilesystemInfo* filesystem, const char* path, u8 mode);
			int Read(FileInfo* file, u8* buffer, int length);
			int Write(FileInfo* file, const u8* buffer, int length);
			int Seek(FileInfo* file, int where, int whence);
			int Tell(FileInfo* file);
			int Sync(FileInfo* file);
			int Close(FileInfo* file);
			
			int Stat(const char* filename, Stats* st);
			int CreateFile(const char* filename);
			int Delete(const char* filename);
			int Rename(const char* source, const char* destination);
			int CreateDir(const char* dirname);
			int OpenDir(const char* dirname);
			int NextDir(int dir, char* filename, Stats* st);
			int CloseDir(int dir);
	};
	
} }
