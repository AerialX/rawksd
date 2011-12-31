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

#include "common.h"
#include "cache.h"
#include "disc.h"

#include "mem_allocate.h"
#include "bit_ops.h"
#include "file_allocation_table.h"

#define CACHE_FREE UINT_MAX

CACHE* _FAT_cache_constructor (unsigned int numberOfPages, unsigned int sectorsPerPage, const DISC_INTERFACE* discInterface, sec_t endOfPartition, unsigned int bytesPerSector) {
	CACHE* cache;
	unsigned int i;
	CACHE_ENTRY* cacheEntries;

	if (numberOfPages < 2) {
		numberOfPages = 2;
	}

	if (sectorsPerPage < 8) {
		sectorsPerPage = 8;
	}

	cache = (CACHE*) _FAT_mem_allocate (sizeof(CACHE));
	if (cache == NULL) {
		return NULL;
	}

	switch (bytesPerSector) {
		case 512:
			cache->bytesPerSectorLog = 9;
			break;
		case 4096:
			cache->bytesPerSectorLog = 12;
			sectorsPerPage >>= 3;
			break;
		default:
			_FAT_mem_free(cache);
			return NULL;
	}

	cache->disc = discInterface;
	cache->endOfPartition = endOfPartition;
	cache->numberOfPages = numberOfPages;
	cache->sectorsPerPage = sectorsPerPage;
	cache->bytesPerSector = bytesPerSector;

	cacheEntries = (CACHE_ENTRY*) _FAT_mem_allocate ( sizeof(CACHE_ENTRY) * numberOfPages);
	if (cacheEntries == NULL) {
		_FAT_mem_free (cache);
		return NULL;
	}

	for (i = 0; i < numberOfPages; i++) {
		cacheEntries[i].sector = CACHE_FREE;
		cacheEntries[i].count = 0;
		cacheEntries[i].last_access = 0;
		cacheEntries[i].dirty = false;
		cacheEntries[i].cache = (uint8_t*) _FAT_mem_align ( sectorsPerPage << cache->bytesPerSectorLog );
	}

	for (i=0; i < EVICTION_HISTORY; i++) {
		cache->evictions[i].sector = CACHE_FREE;
		cache->evictions[i].count = 0;
		cache->evictions[i].last_access = 0;
	}

	cache->cacheEntries = cacheEntries;

	return cache;
}

void _FAT_cache_destructor (CACHE* cache) {
	unsigned int i;
	// Clear out cache before destroying it
	_FAT_cache_flush(cache);

	// Free memory in reverse allocation order
	for (i = 0; i < cache->numberOfPages; i++) {
		_FAT_mem_free (cache->cacheEntries[i].cache);
	}
	_FAT_mem_free (cache->cacheEntries);
	_FAT_mem_free (cache);
}


static u32 accessCounter = 0;

static u32 accessTime(){
	accessCounter++;
	return accessCounter;
}

static int _FAT_cache_getEvictionIndex(CACHE *cache, sec_t sector)
{
	int i;
	EVICTION_ENTRY *entries = cache->evictions;

	for(i=0;i<EVICTION_HISTORY;i++) {
		if (entries[i].sector == sector)
			return i;
	}

	return -1;
}

