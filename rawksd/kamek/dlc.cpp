#include "rb2.h"

#include <unistd.h>
#include <fcntl.h>
#include <files.h>

static DataArray* LoadDTB(const char* filename)
{
	MemStream* mem = Alloc<MemStream, bool>(true);
	mem->DisableEncryption();
	Stats stat;
	if (File_Stat(filename, &stat))
		return NULL;
	int fd = File_Open(filename, O_RDONLY);
	if (fd < 0)
		return NULL;
	u8* buffer = (u8*)memalign(32, 0x1000);
	while (stat.Size > 0) {
		int read = File_Read(fd, buffer, 0x1000);
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
	__rs(mem, dtb);
	return dtb; // TODO: Leaking mem because the destructor crashages
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

static const char* GetSongID(DataArray* song)
{
	const char* ret = !song->size ? NULL : song->nodes[0].Str(song);
	return ret;
}

static const char* GetSongName(DataArray* song)
{
	DataArray* name = song->FindArray(Symbol("song"), Symbol("name"));
	if (!name || name->size < 2)
		return NULL;
	const char* ret = name->nodes[1].Str(name);
	return ret;
}

static const char* GetSongTitle(DataArray* song)
{
	const char* name = GetSongName(song);
	static char title[5];
	strncpy(title, name + 4, 4);
	title[4] = '\0';
	return title;
}

static DataArray* titlearray = NULL;
static DataArray* customs = NULL;
static void AddDTB(DataArray* song)
{
	const char* title = GetSongTitle(song);
	if (!strncmp(title, "sZ", 2))
		return;
	bool found = false;
	for (int i = 0; i < titlearray->size; i++) {
		if (!strcmp(titlearray->nodes[i].Str(titlearray), title)) {
			found = true;
			break;
		}
	}
	if (!found) {
		DataNode node(title);
		titlearray->Insert(0, node);
	}

	DataNode songnode(song, 0x10);
	customs->Insert(0, songnode);
}

void RefreshCustomsList()
{
	File_Init();
	if (!customs)
		customs = Alloc<DataArray, int>(0);
	else {
		while (customs->size)
			customs->Remove(0);
	}

	if (!titlearray)
		titlearray = Alloc<DataArray, int>(0);
	else {
		while (titlearray->size)
			titlearray->Remove(0);
		DataNode crbanode("cRBA");
		titlearray->Insert(0, crbanode);
	}

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
		if (!File_Stat(dtbname, &stats)) {
			DataArray* song = LoadDTB(dtbname);
			if (!strncmp(GetSongID(song), "rwk", 3))
				AddDTB(song);
		}
	}
	File_CloseDir(fd);
}

extern "C" void WiiContentMgrHook(BinStream* stream, DataArray*& dtb)
{
	__rs(stream, dtb);
	for (int i = 0; i < dtb->size; i++) {
		DataNode* node = dtb->nodes + i;
		if (node->type == 0x10 && !strncmp(GetSongID(node->LiteralArray(dtb)), "rwk", 3)) {
			dtb->Remove(i);
			i--;
			continue;
		}
	}

	static bool firstrun = true;
	if (firstrun)
		firstrun = false;
	else
		RefreshCustomsList();

	for (int i = 0; i < customs->size; i++)
		dtb->Insert(0, customs->nodes[i]);
}

extern "C" DataArray* WiiContentMgrInitHook(Symbol dtbfile, Symbol nodename)
{
	RefreshCustomsList();

	DataArray* dtb = SystemConfig(dtbfile, nodename);
	if (titlearray != NULL) {
		for (int i = 0; i < titlearray->size; i++)
			dtb->Insert(1 + 26, titlearray->nodes[i]);
	}
	return dtb;
}

