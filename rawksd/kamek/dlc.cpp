#include "rb2.h"

#include <unistd.h>
#include <fcntl.h>
#include <files.h>

static DataArray* LoadDTB(const char* filename)
{
	MemStream* mem = MemStream::Alloc(true);
	mem->DisableEncryption();
	Stats stat;
	if (File_Stat(filename, &stat))
		return false;
	int fd = File_Open(filename, O_RDONLY);
	if (fd <= 0)
		return false;
	u8* buffer = (u8*)malloc(0x100);
	while (stat.Size > 0) {
		int read = File_Read(fd, buffer, 0x100);
		if (!read)
			break;
		if (read < 0) {
			free(buffer);
			File_Close(fd);
			return NULL;
		}

		mem->Write(buffer, read);
		stat.Size -= read;
	}
	free(buffer);
	File_Close(fd);

	DataArray* dtb;
	mem->Seek(0, BinStream::Start);
	__rs(mem, &dtb);
	return dtb;
}

static void PrintDTB(DataArray* dtb)
{
	String* string = Alloc<String, const char*>("");
	__ls(string, dtb);
	OSReport("--- DTA Start ---\n");
	OSReport(string->c_str());
	OSReport("--- DTA End ---\n");
//	string->Free(); // Why the fuck can't I call destructors without it asploding on me?
}

static void AddDTB(DataArray* song, DataArray* dtb)
{
	// TODO: Disallow any DTAs with Song.Name containing dlc/contents/etc/sZA?/etc
	dtb->Insert(0, Alloc<DataNode, DataArray*, int>(song, 0x10));
}

extern "C" void WiiContentMgrHook(BinStream* stream, DataArray** data)
{
	__rs(stream, data);

	const char* custompath = "/rawk/rb2/customs";
	int fd = File_OpenDir(custompath);
	if (fd <= 0)
		return;
	Stats stats;
	char filename[MAXPATHLEN];
	while (File_NextDir(fd, filename, &stats) >= 0) {
		if (!(stats.Mode & S_IFDIR) || filename[0] == '.')
			continue;
		char dtbname[MAXPATHLEN];
		sprintf(dtbname, "%s/%s/data", custompath, filename);
		if (!File_Stat(dtbname, &stats))
			AddDTB(LoadDTB(dtbname), *data);
	}
	File_CloseDir(fd);
}

extern "C" DataArray* WiiContentMgrInitHook(Symbol* dtbfile, Symbol* nodename)
{
	DataArray* dtb = SystemConfig(dtbfile, nodename);
	char titleid[5];
	strcpy(titleid, "cRB?");
	for (char id = 'Z'; id >= 'A'; id--) {
		titleid[3] = id;
		dtb->Insert(1, Alloc<DataNode, const char*>(titleid));
	}
	return dtb;
}

