#include "dip.h"
#include "emu.h"

#include <gctypes.h>
#include <mem.h>
#include <files.h>

static u8 Heapspace[0x40000];

int main()
{
	InitializeHeap(Heapspace, sizeof(Heapspace), 8);

	File_Init();
	ProxiIOS::DIP::DIP dip;
#if 0
	ProxiIOS::EMU::EMU emu;
	emu.Start();
#endif

	return dip.Loop();
}
