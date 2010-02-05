#pragma once

#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#include <sys/iosupport.h>

#include "gctypes.h"
#include "proxiios.h"
#include "ipc.h"
#include "wiisd_io.h"
#include "usbstorage.h"
#include "usb2storage.h"
#include "files.h"
#include "mem.h"
#include "timer.h"

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
			InitDisc		= IOCTL_InitDisc,
			Mount			= IOCTL_Mount,

			// File Operations
			Stat			= IOCTL_Stat,
			CreateFile		= IOCTL_CreateFile,
			Delete			= IOCTL_Delete,
			Rename			= IOCTL_Rename,

			// These are actually done in os_seek
			// Because we need IOSP to translate the FD for us
			Tell			= SEEK_Tell,
			Sync			= SEEK_Sync,

			// Directory Operations
			CreateDir		= IOCTL_CreateDir,
			OpenDir			= IOCTL_OpenDir,
			NextDir			= IOCTL_NextDir,
			CloseDir		= IOCTL_CloseDir
		};
	}

	namespace Filesystems {
		enum Enum {
			FAT				= FS_FAT,
			ELM				= FS_ELM,
			NTFS			= FS_NTFS,
			Ext2			= FS_EXT2,
			SMB				= FS_SMB,
			ISFS			= FS_ISFS
		};
	}

	namespace Disks {
		enum Enum {
			SD				= SD_DISK,
			USB				= USB_DISK,
			USB2			= USB2_DISK
		};
	}

	namespace Errors {
		enum Enum {
			Success			= ERROR_SUCCESS,
			Unrecognized	= ERROR_UNRECOGNIZED,
			NotMounted		= ERROR_NOTMOUNTED,

			DiskNotStarted	= ERROR_DISKNOTSTARTED,
			DiskNotInserted	= ERROR_DISKNOTINSERTED,
			DiskNotMounted	= ERROR_DISKNOTMOUNTED
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
