#include "file_isfs.h"

#include <syscalls.h>

namespace ProxiIOS { namespace Filesystem {
	FilesystemInfo* IsfsHandler::Mount(DISC_INTERFACE* disk)
	{
		filefd = os_open("/dev/fs", 0);
		if (filefd < 0)
			return null;

		return new IsfsFilesystemInfo(this);
	}

	int IsfsHandler::Unmount(FilesystemInfo* filesystem)
	{
		os_close(filefd);
		filefd = -1;
		return 0;
	}

	FileInfo* IsfsHandler::Open(FilesystemInfo* filesystem, const char* path, u8 mode)
	{
		int ret = -1;
		ret = os_open(path, mode);
		if (ret < 0)
			return null;
		IsfsFileInfo* file = new IsfsFileInfo();
		file->System = filesystem;
		file->File = ret;
		return file;
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

	int IsfsHandler::Stat(const char* filename, Stats* st)
	{
		return -1;
	}

	int IsfsHandler::CreateFile(const char* filename)
	{
		return -1;
	}

	int IsfsHandler::Delete(const char* filename)
	{
		return -1;
	}

	int IsfsHandler::Rename(const char* source, const char* destination)
	{
		return -1;
	}

	int IsfsHandler::CreateDir(const char* dirname)
	{
		return -1;
	}

	int IsfsHandler::OpenDir(const char* dirname)
	{
		return -1;
	}

	int IsfsHandler::NextDir(int dir, char* filename, Stats* st)
	{
		return -1;
	}

	int IsfsHandler::CloseDir(int dir)
	{
		return -1;
	}

} }
