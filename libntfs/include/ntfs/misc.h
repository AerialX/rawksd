/*
 * misc.h : miscellaneous exports
 *		- memory allocation
 *		- LRU caches
 *
 * Copyright (c) 2008 Jean-Pierre Andre
 *
 * This program/include file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program/include file is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (in the main directory of the NTFS-3G
 * distribution in the file COPYING); if not, write to the Free Software
 * Foundation,Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _NTFS_MISC_H_
#define _NTFS_MISC_H_

#include "volume.h"

struct CACHED_GENERIC {
	struct CACHED_GENERIC *next;
	void *variable;
	size_t varsize;
	void *fixed[0];
} ;

struct CACHED_INODE {
	struct CACHED_INODE *next;
	const char *pathname;
	size_t varsize;
		/* above fields must match "struct CACHED_GENERIC" */
	u64 inum;
} ;

typedef int (*cache_compare)(const struct CACHED_GENERIC *cached,
				const struct CACHED_GENERIC *item);

struct CACHE_HEADER {
	const char *name;
	struct CACHED_GENERIC *most_recent_entry;
	struct CACHED_GENERIC *free_entry;
	unsigned long reads;
	unsigned long writes;
	unsigned long hits;
	int fixed_size;
	struct CACHED_GENERIC entry[0];
} ;

	/* cast to generic, avoiding gcc warnings */
#define GENERIC(pstr) ((const struct CACHED_GENERIC*)(const void*)(pstr))

struct CACHED_GENERIC *ntfs_fetch_cache(struct CACHE_HEADER *cache,
		const struct CACHED_GENERIC *wanted, cache_compare compare);
struct CACHED_GENERIC *ntfs_enter_cache(struct CACHE_HEADER *cache,
			const struct CACHED_GENERIC *item, cache_compare compare);
int ntfs_invalidate_cache(struct CACHE_HEADER *cache,
		const struct CACHED_GENERIC *item, cache_compare compare);
void ntfs_create_lru_caches(ntfs_volume *vol);
void ntfs_free_lru_caches(ntfs_volume *vol);

void *ntfs_calloc(size_t size);
void *ntfs_malloc(size_t size);

#endif /* _NTFS_MISC_H_ */

