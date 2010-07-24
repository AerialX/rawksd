#pragma once

#include "filemodule.h"

#include "wrapper.h"

#define FAT_IDLE_TIME 4*1000*1000 // 4 seconds

namespace ProxiIOS { namespace Filesystem {

struct FatFileInfo : public FileInfo
{
	FatFileInfo(FilesystemHandler* system, int fd) : FileInfo(system) { File = fd; }

	int File;
};

class FatHandler : public FilesystemHandler
{
protected:
	int IdleCount;
	int phys;

public:
	char Name[0x20];

	FatHandler(Filesystem* fs) : FilesystemHandler(fs) {
		IdleCount = -1;
		phys = -1;
	}

	int Mount(const void* options, int length);
	int Unmount();
	int CheckPhysical();

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
	int Rename(const char* source, const char* destination);
	int CreateDir(const char* path);
	FileInfo* OpenDir(const char* path);
	int NextDir(FileInfo* dir, char* filename, Stats* st);
	int CloseDir(FileInfo* dir);
	int IdleTick();
	int GetFreeSpace(u64 *free_bytes);
};

} }
