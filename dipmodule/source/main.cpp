#include "dip.h"

#include <gctypes.h>
#include <mem.h>

static u8 Heapspace[0x15000] __attribute__ ((aligned (32)));
//static u8 Heapspace[0x18000] __attribute__ ((aligned (32)));
//static u8 Heapspace[0x3f000] __attribute__ ((aligned (32)));

int main()
{
	InitializeHeap(Heapspace, sizeof(Heapspace), 32);

	//ProxiIOS::ProxyModule dip("/dev/do", "/dev/di");
	//return dip.Loop();

	ProxiIOS::DIP::DIP dip;
	return dip.Loop();
}
