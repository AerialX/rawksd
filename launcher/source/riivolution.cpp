#include "riivolution.h"
#include "riivolution_config.h"
#include "launcher.h"

#include <files.h>

#include <unistd.h>

#include <wdvd.h>

#define OPEN_MODE_BYPASS 0x80

static int fd = -1;
static DiscNode* fst = NULL;
static bool shiftfiles = false;
static u64 shift = 0;
static u32 fstsize;

using std::string;
using std::map;
using std::vector;

namespace Ioctl { enum Enum {
	AddFile			= 0xC1,
	AddPatch		= 0xC2,
	AddShift		= 0xC3,
	SetClusters		= 0xC4,
	Allocate		= 0xC5,
	AddEmu			= 0xC6,
	SetShiftBase	= 0xC8
}; }

static u32 ioctlbuffer[0x08] ATTRIBUTE_ALIGN(32);

int RVL_Initialize()
{
	if (fd < 0)
		fd = IOS_Open("/dev/do", OPEN_MODE_BYPASS);
	shift = 0x200000000ULL; // 8 GB shift
	return fd;
}

void RVL_Close()
{
	if (fd < 0)
		return;
	IOS_Close(fd);
}

void* RVL_GetFST()
{
	return fst;
}

void RVL_SetFST(void* address, u32 size)
{
	fst = (DiscNode*)address;
	fstsize = size;

	if (fst != NULL && size) {
		const void* nametable = (const void*)(fst + fst->Size);
		u32 offset = 0;
		for (DiscNode* node = fst; node < nametable; node++) {
			if (!node->Type)
				offset = MAX(offset, node->DataOffset);
		}
		shift = (u64)offset << 2;
		RVL_GetShiftOffset(0); // Round up

		RVL_SetShiftBase(shift);
	}
}

u32 RVL_GetFSTSize()
{
	return fstsize;
}

int RVL_SetClusters(bool clusters)
{
	ioctlbuffer[0] = clusters;
	return IOS_Ioctl(fd, Ioctl::SetClusters, ioctlbuffer, 4, NULL, 0);
}

int RVL_SetShiftBase(u64 shift)
{
	ioctlbuffer[0] = shift >> 32;
	ioctlbuffer[1] = shift;
	return IOS_Ioctl(fd, Ioctl::SetShiftBase, ioctlbuffer, 8, NULL, 0);
}

void RVL_SetAlwaysShift(bool shift)
{
	shiftfiles = shift;
}

int RVL_Allocate(PatchType::Enum type, int num)
{
	ioctlbuffer[0] = type;
	ioctlbuffer[1] = num;
	return IOS_Ioctl(fd, Ioctl::Allocate, ioctlbuffer, 8, NULL, 0);
}

int RVL_AddFile(const char* filename)
{
	return IOS_Ioctl(fd, Ioctl::AddFile, (void*)filename, strlen(filename) + 1, NULL, 0);
}

int RVL_AddFile(const char* filename, u64 identifier)
{
	return IOS_Ioctl(fd, Ioctl::AddFile, (void*)filename, strlen(filename) + 1, &identifier, 8);
}

int RVL_AddShift(u64 original, u64 offset, u32 length)
{
	ioctlbuffer[0] = length;
	ioctlbuffer[1] = original >> 32;
	ioctlbuffer[2] = original;
	ioctlbuffer[3] = offset >> 32;
	ioctlbuffer[4] = offset;
	return IOS_Ioctl(fd, Ioctl::AddShift, ioctlbuffer, 0x20, NULL, 0);
}

int RVL_AddPatch(int file, u64 offset, u32 fileoffset, u32 length)
{
	ioctlbuffer[0] = file;
	ioctlbuffer[1] = fileoffset;
	ioctlbuffer[2] = offset >> 32;
	ioctlbuffer[3] = offset;
	ioctlbuffer[4] = length;
	return IOS_Ioctl(fd, Ioctl::AddPatch, ioctlbuffer, 0x20, NULL, 0);
}

