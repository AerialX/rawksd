#include "file_fat.h"

namespace ProxiIOS { namespace Filesystem {

static const char __fatName[] = "fat";

static const char HEX_CHARS[] = "0123456789abcdef";
static int HexToInt(const char* hex, int length)
{
	int ret = 0;
	for (int i = length - 1; i >= 0; i--)
		ret |= ((int)strchr(HEX_CHARS, hex[i]) - (int)HEX_CHARS) << (4*(length - 1 - i));
	return ret;
}

inline FILE_STRUCT* GenerateReadonlyFile(u32 cluster) {
	FILE_STRUCT* ffile = new FILE_STRUCT();
	ffile->filesize = 0xCFFFFFF0;
	ffile->startCluster = cluster;
	ffile->currentPosition = 0;
	ffile->rwPosition.cluster = ffile->startCluster;
	ffile->rwPosition.sector =  0;
	ffile->rwPosition.byte = 0;
	ffile->partition = (PARTITION*)GetDeviceOpTab(__fatName)->deviceData;
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

static const char FAT_SIG[3] = {'F', 'A', 'T'};
bool Fat_Mount(const char *point, const DISC_INTERFACE *dev)
{
	int result = -1;
	u8 *sectorBuffer = (u8*)Memalign(32, 2048);
	u8 *bootBuffer = (u8*)Memalign(32, 2048);
	if (dev->readSectors(0,1,sectorBuffer)) {
		u32 lba = 0;
		if ((sectorBuffer[0x1FE]==0x55) && (sectorBuffer[0x1FF]==0xAA)) {
			int i;
			u8 *ptr = sectorBuffer+0x1BE;
			for(i=0;i<4;i++,ptr+=16) {
				lba = ptr[8]  | ptr[9]<<8 | ptr[10] << 16 | ptr[11] << 24;
				if (dev->readSectors(lba,1,bootBuffer)) {
					if ((bootBuffer[0x1FE]==0x55) && (bootBuffer[0x1FF]==0xAA)) {
						if (!memcmp(bootBuffer+0x36,FAT_SIG,sizeof(FAT_SIG)))
							break;
						if (!memcmp(bootBuffer+0x52,FAT_SIG,sizeof(FAT_SIG)))
							break;
					}
				}
				lba=0;
			}
		}
		if(lba)
			result = fatMount(point, dev, lba, 8, 4);
	}
	Dealloc(bootBuffer);
	Dealloc(sectorBuffer);
	return (result>=0);
}

FilesystemInfo* FatHandler::Mount(DISC_INTERFACE* disk)
{
	if (!Fat_Mount(__fatName, disk))
		return null;
	
	chdir("fat:/");
	
	return new FatFilesystemInfo(this, __fatName);
}

int FatHandler::Unmount(FilesystemInfo* filesystem)
{
	fatUnmount(((FatFilesystemInfo*)filesystem)->Name);
	return 0;
}

FileInfo* FatHandler::Open(FilesystemInfo* filesystem, const char* path, u8 mode)
{
	int ret = -1;
	if (strstr(path, "id\\")) {
		if (mode & (O_CREAT | O_TRUNC | O_WRONLY))
			return null; // Cluster-opened files must be read-only
		// strlen("cluster\\") == 8
		u32 cluster = HexToInt(path + 3, 8);
		ret = (int)GenerateReadonlyFile(cluster);
	} else {
		ret = FAT_Open(path, mode);
	}
	if (ret < 0)
		return (FileInfo*)-1;
	FatFileInfo* file = new FatFileInfo();
	file->System = filesystem;
	file->File = (FILE_STRUCT*)ret;
	return file;
}

int FatHandler::Read(FileInfo* file, u8* buffer, int length)
{
	return FAT_Read((int)((FatFileInfo*)file)->File, buffer, length);
}

int FatHandler::Write(FileInfo* file, const u8* buffer, int length)
{
	return FAT_Write((int)((FatFileInfo*)file)->File, (void*)buffer, length);
}

int FatHandler::Seek(FileInfo* file, int where, int whence)
{
	return FAT_Seek((int)((FatFileInfo*)file)->File, where, whence);
}

int FatHandler::Tell(FileInfo* file)
{
	return FAT_Tell((int)((FatFileInfo*)file)->File);
}

int FatHandler::Sync(FileInfo* file)
{
	return FAT_Sync((int)((FatFileInfo*)file)->File);
}

int FatHandler::Close(FileInfo* file)
{
	int ret = FAT_Close((int)((FatFileInfo*)file)->File);
	delete file;
	return ret;
}

int FatHandler::Stat(const char* filename, Stats* stats)
{
	struct stat st;
	int ret = FAT_Stat(filename, &st);
	
	StToStats(stats, &st);
	
	return ret;
}

int FatHandler::CreateFile(const char* filename)
{
	int fd = FAT_Open(filename, O_CREAT);
	if (fd < 0)
		return fd;
	FAT_Close(fd);
	return Errors::Success;
}

int FatHandler::Delete(const char* filename)
{
	return FAT_Delete(filename);
}

int FatHandler::Rename(const char* source, const char* destination)
{
	return FAT_Rename(source, destination);
}

int FatHandler::CreateDir(const char* dirname)
{
	return FAT_CreateDir(dirname);
}

int FatHandler::OpenDir(const char* dirname)
{
	return FAT_OpenDir(dirname);
}

int FatHandler::NextDir(int dir, char* filename, Stats* stats)
{
	struct stat st;
	int ret = FAT_NextDir(dir, filename, &st);
	
	StToStats(stats, &st);
	
	return ret;
}

int FatHandler::CloseDir(int dir)
{
	return FAT_CloseDir(dir);
}

} }
