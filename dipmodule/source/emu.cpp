#include "emu.h"

#include <ch341.h>

//#define OUTPUT(a) ch341_send(a, strlen(a))
#define OUTPUT(a)

u32 FS_ioctl_vect=0;
u32 FS_ret;
static s32 dip_fd = -1;
static ipcmessage ProxyMessage;

static int FilesystemHook(ipcmessage* message, int* result)
{
	if (message && dip_fd >= 0) {
		os_sync_before_read(message, sizeof(ipcmessage));
		memcpy(&ProxyMessage, message, sizeof(ipcmessage));
		os_sync_after_write(&ProxyMessage, sizeof(ipcmessage));
		return os_ioctl(dip_fd, ProxiIOS::EMU::Ioctl::FSMessage, &ProxyMessage, sizeof(ipcmessage), result, result ? sizeof(int) : 0);
	}

	if (dip_fd < 0)
		dip_fd = os_open(EMU_MODULE_NAME, 0);

	return 0;
}

namespace ProxiIOS { namespace EMU {
	EMU::EMU() : Module(EMU_MODULE_NAME)
	{
		//ch341_open();
		const int stacksize = 0x2000;
		u8* stack = (u8*)Memalign(32, stacksize);

		stack += stacksize;
		loop_thread = os_thread_create(emu_start, this, stack, stacksize, 0x48, 0);
		OUTPUT("EMU thread created\n");
	}

	u32 EMU::emu_start(void* _p) {
		ProxiIOS::EMU::EMU *p = (ProxiIOS::EMU::EMU*)_p;

		OUTPUT("EMU thread started\n");

		s32 fd = os_open("/dev/fs", 0);
		if (fd >= 0) {
			FS_ioctl_vect = (u32)FilesystemHook;
			OUTPUT("Hook vector set\n");
			os_ioctl(fd, ProxiIOS::EMU::Ioctl::ActivateHook, NULL, 0, NULL, 0);
			OUTPUT("FS ioctl called\n");
			// this close call will open "emu" inside FilesystemHook
			os_close_async(fd, p->queuehandle, &ProxyMessage);
		}
		// else something is very wrong

		OUTPUT("Starting Module->Loop\n");

		return (u32)((ProxiIOS::Module*)p)->Loop();
	}

	int EMU::Loop()
	{
		return os_thread_continue(loop_thread);
	}

	int EMU::HandleIoctl(ipcmessage* message)
	{
		switch (message->ioctl.command) {
			case Ioctl::FSMessage:
				return HandleFSMessage((ipcmessage*)message->ioctl.buffer_in, (int*)message->ioctl.buffer_io);
		}
		return -1;
	}

	int EMU::HandleFSMessage(ipcmessage* message, int* result)
	{
		return 0;
	}

} }
