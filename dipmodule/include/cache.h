#pragma once

#include <gctypes.h>
#include <limits.h>
#include <mem.h>

#include "diprovider.h"

#ifdef YARR

#define CACHE_FREE UINT_MAX
typedef u32 sec_t;

typedef bool (*ReadSectorsFunction)(void* userdata, sec_t sector, sec_t numSectors, void* buffer);

struct CacheEntry
{
	u32 Sector;
	u32 Count;
	u32 LastAccess;
	u8* Data;
	
	CacheEntry()
	{
		Sector = CACHE_FREE;
		Count = 0;
		LastAccess = 0;
		Data = NULL;
	}
	
	void SetSize(u32 size)
	{
		if (Data)
			delete[] Data;
		Data = new u8[size];
	}
	
	void Clear()
	{
		if (Data)
			delete[] Data;
		Data = NULL;
		LastAccess = 0;
		Count = 0;
		Sector = CACHE_FREE;
	}

	~CacheEntry()
	{
		Clear();
	}
};

class Cache
{
private:
	ReadSectorsFunction ReadDiskSectors;
	void* UserData;
	u32 Pages;
	u32 SectorsPerPage;
	CacheEntry* Entries;
	u32 SectorSize;
	CacheEntry* GetPage(u32 sector);
	
public:
	Cache(u32 pages, u32 sectorsPerPage, u32 sectorSize, ReadSectorsFunction readsectors, void* userdata);
	~Cache();
	
	bool ReadPartialSector(void* buffer, u32 sector, u32 offset, u32 size);
	bool ReadSectors(u32 sector, u32 numSectors, void* buffer);
	bool ReadSector(void* buffer, u32 sector) { return ReadPartialSector(buffer, sector, 0, SectorSize); }
	bool Read(void* buffer, u64 offset, u32 size);

	void Clear();
};
#endif

