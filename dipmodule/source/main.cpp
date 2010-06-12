#include "dip.h"
#include "emu.h"

#include <gctypes.h>
#include <mem.h>
#include <files.h>

static u8 Heapspace[0x39000];
static u8 emu_stack[0x2000] ATTRIBUTE_ALIGN(32);

int main()
{
	InitializeHeap(Heapspace, sizeof(Heapspace), 8);

	File_Init();
	ProxiIOS::DIP::DIP dip;

	ProxiIOS::EMU::EMU emu(emu_stack, sizeof(emu_stack));
	emu.Start();

	return dip.Loop();
}
