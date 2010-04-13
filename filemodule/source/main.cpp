#include "filemodule.h"

#include "gctypes.h"
#include "mem.h"

static u8 Heapspace[0x28000];

int main()
{
	InitializeHeap(Heapspace, sizeof(Heapspace), 8);

	ProxiIOS::Filesystem::Filesystem filesystem;

	return filesystem.Loop();
}
