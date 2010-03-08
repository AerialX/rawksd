#pragma once

#include "filemodule.h"

namespace ProxiIOS { namespace Filesystem {
	struct IsfsFileInfo : public FileInfo
	{
		IsfsFileInfo(FilesystemHandler* system, int fd) : FileInfo(system) { File = fd; }

		int File;
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
	};
} }
