#pragma once

#include "common.h"

void* MemAlloc(int size, int align);
void MemFree(void* pointer);
void* PoolAlloc(int, int);
void PoolFree(int, void*);

#define memalign(align, size) \
	MemAlloc(size, align)
#define malloc(size) \
	MemAlloc(size, 0)
#define free(ptr) \
	MemFree(ptr)

inline void* operator new(size_t size, void* ptr)
{
	return ptr;
}

template<typename T, typename... Args> T* Alloc(Args... args)
{
	T* t = T::Alloc();
	if (!t)
		return NULL;
	return new((void*)t) T(args...);
}

#include "rb2/Main.h"
#include "rb2/App.h"
#include "rb2/Splash.h"
#include "rb2/String.h"
#include "rb2/BinStream.h"
#include "rb2/DataArray.h"

#include "rb2/PlatformMgr.h"
#include "rb2/SongMgr.h"

#include "rb2/HttpWii.h"

#include "rb2/RockCentralGateway.h"

#include "rb2/PassiveMessenger.h"

