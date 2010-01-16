#pragma once

#include <gctypes.h>

#ifdef __cplusplus
	extern "C" {
#endif

typedef struct _heap_block_st heap_block;
struct _heap_block_st {
	u32 back_flag;
	u32 front_flag;
	heap_block *next;
	heap_block *prev;
};

typedef struct _heap_iblock_st {
	u32 free_blocks;
	u32 free_size;
	u32 used_blocks;
	u32 used_size;
} heap_iblock;

typedef struct _heap_cntrl_st {
	heap_block *start;
	heap_block *final;

	heap_block *first;
	heap_block *perm_null;
	heap_block *last;
	u32 pg_size;
	u32 reserved;
} heap_cntrl;

u32 __lwp_heap_init(heap_cntrl *theheap,void *start_addr,u32 size,u32 pg_size);
void* __lwp_heap_allocate(heap_cntrl *theheap,u32 size,u32 align);
BOOL __lwp_heap_free(heap_cntrl *theheap,void *ptr);
u32 __lwp_heap_getinfo(heap_cntrl *theheap,heap_iblock *theinfo);
// WARNING: if you realloc aligned memory it may not be aligned any more
void* __lwp_heap_realloc(heap_cntrl *theheap, void *src, u32 size);

#ifdef __cplusplus
	}
#endif
