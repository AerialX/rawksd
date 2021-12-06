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

	int IsfsHandler::CheckPhysical()
	{
		return filefd;
	}

	FileInfo* IsfsHandler::Open(const char* path, int mode)
	{
		int ret = -1;
		ret = os_open(path, mode + 1); // Conversion from O_RDONLY/O_WRONLY to ISFS_OPEN_READ/ISFS_OPEN_WRITE
		if (ret < 0)
			return NULL;
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

	int IsfsHandler::Close(FileInfo* file)
	{
		int ret = os_close(((IsfsFileInfo*)file)->File);
		delete file;
		return ret;
	}

	int IsfsHandler::Stat(const char* _path, Stats* st)
	{
		s32 result;
		char path[ISFS_MAXPATH_LEN];
		ISFS::Stats *isfs_stats;
		u32 num_contents; // for checking if it's a directory
		ioctlv vec[2];

		path[ISFS_MAXPATH_LEN-1] = 0;
		strncpy(path, _path, ISFS_MAXPATH_LEN-1);

		if (path[strlen(path)-1] == '/')
			path[strlen(path)-1] = 0;

		vec[0].data = (void*)path;
		vec[0].len = ISFS_MAXPATH_LEN;
		vec[1].data = &num_contents;
		vec[1].len = sizeof(u32);
		os_sync_after_write(path, ISFS_MAXPATH_LEN);
		s32 type = os_ioctlv(filefd, ISFS::ReadDir, 1, 1, vec);
		if (type==0) {
			st->Mode = S_IFDIR|S_IFREG;
			st->Size = 0;
		} else if (type==-101) { // invalid argument = file, not directory
			st->Mode = S_IFREG;

			s32 fd = os_open(path, 1);
			if (fd<0)
				return -1;

			isfs_stats = (ISFS::Stats*)Memalign(32, sizeof(ISFS::Stats));
			isfs_stats->Length = 0;

			result = os_ioctl(fd, ISFS::GetFileStats, NULL, 0, isfs_stats, 8);
			os_close(fd);
			if (result < 0)
				return -1;
			st->Size = isfs_stats->Length;
			Dealloc(isfs_stats);
		} else
			return -1;

		st->Identifier = 0;
		st->Device = 0;

		return 0;
	}

	int IsfsHandler::CreateFile(const char* path)
	{
		ISFS::FSattr attrib;

		attrib.path[ISFS_MAXPATH_LEN-1] = 0;
		strncpy(attrib.path, path, ISFS_MAXPATH_LEN-1);
		attrib.ownerperm = 3;
		attrib.groupperm = 1;
		attrib.otherperm = 1;
		attrib.attributes = 0;

		os_sync_after_write(&attrib, sizeof(attrib));
		return os_ioctl(filefd, ISFS::CreateFile, &attrib, sizeof(attrib), NULL, 0);
	}

	int IsfsHandler::Delete(const char* path)
	{
		return os_ioctl(filefd, ISFS::Delete, path, ISFS_MAXPATH_LEN, NULL, 0);
	}

	IsfsDirInfo::IsfsDirInfo(FilesystemHandler* system, const char *_path, u32 count, int &filefd) : FileInfo(system)
	{
		ioctlv vec[4];
		path = (char*)Alloc(ISFS_MAXPATH_LEN);
		path[ISFS_MAXPATH_LEN-1] = 0;
		strncpy(path, _path, ISFS_MAXPATH_LEN-1);

		dir_names = (u8*)Memalign(32, (count+1)*(NANDFILE_MAX+1));
		dir_names[0] = 0;
		next_name = (char*)dir_names;

		vec[0].data = path;
		vec[0].len = ISFS_MAXPATH_LEN;
		vec[1].data = &count;
		vec[1].len = sizeof(count);
		vec[2].data = dir_names;
		vec[2].len = count*(NANDFILE_MAX+1);
		vec[3].data = &dir_count;
		vec[3].len = sizeof(dir_count);

		os_sync_after_write(path, ISFS_MAXPATH_LEN);
		os_sync_after_write(&count, sizeof(count));
		os_ioctlv(filefd, ISFS::ReadDir, 2, 2, vec);
	}

	IsfsDirInfo::~IsfsDirInfo()
	{
		Dealloc(dir_names);
		Dealloc(path);
	}

	FileInfo* IsfsHandler::OpenDir(const char* _path)
	{
		char path[ISFS_MAXPATH_LEN];
		ioctlv vec[2];
		u32 count = 0;
		s32 result;

		path[ISFS_MAXPATH_LEN-1] = 0;
		strncpy(path, _path, ISFS_MAXPATH_LEN-1);

		// remove any trailing slash
		if (path[strlen(path)-1] == '/')
			path[strlen(path)-1] = 0;

		vec[0].data = path;
		vec[0].len = ISFS_MAXPATH_LEN;
		vec[1].data = &count;
		vec[1].len = sizeof(count);

		os_sync_after_write(path, ISFS_MAXPATH_LEN);
		result = os_ioctlv(filefd, ISFS::ReadDir, 1, 1, vec);
		if (result < 0)
			return NULL;

		return new IsfsDirInfo(this, path, count, filefd);
	}

	int IsfsHandler::NextDir(FileInfo* _dir, char* dirname, Stats* st)
	{
		char full_path[ISFS_MAXPATH_LEN];
		IsfsDirInfo *dir = (IsfsDirInfo*)_dir;
		if (!dir->dir_count)
			return -1;

		memcpy(dirname, dir->next_name, (NANDFILE_MAX+1+3)&~3);
		os_sync_after_write(dirname, NANDFILE_MAX+1);
		dir->next_name += strlen(dir->next_name)+1;
		dir->dir_count--;

		strcpy(full_path, dir->path);
		strcat(full_path, "/");
		strcat(full_path, dirname);

		if (st==NULL)
			return 0;
		return Stat(full_path, st);
	}

	int IsfsHandler::CloseDir(FileInfo* dir)
	{
		delete (IsfsDirInfo*)dir;
		return 1;
	}

} }
