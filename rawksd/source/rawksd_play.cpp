#include "rawksd_menu.h"
#include "launcher.h"
#include "riivolution.h"
#include "riivolution_config.h"
#include "haxx.h"
#include "init.h"
#include "wdvd.h"
#include "motd.h"

#include <files.h>

#include <unistd.h>
#include <vector>
#include <map>

extern std::map<int, bool> UsedFilesystems;

extern "C" {
	extern const u8 rb2_ntsc_xml[];
	extern const u32 rb2_ntsc_xml_size;
	extern const u8 rb2_pal_xml[];
	extern const u32 rb2_pal_xml_size;

	/* sigh */
	extern const u8 ntsc_usb_tpl[];
	extern const u32 ntsc_usb_tpl_size;
	extern const u8 ntsc_sd_tpl[];
	extern const u32 ntsc_sd_tpl_size;
	extern const u8 ntsc_wifi_tpl[];
	extern const u32 ntsc_wifi_tpl_size;
	extern const u8 pal_usb_tpl[];
	extern const u32 pal_usb_tpl_size;
	extern const u8 pal_sd_tpl[];
	extern const u32 pal_sd_tpl_size;
	extern const u8 pal_wifi_tpl[];
	extern const u32 pal_wifi_tpl_size;
};

static const struct {const u8 *data; const u32 size;} splash_tpls[] = {
	{ntsc_sd_tpl, ntsc_sd_tpl_size}, {pal_sd_tpl, pal_sd_tpl_size},
	{ntsc_usb_tpl, ntsc_usb_tpl_size}, {pal_usb_tpl, pal_usb_tpl_size},
	{ntsc_wifi_tpl, ntsc_wifi_tpl_size}, {pal_wifi_tpl, pal_wifi_tpl_size}
};

static const char *load_error[] = {
	"CONTINUE",
	NULL
};

class PlayError : public RawkMenu
{
private:
	GuiWindow *Main;
public:
	PlayError(GuiWindow *_Main, const char *title, const char *text);
	RawkMenu *Process();
};

PlayError::PlayError(GuiWindow *_Main, const char *title, const char *text) :
RawkMenu(load_error, text, title),
Main(_Main)
{
}

RawkMenu* PlayError::Process()
{
	if (GetClicked()>=0) {
		HaltGui();
		Main->SetState(STATE_DEFAULT);
		return new MenuMain(Main);
	}
	return this;
}

MenuPlay::MenuPlay(GuiWindow *_Main) :
RawkMenu(NULL, "\nInitializing.....", "Launching RB2"),
Main(_Main)
{
}

RawkMenu *MenuPlay::Process()
{
	char mountpoint[MAXPATHLEN];
	RVL_SetFST(NULL, 0);
	LauncherStatus::Enum status = Launcher_ReadDisc();
	HaltGui();
	switch (status) {
		case LauncherStatus::NoDisc:
			return new PlayError(Main, "No Disc Found", "\nPlease insert the RB2 disc and try again.");
		case LauncherStatus::IosError:
			return new PlayError(Main, "IOS Error", "\nCouldn't reset the disc drive. Try restarting RawkSD before attempting to launch again.");
		case LauncherStatus::ReadError:
			return new PlayError(Main, "Read Error", "\nCouldn't read the disc, are you sure it is a proper Wii disc?");
		default: // LauncherStatus::OK
			if (memcmp(MEM_BASE, "SZA", 3))
				return new PlayError(Main, Launcher_GetGameName(), "\nOnly Rock Band 2 can be launched. Insert the correct disc and try again.");
			popup_text[0]->SetText(Launcher_GetGameNameWide());
			popup_text[1]->SetText("\nReading Disc.....");
			ResumeGui();
	}

	status = Launcher_RVL();
	HaltGui();
	switch (status) {
		case LauncherStatus::ReadError:
			return new PlayError(Main, "Read Error", "\nCouldn't read the disc info for Riivolution.");
		case LauncherStatus::OutOfMemory:
			return new PlayError(Main, "Memory Error", "\nOut of Memory. Not sure how this happened, but it's bad.");
		default: // LauncherStatus::OK
			popup_text[1]->SetText("\nApplying Patches.....");
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

			Disc.Sections[0].Options[1].Default = 1;
			
			File_CreateFile("/mnt/isfs/tmp/ntsc.tpl");
			out_fd = File_Open("/mnt/isfs/tmp/ntsc.tpl", O_WRONLY);
			if (out_fd >=0) {
				File_Write(out_fd, splash_tpls[index].data, splash_tpls[index].size);
				File_Close(out_fd);
			}
			File_CreateFile("/mnt/isfs/tmp/pal.tpl");
			out_fd = File_Open("/mnt/isfs/tmp/pal.tpl", O_WRONLY);
			if (out_fd >=0) {
				File_Write(out_fd, splash_tpls[index+1].data, splash_tpls[index+1].size);
				File_Close(out_fd);
			}
		} else
			default_mount = -1;
	}
	if (sd_mounted>=0 && sd_mounted!=default_mount)
		File_Unmount(sd_mounted);
	if (usb_mounted>=0 && usb_mounted!=default_mount)
		File_Unmount(usb_mounted);
	if (wifi_mounted>=0 && wifi_mounted!=default_mount)
		File_Unmount(wifi_mounted);

	RVL_Patch(&Disc);

	HaltGui();
	popup_text[1]->SetText("\nRunning Apploader.....");
	ResumeGui();
	status = Launcher_RunApploader();
	HaltGui();
	if (status != LauncherStatus::OK)
		return new PlayError(Main, "Read Error", "\nApploader Error, couldn't read the disc");
	popup_text[1]->SetText("\nAdding playtime entry and setting video mode.....");
	ResumeGui();
	Launcher_CommitRVL(false);
	Launcher_AddPlaytimeEntry();
	Launcher_SetVideoMode();

	HaltGui();
	popup_text[1]->SetText("\nBooting Disc.....");
	ResumeGui();

	RVL_PatchMemory(&Disc);
	RVL_Unmount();
	if (File_GetLogFS() < 0)
		File_Deinit();

	Launcher_Launch();

	HaltGui();
	return new PlayError(Main, "HALP", "\nCatastrophic error while launching the game.");
}
