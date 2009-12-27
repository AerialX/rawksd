#include "translatios.h"
#include <vector>
#include <ogc/ipc.h>
#include <gccore.h>
#include <unistd.h>

#include <files.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <files.h>
#include <stdio.h>

using namespace std;

namespace Ioctl {
	enum Enum {
		AddFile			= 0xC1,
		AddPatch		= 0xC2,
		AddShift		= 0xC3,
		SetClusters		= 0xC4,
		Allocate		= 0xC5,
		Yarr_Enable		= 0xCA,
		Yarr_AddIso		= 0xCB
	};
}

static void* fstaddress;
static u32 shiftoffset = 0;
static int dipfd = -1;
static u32 Command[8] ATTRIBUTE_ALIGN(32);
static u32 Output[8] ATTRIBUTE_ALIGN(32);

#define ROUND_UP(p, round) \
	((p + round - 1) & ~(round - 1))

#define REFRESH_PATH() { \
	strcpy(curpath, "/"); \
	if (nodes.size() > 1) \
		for (vector<DiscNode*>::iterator iter = nodes.begin() + 1; iter != nodes.end(); iter++) { \
			strcat(curpath, nametable + (*iter)->GetNameOffset()); \
			strcat(curpath, "/"); \
		} \
	pos = strlen(curpath); \
}

void InitializePatcher(int fd)
{
	dipfd = fd;
	shiftoffset = 0;
}

void SetFST(void* address)
{
	fstaddress = address;
}

u32 GetShiftOffset()
{
	return shiftoffset;
}

// {Dis,En}able the memory-saving-but-FAT-specific cluster hack
void SetClusters(bool clusters)
{
	Command[0] = clusters;
	IOS_Ioctl(dipfd, Ioctl::SetClusters, Command, 0x04, Output, 0x04);
}

DiscNode* FindNode(const char* fstname)
{
	//DiscNode* fst = (DiscNode*)*(u32*)0x80000038; // Start of FST
	DiscNode* fst = (DiscNode*)fstaddress; // Start of FST
	
	char* nametable = (char*)(fst + fst->Size);
	
	int count = 0;
	vector<DiscNode*> nodes;
	char curpath[MAXPATHLEN];
	int pos = 0;
	
	for (DiscNode* node = fst; (void*)node < (void*)nametable; node++) {
		char* name = nametable + node->GetNameOffset();
		
		if (node->Type == 0x01) {
			nodes.push_back(node);
			
			REFRESH_PATH();
		} else if (node->Type == 0x00) {
			curpath[pos] = '\0'; // Null-terminate it to the path
			strcat(curpath, name);
			
			if (!strcasecmp(curpath, fstname) || !strcasecmp(name, fstname))
				return node;
		}
		
		while (nodes.size() > 0 && count == nodes.back()->Size - 1) {
			nodes.pop_back();
			
			REFRESH_PATH();
		}
		count++;
	}
	
	return NULL;
}

int AddPatchFile(const char* filename)
{
	return IOS_Ioctl(dipfd, Ioctl::AddFile, (void*)filename, strlen(filename) + 1, NULL, 0);
}

int AddPatchFile(const char* filename, int id)
{
	Output[0] = id;
	return IOS_Ioctl(dipfd, Ioctl::AddFile, (void*)filename, strlen(filename) + 1, Output, 4);
}

void AddPatch(int fd, u32 fileoffset, u64 offset, u32 length)
{
	Command[0] = fd;
	Command[1] = fileoffset;
	//*(u64*)&Command[2] = offset;
	Command[2] = offset >> 32;
	Command[3] = (u32)offset;
	Command[4] = length;
	
	IOS_Ioctl(dipfd, Ioctl::AddPatch, Command, 0x20, Output, 0x20);
}

