#include "rawksd_menu.h"
#include "launcher.h"
#include "riivolution.h"
#include "riivolution_config.h"
#include "haxx.h"
#include "init.h"
#include "wdvd.h"
#include "motd.h"

#include <files.h>

#include <math.h>
#include <unistd.h>
#include <vector>
using std::vector;

enum { REGION_NTSC = 0, REGION_PAL };

extern "C" {
	extern u8 rb2_ntsc_xml[];
	extern u32 rb2_ntsc_xml_size;
	extern u8 rb2_pal_xml[];
	extern u32 rb2_pal_xml_size;

	extern u8 menu_play_png[];
	extern u8 menu_play_sel_png[];
	extern u8 menu_rip_png[];
	extern u8 menu_rip_sel_png[];
	extern u8 menu_scores_png[];
	extern u8 menu_scores_sel_png[];
	extern u8 menu_save_png[];
	extern u8 menu_save_sel_png[];
	extern u8 menu_exit_png[];
	extern u8 menu_exit_sel_png[];
}

bool CheckDisks();

class GuiImageCache
{
public:
	typedef vector<GuiImageData*> ImageList;
	ImageList Data;

	~GuiImageCache()
	{
		for (ImageList::iterator iter = Data.begin(); iter != Data.end(); iter++)
			delete *iter;
	}
};

Menus::Enum MenuMain()
{
	GuiImageCache cache;
	cache.Data.push_back(new GuiImageData(menu_play_png));
	cache.Data.push_back(new GuiImageData(menu_play_sel_png));
	cache.Data.push_back(new GuiImageData(menu_rip_png));
	cache.Data.push_back(new GuiImageData(menu_rip_sel_png));
	cache.Data.push_back(new GuiImageData(menu_scores_png));
	cache.Data.push_back(new GuiImageData(menu_scores_sel_png));
	cache.Data.push_back(new GuiImageData(menu_save_png));
	cache.Data.push_back(new GuiImageData(menu_save_sel_png));
	cache.Data.push_back(new GuiImageData(menu_exit_png));
	cache.Data.push_back(new GuiImageData(menu_exit_sel_png));
/*	FX work in progress
	float shine_pos = 0;
	GuiImageData x(*cache.Data[1]);
	GuiImage y(&x);
*/
	HaltGui();
	ButtonList buttons(Window, 5);
	//buttons.SetButton(0, cache.Data[0], &x);
	buttons.SetButton(0, cache.Data[0], cache.Data[1]);
	buttons.SetButton(1, cache.Data[2], cache.Data[3]);
	buttons.SetButton(2, cache.Data[4], cache.Data[5]);
	buttons.SetButton(3, cache.Data[6], cache.Data[7]);
	buttons.SetButton(4, cache.Data[8], cache.Data[9]);
	buttons.GetButton(4)->SetTrigger(&Trigger[Triggers::Home]);
	buttons.GetButton(0)->SetState(STATE_SELECTED, 0);
	ResumeGui();

	Launcher_ScrubPlaytimeEntry();

	RVL_Initialize();

	Launcher_Init();

	while (true) {
		switch (buttons.Pressed()) {
			case 0:
				return Menus::Launch;
			case 1:
				return Menus::Rip;
			case 2:
				return Menus::Scores;
			case 3:
				return Menus::Save;
			case 4:
				return Menus::Exit;
		}
		CheckShutdown();
/*
		HaltGui();
		memcpy(x.GetImage(), cache.Data[1]->GetImage(), x.GetWidth()*x.GetHeight()*4);
		DCFlushRange(x.GetImage(), x.GetWidth()*x.GetHeight()*4);
		y.SetShine(180, (sin(shine_pos)+1)*120+15, 40);
		shine_pos += 0.01;
		if (shine_pos >= 2*M_PI)
			shine_pos = 0;
		ResumeGui();
*/
		CheckDisks();
	}
}

Menus::Enum MenuLaunch()
{
	if (!CheckDisks()) {
		HaltGui(); Subtitle->SetText("Insert Wii game disc."); ResumeGui();
		sleep(3);
		if (!CheckDisks())
			return Menus::Main;
	}

	ShutoffRumble();

	RVL_SetFST(NULL, 0);

	Launcher_RVL();

	vector<RiiDisc> discs;
	ParseXML((const char*)rb2_ntsc_xml, rb2_ntsc_xml_size, &discs, "", "", 0);
	ParseXML((const char*)rb2_pal_xml, rb2_pal_xml_size, &discs, "", "", 0);
	Disc = CombineDiscs(&discs);

	for (vector<int>::iterator mount = Mounted.begin(); mount != Mounted.end(); mount++) {
		char mountpoint[MAXPATHLEN];
		if (!File_GetMountPoint(*mount, mountpoint, MAXPATHLEN))
			RVL_DLC(mountpoint);
	}

	RVL_Patch(&Disc);

	Launcher_RunApploader();
	Launcher_CommitRVL(false);
	Launcher_AddPlaytimeEntry();
	Launcher_SetVideoMode();
	RVL_PatchMemory(&Disc);
	RVL_Unmount();
	if (File_GetLogFS() < 0)
		File_Deinit();

	Launcher_Launch();

	return Menus::Exit;
}

#define DEFAULT() if (!hasdefault) { ret = File_SetDefault(ret); if (ret >= 0) hasdefault = true; }
void RawkSD_Mount(vector<int>* mounted)
{
	int fd = File_Init();
	if (fd < 0)
		return;

	bool hasdefault = false;
	int ret;

	ret = File_Fat_Mount(SD_DISK, "sd");
	if (ret >= 0) {
		mounted->push_back(ret);
		DEFAULT();
	}

	ret = File_Fat_Mount(USB_DISK, "usb");
	if (ret >= 0) {
		mounted->push_back(ret);
		DEFAULT();
	}

	ret = File_RiiFS_Mount("", 5256);
	if (ret >= 0) {
		mounted->push_back(ret);
		if (!hasdefault) {
			ToMount.push_back(ret);
			File_SetLogFS(ret);
		}
		DEFAULT();
	}
}

bool CheckDisks()
{
	if (!Mounted.size())
		RawkSD_Mount(&Mounted);

	static bool disc = false;

	if (!disc) {
		LauncherStatus::Enum status = Launcher_ReadDisc();
		switch (status) {
			case LauncherStatus::ReadError: // TODO: Special-case PS2 discs?
				HaltGui(); Subtitle->SetText("Error reading disc."); ResumeGui();
				break;
			case LauncherStatus::OK:
				disc = true;
			default: // Passthrough
				HaltGui(); Subtitle->SetText(GetMotd()); ResumeGui();
				break;
		}
	} else if (!Launcher_DiscInserted())
		disc = false;

	return disc;
}

