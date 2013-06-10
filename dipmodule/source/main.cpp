#include "dip.h"
#include "emu.h"
#include "usbhid.h"

#include <gctypes.h>
#include <mem.h>
#include <files.h>

extern u8 HEAP_START[], HEAP_END[];
static u8 emu_stack[0x2000] ATTRIBUTE_ALIGN(32);
static u8 hid_stack[0x2000] ATTRIBUTE_ALIGN(32);
//static u8 ssl_stack[0x2000] ATTRIBUTE_ALIGN(32);

int main()
{
	InitializeHeap(HEAP_START, HEAP_END-HEAP_START, 8);

	File_Init();
	ProxiIOS::DIP::DIP dip;

	ProxiIOS::EMU::EMU emu(emu_stack, sizeof(emu_stack));
	ProxiIOS::USB::HID usbhid(hid_stack, sizeof(hid_stack));
	//ProxiIOS::DEV::DUMP_DEV ssl(ssl_stack, sizeof(ssl_stack), "/dev/net/ss0", "/dev/net/ssl");
	emu.Start();
	usbhid.Start();
	//ssl.Start();

	return dip.Loop();
}
