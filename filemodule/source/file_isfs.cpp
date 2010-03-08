#include "file_isfs.h"

#include <syscalls.h>

namespace ProxiIOS { namespace Filesystem {
	int IsfsHandler::Mount(const void* options, int optionslen)
	{
		filefd = os_open("/dev/fs", 0);
		if (filefd < 0)
			return Errors::DiskNotMounted;
		
		return Errors::Success;
	}
	
	int IsfsHandler::Unmount()
	{
		os_close(filefd);
		filefd = -1;
		return 0;
	}
	
	FileInfo* IsfsHandler::Open(const char* path, int mode)
	{
		int ret = -1;
		ret = os_open(path, mode + 1); // Conversion from O_RDONLY/O_WRONLY to ISFS_OPEN_READ/ISFS_OPEN_WRITE
		if (ret < 0)
			return null;
		return new IsfsFileInfo(this, ret);
	}
	
	int IsfsHandler::Read(FileInfo* file, u8* buffer, int length)
	{
		return os_read(((IsfsFileInfo*)file)->File, buffer, length);
	}
	
	int IsfsHandler::Write(FileInfo* file, const u8* buffer, int length)
	{
		return os_write(((IsfsFileInfo*)file)->File, buffer, length);
	}
	
	int IsfsHandler::Seek(FileInfo* file, int where, int whence)
	{
		return os_seek(((IsfsFileInfo*)file)->File, where, whence);
	}
	
	int IsfsHandler::Tell(FileInfo* file)
	{
		return os_seek(((IsfsFileInfo*)file)->File, 0, SEEK_CUR);
	}
	
	int IsfsHandler::Sync(FileInfo* file)
	{
		return 0;
	}
	
	int IsfsHandler::Close(FileInfo* file)
	{
		int ret = os_close(((IsfsFileInfo*)file)->File);
		delete file;
		return ret;
	}
} }