static CACHE_ENTRY* _FAT_cache_getPage(CACHE *cache,sec_t sector)
{
	unsigned int i;
	CACHE_ENTRY* cacheEntries = cache->cacheEntries;
	unsigned int numberOfPages = cache->numberOfPages;
	unsigned int sectorsPerPage = cache->sectorsPerPage;
	EVICTION_ENTRY* Evictions = cache->evictions;

	bool foundFree = false;
	unsigned int oldUsed = 0;
	unsigned int oldAccess = UINT_MAX;
	unsigned int oldEvicted = UINT_MAX;
	unsigned int oldEvictedAccess = 0;

	for(i=0;i<numberOfPages;i++) {
		int eviction_index = _FAT_cache_getEvictionIndex(cache, cacheEntries[i].sector);

		if(sector>=cacheEntries[i].sector && sector<(cacheEntries[i].sector + cacheEntries[i].count)) {
			cacheEntries[i].last_access = accessTime();
			if (eviction_index>=0)
				Evictions[eviction_index].last_access = cacheEntries[i].last_access;
			return &(cacheEntries[i]);
		}

		if (cacheEntries[i].sector==CACHE_FREE) {
			oldUsed = i;
			foundFree = true;
			break;
		}

		if(cacheEntries[i].last_access<oldAccess) {
			if (eviction_index>=0) {
				EVICTION_ENTRY *evicted = Evictions+eviction_index;
				if (evicted->count >= oldEvicted && evicted->last_access >= oldEvictedAccess)
					continue;
				oldAccess = UINT_MAX; // set this so any pages without eviction history will be preferred
				oldEvicted = evicted->count;
				oldEvictedAccess = evicted->last_access;
			} else {
				oldAccess = cacheEntries[i].last_access;
				oldEvicted = 0; // prefer this page over any with eviction history
				oldEvictedAccess = 0;
			}
			oldUsed = i;
		}
	}

	if (foundFree==false) { // something is being evicted, keep track of it
		int eviction_index = _FAT_cache_getEvictionIndex(cache, cacheEntries[oldUsed].sector);

		if (eviction_index<0) { // no eviction history for this page, overwrite the oldest record
			oldEvictedAccess = UINT_MAX;
			for (i=0;i<EVICTION_HISTORY;i++) {
				if (cache->evictions[i].last_access < oldEvictedAccess) {
					oldEvictedAccess = Evictions[i].last_access;
					eviction_index = i;
				}
			}
			Evictions[eviction_index].sector = cacheEntries[oldUsed].sector;
			Evictions[eviction_index].count = 0;
		}

		Evictions[eviction_index].count++;
		Evictions[eviction_index].last_access = accessTime();
	}

	if(foundFree==false && cacheEntries[oldUsed].dirty==true) {
		if(!_FAT_disc_writeSectors(cache->disc,cacheEntries[oldUsed].sector,cacheEntries[oldUsed].count,cacheEntries[oldUsed].cache)) return NULL;
		cacheEntries[oldUsed].dirty = false;
	}

	sector = (sector/sectorsPerPage)*sectorsPerPage; // align base sector to page size
	sec_t next_page = sector + sectorsPerPage;
	if(next_page > cache->endOfPartition)	next_page = cache->endOfPartition;

	if(!_FAT_disc_readSectors(cache->disc,sector,next_page-sector,cacheEntries[oldUsed].cache)) return NULL;

	cacheEntries[oldUsed].sector = sector;
	cacheEntries[oldUsed].count = next_page-sector;
	cacheEntries[oldUsed].last_access = accessTime();

	return &(cacheEntries[oldUsed]);
}

bool _FAT_cache_readSectors(CACHE *cache,sec_t sector,sec_t numSectors,void *buffer)
{
	sec_t sec;
	sec_t secs_to_read;
	CACHE_ENTRY *entry;
	uint8_t *dest = (uint8_t *)buffer;

	while(numSectors>0) {
		entry = _FAT_cache_getPage(cache,sector);
		if(entry==NULL) return false;

		sec = sector - entry->sector;
		secs_to_read = entry->count - sec;
		if(secs_to_read>numSectors) secs_to_read = numSectors;

		memcpy(dest,entry->cache + (sec<<cache->bytesPerSectorLog),(secs_to_read<<cache->bytesPerSectorLog));

		dest += (secs_to_read<<cache->bytesPerSectorLog);
		sector += secs_to_read;
		numSectors -= secs_to_read;
	}

	return true;
}

/*
Reads some data from a cache page, determined by the sector number
*/
bool _FAT_cache_readPartialSector (CACHE* cache, void* buffer, sec_t sector, unsigned int offset, size_t size)
{
	sec_t sec;
	CACHE_ENTRY *entry;

	if (offset + size > cache->bytesPerSector) return false;

	entry = _FAT_cache_getPage(cache,sector);
	if(entry==NULL) return false;

	// hack
	if ((int)buffer < 0x10000000)
		size = (size+3)&~3;
	sec = sector - entry->sector;
	memcpy(buffer,entry->cache + ((sec<<cache->bytesPerSectorLog) + offset), size);

	return true;
}

bool _FAT_cache_readLittleEndianValue (CACHE* cache, uint32_t *value, sec_t sector, unsigned int offset, int num_bytes) {
  uint8_t buf[4];
  if (!_FAT_cache_readPartialSector(cache, buf, sector, offset, num_bytes)) return false;

  switch(num_bytes) {
  case 1: *value = buf[0]; break;
  case 2: *value = u8array_to_u16(buf,0); break;
  case 4: *value = u8array_to_u32(buf,0); break;
  default: return false;
  }
  return true;
}

