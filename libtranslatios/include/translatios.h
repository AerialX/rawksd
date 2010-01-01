#pragma once

#include <gctypes.h>

#include <string>
#include <vector>

#define MEM_BASE			((u8*)0x80000000)
#define MEM_BOOTCODE		((u32*)0x80000020)
#define MEM_VERSION			((u32*)0x80000024)
#define MEM_ARENA1LOW		((u32*)0x80000030)
#define MEM_BUSSPEED		((u32*)0x800000F8)
#define MEM_CPUSPEED		((u32*)0x800000FC)
#define MEM_IOSVERSION		((u32*)0x80003140)
#define MEM_GAMEONLINE		((u32*)0x80003180)
#define MEM_GAMEIDADDRESS	((u8**)0x80003184)
#define MEM_IOSEXPECTED		((u32*)0x80003188)

#define MEM_FSTADDRESS		((u32*)0x80000038)
#define MEM_APPLOADER		((u32*)0x81200000)

#define MEM_ARENA1HIGH		((u32*)0x80000034)
#define MEM_FSTSIZE			((u32*)0x8000003C)

// 10 GB shift
#define SHIFT_BASE (0x280000000ULL)

struct DiscNode
{
public:
	u8 Type;
private:
	u8 NameOffsetMSB;
	u16 NameOffset; //really a u24
public:
	u32 DataOffset;
	u32 Size;
	u32 GetNameOffset() { return ((u32)NameOffsetMSB << 16) | (u32)NameOffset; }
	void SetNameOffset(u32 offset) { NameOffset = (u16)offset; NameOffsetMSB = offset & 0x00FF0000; }
} __attribute__((packed));

struct PatchFile
{
	std::string DiscFile;
	s64 Offset;
	std::string External;
	u64 FileOffset;
	s64 Length;
	u32 ExternalID;
	bool Stated;
	bool Resize;
	bool Create;
};

struct PatchShift
{
	std::string Source;
	std::string Destination;
};

void InitializePatcher(int fd);
void SetFST(void* address);
void SetClusters(bool clusters);

DiscNode* FindNode(const char* fstname);

int AddPatchFile(const char* filename);
int AddPatchFile(const char* filename, int id);
void AddPatch(int fd, u32 fileoffset, u64 offset, u32 length);
void AddShift(u64 original, u64 newoffset, u32 size);
void ResizeFST(DiscNode* node, u32 length, bool telldip);
bool PatchFST(const char* fstname, u32 offset, const char* filename, u32 fileoffset, u32 length);
void ExecuteQueue(std::vector<PatchShift*>* shifts, std::vector<PatchFile*>* files, u32* fstsize);

u32 GetShiftOffset();
