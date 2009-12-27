#include <string.h>

#include "mem.h"

#define USE_LWP

#ifndef USE_LWP
	#include "syscalls.h"
	static u32 heaphandle;
#else
	static heap_cntrl heap;
#endif

bool InitializeHeap(void* heapspace, u32 size, u32 pagesize)
{
#ifdef USE_LWP
	return __lwp_heap_init(&heap, heapspace, size, pagesize) > 0;
#else
	heaphandle = os_heap_create(heapspace, size);
	return heaphandle > 0;
#endif
}

void* Alloc(u32 size)
{
#ifdef USE_LWP
	return __lwp_heap_allocate(&heap, size);
#else
	return os_heap_alloc(heaphandle, size);
#endif
}

void* Memalign(u32 align, u32 size)
{
#ifdef USE_LWP
	return __lwp_heap_allocate(&heap, size);
#else
	return os_heap_alloc_aligned(heaphandle, size, align);
#endif
}

bool Dealloc(void* data)
{
	if (data) {
#ifdef USE_LWP
		__lwp_heap_free(&heap, data);
#else
		os_heap_free(heaphandle, data);
#endif
	}
	return true;
}

void *Realloc(void* data, u32 size, u32 oldsize)
{
	void *newptr = Alloc(size);
	if (data) {
		if (newptr && oldsize)
			memcpy(newptr, data, oldsize);
		Dealloc(data);
	}
	return newptr;
}

u32 HeapInfo()
{
#ifdef USE_LWP
	heap_iblock heapinfo;
	if (!__lwp_heap_getinfo(&heap, &heapinfo))
		return heapinfo.free_size;
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

