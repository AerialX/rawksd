/****************************************************************************
 * libwiigui Template
 * Tantric 2009
 *
 * demo.cpp
 * Basic template/demonstration of libwiigui capabilities. For a
 * full-featured app using many more extensions, check out Snes9x GX.
 ***************************************************************************/

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
#include "demo.h"
#include "launcher.h"

#include "patchmii_core.h"

struct SSettings Settings;
int ExitRequested = 0;

void ExitApp()
{
	ShutoffRumble();
	StopGX();
	exit(0);
}

void
DefaultSettings()
{
	Settings.LoadMethod = METHOD_AUTO;
	Settings.SaveMethod = METHOD_AUTO;
	sprintf (Settings.Folder1,"libwiigui/first folder");
	sprintf (Settings.Folder2,"libwiigui/second folder");
	sprintf (Settings.Folder3,"libwiigui/third folder");
	Settings.AutoLoad = 1;
	Settings.AutoSave = 1;
}

extern void startup();

#include <files.h>
#include <fcntl.h>

u32 MALLOC_MEM2 = 0;

void installer_downgrade();
void installer_go();
void installer_init();
void installer_cleanup();
void itdied(int param);
int main(int argc, char *argv[])
{
	bool needpatch = IOS_ReloadIOS(HAXXED_NEW_IOS) < 0;
	bool ohshiterror = false;
	int version = IOS_GetRevision();
	if (needpatch) {
		int initios = IOS_GetPreferredVersion();
		if (initios < 0)
			initios = 36;
		if (IOS_ReloadIOS(initios) < 0)
			ohshiterror = true;
	} else if (version < HAXXED_NEW_VERSION)
		needpatch = true;

	WPAD_Init();
	PAD_Init();
	InitVideo(); // Initialise video
	
	printf("\n\n");
	
	if (ohshiterror) {
		printf("There is something seriously wrong with IOS, unable to patch at this time.\nYou should try manually installing a fresh IOS36 and run the patcher again.\n");
		itdied(0);
	}
	
	if (needpatch) {
		MALLOC_MEM2 = 1; // We do use a shitload of memory afterall
		printf("Please be patient as the required files are located.\nThis may take a few minutes depending on your internet connection...");
		installer_init();
		
		printf("\nEverything has been found, now patching...");
		installer_downgrade();
		installer_go();
		
		printf("\nThe patching process has completed successfully!\n");
		
		itdied(0);
	}
	
	if (version > HAXXED_NEW_VERSION) {
		printf("The current installed version is too new for this launcher.\nMake sure you are running the newest version.");
		
		itdied(0);
	}
	/* test
	int ret = LoadDisc_Init();
	if (ret < 0)
		printf("\t%s\n", LoadDisc_Error(ret));
	else {
		printf("\tSUCCESS!\n");
		printf("\tTrying to read sd://test.txt...\n");
		char buf[0x100];
		int fd = File_Open("/test.txt", O_RDONLY);
		if (fd <= 0) {
			printf("\tNope, couldn't open the file!\n");
		}
		int read = File_Read(fd, (u8*)buf, 0x100);
		buf[read == 0x100 ? read - 1 : read] = '\0';
		
		printf("\t\"%s\"\n", buf);
		
		File_Close(fd);
	}
	
	printf("\tPress Home to quit...");
	while (true) {
		WPAD_ScanPads();
		if (WPAD_ButtonsDown(WPAD_CHAN_0) & WPAD_BUTTON_HOME)
			break;
	}
	
	exit(0);
	*/
	InitAudio(); // Initialize audio

	// read wiimote accelerometer and IR data
	WPAD_SetDataFormat(WPAD_CHAN_ALL,WPAD_FMT_BTNS_ACC_IR);
	WPAD_SetVRes(WPAD_CHAN_ALL, screenwidth, screenheight);

	// Initialize font system
	InitFreeType((u8*)font_ttf, font_ttf_size);

	InitGUIThreads();
	DefaultSettings();
	MainMenu(MENU_INIT);
}