int RVL_AddEmu(const char* nandpath, const char* external)
{
	return IOS_Ioctl(fd, Ioctl::AddEmu, (void*)nandpath, strlen(nandpath) + 1, (void*)external, strlen(external) + 1);
}

#define ROUND_UP(p, round) \
	((p + round - 1) & ~(round - 1))

u64 RVL_GetShiftOffset(u32 length)
{
	u64 ret = shift;
	shift = ROUND_UP(shift + length, 0x40);
	return ret;
}

#define REFRESH_PATH() { \
	strcpy(curpath, "/"); \
	if (nodes.size() > 1) \
		for (DiscNode** iter = nodes.begin() + 1; iter != nodes.end(); iter++) { \
			strcat(curpath, nametable + (*iter)->GetNameOffset()); \
			strcat(curpath, "/"); \
		} \
	pos = strlen(curpath); \
}

#if 0 // Old filenode
DiscNode* RVL_FindNode(const char* fstname)
{
	const char* nametable = (const char*)(fst + fst->Size);
	
	u32 count = 1;
	DiscNode*> nodes;
	char curpath[MAXPATHLEN];
	int pos = 0;
	
	REFRESH_PATH();
	
	for (DiscNode* node = fst; (void*)node < (void*)nametable; node++, count++) {
		const char* name = nametable + node->GetNameOffset();
		
		if (node->Type == 0x01) {
			nodes.push_back(node);
			
			REFRESH_PATH();
		} else if (node->Type == 0x00) {
			curpath[pos] = '\0'; // Null-terminate it to the path
			strcat(curpath, name);
			
			if (!strcasecmp(curpath, fstname) || !strcasecmp(name, fstname))
				return node;
		}
		
		while (nodes.size() > 0 && count == nodes.back()->Size) {
			nodes.pop_back();
			
			REFRESH_PATH();
		}
	}
	
	return NULL;
}
#endif

static DiscNode* RVL_FindNode(const char* name, DiscNode* root, bool recursive)
{
	const char* nametable = (const char*)(fst + fst->Size);
	int offset = root - fst;
	DiscNode* node = root;
	while ((void*)node < (void*)nametable) {
		if (!strcasecmp(nametable + node->GetNameOffset(), name))
			return node;

		if (recursive || node->Type == 0)
			node++;
		else
			node = root + node->Size - offset;
	}

	return NULL;
}

DiscNode* RVL_FindNode(const char* fstname)
{
	if (fstname[0] != '/') {
		if (!strcasecmp(fstname, "main.dol")) {
			static DiscNode maindol;
			maindol.Size = 0;
			maindol.Type = 0;
			maindol.SetNameOffset(0);
			maindol.DataOffset = Launcher_GetFstData()[1];
			return &maindol;
		}

		return RVL_FindNode(fstname, fst, true);
	}

	char namebuffer[MAXPATHLEN];
	char* name = namebuffer;
	strcpy(name, fstname + 1);

	DiscNode* root = fst;

	while (root) {
		char* slash = strchr(name, '/');
		if (!slash)
			return RVL_FindNode(name, root + 1, false);

		*slash = '\0';
		root = RVL_FindNode(name, root + 1, false);
		name = slash + 1;
	}

	return NULL;
}

static void ShiftFST(DiscNode* node)
{
	node->DataOffset = (u32)(RVL_GetShiftOffset(node->Size) >> 2);
}

static void ResizeFST(DiscNode* node, u32 length, bool telldip)
{
	u64 oldoffset = (u64)node->DataOffset << 2;
	u32 oldsize = node->Size;
	node->Size = length;

	if (length > oldsize || shiftfiles)
		ShiftFST(node);

	if (telldip)
		RVL_AddShift(oldoffset, (u64)node->DataOffset << 2, oldsize);
}