/*
Writes some data to a cache page, making sure it is loaded into memory first.
*/
bool _FAT_cache_writePartialSector (CACHE* cache, const void* buffer, sec_t sector, unsigned int offset, size_t size)
{
	sec_t sec;
	CACHE_ENTRY *entry;

	if (offset + size > cache->bytesPerSector) return false;

	entry = _FAT_cache_getPage(cache,sector);
	if(entry==NULL) return false;

	sec = sector - entry->sector;
	memcpy(entry->cache + ((sec<<cache->bytesPerSectorLog) + offset),buffer,size);

	entry->dirty = true;
	return true;
}

bool _FAT_cache_writeLittleEndianValue (CACHE* cache, const uint32_t value, sec_t sector, unsigned int offset, int size) {
  uint8_t buf[4] = {0, 0, 0, 0};

  switch(size) {
  case 1: buf[0] = value; break;
  case 2: u16_to_u8array(buf, 0, value); break;
  case 4: u32_to_u8array(buf, 0, value); break;
  default: return false;
  }

  return _FAT_cache_writePartialSector(cache, buf, sector, offset, size);
}

/*
Writes some data to a cache page, zeroing out the page first
*/
bool _FAT_cache_eraseWritePartialSector (CACHE* cache, const void* buffer, sec_t sector, unsigned int offset, size_t size)
{
	sec_t sec;
	CACHE_ENTRY *entry;

	if (offset + size > cache->bytesPerSector) return false;

	entry = _FAT_cache_getPage(cache,sector);
	if(entry==NULL) return false;

	sec = sector - entry->sector;
	memset(entry->cache + (sec<<cache->bytesPerSectorLog),0,cache->bytesPerSector);
	memcpy(entry->cache + ((sec<<cache->bytesPerSectorLog) + offset),buffer,size);

	entry->dirty = true;
	return true;
}

static CACHE_ENTRY* _FAT_cache_findPage(CACHE *cache, sec_t sector, sec_t count) {
	unsigned int i;
	CACHE_ENTRY* cacheEntries = cache->cacheEntries;
	unsigned int numberOfPages = cache->numberOfPages;

	for (i=0;i<numberOfPages;i++) {
		bool intersect;
		if (sector > cacheEntries[i].sector)
			intersect = sector - cacheEntries[i].sector < cacheEntries[i].count;
		else
			intersect = cacheEntries[i].sector - sector < count;

		if (intersect)
			return &cacheEntries[i];
	}

	return NULL;
}

bool _FAT_cache_writeSectors (CACHE* cache, sec_t sector, sec_t numSectors, const void* buffer)
{
	sec_t sec;
	sec_t secs_to_write;
	CACHE_ENTRY* entry;
	const uint8_t *src = (const uint8_t *)buffer;

	if ((u32)src >= 0x10000000 && ((u32)src & 0x1F)==0 && _FAT_cache_findPage(cache,sector,numSectors)==NULL)
		return _FAT_disc_writeSectors(cache->disc,sector,numSectors,src);

	while(numSectors>0)
	{
		entry = _FAT_cache_getPage(cache,sector);
		if(entry==NULL) return false;

		sec = sector - entry->sector;
		secs_to_write = entry->count - sec;

		if(secs_to_write>numSectors) secs_to_write = numSectors;

		memcpy(entry->cache + (sec<<cache->bytesPerSectorLog),src,(secs_to_write<<cache->bytesPerSectorLog));

		src += (secs_to_write<<cache->bytesPerSectorLog);
		sector += secs_to_write;
		numSectors -= secs_to_write;

		entry->dirty = true;
	}
	return true;
}

/*
Flushes all dirty pages to disc, clearing the dirty flag.
*/
bool _FAT_cache_flush (CACHE* cache) {
	unsigned int i;

	for (i = 0; i < cache->numberOfPages; i++) {
		if (cache->cacheEntries[i].dirty) {
			if (!_FAT_disc_writeSectors (cache->disc, cache->cacheEntries[i].sector, cache->cacheEntries[i].count, cache->cacheEntries[i].cache)) {
				return false;
			}
		}
		cache->cacheEntries[i].dirty = false;
	}

	return true;
}

void _FAT_cache_invalidate (CACHE* cache) {
	unsigned int i;
	_FAT_cache_flush(cache);
	for (i = 0; i < cache->numberOfPages; i++) {
		cache->cacheEntries[i].sector = CACHE_FREE;
		cache->cacheEntries[i].last_access = 0;
		cache->cacheEntries[i].count = 0;
		cache->cacheEntries[i].dirty = false;
	}
}
