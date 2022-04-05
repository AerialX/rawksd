#include <gccore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ogcsys.h>
#include <unistd.h>
#include <wiiuse/wpad.h>

#include "video.h"
#include "audio.h"
#include "input.h"

#include "haxx.h"
#include "installer.h"
#include "wdvd.h"
#include "files.h"
#include "init.h"

extern u8 _start[], __RO_END[];

u32 is_wiiu;

bool PressA()
{
	printf("\tPress A to continue or press Home to exit.\n");
	while (true) {
		VIDEO_WaitVSync();
		WPAD_ScanPads();

		int down = WPAD_ButtonsDown(0) |
			WPAD_ButtonsDown(1) |
			WPAD_ButtonsDown(2) |
			WPAD_ButtonsDown(3);
		if (down & WPAD_BUTTON_A)
			return true;
		if (down & WPAD_BUTTON_HOME)
			return false;
	}
}

void PressHome()
{
	printf("\tPress Home to exit.\n");
	do {
		VIDEO_WaitVSync();
		WPAD_ScanPads();
	} while (!((WPAD_ButtonsDown(0) |
		WPAD_ButtonsDown(1) |
		WPAD_ButtonsDown(2) |
		WPAD_ButtonsDown(3)) & WPAD_BUTTON_HOME));
}

static volatile int ShutdownParam = 0;

static void CallbackReset(u32 a, void* b)
{
	ShutdownParam = SYS_RETURNTOMENU;
}

static void CallbackPoweroffWiimote(int chan)
{
	ShutdownParam = SYS_POWEROFF;
}

static void CallbackPoweroff()
{
	CallbackPoweroffWiimote(0);
}

void CheckShutdown()
{
	bool disc;
	int i;
	static union {
		struct {
			u32 checksum;
			u8 flags;
			u8 type;
			u8 discstate;
			u8 returnto;
		};
		u32 padding[8];
	} state ATTRIBUTE_ALIGN(32);

	switch (ShutdownParam) {
		case SYS_POWEROFF:
			memset(&state, 0, sizeof(state));
			i = File_Open("/mnt/isfs/title/00000001/00000002/data/state.dat", O_RDONLY);
			if (i>=0) {
				File_Read(i, &state, sizeof(state));
				File_Close(i);
			}

			state.type = 5;
			state.discstate = 0;
			if (!WDVD_VerifyCover(&disc) && disc)
				state.discstate = 1;

			state.padding[0] = 0; // fix checksum
			for (i=1; i < 8; i++)
				state.padding[0] += state.padding[i];

			i = File_Open("/mnt/isfs/title/00000001/00000002/data/state.dat", O_WRONLY);
			if (i>=0) {
				File_Write(i, &state, sizeof(state));
				File_Close(i);
			}
			WII_LaunchTitle(0x100000002LL);

		case SYS_RETURNTOMENU:
			SYS_ResetSystem(ShutdownParam, 0, 0);
	}
}

static void fix_mem_hdlr()
{
	SYS_ProtectRange(SYS_PROTECTCHAN3, _start, __RO_END-_start, SYS_PROTECTREAD);
}

u32 MALLOC_MEM2 = 1;
enum { INSTALL_APPROACH_NOTHING = 0, INSTALL_APPROACH_UPDATE, INSTALL_APPROACH_DOWNGRADE };
void Initialise()
{
	fix_mem_hdlr();

	is_wiiu = 0;
	if (ES_GetDeviceID(&is_wiiu)<0 || is_wiiu >= 0x20000000)
		is_wiiu = 1;
	else
		is_wiiu = 0;

	InitVideo();

	if (Haxx_Init() < 0) {
		int approach = 0;
		WPAD_Init();
		printf("\n\n");
		if (is_wiiu) {
			printf("IOS Error. Please try relaunching this program from HBC.\n");
			PressHome();
			exit(0);
		} else if (IOS_GetVersion() != (u32)HAXX_IOS) {
			printf("\tIOS%d does not seem to be installed on your system.\n\tIt's perfectly safe to install it; do you want to do so now?\n", (u32)HAXX_IOS);
			if (!PressA())
				exit(0);
			approach = INSTALL_APPROACH_UPDATE;
		} else if (IOS_GetRevision() < HAXX_IOS_MINIMUM) {
			printf("\tIOS%d must be updated to continue.\n\tIt's perfectly safe to update it; do you want to do so now?\n", (u32)HAXX_IOS);
			if (!PressA())
				exit(0);
			approach = INSTALL_APPROACH_UPDATE;
		} else if (IOS_GetRevision() > HAXX_IOS_MAXIMUM) {
			// Either cIOScrap (which will make the downgrade exploit fail) or a future update
			printf("\tIOS%d must be downgraded to continue. Do you want to attempt this now?\n", (u32)HAXX_IOS);
			if (!PressA())
				exit(0);
			approach = INSTALL_APPROACH_DOWNGRADE;
		} else {
			// Proper version, but a patch failed. RawkSD patcher or DOP-IOS or something.
			printf("\tIOS%d must be reinstalled to continue.\n\tThis is a perfectly safe to do; do you want to reinstall it now?\n", (u32)HAXX_IOS);
			if (!PressA())
				exit(0);
			approach = INSTALL_APPROACH_UPDATE;
		}

		int ret = 0;
		Installer_Initialize();
		switch (approach) {
			case INSTALL_APPROACH_UPDATE:
				ret = Install(HAXX_IOS, /*HAXX_IOS_MAXIMUM*/5663, false);
				break;
			case INSTALL_APPROACH_DOWNGRADE:
				ret = Install(HAXX_IOS, /*HAXX_IOS_MAXIMUM*/5663, true);
				break;
			default:
				exit(0);
		}
		Installer_Deinitialize();

		if (ret < 0) {
			printf("\tThe installation did not complete successfully.\n");
			PressHome();
			exit(0);
		} else {
			printf("\tThe installation was completed successfully!\n");
			WPAD_Shutdown();
			if (Haxx_Init()<0) {
				WPAD_Init();
				printf("\tSomething still seems to be wrong; I'm getting outta here!\n");
				PressHome();
				exit(0);
			}
		}
	}

	Init_DebugConsole();

	SetupPads();
	InitAudio();

	SYS_SetResetCallback(CallbackReset);
	SYS_SetPowerCallback(CallbackPoweroff);
	WPAD_SetPowerButtonCallback(CallbackPoweroffWiimote);
}

