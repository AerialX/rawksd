#pragma once

#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#include <sys/iosupport.h>
#include <sys/param.h>

#include "gctypes.h"
#include "proxiios.h"
#include "ipc.h"
#include "wiisd_io.h"
#include "usbstorage.h"
#include "files.h"
#include "mem.h"
#include "timer.h"

#define FSIOCTL_GETSTATS		2
#define FSIOCTL_CREATEDIR		3
#define FSIOCTL_READDIR			4
#define FSIOCTL_SETATTR			5
#define FSIOCTL_GETATTR			6
#define FSIOCTL_DELETE			7
#define FSIOCTL_RENAME			8
#define FSIOCTL_CREATEFILE		9

#define FILE_MAX_MOUNTED		0x20
#define FILE_MAX_DISKS			0x20

#define FSIDLE_TICK		1000000 // 1s, minimum FilesystemHandler Idle() tick

namespace ProxiIOS { namespace Filesystem {
	namespace Ioctl {
		enum Enum {
			// Mount stuff
			InitDisc		= IOCTL_InitDisc,
			Mount			= IOCTL_Mount,
			Unmount			= IOCTL_Unmount,
			GetMountPoint	= IOCTL_MountPoint,
			SetDefault		= IOCTL_SetDefault,
			SetLogFS		= IOCTL_SetLogFS,
			GetLogFS		= IOCTL_GetLogFS,

			// File Operations
			Stat			= IOCTL_Stat,
			CreateFile		= IOCTL_CreateFile,
			Delete			= IOCTL_Delete,
			Rename			= IOCTL_Rename,

			// These are actually done in os_seek
			// Because we need IOS to translate the FD for us
			Tell			= SEEK_Tell,
			Sync			= SEEK_Sync,

			// Directory Operations
			CreateDir		= IOCTL_CreateDir,
			OpenDir			= IOCTL_OpenDir,
			NextDir			= IOCTL_NextDir, // IOCTLV
			CloseDir		= IOCTL_CloseDir,

			// Shorten a long path (>64) for IOS_Open
			Shorten			= IOCTL_Shorten,
			// Emit a log buffer
			Log				= IOCTL_Log,
			// Set RTC epoch
			Epoch			= IOCTL_Epoch,
			// Hacky context dumper
			Context         = IOCTL_Context,
			// Check the physical medium for a filesystem
			CheckPhysical   = IOCTL_CheckPhys,
			// Get the amount of free space in bytes
			GetFreeSpace    = IOCTL_GetFreeSpace,
			// Set slot LED activity indicator
			SetSlotLED      = IOCTL_SetSlotLED,
		};
	}

	namespace Filesystems {
		enum Enum {
			FAT				= FS_FAT,
			ELM				= FS_ELM,
			NTFS			= FS_NTFS,
			Ext2			= FS_EXT2,
			SMB				= FS_SMB,
			ISFS			= FS_ISFS,
			RiiFS			= FS_RIIFS
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
			DiskNotMounted	= ERROR_DISKNOTMOUNTED,

			OutOfMemory		= ERROR_OUTOFMEMORY,

			NotOpened		= ERROR_NOTOPENED
		};
	}

	class FilesystemHandler;
	struct FileInfo
	{
		FileInfo(FilesystemHandler* system) : System(system) { }

		FilesystemHandler* System;
	};

	class Filesystem;
	class FilesystemHandler
	{
		public:
			Filesystem* Module;
			char MountPoint[0x40];

			FilesystemHandler(Filesystem* fs) { Module = fs; memset(MountPoint, 0, 0x40); }
			virtual ~FilesystemHandler() { }

			virtual int Mount(const void* options, int length) { return -1; };
			virtual int Unmount() { return -1; };
			virtual int CheckPhysical() { return -1; };
			virtual FileInfo* Open(const char* path, int mode) { return null; };

			virtual int Read(FileInfo* file, u8* buffer, int length) { return -1; };
			virtual int Write(FileInfo* file, const u8* buffer, int length) { return -1; };
			virtual int Seek(FileInfo* file, int where, int whence) { return -1; };
			virtual int Tell(FileInfo* file) { return -1; };
			virtual int Sync(FileInfo* file) { return -1; };
			virtual int Close(FileInfo* file) { return -1; };

			virtual int Stat(const char* path, Stats* st) { return -1; };
			virtual int CreateFile(const char* path) { return -1; };
			virtual int Delete(const char* path) { return -1; };
			virtual int Rename(const char* path, const char* destination) { return -1; };
			virtual int CreateDir(const char* path) { return -1; };
			virtual FileInfo* OpenDir(const char* path) { return null; };
			virtual int NextDir(FileInfo* dir, char* dirname, Stats* st) { return -1; };
			virtual int CloseDir(FileInfo* dir) { return -1; };
			virtual int IdleTick() { return -1; };
			virtual int Log(const void* buffer, int length) { return 0; };
			virtual int GetFreeSpace(u64 *free_bytes) {return -1; };
	};

	class Filesystem : public ProxiIOS::Module
	{
	private:
		void PrintLog(const char* fmt, ...);
	public:
		DISC_INTERFACE* Disk[FILE_MAX_DISKS];
		FilesystemHandler* Mounted[FILE_MAX_MOUNTED];
		int Default;
		ostimer_t Idle_Timer;
		void *Long_Path;
		int LogFS;

		Filesystem();

		int HandleOpen(ipcmessage* message);
		int HandleClose(ipcmessage* message);
		int HandleIoctl(ipcmessage* message);
		int HandleIoctlv(ipcmessage* message);
		int HandleRead(ipcmessage* message);
		int HandleWrite(ipcmessage* message);
		int HandleSeek(ipcmessage* message);
		bool HandleOther(u32 message, int &result, bool &ack);
	};
} }
