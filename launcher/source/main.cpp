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

int main(int argc, char *argv[])
{
	InitVideo();
	//videoinit();

	Haxx_Init();

	//return Everything();

	SetupPads();
	InitAudio();

	InitFreeType((u8*)font_ttf, font_ttf_size);
	InitGUIThreads();

	MainMenu(Menus::Init);
}
#if 0
int Everything()
{
	printf("\n\nk we're cool.");

	if (Haxx_Mount() < 0)
		return Menus::Exit;

	printf("1\n");
	RVL_Initialize();
	printf("2\n");
	RVL_SetClusters(false);

	printf("3\n");
	Launcher_Init();
	printf("4\n");
	Launcher_ReadDisc();

	printf("5\n");
	RiiDisc Disc = ParseXMLs();
	ParseConfigXMLs(&Disc);

	SaveConfigXML(&Disc);

	if (Launcher_RVL() < 0)
		return Menus::Exit;
	RVL_Patch(&Disc);

	// Launcher_CommitRVL(true); // TODO: CommitRVL properly?

	Launcher_RunApploader();

	Launcher_CommitRVL(false);

	RVL_PatchMemory(&Disc);

	Launcher_Launch();

	return 1;
}

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;
void videoinit()
{
    VIDEO_Init();
    WPAD_Init();
    rmode = VIDEO_GetPreferredMode(NULL);
    xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
    console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);
    VIDEO_Configure(rmode);
    VIDEO_SetNextFramebuffer(xfb);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if (rmode->viTVMode & VI_NON_INTERLACE)
    	VIDEO_WaitVSync();
}
#endif
