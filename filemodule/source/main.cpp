#include "filemodule.h"

#include "gctypes.h"
#include "mem.h"

static u8 Heapspace[0xa000] __attribute__ ((aligned (32)));

int main()
{
	InitializeHeap(Heapspace, sizeof(Heapspace), 32);

	ProxiIOS::Filesystem::Filesystem filesystem;

	return filesystem.Loop();
}