void AddShift(u64 original, u64 newoffset, u32 size)
{
	Command[0] = size;
	//*(u64*)&Command[1] = original;
	Command[1] = original >> 32;
	Command[2] = (u32)original;
	//*(u64*)&Command[3] = newoffset;
	Command[3] = newoffset >> 32;
	Command[4] = (u32)newoffset;
	
	IOS_Ioctl(dipfd, Ioctl::AddShift, Command, 0x20, Output, 0x20);
}

void PatchAllocate(int type, int toadd)
{
	Command[0] = type;
	Command[1] = toadd;
	
	IOS_Ioctl(dipfd, Ioctl::Allocate, Command, 0x20, Output, 0x20);
}

void ResizeFST(DiscNode* node, u32 length, bool telldip)
{
	if (telldip)
		AddShift((u64)node->DataOffset << 2, SHIFT_BASE + shiftoffset, node->Size);
	node->Size = length;
	node->DataOffset = (u32)((SHIFT_BASE + shiftoffset) >> 2);
	
	shiftoffset += length;
	shiftoffset = ROUND_UP(shiftoffset, 0x40);
}

bool PatchFST(const char* fstname, u32 offset, const char* filename, u32 fileoffset, u32 length)
{
	DiscNode* node = FindNode(fstname);
	
	if (node == NULL || node->Type == 0x01)
		return false;
	
	if (length == 0)
		length = node->Size;
	else if (offset + length > node->Size)
		ResizeFST(node, offset + length, true);
	s32 fd = AddPatchFile(filename);
	AddPatch(fd, fileoffset, ((u64)node->DataOffset << 2) + (u64)offset, length);
	return true;
}

