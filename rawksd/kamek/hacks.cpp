#include "common.h"

#ifdef DEBUG
extern "C" int HeapFreeBlockStatsHook(int ret, int& totalsize, int& blockcount, int& largestblock)
{
	__OSReport("Heap::FreeBlockStats(): 0x%08x 0x%08x 0x%08x\n", totalsize, blockcount, largestblock);
	return ret;
}
#endif

struct HIDDevice
{
	u32 Unknown;
	u16 PID1;
	u16 PID2;
};
struct UsbWii
{
	static int GetType(HIDDevice*);
};

extern "C" int UsbTypeHook(HIDDevice* hid)
{
	if (hid->PID1 != 0x12BA)
		return UsbWii::GetType(hid);

	switch (hid->PID2) {
		case 0x0100:
		case 0x0200:
			return 1; // Guitar
		case 0x0210:
		case 0x0120:
			return 3; // Drums
	}

	return 0;
}

