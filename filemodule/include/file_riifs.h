#pragma once

#include "filemodule.h"

// Actions
#define RII_SEND 0x01
#define RII_RECEIVE 0x02

// Commands
#define RII_HANDSHAKE			0x00
#define RII_GOODBYE				0x01
#define RII_LOG					0x02
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
#define RII_FILE_NEXTDIR_CACHE	0x25

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
#define RII_OPTION_PING					0x10

#define RII_IDLE_TIME 30*1000*1000

#define RIIFS_LOCAL_OPTIONS
#define RIIFS_LOCAL_SEEKING
//#define RIIFS_LOCAL_DIRNEXT
#define RIIFS_LOCAL_DIRNEXT_SIZE 0x1000

#define RII_VERSION 		"1.03"

#define RII_VERSION_RET		0x03

namespace ProxiIOS { namespace Filesystem {
	struct RiiFileInfo : public FileInfo
	{
		RiiFileInfo(FilesystemHandler* system, int fd) : FileInfo(system)
		{
			File = fd;
#ifdef RIIFS_LOCAL_SEEKING
			Position = 0;
			SeekDirty = false;
#endif
#ifdef RIIFS_LOCAL_DIRNEXT
			DirCache = NULL;
#endif
		}

#ifdef RIIFS_LOCAL_SEEKING
		u64 Position;
		bool SeekDirty;
#endif
#ifdef RIIFS_LOCAL_DIRNEXT
		void* DirCache;
#endif
		int File;
	};

	class RiiHandler : public FilesystemHandler
	{
		protected:
			char Host[0x40];
			int Port;
			int Socket;
			int ServerVersion;
			int IdleCount;
			u8 *LogBuffer;
			int LogSize;

#ifdef RIIFS_LOCAL_OPTIONS
			int Options[RII_OPTION_RENAME_DESTINATION];
			u8 OptionsInit[RII_OPTION_RENAME_DESTINATION];
#endif

			bool SendCommand(int type, const void* data=NULL, int size=0);
			int ReceiveCommand(int type, void* data=NULL, int size=0);

		public:
			RiiHandler(Filesystem* fs) : FilesystemHandler(fs) {
#ifdef RIIFS_LOCAL_OPTIONS
				memset(Options, 0, sizeof(Options));
				memset(OptionsInit, 0, sizeof(OptionsInit));
#endif
				Socket = -1;
				IdleCount = -1;
				LogBuffer = NULL;
				LogSize = 0;
			}

			~RiiHandler() {
				Unmount();
			}

			int Mount(const void* options, int length);
			int Unmount();
			int CheckPhysical();

			FileInfo* Open(const char* path, int mode);
			int Read(FileInfo* file, u8* buffer, int length);
			int Write(FileInfo* file, const u8* buffer, int length);
			int Seek(FileInfo* file, int where, int whence);
			int RiiSeek(RiiFileInfo* file, int where, int whence);
			int Tell(FileInfo* file);
			int Sync(FileInfo* file);
			int Close(FileInfo* file);

			int Stat(const char* path, Stats* st);
			int CreateFile(const char* path);
			int Delete(const char* path);
			int Rename(const char* source, const char* destination);
			int CreateDir(const char* path);
			FileInfo* OpenDir(const char* path);
			int NextDir(FileInfo* dir, char* filename, Stats* st);
			int CloseDir(FileInfo* dir);

			int IdleTick();
			int Log(const void* buffer, int length);
			int GetFreeSpace(u64 *free_bytes);

#ifdef RIIFS_LOCAL_DIRNEXT
			int NextDirCache(RiiFileInfo* dir, char* filename, Stats* st);
#endif
	};
} }
