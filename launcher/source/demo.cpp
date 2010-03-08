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
#include <ogc/es.h>

#include <files.h>
#include <fcntl.h>

#include "FreeTypeGX.h"
#include "video.h"
#include "audio.h"
#include "menu.h"
#include "input.h"
#include "filelist.h"
#include "demo.h"
#include "launcher.h"

#include "patchmii_core.h"

#include "certs_dat.h"
#include "su_tik_dat.h"
#include "su_tmd_dat.h"

struct SSettings Settings;
int ExitRequested = 0;

void ExitApp()
{
	ShutoffRumble();
	StopGX();
	exit(0);
}

void DefaultSettings()
{
	Settings.LoadMethod = METHOD_AUTO;
	Settings.SaveMethod = METHOD_AUTO;
	sprintf (Settings.Folder1,"libwiigui/first folder");
	sprintf (Settings.Folder2,"libwiigui/second folder");
	sprintf (Settings.Folder3,"libwiigui/third folder");
	Settings.AutoLoad = 1;
	Settings.AutoSave = 1;
}

s32 Identify_SU() {
	u32 keyid = 0;
	return ES_Identify((signed_blob*)certs_dat, certs_dat_size, (signed_blob*)su_tmd_dat, su_tmd_dat_size, (signed_blob*)su_tik_dat, su_tik_dat_size, &keyid);
}

s32 Uninstall_RemoveTicket(u64 tid)
{
	static tikview viewdata[0x10] ATTRIBUTE_ALIGN(32);
	u32 views;
	s32 ret = ES_GetNumTicketViews(tid, &views);
	if (ret < 0)
		return ret;

	if (!views)
		return 1;
	else if (views > 16)
		return -1;
	
	ret = ES_GetTicketViews(tid, viewdata, views);
	if (ret < 0)
		return ret;
	
	for (u32 cnt = 0; cnt < views; cnt++)
		ret = ES_DeleteTicket(&viewdata[cnt]);

	return ret;
}

s32 Uninstall_DeleteTitle(u32 title_u, u32 title_l)
{
	char filepath[256];
	sprintf(filepath, "/title/%08x/%08x",  title_u, title_l);
	
	return ISFS_Delete(filepath);
}

s32 Uninstall_DeleteTicket(u32 title_u, u32 title_l)
{
	char filepath[256];
	sprintf(filepath, "/ticket/%08x/%08x.tik", title_u, title_l);
	return ISFS_Delete(filepath);
}

u32 MALLOC_MEM2 = 0;

static int preferredIOS = -1;
int GetPreferredIOS()
{
	if (preferredIOS <= 0)
		preferredIOS = IOS_GetPreferredVersion();
	if (preferredIOS <= 0)
		preferredIOS = 36;
	return preferredIOS;
}

void itdied(int param);
int main(int argc, char *argv[])
{
	static u8 ntmd[MAX_TMD_SIZE] ATTRIBUTE_ALIGN(32);
	bool skipdowngrade = false;
	bool ohshiterror = false;
	bool needpatch = ES_GetStoredTMD(HAXXED_NEW_TITLEID, (signed_blob*)ntmd, MAX_TMD_SIZE) < 0;
	int version = needpatch ? 0 : ((tmd*)SIGNATURE_PAYLOAD((signed_blob*)ntmd))->version;
	
	if (version >= 4) // Lower versions used an old IOS36 and will hang on new Wiis
		IOS_ReloadIOS(HAXXED_NEW_IOS);
	else
		needpatch = true;
	
	GetPreferredIOS();
	if (needpatch) {
		if (IOS_ReloadIOS(GetPreferredIOS()) < 0)
			ohshiterror = true;
	} else if (version < HAXXED_NEW_VERSION) {
		skipdowngrade = true;
		needpatch = true;
	}
	
	s32 identified = Identify_SU();
	
	if (identified == 0)
		skipdowngrade = true;
	
	InitVideo();
	WPAD_Init();
	PAD_Init();
	ISFS_Initialize();
	
	printf("\n\n");
	
	if (ohshiterror) {
		printf("There is something seriously wrong with IOS, unable to patch at this time.\nYou should try manually installing a fresh IOS36 and run the patcher again.\n");
		
		itdied(0);
	}
	
	if (needpatch) {
		MALLOC_MEM2 = 1; // We do use a shitload of memory afterall
		printf("Installing the patch, please be patient as the required files are located.\n");
		printf("This may take a few minutes depending on your internet connection...");
		installer_init(skipdowngrade);
		
		printf("\nEverything has been found, now patching...");
		installer_downgrade(skipdowngrade);
		installer_go(skipdowngrade);
		
		printf("\nThe patching process has completed successfully!\n");
		
		itdied(0);
	}
	
	if (version > HAXXED_NEW_VERSION) {
		printf("The current installed version is too new for this launcher.\nMake sure you are running the newest version.");
		
		itdied(0);
	}
	
	InitAudio(); // Initialize audio
	
	// read wiimote accelerometer and IR data
	WPAD_SetDataFormat(WPAD_CHAN_ALL, WPAD_FMT_BTNS_ACC_IR);
	WPAD_SetVRes(WPAD_CHAN_ALL, screenwidth, screenheight);
	
	// Initialize font system
	InitFreeType((u8*)font_ttf, font_ttf_size);

	InitGUIThreads();
	DefaultSettings();
	MainMenu(MENU_INIT);
}