static void RVL_Patch(RiiFilePatch* file, bool stat, u64 externalid, string commonfs)
{
	DiscNode* node = RVL_FindNode(file->Disc.c_str());

	if (!node) {
		if (file->Create) {
			// TODO: Add FST entries
			return;
		} else
			return;
	}

	if (node->Type)
		return; // Patching a directory will not end well.

	string external = file->External;
	if (commonfs.size() && !commonfs.compare(0, commonfs.size(), external))
		external = external.substr(commonfs.size());

	if (!stat && file->Length == 0) {
		Stats st;
		if (File_Stat(external.c_str(), &st) == 0) {
			file->Length = st.Size - file->FileOffset;
			stat = true;
			externalid = st.Identifier;
		} else
			return;
	}

	int fd;
	if (stat)
		fd = RVL_AddFile(external.c_str(), externalid);
	else
		fd = RVL_AddFile(external.c_str());

	if (fd >= 0) {
		bool shifted = false;

		if (file->Resize) {
			if (file->Length + file->Offset > node->Size) {
				ResizeFST(node, file->Length + file->Offset, file->Offset > 0);
				shifted = true;
			} else
				node->Size = file->Length + file->Offset;
		}

		if (!shifted && shiftfiles)
			ShiftFST(node);

		RVL_AddPatch(fd, ((u64)node->DataOffset << 2) + file->Offset, file->FileOffset, file->Length);
	}
}

static void RVL_Patch(RiiFilePatch* file, string commonfs)
{
	RVL_Patch(file, false, 0, commonfs);
}

static void RVL_Patch(RiiFolderPatch* folder, string commonfs)
{
	string external = folder->External;
	if (commonfs.size() && !commonfs.compare(0, commonfs.size(), external))
		external = external.substr(commonfs.size());

	char fdirname[MAXPATHLEN];
	int fdir = File_OpenDir(external.c_str());
	if (fdir < 0)
		return;
	Stats stats;
	while (!File_NextDir(fdir, fdirname, &stats)) {
		if (fdirname[0] == '.')
			continue;
		if (stats.Mode & S_IFDIR) {
			if (folder->Recursive){
				RiiFolderPatch newfolder = *folder;
				newfolder.Disc = PathCombine(newfolder.Disc, fdirname);
				newfolder.External = PathCombine(external, fdirname);
				RVL_Patch(&newfolder, commonfs);
			}
		} else {
			RiiFilePatch file;
			file.Create = folder->Create;
			file.Resize = folder->Resize;
			file.Offset = 0;
			file.FileOffset = 0;
			file.Length = stats.Size;
			file.Disc = PathCombine(folder->Disc, fdirname);
			file.External = PathCombine(external, fdirname);
			RVL_Patch(&file, true, stats.Identifier, commonfs);
		}
	}
	File_CloseDir(fdir);
}

static void RVL_Patch(RiiSavegamePatch* save, string commonfs)
{
	string external = save->External;
	if (commonfs.size() && !commonfs.compare(0, commonfs.size(), external))
		external = external.substr(commonfs.size());

	char nandpath[0x40];
	sprintf(nandpath, "/title/%08x/%08x/data", (int)(WDVD_GetTMD()->title_id >> 32), (int)WDVD_GetTMD()->title_id);
	RVL_AddEmu(nandpath, external.c_str());
}

static void ApplyParams(std::string* str, map<string, string>* params)
{
	bool found = false;
	string::size_type pos;
	while ((pos = str->find("{$")) != string::npos) {
		string::size_type pend = str->find("}", pos);
		string paramname = str->substr(pos + 2, pend - pos - 2);
		map<string, string>::iterator param = params->find(paramname);
		if (param == params->end())
			paramname = "";
		else
			paramname = param->second;
		*str = str->substr(0, pos) + paramname + str->substr(pend + 1);
		found = true;
	}
}

