#pragma once

#include "lwp_heap.h"

#ifdef __cplusplus
	extern "C" {
#endif

bool InitializeHeap(void* heapspace, u32 size, u32 pagesize);
void* Alloc(u32 size);
void* Memalign(u32 align, u32 size);
bool Dealloc(void* data);
void* Realloc(void* data, u32 size, u32 oldsize);
u32 HeapInfo();

#ifdef __cplusplus
	}
	inline void* operator new(size_t size) {
		return Alloc(size);
	}
	inline void operator delete(void* data) {
		Dealloc(data);
	}
	inline void* operator new[](size_t size) {
		return Alloc(size);
	}
	inline void operator delete[](void* data) {
		Dealloc(data);
	}
#endif
