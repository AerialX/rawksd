#include "rawksd_menu.h"
#include "launcher.h"
#include "riivolution.h"
#include "riivolution_config.h"
#include "haxx.h"
#include "init.h"
#include "wdvd.h"
#include "motd.h"

#include <files.h>

#include <sys/param.h>
#include <unistd.h>
#include <vector>
#include <map>

#include "rb2_ntsc_xml.h"
#include "rb2_pal_xml.h"
#include "ntsc_usb_tpl.h"
#include "ntsc_sd_tpl.h"
#include "ntsc_wifi_tpl.h"
#include "pal_usb_tpl.h"
#include "pal_sd_tpl.h"
#include "pal_wifi_tpl.h"

extern std::map<int, bool> UsedFilesystems;

static const struct {const u8 *data; const u32 size;} splash_tpls[] = {
	{ntsc_sd_tpl, ntsc_sd_tpl_size}, {pal_sd_tpl, pal_sd_tpl_size},
	{ntsc_usb_tpl, ntsc_usb_tpl_size}, {pal_usb_tpl, pal_usb_tpl_size},
	{ntsc_wifi_tpl, ntsc_wifi_tpl_size}, {pal_wifi_tpl, pal_wifi_tpl_size}
};

MenuPlay::MenuPlay(GuiWindow *_Main) :
RawkMenu(NULL, "\nInitializing.....", "Launching RB2"),
Main(_Main)
{
}

RawkMenu *MenuPlay::Process()
{
	char status_text[200];
	char mountpoint[MAXPATHLEN];
	RVL_SetFST(NULL, 0);
	LauncherStatus::Enum status = Launcher_ReadDisc();
	HaltGui();
	switch (status) {
		case LauncherStatus::NoDisc:
			return new MenuSaves(Main, "No Disc Found", "\nPlease insert the RB2 disc and try again.");
		case LauncherStatus::IosError:
			return new MenuSaves(Main, "IOS Error", "\nCouldn't reset the disc drive. Try restarting RawkSD before attempting to launch again.");
		case LauncherStatus::ReadError:
			return new MenuSaves(Main, "Read Error", "\nCouldn't read the disc, are you sure it is a proper Wii disc?");
		default: // LauncherStatus::OK
			if (memcmp(MEM_BASE, "SZA", 3) || (MEM_BASE[3] != 'E' && MEM_BASE[3] != 'P'))
				return new MenuSaves(Main, Launcher_GetGameName(), "\nOnly Rock Band 2 can be launched. Insert the correct disc and try again.");
			popup_text[0]->SetText(Launcher_GetGameNameWide());
			sprintf(status_text, "\nReading Disc.....");
			popup_text[1]->SetText(status_text);
			ResumeGui();
	}

	status = Launcher_RVL();
	HaltGui();
	switch (status) {
		case LauncherStatus::ReadError:
			return new MenuSaves(Main, "Read Error", "\nCouldn't read the disc info for Riivolution.");
		case LauncherStatus::OutOfMemory:
			return new MenuSaves(Main, "Memory Error", "\nOut of Memory. Not sure how this happened, but it's bad.");
		default: // LauncherStatus::OK
			strcat(status_text, "\nApplying Patches.....");
			popup_text[1]->SetText(status_text);
			ResumeGui();
	}

	std::vector<RiiDisc> discs;
	ParseXML((const char*)rb2_ntsc_xml, rb2_ntsc_xml_size, &discs, "", "", 0);
	ParseXML((const char*)rb2_pal_xml, rb2_pal_xml_size, &discs, "", "", 0);
	Disc = CombineDiscs(&discs);

	if (default_mount>=0) {
		File_SetDefault(default_mount);
		if (!File_GetMountPoint(default_mount, mountpoint, sizeof(mountpoint))) {
			int out_fd;
			int index = 0;
			RVL_DLC(mountpoint);
			UsedFilesystems[default_mount] = true;
			if (default_mount==wifi_mounted) {
				ToMount.push_back(wifi_mounted);
				index = 4;
			} else if (default_mount==usb_mounted)
				index = 2;

			HaltGui();
			strcat(status_text, "\nSaving Configuration.....");
			popup_text[1]->SetText(status_text);
			ResumeGui();

			if (MEM_BASE[3] == 'E') {
				File_CreateFile("/mnt/isfs/tmp/ntsc.tpl");
				out_fd = File_Open("/mnt/isfs/tmp/ntsc.tpl", O_WRONLY);
				if (out_fd >=0) {
					File_Write(out_fd, splash_tpls[index].data, splash_tpls[index].size);
					File_Close(out_fd);
				}
			}
			else if (MEM_BASE[3] == 'P') {
				File_CreateFile("/mnt/isfs/tmp/pal.tpl");
				out_fd = File_Open("/mnt/isfs/tmp/pal.tpl", O_WRONLY);
				if (out_fd >=0) {
					File_Write(out_fd, splash_tpls[index+1].data, splash_tpls[index+1].size);
					File_Close(out_fd);
				}
			}
			File_CreateDir("/rawk");
			File_CreateDir("/rawk/rb2");
			File_CreateFile("/rawk/rb2/config");
			out_fd = File_Open("/rawk/rb2/config", O_WRONLY);
			if (out_fd >=0) {
				global_config.timestamp = time(NULL);
				global_config.version = 2;
				File_Write(out_fd, &global_config, sizeof(global_config));
				File_Close(out_fd);
			}
			File_CreateFile("/rawk/rb2/id");
			out_fd = File_Open("/rawk/rb2/id", O_WRONLY);
			if (out_fd >=0) {
				File_Write(out_fd, &otp.ng_id, sizeof(otp.ng_id));
				File_Close(out_fd);
			}
		} else
			default_mount = -1;
		if (sd_mounted>=0 && sd_mounted!=default_mount)
			File_Unmount(sd_mounted);
		if (usb_mounted>=0 && usb_mounted!=default_mount)
			File_Unmount(usb_mounted);
		if (wifi_mounted>=0 && wifi_mounted!=default_mount)
			File_Unmount(wifi_mounted);
	} else
		Disc.Sections[0].Options[1].Default = 0;

	if (!global_config.leaderboards) // disable magic leaderboard word if necessary
		Disc.Sections[0].Options[2].Default = 2;

//	flip video if April 1st
	time_t now = time(NULL);
	struct tm *tm_now = localtime(&now);
	if (tm_now && tm_now->tm_mon==3 && tm_now->tm_mday==1)
	{
		Disc.Sections[0].Options[3].Default = 1;
	}

	RVL_Patch(&Disc);

	HaltGui();
	strcat(status_text, "\nRunning Apploader.....");
	popup_text[1]->SetText(status_text);
	ResumeGui();
	status = Launcher_RunApploader();
	HaltGui();
	if (status != LauncherStatus::OK)
		return new MenuSaves(Main, "Read Error", "\nApploader Error, couldn't read the disc");
	strcat(status_text, "\nAdding playtime entry and setting video mode.....");
	popup_text[1]->SetText(status_text);
	ResumeGui();
	Launcher_CommitRVL(false);
	Launcher_AddPlaytimeEntry();
	Launcher_SetVideoMode();

	HaltGui();
	strcat(status_text, "\nBooting Disc.....");
	popup_text[1]->SetText(status_text);
	ResumeGui();

	RVL_PatchMemory(&Disc);
	RVL_Unmount();
	if (File_GetLogFS() < 0)
		File_Deinit();

	Launcher_Launch();

	HaltGui();
	return new MenuSaves(Main, "HALP", "\nCatastrophic error while launching the game.");
}