static void RVL_Patch(RiiPatch* patch, map<string, string>* params, string commonfs)
{
	for (vector<RiiFilePatch>::iterator file = patch->Files.begin(); file != patch->Files.end(); file++) {
		RiiFilePatch temp = *file;
		ApplyParams(&temp.External, params);
		ApplyParams(&temp.Disc, params);
		RVL_Patch(&temp, commonfs);
	}
	for (vector<RiiFolderPatch>::iterator folder = patch->Folders.begin(); folder != patch->Folders.end(); folder++) {
		RiiFolderPatch temp = *folder;
		ApplyParams(&temp.External, params);
		ApplyParams(&temp.Disc, params);
		RVL_Patch(&temp, commonfs);
	}
	for (vector<RiiSavegamePatch>::iterator save = patch->Savegames.begin(); save != patch->Savegames.end(); save++) {
		RiiSavegamePatch temp = *save;
		ApplyParams(&temp.External, params);
		RVL_Patch(&temp, commonfs);
	}
}

void RVL_Patch(RiiDisc* disc)
{
	// Search for a common filesystem so we can optimize memory
	string filesystem;
	for (vector<RiiSection>::iterator section = disc->Sections.begin(); section != disc->Sections.end(); section++) {
		for (vector<RiiOption>::iterator option = section->Options.begin(); option != section->Options.end(); option++) {
			if (option->Default == 0)
				continue;
			RiiChoice* choice = &option->Choices[option->Default - 1];
			if (!filesystem.size()) {
				filesystem = choice->Filesystem;
			}
			else if (filesystem != choice->Filesystem) {
				filesystem = string();
				goto no_common_fs;
			}
		}
	}
no_common_fs:

	if (filesystem.size()) {
		File_SetDefaultPath(filesystem.c_str());
		RVL_SetClusters(true);
	} else
		RVL_SetClusters(false);

	for (vector<RiiSection>::iterator section = disc->Sections.begin(); section != disc->Sections.end(); section++) {
		for (vector<RiiOption>::iterator option = section->Options.begin(); option != section->Options.end(); option++) {
			if (option->Default == 0)
				continue;
			map<string, string> params;
			params.insert(option->Params.begin(), option->Params.end());
			RiiChoice* choice = &option->Choices[option->Default - 1];
			params.insert(choice->Params.begin(), choice->Params.end());
			u32 end = params.size();
			for (vector<RiiChoice::Patch>::iterator patch = choice->Patches.begin(); patch != choice->Patches.end(); patch++) {
				map<string, RiiPatch>::iterator currentpatch = disc->Patches.find(patch->ID);
				if (currentpatch != disc->Patches.end()) {
					params.insert(patch->Params.begin(), patch->Params.end());

					RVL_Patch(&currentpatch->second, &params, filesystem);

					if (patch->Params.size()) {
						map<string, string>::iterator endi = params.begin();
						for (u32 i = 0; i < end; i++) // Fucking iterator needs operator+()
							endi++;
						params.erase(endi, patch->Params.end());
					}
				}
			}
		}
	}
}

static void RVL_Patch(RiiMemoryPatch* memory)
{
	if (memory->Original && memcmp((void*)memory->Offset, memory->Original, memory->Length))
		return;

	if (memory->Value && memory->Length)
		memcpy((void*)memory->Offset, memory->Value, memory->Length);
}

void RVL_PatchMemory(RiiDisc* disc)
{
	for (vector<RiiSection>::iterator section = disc->Sections.begin(); section != disc->Sections.end(); section++) {
		for (vector<RiiOption>::iterator option = section->Options.begin(); option != section->Options.end(); option++) {
			if (option->Default == 0)
				continue;
			RiiChoice* choice = &option->Choices[option->Default - 1];
			for (vector<RiiChoice::Patch>::iterator patch = choice->Patches.begin(); patch != choice->Patches.end(); patch++) {
				RiiPatch* mem = &disc->Patches[patch->ID];
				for (vector<RiiMemoryPatch>::iterator memory = mem->Memory.begin(); memory != mem->Memory.end(); memory++) {
					RVL_Patch(&*memory);
				}
			}
		}
	}
}