void ExecuteQueue(vector<PatchShift>* shifts, vector<PatchFile>* patches, u32* fstsize)
{
	//DiscNode* fst = (DiscNode*)*MEM_FSTADDRESS; // Start of FST
	DiscNode* fst = (DiscNode*)fstaddress; // Start of FST
	
	char* nametable = (char*)(fst + fst->Size);
	
	int count = 0;
	vector<DiscNode*> nodes;
	char curpath[MAXPATHLEN];
	int pos = 0;
	
	if (patches != NULL && patches->size() > 0) {
		// Preallocate patches and files
		PatchAllocate(0, patches->size() + 2);
		PatchAllocate(1, patches->size() + 2);
	}
	
	DiscNode** shiftstodo = new DiscNode*[shifts->size() * 2];
	memset(shiftstodo, 0, shifts->size() * 2 * sizeof(DiscNode*));
	
	REFRESH_PATH();
	
	for (DiscNode* node = fst; (void*)node < (void*)nametable; node++, count++) {
		char* name = nametable + node->GetNameOffset();
		
		if (node->Type == 0x01) {
			nodes.push_back(node);
			
			REFRESH_PATH();
		} else if (node->Type == 0x00) {
			curpath[pos] = '\0'; // Null-terminate it to the path
			strcat(curpath, name);
			
			if (patches != NULL) {
				for (vector<PatchFile>::iterator patch = patches->begin(); patch != patches->end(); patch++) {
					if (!strcasecmp(curpath, patch->DiscFile.c_str()) || !strcasecmp(name, patch->DiscFile.c_str())) {
						if (patch->Length == 0) {
							Stats st;
							if (File_Stat(patch->External.c_str(), &st) == 0) {
								patch->Length = st.Size;
								patch->Stated = true;
								patch->ExternalID = st.Identifier;
							} else {
								patches->erase(patch);
								break;
							}
						}
						
						s32 fd = -1;
						if (patch->Stated)
							fd = AddPatchFile(patch->External.c_str(), patch->ExternalID);
						else
							fd = AddPatchFile(patch->External.c_str());
						
						if (fd >= 0) {
							if (patch->Resize) {
								if (patch->Length + patch->Offset > node->Size)
									ResizeFST(node, patch->Length + patch->Offset, patch->Offset > 0);
								else 
									node->Size = patch->Length + patch->Offset;
								
							}
							
							AddPatch(fd, patch->FileOffset, ((u64)node->DataOffset << 2) + (u64)patch->Offset, patch->Length);
						}
						
						patches->erase(patch);
						break;
					}
				}
			}
			
			if (shifts != NULL) {
				int patchnum = 0;
				for (vector<PatchShift>::iterator patch = shifts->begin(); patch != shifts->end(); patch++) {
					if (!strcasecmp(curpath, patch->Source.c_str()) || !strcasecmp(name, patch->Source.c_str())) {
						shiftstodo[patchnum * 2] = node;
						break;
					} else if (!strcasecmp(curpath, patch->Destination.c_str()) || !strcasecmp(name, patch->Destination.c_str())) {
						shiftstodo[patchnum * 2 + 1] = node;
						break;
					}
					patchnum++;
				}
			}
		}
		
		while (nodes.size() > 0 && count == nodes.back()->Size - 1) {
#ifdef FUCKYOU
			// Before popping back, check if we need to create any files here
			curpath[pos] = '\0';
			// When we pop out of a directory, we can assume anything that matches it is fucked.
			for (vector<PatchFile>::iterator patch = patches->begin(); patch != patches->end(); patch++) {
				if (patch->Create && !strncasecmp(curpath, patch->DiscFile.c_str(), pos)) {
					char* token = patch->DiscFile.c_str() + pos;
					vector<string> directories;
					
					char* nexttoken = token - 1;
					int filenamesize = 0;
					while (nexttoken = strchr(nexttoken + 1, '/')) {
						string dir(token, (int)(nexttoken - token));
						filenamesize += dir.size() + 1;
						directories.push_back(dir);
					}
					nexttoken = strrchr(token, '/');
					nexttoken++;
					
					int expansion = sizeof(DiscNode) * (directories.size() + 1) + filenamesize;
					
					memmove(*MEM_FSTADDRESS - expansion, *MEM_FSTADDRESS, *MEM_FSTSIZE);
					
					*MEM_FSTADDRESS -= expansion;
					*MEM_ARENA1HIGH = *MEM_FSTADDRESS;
					
					//memmove(FST + curpos + expansionofnotes, FST + curpos, FSTSIZE - curpossize);
					int offset = (count + directories.size() + 1) * sizeof(DiscNode);
					memmove(*MEM_FSTADDRESS + offset, FST + cound * sizeof(DiscNode), *MEM_FSTSIZE - offset);
					
					node = (DiscNode*)*MEM_FSTADDRESS + count + 1;
					for (int i = 0; i < directories.size(); i++, node++) {
						int subdirs = directories.size() - i - 1;
						node->Type = 0;
						node->DataOffset = 0;
						node->Size = 0; // Next sibling
						node->SetNameOffset(...);
					}
					
					//Write add nodes, etc.
					
					Add the number of nodes to every directory node size that is above count (or off by one?)
					
					*MEM_FSTSIZE += expansion;
					
					
					
					// Fix pointers...
					
					// Find the end of the nametable
					
					// Step 1: Start by creating any subfolders, then the file node. Each node needs 
					
					// Step 3: memmove everything else out of there
					
					// Step 4: Insert new node
					
					// endloop
					
					// Step 5: Add the right number of nodes to EVERY FUCKING DIRECTORY THAT EVER EXISTED
					
					newnode.SetNameOffset(0);
				}
			}
#endif
			nodes.pop_back();
			
			REFRESH_PATH();
		}
	}
	for (int i = 0; i < shifts->size(); i++) {
		if (shiftstodo[i * 2] != NULL && shiftstodo[i * 2 + 1] != NULL) {
			shiftstodo[i * 2 + 1]->DataOffset = shiftstodo[i * 2]->DataOffset;
			shiftstodo[i * 2 + 1]->Size = shiftstodo[i * 2]->Size;
		}
	}
}
