#include "dip.h"

#include <gctypes.h>
#include <mem.h>

#ifdef DIP_RIIVOLUTION
static u8 Heapspace[0x42000];
#endif
#ifdef DIP_RAWKSD
static u8 Heapspace[0x1c000];
#endif

int main()
{
	InitializeHeap(Heapspace, sizeof(Heapspace), 8);

	ProxiIOS::DIP::DIP dip;
	return dip.Loop();
}
