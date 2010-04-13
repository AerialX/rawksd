#include "file_fat.h"

#include <fcntl.h>

namespace ProxiIOS { namespace Filesystem {

static const char __fatName[] = "fat";

static u64 HexToInt(const char* hex, int length)
{
	u64 ret = 0;
	for (int i = 0; i < length; i++)
		ret = (ret << 4) + hex[i] - ((hex[i] > '9') ? ('a' - 10) : '0');
	return ret;
}

static inline FILE_STRUCT* GenerateReadonlyFile(FatHandler* fs, u32 cluster) {
	FILE_STRUCT* ffile = new FILE_STRUCT();
	ffile->filesize = 0xCFFFFFF0;
	ffile->startCluster = cluster;
	ffile->currentPosition = 0;
	ffile->rwPosition.cluster = ffile->startCluster;
	ffile->rwPosition.sector =  0;
	ffile->rwPosition.byte = 0;
	ffile->partition = (PARTITION*)GetDeviceOpTab(fs->Name)->deviceData;
	ffile->read = true;
	ffile->write = false;
	ffile->append = false;
	ffile->inUse = true;
	ffile->modified = false;

	return ffile;
}

inline void StToStats(Stats* stats, struct stat* st) {
	stats->Identifier = st->st_ino;
	stats->Size = st->st_size;
	stats->Device = st->st_dev;
	stats->Mode = st->st_mode;
}

int FatHandler::Mount(const void* options, int length)
{
	if (length < 4)
		return Errors::Unrecognized;

	int disk;
	memcpy(&disk, options, sizeof(int));

	if (length > 4) { // optional forced fs name
		strcpy(Name, (const char*)options + 4);
	} else
		strcpy(Name, __fatName);

	// better performance, but requires a lot more memory
	// if (fatMount(Name, Module->Disk[disk], 0, 12, 16) < 0)
	if (fatMount(Name, Module->Disk[disk], 0, 5, 4) < 0)
		return Errors::DiskNotMounted;

	strcpy(MountPoint, "/mnt/");
	strcat(MountPoint, Name);

	strcat(Name, ":/");

	return Errors::Success;
}

int FatHandler::Unmount()
{
	Name[strlen(Name) - 1] = '\0'; // Get rid of the '/'
	fatUnmount(Name);
	return 0;
}

FileInfo* FatHandler::Open(const char* path, int mode)
{
	chdir(Name);

	int ret = -1;

	if (!strncmp(path, FILE_ID_PATH, FILE_ID_PATH_LEN)) {
		if (mode != O_RDONLY)
			return null;
		u32 cluster = (u32)HexToInt(path + FILE_ID_PATH_LEN, 0x10);
		ret = (int)GenerateReadonlyFile(this, cluster);
	} else
		ret = FAT_Open(path, mode);

	if (ret < 0)
		return null;

	return new FatFileInfo(this, ret);
}

int FatHandler::Read(FileInfo* file, u8* buffer, int length)
{
	return FAT_Read(((FatFileInfo*)file)->File, buffer, length);
}

int FatHandler::Write(FileInfo* file, const u8* buffer, int length)
{
	return FAT_Write(((FatFileInfo*)file)->File, (void*)buffer, length);
}

int FatHandler::Seek(FileInfo* file, int where, int whence)
{
	return FAT_Seek(((FatFileInfo*)file)->File, where, whence);
}

int FatHandler::Tell(FileInfo* file)
{
	return FAT_Tell(((FatFileInfo*)file)->File);
}

int FatHandler::Sync(FileInfo* file)
{
	return FAT_Sync(((FatFileInfo*)file)->File);
}

int FatHandler::Close(FileInfo* file)
{
	int ret = FAT_Close(((FatFileInfo*)file)->File);
	delete file;
	return ret;
}

int FatHandler::Stat(const char* path, Stats* stats)
{
	chdir(Name);

	struct stat st;
	int ret = FAT_Stat(path, &st);

	StToStats(stats, &st);

	return ret;
}

int FatHandler::CreateFile(const char* path)
{
	chdir(Name);

	int fd = FAT_Open(path, O_CREAT);
	if (fd < 0)
		return fd;
	FAT_Close(fd);
	return Errors::Success;
}

int FatHandler::Delete(const char* path)
{
	chdir(Name);

	return FAT_Delete(path);
}

int FatHandler::Rename(const char* path, const char* destination)
{
	chdir(Name);

	return FAT_Rename(path, destination);
}

int FatHandler::CreateDir(const char* path)
{
	chdir(Name);

	return FAT_CreateDir(path);
}

FileInfo* FatHandler::OpenDir(const char* path)
{
	chdir(Name);

	int fd = FAT_OpenDir(path);

	if (fd < 0)
		return null;

	return new FatFileInfo(this, fd);
}

int FatHandler::NextDir(FileInfo* dir, char* filename, Stats* stats)
{
	struct stat st;
	int ret = FAT_NextDir(((FatFileInfo*)dir)->File, filename, &st);

	StToStats(stats, &st);

	return ret;
}

int FatHandler::CloseDir(FileInfo* dir)
{
	int ret = FAT_CloseDir(((FatFileInfo*)dir)->File);
	delete dir;
	return ret;
}

} }
