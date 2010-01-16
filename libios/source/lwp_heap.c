#include <stdlib.h>
#include <string.h> // for memcpy

#include "lwp_heap.h"

#define HEAP_BLOCK_USED					1
#define HEAP_BLOCK_FREE					0

#define HEAP_DUMMY_FLAG					(0+HEAP_BLOCK_USED)

#define HEAP_OVERHEAD					(sizeof(u32)*2)
#define HEAP_BLOCK_USED_OVERHEAD		(sizeof(void*)*2)
#define HEAP_MIN_SIZE					(HEAP_OVERHEAD+sizeof(heap_block))

#define STARLET_ALIGNMENT 4

static __inline__ heap_block* __lwp_heap_head(heap_cntrl *theheap)
{
	return (heap_block*)&theheap->start;
}

static __inline__ heap_block* __lwp_heap_tail(heap_cntrl *heap)
{
	return (heap_block*)&heap->final;
}

static __inline__ heap_block* __lwp_heap_prevblock(heap_block *block)
{
	return (heap_block*)((char*)block - (block->back_flag&~HEAP_BLOCK_USED));
}

static __inline__ heap_block* __lwp_heap_nextblock(heap_block *block)
{
	return (heap_block*)((char*)block + (block->front_flag&~HEAP_BLOCK_USED));
}

static __inline__ heap_block* __lwp_heap_blockat(heap_block *block,u32 offset)
{
	return (heap_block*)((char*)block + offset);
}

static __inline__ heap_block* __lwp_heap_usrblockat(void *ptr)
{
	u32 offset = *(((u32*)ptr)-1);
	return (heap_block*)__lwp_heap_blockat((heap_block*)ptr,-offset+-HEAP_BLOCK_USED_OVERHEAD);
}

static __inline__ boolean __lwp_heap_prev_blockfree(heap_block *block)
{
	return !(block->back_flag&HEAP_BLOCK_USED);
}

static __inline__ boolean __lwp_heap_blockfree(heap_block *block)
{
	return !(block->front_flag&HEAP_BLOCK_USED);
}

static __inline__ boolean __lwp_heap_blockused(heap_block *block)
{
	return (block->front_flag&HEAP_BLOCK_USED);
}

static __inline__ u32 __lwp_heap_blocksize(heap_block *block)
{
	return (block->front_flag&~HEAP_BLOCK_USED);
}

static __inline__ void* __lwp_heap_startuser(heap_block *block)
{
	return (void*)&block->next;
}

static __inline__ boolean __lwp_heap_blockin(heap_cntrl *heap,heap_block *block)
{
	return ((u32)block>=(u32)heap->start && (u32)block<=(u32)heap->final);
}

static __inline__ boolean __lwp_heap_pgsize_valid(u32 pgsize)
{
	return (pgsize!=0 && ((pgsize%STARLET_ALIGNMENT)==0));
}

static __inline__ u32 __lwp_heap_buildflag(u32 size,u32 flag)
{
	return (size|flag);
}

u32 __lwp_heap_init(heap_cntrl *theheap,void *start_addr,u32 size,u32 pg_size)
{
	u32 dsize;
	heap_block *block;

	if(!__lwp_heap_pgsize_valid(pg_size) || size<HEAP_MIN_SIZE) return 0;

	theheap->pg_size = pg_size;
	dsize = (size - HEAP_OVERHEAD);

	block = (heap_block*)start_addr;
	block->back_flag = HEAP_DUMMY_FLAG;
	block->front_flag = dsize;
	block->next	= __lwp_heap_tail(theheap);
	block->prev = __lwp_heap_head(theheap);

	theheap->start = block;
	theheap->first = block;
	theheap->perm_null = NULL;
	theheap->last = block;

	block = __lwp_heap_nextblock(block);
	block->back_flag = dsize;
	block->front_flag = HEAP_DUMMY_FLAG;
	theheap->final = block;

	return (dsize - HEAP_BLOCK_USED_OVERHEAD);
}

void* __lwp_heap_allocate(heap_cntrl *theheap,u32 size,u32 alignment)
{
	u32 excess;
	u32 dsize;
	heap_block *block;
	heap_block *next_block;
	heap_block *tmp_block;
	void *ptr;
	u32 offset;


	if(size>=(u32)(-1-HEAP_BLOCK_USED_OVERHEAD)) return NULL;

	if (alignment < theheap->pg_size)
		alignment = theheap->pg_size;

	// alignment must be a power of 2
	excess = (size & (alignment-1));
	dsize = (size + alignment + HEAP_BLOCK_USED_OVERHEAD);

	if(excess)
		dsize += (alignment - excess);

	if(dsize<sizeof(heap_block)) dsize = sizeof(heap_block);

	for(block=theheap->first;;block=block->next) {
		if(block==__lwp_heap_tail(theheap))
			return NULL;
		if(block->front_flag>=dsize) break;
	}

	if((block->front_flag-dsize)>(alignment+HEAP_BLOCK_USED_OVERHEAD)) {
		block->front_flag -= dsize;
		next_block = __lwp_heap_nextblock(block);
		next_block->back_flag = block->front_flag;

		tmp_block = __lwp_heap_blockat(next_block,dsize);
		tmp_block->back_flag = next_block->front_flag = __lwp_heap_buildflag(dsize,HEAP_BLOCK_USED);

		ptr = __lwp_heap_startuser(next_block);
	} else {
		next_block = __lwp_heap_nextblock(block);
		next_block->back_flag = __lwp_heap_buildflag(block->front_flag,HEAP_BLOCK_USED);

		block->front_flag = next_block->back_flag;
		block->next->prev = block->prev;
		block->prev->next = block->next;

		ptr = __lwp_heap_startuser(block);
	}

	offset = (alignment - ((u32)ptr&(alignment-1)));
	ptr = (u8*)ptr+offset;
	*(((u32*)ptr)-1) = offset;

	return ptr;
}

