#pragma once

#include "filemodule.h"

// maximum length of a nand filename (8.3/dos style)
#define NANDFILE_MAX		0x0C

namespace ProxiIOS { namespace Filesystem {
	namespace ISFS {
		enum Enum {
			Format          = 0x01,
			GetStats        = 0x02,
			CreateDir       = 0x03,
			ReadDir         = 0x04,
			SetAttrib       = 0x05,
			GetAttrib       = 0x06,
			Delete          = 0x07,
			Rename          = 0x08,
			CreateFile      = 0x09,
			SetFileVerCtrl  = 0x0A,
			GetFileStats    = 0x0B,
			GetUsage        = 0x0C,
			Shutdown        = 0x0D
		};

		struct Stats {
			u32 Length;
			u32 Pos;
		};

		struct FSattr {
			u32 owner_id;
			u16 group_id;
			char path[ISFS_MAXPATH_LEN];
			u8 ownerperm;
			u8 groupperm;
			u8 otherperm;
			u8 attributes;
			u8 pad[2];
		};
	}

	struct IsfsFileInfo : public FileInfo
	{
		IsfsFileInfo(FilesystemHandler* system, int fd) : FileInfo(system) { File = fd; }

		int File;
	};

	struct IsfsDirInfo : public FileInfo
	{
		IsfsDirInfo(FilesystemHandler* system, const char *_path, u32 count, int &filefd);
		~IsfsDirInfo() {Dealloc(dir_names); Dealloc(path);}

		const char *next_name;
		u32 dir_count;
		u8 *dir_names;
		char *path;
	};

	class IsfsHandler : public FilesystemHandler
	{
		protected:
			int filefd;

		public:
			IsfsHandler(Filesystem* fs) : FilesystemHandler(fs) { strcpy(MountPoint, "/mnt/isfs"); }

			int Mount(const void* options, int optionslen);
			int Unmount();

			FileInfo* Open(const char* path, int mode);
			int Read(FileInfo* file, u8* buffer, int length);
			int Write(FileInfo* file, const u8* buffer, int length);
			int Seek(FileInfo* file, int where, int whence);
			int Tell(FileInfo* file);
			int Sync(FileInfo* file);
			int Close(FileInfo* file);

			int Stat(const char* path, Stats* st);
			int CreateFile(const char* path);
			int Delete(const char* path);
			FileInfo* OpenDir(const char* path);
			int NextDir(FileInfo* _dir, char* dirname, Stats* st);
			int CloseDir(FileInfo* dir);

	};
} }
