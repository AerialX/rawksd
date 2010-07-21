#include "rawksd_menu.h"

#include "files.h"
#include "haxx.h"

extern "C" {
	extern const u8 menu_sd_png[];
	extern const u8 menu_sd_sel_png[];
	extern const u8 menu_sd_dis_png[];
	extern const u8 menu_usb_png[];
	extern const u8 menu_usb_sel_png[];
	extern const u8 menu_usb_dis_png[];
	extern const u8 menu_wifi_png[];
	extern const u8 menu_wifi_sel_png[];
	extern const u8 menu_wifi_dis_png[];
	extern const u8 menu_scores_png[];
	extern const u8 menu_scores_sel_png[];
	extern const u8 menu_devices_png[];
	extern const u8 menu_devices_sel_png[];
	extern const u8 menu_devices_dis_png[];
}

class LeaderboardPopup : public RawkMenu
{
private:
	GuiWindow *Main;
public:
	LeaderboardPopup(GuiWindow *_Main);
	RawkMenu *Process();
};

LeaderboardPopup::LeaderboardPopup(GuiWindow *_Main) :
RawkMenu(basic_popup_options, "\nDo you want to have your high scores sent to the RawkSD Leaderboards?" \
							  " Your wii's unique id will be used for tracking purposes.", "Link Code: "),
Main(_Main)
{
	char link[50];
	sprintf(link, "Link Code: %d", otp.ng_id);
	popup_text[0]->SetText(link);
}

RawkMenu* LeaderboardPopup::Process()
{
	int clicked = GetClicked();
	if (clicked>=0) {
		HaltGui();
		if (clicked==0) // NO
			global_config.leaderboards = 0;
		else // YES
			global_config.leaderboards = 1;
		return new MenuSettings(Main);
	}
	return this;
}

class MenuDevices : public RawkMenu
{
private:
	enum {OPTION_SD, OPTION_USB, OPTION_WIFI, OPTION_DEVICES_COUNT};
	static const u8 *devices_images[OPTION_DEVICES_COUNT*3+1];
public:
	MenuDevices(GuiWindow *Parent);
	RawkMenu *Process();
};

const u8 *MenuDevices::devices_images[OPTION_DEVICES_COUNT*3+1] = {
	menu_sd_png,
	menu_sd_sel_png,
	menu_sd_dis_png,
	menu_usb_png,
	menu_usb_sel_png,
	menu_usb_dis_png,
	menu_wifi_png,
	menu_wifi_sel_png,
	menu_wifi_dis_png,
	NULL
};

MenuDevices::MenuDevices(GuiWindow *Parent) : RawkMenu(Parent, devices_images, 238, 188)
{
	MenuButton *back = new MenuButton(Parent, 0, 0, NULL, Triggers::Back, 3);
	back->button->SetSelectable(false);
	Buttons.push_back(back);

	if (sd_mounted<0) {
		Buttons[0]->Disable();
		Buttons[1]->Select();
	}
	if (usb_mounted<0) {
		if (Buttons[1]->button->GetState() == STATE_SELECTED)
			Buttons[2]->Select();
		Buttons[1]->Disable();
	}
	if (wifi_mounted<0)
		Buttons[2]->Disable();
}

RawkMenu *MenuDevices::Process()
{
	// make sure any state changes are handled
	if (sd_mounted>=0 && Buttons[0]->button->GetState() == STATE_DISABLED)
		Buttons[0]->Enable();
	if (usb_mounted>=0 && Buttons[1]->button->GetState() == STATE_DISABLED)
		Buttons[1]->Enable();
	if (wifi_mounted>=0 && Buttons[2]->button->GetState() == STATE_DISABLED)
		Buttons[2]->Enable();
	if (wifi_mounted<0) {
		if (Buttons[2]->button->GetState() == STATE_SELECTED)
			Buttons[1]->Select();
		Buttons[2]->Disable();
	}
	if (usb_mounted<0) {
		if (Buttons[1]->button->GetState() == STATE_SELECTED)
			Buttons[0]->Select();
		Buttons[1]->Disable();
	}
	if (sd_mounted<0) {
		if (Buttons[0]->button->GetState() == STATE_SELECTED) {
			if (Buttons[1]->button->GetState() == STATE_DISABLED)
				Buttons[2]->Select();
			else
				Buttons[1]->Select();
		}
		Buttons[0]->Disable();
	}

	int clicked = GetClicked();
	if (clicked>=0)
		global_config.timestamp = 0xFFFFFFFF; // don't override this device if a new device is inserted
	if ((default_mount<0) || clicked>=0) {
		HaltGui();
		switch(clicked) {
			case OPTION_SD:
				default_mount = sd_mounted;
				break;
			case OPTION_USB:
				default_mount = usb_mounted;
				break;
			case OPTION_WIFI:
				default_mount = wifi_mounted;
				break;
		}
		return new MenuSettings(Parent);
	}

	return this;
}

const u8 *MenuSettings::settings_images[OPTION_SETTINGS_COUNT*3+1] = {
	menu_devices_png,
	menu_devices_sel_png,
	menu_devices_dis_png,
	menu_scores_png,
	menu_scores_sel_png,
	NULL,
	NULL
};

static const char device_subtitle[] = "Choose the default storage device";
static const char leaderboard_subtitle[] = "Opt-in/out of the RawkSD Leaderboards or see your Link Code";

MenuSettings::MenuSettings(GuiWindow *Parent) : RawkMenu(Parent, settings_images, 238, 188),
Subtitle(device_subtitle, 18, (GXColor){255, 255, 255, 255})
{
	MenuButton *back = new MenuButton(Parent, 0, 0, NULL, Triggers::Back, 2);
	back->button->SetSelectable(false);
	Buttons.push_back(back);
	Subtitle.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	Subtitle.SetPosition(264, 300);
	Subtitle.SetWrap(true, 300);
	if (default_mount<0) {
		Subtitle.SetText(leaderboard_subtitle);
		sel = 1;
		Buttons[0]->Disable();
		Buttons[1]->Select();
	} else
		sel = 0;
	Parent->Append(&Subtitle);
}

MenuSettings::~MenuSettings()
{
	Parent->Remove(&Subtitle);
}

RawkMenu* MenuSettings::Process()
{
	if (!sel && Buttons[1]->button->GetState() == STATE_SELECTED) {
		sel = 1;
		HaltGui();
		Subtitle.SetText(leaderboard_subtitle);
		ResumeGui();
	} else if (sel && Buttons[0]->button->GetState() == STATE_SELECTED) {
		sel = 0;
		HaltGui();
		Subtitle.SetText(device_subtitle);
		ResumeGui();
	}

	int clicked = GetClicked();
	if (clicked>=0) {
		HaltGui();
		if (clicked==OPTION_LEADERBOARDS)
			return new LeaderboardPopup(Parent);
		if (clicked==OPTION_DEVICES)
			return new MenuDevices(Parent);
		return new MenuMain(Parent);
	}

	if (default_mount<0) {
		if (Buttons[0]->button->GetState() != STATE_DISABLED) {
			Buttons[0]->Disable();
			Buttons[1]->Select();
			sel = 0; // make sure the right subtitle is shown
		}
	} else if (Buttons[0]->button->GetState() == STATE_DISABLED)
		Buttons[0]->Enable();

	return this;
}
