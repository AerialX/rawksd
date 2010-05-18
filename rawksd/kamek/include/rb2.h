#pragma once

#include "common.h"

void* MemAlloc(int size, int align);
void MemFree(void* pointer);

#include "rb2/Main.h"
#include "rb2/RockCentralGateway.h"
#include "rb2/PlatformMgr.h"
#include "rb2/PassiveMessenger.h"
#include "rb2/HttpWii.h"

#define memalign(align, size) \
	MemAlloc(size, align)
#define malloc(size) \
	MemAlloc(size, 0)
#define free(ptr) \
	MemFree(ptr)

