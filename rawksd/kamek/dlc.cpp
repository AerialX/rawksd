#include "rb2.h"

#include <ogc/machine/processor.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/param.h>
#include <files.h>

static DataArray* LoadDTB(const char* filename)
{
	DataArray* dtb = NULL;
	MemStream* mem = Alloc<MemStream, bool>(true);
	mem->DisableEncryption();
	int fd = File_Open(filename, O_RDONLY);
	if (fd>=0) {
		u8* buffer = (u8*)memalign(32, 0x8000);
		if (buffer) {
			int read;
			do {
				read = File_Read(fd, buffer, 0x8000);
				if (read > 0)
					mem->Write(buffer, read);
			} while (read == 0x8000);

			free(buffer);
			mem->Seek(0, BinStream::Start);
			//dtb->Load(mem);
			__rs(mem, dtb);
		}
		File_Close(fd);
	}

	mem->~MemStream();
	return dtb;
}

static DataNode* GetSongDataNode(DataArray* song, const char* id)
{
	DataArray* dtb = song->FindArray(Symbol(id), false);
	if (!dtb)
		return NULL;
	return dtb->nodes + 1;
}

static const char* GetSongID(DataArray* song)
{
	const char* ret = !song->size ? NULL : song->nodes[0].Str(song);
	return ret;
}

static const char* GetSongName(DataArray* song)
{
	DataArray* name = song->FindArray(Symbol("song"), Symbol("name"));
	const char* ret = NULL;
	if (name)
		ret = name->nodes[1].Str(NULL);
	return ret;
}

static const char* GetSongPack(DataArray* song)
{
	DataNode* node = GetSongDataNode(song, "pack_name");
	if (!node)
		return NULL;
	return node->Str(NULL);
}

static bool GetSongDownloaded(DataArray* song)
{
	DataNode* node = GetSongDataNode(song, "downloaded");
	if (!node)
		return false;
	return node->Int(NULL);
}

static const char* GetSongTitle(DataArray* song)
{
	static char title[5];
	const char* name = GetSongName(song);
	if (name) {
		strncpy(title, name + 4, 4);
		title[4] = 0;
	} else
		title[0]= 0;
	return title;
}

static DataArray* titlearray = NULL;
static DataArray* customs = NULL;
static void AddToTitleArray(DataArray* song)
{
	const char* title = GetSongTitle(song);
	if (title[0]=='s' && title[1]=='Z')
		return;

	for (int i = 0; i < titlearray->size; i++) {
		if (!strcmp(titlearray->nodes[i].Str(titlearray), title))
			return;
	}

	DataNode node(title);
	titlearray->Insert(0, node);
}

static void AddDTB(DataArray* song)
{
	AddToTitleArray(song);
	DataNode songnode(song, 0x10);
	customs->Insert(0, songnode);
}

