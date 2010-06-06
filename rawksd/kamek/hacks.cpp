#include "common.h"

extern "C" int HeapFreeBlockStatsHook(int ret, int& totalsize, int& blockcount, int& largestblock)
{
	__OSReport("Heap::FreeBlockStats(): 0x%08x 0x%08x 0x%08x\n", totalsize, blockcount, largestblock);
	return ret;
}

