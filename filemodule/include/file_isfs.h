#pragma once

#include "module.h"

namespace ProxiIOS { namespace Filesystem {
	
	struct IsfsFilesystemInfo : public FilesystemInfo
	{
		IsfsFilesystemInfo(FilesystemHandler* handler) :
			FilesystemInfo(Filesystems::ISFS, handler) { }
	};
	
	struct IsfsFileInfo : public FileInfo
	{
		int File;
	};
	
	class IsfsHandler : public FilesystemHandler
	{
		protected:
			int filefd;
		
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
