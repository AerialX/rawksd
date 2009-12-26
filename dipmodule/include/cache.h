#pragma once

#include "gctypes.h"
#include "disc_io.h"
#include <limits.h>

#define CACHE_FREE UINT_MAX

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
		Data = new u8[size];
	}
	
	~CacheEntry()
	{
		if (Data)
			delete[] Data;
	}
};

class Cache
{
private:
	const DISC_INTERFACE* Disc;
	u32 Pages;
	u32 SectorsPerPage;
	CacheEntry* Entries;
	u32 SectorSize;
	CacheEntry* GetPage(u32 sector);
	
public:
	Cache(u32 pages, u32 sectorsPerPage, const DISC_INTERFACE* disc, u32 sectorSize);
	~Cache();
	
	bool ReadPartialSector(void* buffer, u32 sector, u32 offset, u32 size);
	bool ReadSectors(u32 sector, u32 numSectors, void* buffer);
	bool ReadSector(void* buffer, u32 sector) { return ReadPartialSector(buffer, sector, 0, SectorSize); }
	bool Read(void* buffer, u64 offset, u32 size);
};