BOOL __lwp_heap_free(heap_cntrl *theheap,void *ptr)
{
	heap_block *block;
	heap_block *next_block;
	heap_block *new_next;
	heap_block *prev_block;
	heap_block *tmp_block;
	u32 dsize;

	if (ptr==NULL||theheap==NULL)
		return FALSE;

	block = __lwp_heap_usrblockat(ptr);
	if(!__lwp_heap_blockin(theheap,block) || __lwp_heap_blockfree(block)) {
		return FALSE;
	}

	dsize = __lwp_heap_blocksize(block);
	next_block = __lwp_heap_blockat(block,dsize);

	if(!__lwp_heap_blockin(theheap,next_block) || (block->front_flag!=next_block->back_flag)) {
		return FALSE;
	}

	if(__lwp_heap_prev_blockfree(block)) {
		prev_block = __lwp_heap_prevblock(block);
		if(!__lwp_heap_blockin(theheap,prev_block)) {
			return FALSE;
		}

		if(__lwp_heap_blockfree(next_block)) {
			prev_block->front_flag += next_block->front_flag+dsize;
			tmp_block = __lwp_heap_nextblock(prev_block);
			tmp_block->back_flag = prev_block->front_flag;
			next_block->next->prev = next_block->prev;
			next_block->prev->next = next_block->next;
		} else {
			prev_block->front_flag = next_block->back_flag = prev_block->front_flag+dsize;
		}
	} else if(__lwp_heap_blockfree(next_block)) {
		block->front_flag = dsize+next_block->front_flag;
		new_next = __lwp_heap_nextblock(block);
		new_next->back_flag = block->front_flag;
		block->next = next_block->next;
		block->prev = next_block->prev;
		next_block->prev->next = block;
		next_block->next->prev = block;

		if(theheap->first==next_block) theheap->first = block;
	} else {
		next_block->back_flag = block->front_flag = dsize;
		block->prev = __lwp_heap_head(theheap);
		block->next = theheap->first;
		theheap->first = block;
		block->next->prev = block;
	}

	return TRUE;
}

void *__lwp_heap_realloc(heap_cntrl *theheap, void *src, u32 size)
{
	heap_block *block;
	u32 dsize;
	void *ptr;

	if (theheap==NULL)
		return NULL;

	if (src)
	{
		block = __lwp_heap_usrblockat(src);
		if(!__lwp_heap_blockin(theheap,block) || __lwp_heap_blockfree(block))
			src = NULL;
	}

	if (src==NULL)
		return __lwp_heap_allocate(theheap, size, 0);

	dsize = (char*)__lwp_heap_nextblock(block)-(char*)src;
	if (dsize>=size)
		return src;

	ptr = __lwp_heap_allocate(theheap, size, 0);
	if (ptr)
		memcpy(ptr, src, dsize);

	__lwp_heap_free(theheap, src);

	return ptr;
}

u32 __lwp_heap_getinfo(heap_cntrl *theheap,heap_iblock *theinfo)
{
	u32 not_done = 1;
	heap_block *theblock = NULL;
	heap_block *nextblock = NULL;

	theinfo->free_blocks = 0;
	theinfo->free_size = 0;
	theinfo->used_blocks = 0;
	theinfo->used_size = 0;

	theblock = theheap->start;
	if(theblock->back_flag!=HEAP_DUMMY_FLAG) return 2;

	while(not_done) {
		if(__lwp_heap_blockfree(theblock)) {
			theinfo->free_blocks++;
			theinfo->free_size += __lwp_heap_blocksize(theblock);
		} else {
			theinfo->used_blocks++;
			theinfo->used_size += __lwp_heap_blocksize(theblock);
		}

		if(theblock->front_flag!=HEAP_DUMMY_FLAG) {
			nextblock = __lwp_heap_nextblock(theblock);
			if(theblock->front_flag!=nextblock->back_flag) return 2;
		}

		if(theblock->front_flag==HEAP_DUMMY_FLAG)
			not_done = 0;
		else
			theblock = nextblock;
	}
	return 0;
}
