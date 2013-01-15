
#define USE_LWP

#include "mem.h"
#include "lock.h"

#include <string.h>

#ifndef USE_LWP
#include "syscalls.h"
static u32 heaphandle;

#else // USE_LWP

#define MEM_LOCK_MSG 0x5afec0de
static heap_cntrl heap;
static u32 mem_msg;
static lock_t mem_lock = -1;
#endif

bool InitializeHeap(void* heapspace, u32 size, u32 pagesize)
{
#ifdef USE_LWP
	bool ret = __lwp_heap_init(&heap, heapspace, size, pagesize) > 0;
	if (ret)
		mem_lock = InitializeLock(&mem_msg, 0, 1);
	return ret;
#else
	heaphandle = os_heap_create(heapspace, size);
	return heaphandle > 0;
#endif
}

void* Alloc(u32 size)
{
#ifdef USE_LWP
	void *ret;
	GetLock(mem_lock);
	ret = __lwp_heap_allocate(&heap, size, 0);
	ReleaseLock(mem_lock);
	return ret;
#else
	return os_heap_alloc(heaphandle, size);
#endif
}

void* Memalign(u32 align, u32 size)
{
#ifdef USE_LWP
	void *ret;
	GetLock(mem_lock);
	ret = __lwp_heap_allocate(&heap, size, align);
	ReleaseLock(mem_lock);
	return ret;
#else
	return os_heap_alloc_aligned(heaphandle, size, align);
#endif
}

bool Dealloc(void* data)
{
	if (data) {
#ifdef USE_LWP
		GetLock(mem_lock);
		__lwp_heap_free(&heap, data);
		ReleaseLock(mem_lock);
#else
		os_heap_free(heaphandle, data);
#endif
	}
	return true;
}

void *Realloc(void* data, u32 size, u32 oldsize)
{
#ifdef USE_LWP
	void *ret;
	GetLock(mem_lock);
	ret = __lwp_heap_realloc(&heap, data, size);
	ReleaseLock(mem_lock);
	return ret;
#else
	void *new_mem = data;
	if (size > oldsize)
	{
		new_mem = Alloc(size);
		if (new_mem)
		{
			memcpy(new_mem, data, (size > oldsize) ? oldsize : size);
			Dealloc(data);
		}
	}
	return new_mem;
#endif
}

u32 HeapInfo()
{
#ifdef USE_LWP
	u32 free_bytes;
	heap_iblock heapinfo;
	GetLock(mem_lock);
	if (!__lwp_heap_getinfo(&heap, &heapinfo))
		free_bytes = heapinfo.free_size;
	else
		free_bytes = 0;
	ReleaseLock(mem_lock);
	return free_bytes;
#endif
	return 0;
}

void* malloc(size_t n)
{
	return Alloc(n);
}

void* memalign(size_t align, size_t n)
{
	return Memalign(align, n);
}

void free(void* ptr)
{
	Dealloc(ptr);
}
