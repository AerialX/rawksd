#pragma once

#include <proxiios.h>

#define EMU_MODULE_NAME "emu"
#define EMU_FD 0x99996666
#define FS_ACTIVATE_HOOK 0x64


namespace ProxiIOS { namespace EMU {
	namespace Ioctl {
		enum Enum {
			FSMessage        = 0x60,
			RedirectDir,
			RedirectFile,
			ActivateHook     = 0x64,
			DeactivateHook   = 0x6F
		};
	}

	namespace FsErrors {
		enum Enum {
			OK                   = IPC_OK,
			InvalidArgument      = -101,
			PermissionDenied     = -102,
			IOError              = -103,
			FileExists           = -105,
			FileNotFound         = -106,
			TooManyFiles         = -107,
			OutOfMemory          = -108,
			NameTooLong          = -110,
			DirNotEmpty          = -115,
			DirDepthExceeded     = -116
		};
	}

	class EMU : public ProxiIOS::Module
	{
	private:
		int loop_thread;
	public:
		EMU();

		int Loop();

		int HandleIoctl(ipcmessage* message);
		int HandleFSMessage(ipcmessage* message, int* result);

		static u32 emu_start(void*);
	};

} }