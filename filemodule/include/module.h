#pragma once

#include <proxiios.h>
#include <ipc.h>
#include <sys/stat.h>

#include <disc_io.h>

#include <wiisd_io.h>
#include <usbstorage.h>

#include <unistd.h>

#include <stdio.h>
#include <sys/iosupport.h>

#include <timer.h>

#include <files.h>

#define MAX_OPEN_FILES 0x40

#define FSIOCTL_GETSTATS		2
#define FSIOCTL_CREATEDIR		3
#define FSIOCTL_READDIR			4
#define FSIOCTL_SETATTR			5
#define FSIOCTL_GETATTR			6
#define FSIOCTL_DELETE			7
#define FSIOCTL_RENAME			8
#define FSIOCTL_CREATEFILE		9

namespace ProxiIOS { namespace Filesystem {
	namespace Ioctl {
		enum Enum {
			// Mount stuff
			InitDisc		= 0x30,
			Mount			= 0x31,
			
			// File Operations
			Stat			= 0x40,
			CreateFile		= 0x41,
			Delete			= 0x42,
			Rename			= 0x43,
			
			// These are actually done in os_seek
			// Because we need IOSP to translate the FD for us
			Tell			= 0x48,
			Sync			= 0x49,
			
			// Directory Operations
			CreateDir		= 0x50,
			OpenDir			= 0x51,
			NextDir			= 0x52,
			CloseDir		= 0x53
		};
	}
	
	namespace Filesystems {
		enum Enum {
			FAT				= 0x01,
			ELM				= 0x02,
			NTFS			= 0x03,
			Ext2			= 0x04,
			SMB				= 0x05,
			ISFS			= 0x06
		};
	}
	
	namespace Disks {
		enum Enum {
			SD				= 0x01,
			USB				= 0x02
		};
	}
	
	namespace Errors {
		enum Enum {
			Success			= 0x00,
			Unrecognized	= -0x80,
			NotMounted		= -0x81,
			
			DiskNotStarted	= -0x90,
			DiskNotInserted	= -0x91,
			DiskNotMounted	= -0x92
		};
	}
	
	class FilesystemHandler;
	
	struct FilesystemInfo
	{
		FilesystemInfo(Filesystems::Enum type, FilesystemHandler* handler) :
			Type(type), Handler(handler) { }
		
		Filesystems::Enum Type;
		FilesystemHandler* Handler;
	};
	
	struct FileInfo
	{
		FilesystemInfo* System;
	};
	
	class FilesystemHandler
	{
		public:
			virtual FilesystemInfo* Mount(DISC_INTERFACE* disk) = 0;
			virtual int Unmount(FilesystemInfo* filesystem) = 0;
			virtual FileInfo* Open(FilesystemInfo* filesystem, const char* path, u8 mode) = 0;
			
			virtual int Read(FileInfo* file, u8* buffer, int length) = 0;
			virtual int Write(FileInfo* file, const u8* buffer, int length) = 0;
			virtual int Seek(FileInfo* file, int where, int whence) = 0;
			virtual int Tell(FileInfo* file) = 0;
			virtual int Sync(FileInfo* file) = 0;
			virtual int Close(FileInfo* file) = 0;
			
			virtual int Stat(const char* filename, Stats* st) = 0;
			virtual int CreateFile(const char* filename) = 0;
			virtual int Delete(const char* filename) = 0;
			virtual int Rename(const char* source, const char* destination) = 0;
			virtual int CreateDir(const char* dirname) = 0;
			virtual int OpenDir(const char* dirname) = 0;
			virtual int NextDir(int dir, char* filename, Stats* st) = 0;
			virtual int CloseDir(int dir) = 0;
	};
	
	class Filesystem : public ProxiIOS::Module
	{
	public:
		void* OpenFiles[MAX_OPEN_FILES];
		
		DISC_INTERFACE* Disk;
		FilesystemInfo* Mounted;
		FilesystemInfo* Isfs;
		
		Filesystem();
		
		int HandleOpen(ipcmessage* message);
		int HandleClose(ipcmessage* message);
		int HandleIoctl(ipcmessage* message);
		int HandleRead(ipcmessage* message);
		int HandleWrite(ipcmessage* message);
		int HandleSeek(ipcmessage* message);
	};
} }
