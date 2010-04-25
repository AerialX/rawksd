#include "dip.h"
#include "emu.h"

#include <gctypes.h>
#include <mem.h>
#include <files.h>

static u8 Heapspace[0x42000];

int main()
{
	InitializeHeap(Heapspace, sizeof(Heapspace), 8);

	File_Init();
	ProxiIOS::DIP::DIP dip;
	ProxiIOS::EMU::EMU emu;

	emu.Loop();
	return dip.Loop();
}
