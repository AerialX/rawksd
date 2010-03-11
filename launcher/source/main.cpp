#include <gccore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ogcsys.h>
#include <unistd.h>
#include <wiiuse/wpad.h>
#include <fat.h>

#include "FreeTypeGX.h"
#include "video.h"
#include "audio.h"
#include "menu.h"
#include "input.h"
#include "filelist.h"

#include "haxx.h"
#include "riivolution.h"
#include "riivolution_config.h"
#include "launcher.h"
#include "installer.h"

bool PressA()
{
	printf("Press A to continue or press Home to exit.\n");
	while (true) {
		WPAD_ScanPads();

		int down = WPAD_ButtonsDown(WPAD_CHAN_0);
		if (down & WPAD_BUTTON_A)
			return true;
		if (down & WPAD_BUTTON_HOME)
			return false;
	}
}

void PressHome()
{
	printf("Press Home to exit.\n");
	while (true) {
		WPAD_ScanPads();

		if (WPAD_ButtonsDown(WPAD_CHAN_0) & WPAD_BUTTON_HOME)
			return;
	}
}

enum { INSTALL_APPROACH_NOTHING = 0, INSTALL_APPROACH_UPDATE, INSTALL_APPROACH_DOWNGRADE };
int main(int argc, char *argv[])
{
	InitVideo();

	if (Haxx_Init() < 0) {
		int approach = 0;
		printf("\n\n");
		if (IOS_GetVersion() != 37) {
			printf("IOS37 does not seem to be installed on your system.\nIt's perfectly safe to install it; do you want to do so now?\n");
			if (!PressA())
				return 0;
			approach = INSTALL_APPROACH_UPDATE;
		} else if (IOS_GetRevision() < 3869) {
			printf("IOS37 must be updated to continue.\nIt's perfectly safe to update it; do you want to do so now?\n");
			if (!PressA())
				return 0;
			approach = INSTALL_APPROACH_UPDATE;
		} else if (IOS_GetRevision() > 3869) {
			// Either cIOScrap (which will make the downgrade exploit fail) or a future update
			printf("IOS37 must be downgraded to continue. Do you want to do this now?\n");
			if (!PressA())
				return 0;
			approach = INSTALL_APPROACH_DOWNGRADE;
		} else {
			// Proper version, but a patch failed. RawkSD patcher or DOP-IOS or something.
			printf("IOS37 must be reinstalled to continue. This is a perfectly safe to do; do you want to reinstall it now?\n");
			if (!PressA())
				return 0;
			approach = INSTALL_APPROACH_UPDATE;
		}

		int ret = 0;
		Installer_Initialize();
		switch (approach) {
			case INSTALL_APPROACH_UPDATE:
				ret = Install(HAXX_IOS, HAXX_IOS_VERSION, false);
				break;
			case INSTALL_APPROACH_DOWNGRADE:
				ret = Install(HAXX_IOS, HAXX_IOS_VERSION, true);
				break;
			default:
				return 0;
		}
		Installer_Deinitialize();

		if (ret < 0) {
			printf("The installation did not complete successfully.\n");
			PressHome();
			return 0;
		} else {
			printf("The installation was completed successfully!\n");
			WPAD_Shutdown();
			Haxx_Init();
		}
	}

	SetupPads();
	InitAudio();

	InitFreeType((u8*)font_ttf, font_ttf_size);
	InitGUIThreads();

	MainMenu(Menus::Init);
}
