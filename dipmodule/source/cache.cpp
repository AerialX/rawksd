/*
 cache.c
 The cache is not visible to the user. It should be flushed
 when any file is closed or changes are made to the filesystem.

 This cache implements a least-used-page replacement policy. This will
 distribute sectors evenly over the pages, so if less than the maximum
 pages are used at once, they should all eventually remain in the cache.
 This also has the benefit of throwing out old sectors, so as not to keep
 too many stale pages around.

 Copyright (c) 2006 Michael "Chishm" Chisholm

 Redistribution and use in source and binary forms, with or without modification,
 are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation and/or
     other materials provided with the distribution.
  3. The name of the author may not be used to endorse or promote products derived
     from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <string.h>
#include <limits.h>
#include <syscalls.h>

#include "cache.h"

#ifdef YARR

Cache::Cache(u32 pages, u32 sectorsPerPage, u32 sectorSize, ReadSectorsFunction readsectors, void* userdata) :
	ReadDiskSectors(readsectors), UserData(userdata), Pages(pages), SectorsPerPage(sectorsPerPage), SectorSize(sectorSize) {
	
	if (Pages < 2)
		Pages = 2;
	
	Entries = new CacheEntry[Pages];
	
	for (u32 i = 0; i < Pages; i++) {
		Entries[i].SetSize(SectorSize * SectorsPerPage);
	}
}

Cache::~Cache()
{
	delete[] Entries;
}

static u32 accessCounter = 0;
static u32 AccessTime()
{
	accessCounter++;
	return accessCounter;
}


CacheEntry* Cache::GetPage(u32 sector)
{
	bool found = false;
	u32 oldUsed = 0;
	u32 oldAccess = UINT_MAX;
	
	for (u32 i = 0; i < Pages; i++) {
		if (sector >= Entries[i].Sector && sector < (Entries[i].Sector + Entries[i].Count)) {
			Entries[i].LastAccess = AccessTime();
			return Entries + i;
		}
		
		if (!found && (Entries[i].Sector == CACHE_FREE || Entries[i].LastAccess < oldAccess)) {
			if (Entries[i].Sector == CACHE_FREE)
				found = true;
			oldUsed = i;
			oldAccess = Entries[i].LastAccess;
		}
	}
	
	sector = SectorsPerPage * (sector / SectorsPerPage); // Align to page size
	
	if (!ReadDiskSectors(UserData, sector, SectorsPerPage, Entries[oldUsed].Data))
		return NULL;
	
	Entries[oldUsed].Sector = sector;
	Entries[oldUsed].Count = SectorsPerPage;
	Entries[oldUsed].LastAccess = AccessTime();
	
	return Entries + oldUsed;
}

bool Cache::ReadSectors(u32 sector, u32 numSectors, void* buffer)
{
	u8* dest = (u8*)buffer;
	
	while (numSectors > 0) {
		CacheEntry* entry = GetPage(sector);
		if (!entry)
			return false;

		u32 sec = sector - entry->Sector;
		u32 secs_to_read = entry->Count - sec;
		if (secs_to_read > numSectors)
			secs_to_read = numSectors;

		memcpy(dest, entry->Data + (sec * SectorSize), secs_to_read * SectorSize);
		os_sync_after_write(dest, secs_to_read * SectorSize);

		dest += secs_to_read * SectorSize;
		sector += secs_to_read;
		numSectors -= secs_to_read;
	}
	
	return true;
}

bool Cache::ReadPartialSector(void* buffer, u32 sector, u32 offset, u32 size)
{
	if (offset + size > SectorSize)
		return false;
	
	CacheEntry* entry = GetPage(sector);
	if (!entry)
		return false;
	
	u32 sec = sector - entry->Sector;
	memcpy(buffer, entry->Data + (sec * SectorSize) + offset, size);
	os_sync_after_write(buffer, size);

	return true;
}

bool Cache::Read(void* buffer, u64 offset, u32 size)
{
	// RE: The werid shifting... I blame the b0rked u64 division

	u8* data = (u8*)buffer;
	u32 sector = (u32)(offset >> 2) / (SectorSize >> 2);
	u32 sectoroffset = ((u32)(offset >> 2) % (SectorSize >> 2)) << 2;
	u32 read = MIN(size, SectorSize - sectoroffset);
	ReadPartialSector(data, sector, sectoroffset, read);
	data += read;
	size -= read;
	sector++;
	
	if (size > SectorSize) {
		u32 sectors = (u32)(size >> 2) / (SectorSize >> 2);
		ReadSectors(sector, sectors, data);
		sector += sectors;
		
		sectors *= SectorSize;
		data += sectors;
		size -= sectors;
	}
	
	if (size > 0) {
		ReadPartialSector(data, sector, 0, size);
	}
	
	return true;
}

void Cache::Clear()
{
	for (u32 i = 0; i < Pages; i++) {
		Entries[i].LastAccess = 0;
		Entries[i].Count = 0;
		Entries[i].Sector = CACHE_FREE;
	}
}

#endif

