#pragma once

#include "filemodule.h"

// maximum length of a nand filename (8.3/dos style)
#define NANDFILE_MAX		12

namespace ProxiIOS { namespace Filesystem {
	struct IsfsFileInfo : public FileInfo
	{
		IsfsFileInfo(FilesystemHandler* system, int fd) : FileInfo(system) { File = fd; }

		int File;
	};

	struct IsfsDirInfo : public FileInfo
	{
		IsfsDirInfo(FilesystemHandler* system, const char *_path, u32 count, int &filefd);
		~IsfsDirInfo();

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
			int CheckPhysical();

			FileInfo* Open(const char* path, int mode);
			int Read(FileInfo* file, u8* buffer, int length);
			int Write(FileInfo* file, const u8* buffer, int length);
			int Seek(FileInfo* file, int where, int whence);
			int Tell(FileInfo* file);
			int Close(FileInfo* file);

			int Stat(const char* path, Stats* st);
			int CreateFile(const char* path);
			int Delete(const char* path);
			FileInfo* OpenDir(const char* path);
			int NextDir(FileInfo* _dir, char* dirname, Stats* st);
			int CloseDir(FileInfo* dir);

	};
} }