static void RefreshCustomsList()
{
	if (!customs) {
		customs = Alloc<DataArray, int>(0);
	} else {
		while (customs->size)
			customs->Remove(0);
	}
	if (!titlearray) {
		titlearray = Alloc<DataArray, int>(0);
		DataNode crbanode("cRBA");
		titlearray->Insert(0, crbanode);
	}

	STACK_ALIGN(char, custompath, MAXPATHLEN + 20, 0x20);
	strcpy(custompath + 2, "/rawk/rb2/customs/data");

	DataArray* songs = LoadDTB(custompath + 2);
	if (songs) {
		/*while (songs->size) {
			if (songs->nodes->type == 0x10) {
				DataArray* song = songs->nodes->LiteralArray(songs);
				const char* id = GetSongID(song);
				if (id && !strncmp(id, "rwk", 3)) {
					AddToTitleArray(song);
					customs->Insert(0, songs->nodes[0]);
				}
			}
			songs->Remove(0);
		}*/
		for (int i = 0; i < songs->size; i++) {
			if (songs->nodes[i].type == 0x10) {
				DataArray* song = songs->nodes[i].LiteralArray(songs);
				const char* id = GetSongID(song);
				if (id && !strncmp(id, "rwk", 3)) {
					AddToTitleArray(song);
					customs->Insert(0, songs->nodes[i]);
				}
			}
		}
		/*while (songs->size) {
			if (songs->nodes->type == 0x10) {
				DataArray* song = songs->nodes->LiteralArray(songs);
				const char* id = GetSongID(song);
				if (id && !strncmp(id, "rwk", 3)) {
					AddToTitleArray(song);
					customs->Insert(0, songs->nodes[0]);
				}
			}
			songs->Remove(0);
		}*/
		songs->TryDestruct();
	} else {
		custompath[20] = '\0';
		int fd = File_OpenDir(custompath + 2);
		if (fd <= 0)
			return;
		Stats stats;
		while (File_NextDir(fd, custompath + 20, &stats) >= 0) {
			if (!(stats.Mode & S_IFDIR) || custompath[20] == '.')
				continue;
			strcat(custompath + 2, "/data");
			DataArray* song = LoadDTB(custompath + 2);
			if (song) {
				const char* id = GetSongID(song);
				if (id && !strncmp(id, "rwk", 3))
					AddDTB(song);
				song->TryDestruct();
			}
		}
		File_CloseDir(fd);
	}
}

extern "C" void WiiContentMgrHook(BinStream* stream, DataArray*& dtb)
{
	__rs(stream, dtb);
	if (!dtb) {
		if (!customs->size)
			return;
		else
			dtb = Alloc<DataArray, int>(0);
	} else {
		for (int i = 0; i < dtb->size; i++) {
			DataNode* node = dtb->nodes + i;
			if (node->type == 0x10 && !strncmp(GetSongID(node->LiteralArray(dtb)), "rwk", 3)) {
				dtb->Remove(i);
				i--;
			}
		}
	}

	static bool firstrun = true;
	if (!firstrun)
		RefreshCustomsList();
	firstrun = false;

	for (int i = 0; i < customs->size; i++)
		dtb->Insert(0, customs->nodes[i]);
}

extern "C" DataArray* WiiContentMgrInitHook(Symbol dtbfile, Symbol nodename)
{
	File_Init();

	RefreshCustomsList();

	DataArray* dtb = SystemConfig(dtbfile, nodename);
	for (int i = 0; i < titlearray->size; i++)
		dtb->Insert(1 + 26, titlearray->nodes[i]);
	return dtb;
}

extern "C" Symbol SongOfferGetIconHook(SongOffer* offer)
{
	if (!offer->data)
		return Symbol("");
	const char* songid = GetSongID(offer->data);
	if (!songid || strncmp(songid, "rwk", 3))
		return GetSongDownloaded(offer->data) ? Symbol("download") : Symbol("rb2_icon");
	const char* pack = GetSongPack(offer->data);
	if (!pack || !pack[0])
		return Symbol("Custom Songs");
	return Symbol(pack);
}

extern "C" Symbol SongOfferProviderGetIconHook(SongOfferProvider* provider, int index)
{
	const char* symbol = provider->DataSymbol(index);
	if (symbol && !strncmp(symbol, "rwk", 3)) {
		if (symbol[4] == 'r' && symbol[5] == 'b')
			return Symbol("rb1_icon");
		return Symbol("download");
	}
	Symbol song(symbol);
	DataArray* data = gSongMgrWii.Data(song);
	if (data && GetSongDownloaded(data))
		return Symbol("download");
	return Symbol("rb2_icon");
}

extern "C" int StoreSortOriginCmpHook(void* storesort, SongOffer* offer1, SongOffer* offer2)
{
	const char* icon1 = SongOfferGetIconHook(offer1).name;
	const char* icon2 = SongOfferGetIconHook(offer2).name;
	int cmp = strcmp(icon1, icon2);
	if (!cmp)
		return cmp;
	return cmp < 0 ? -1 : 1;
}

