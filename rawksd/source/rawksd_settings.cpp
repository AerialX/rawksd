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
	extern const u8 menu_activity_png[];
	extern const u8 menu_activity_sel_png[];
}

class LeaderboardPopup : public RawkMenu
{
private:
	GuiWindow *Main;
public:
	LeaderboardPopup(GuiWindow *_Main);
	RawkMenu *Process();
};

class ActivityPopup : public RawkMenu
{
private:
	GuiWindow *Main;
public:
	ActivityPopup(GuiWindow *_Main);
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
	if (!global_config.leaderboards) {
		Buttons[0]->Select();
		Buttons[1]->Enable();
	}
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

ActivityPopup::ActivityPopup(GuiWindow *_Main) :
RawkMenu(basic_popup_options, "\nDo you want the disc slot LED to indicate loading activity?", NULL),
Main(_Main)
{
	if (!global_config.slot_led) {
		Buttons[0]->Select();
		Buttons[1]->Enable();
	}
}

RawkMenu* ActivityPopup::Process()
{
	int clicked = GetClicked();
	if (clicked>=0) {
		HaltGui();
		if (clicked==0)
			global_config.slot_led = 0;
		else
			global_config.slot_led = 1;
		File_SetSlotLED(global_config.slot_led);
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
	menu_activity_png,
	menu_activity_sel_png,
	NULL,
	NULL
};

static const char* option_subtitles[] = {
	"Choose the default storage device",
	"Opt-in/opt-out of the\nRawkSD Leaderboards or\nview your Link Code",
	"Enable/disable the slot\nled activity indicator"
};

MenuSettings::MenuSettings(GuiWindow *Parent) : RawkMenu(Parent, settings_images, 238, 188),
Subtitle(option_subtitles[0], 18, (GXColor){255, 255, 255, 255})
{
	if (is_wiiu)
	{
		Buttons[OPTION_ACTIVITY]->button->SetSelectable(false);
		Buttons[OPTION_ACTIVITY]->button->SetVisible(false);
	}

	MenuButton *back = new MenuButton(Parent, 0, 0, NULL, Triggers::Back, OPTION_SETTINGS_COUNT);
	back->button->SetSelectable(false);
	Buttons.push_back(back);
	Subtitle.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	Subtitle.SetPosition(264, 300);
	Subtitle.SetWrap(true, 300);
	if (default_mount<0) {
		Subtitle.SetText(option_subtitles[OPTION_LEADERBOARDS]);
		sel = OPTION_LEADERBOARDS;
		Buttons[OPTION_DEVICES]->Disable();
		Buttons[OPTION_LEADERBOARDS]->Select();
	} else
		sel = OPTION_DEVICES;
	Parent->Append(&Subtitle);
}

MenuSettings::~MenuSettings()
{
	Parent->Remove(&Subtitle);
}

RawkMenu* MenuSettings::Process()
{
	int i;

	for (i=0; i < OPTION_SETTINGS_COUNT; i++) {
		if (sel!=i && Buttons[i]->button->GetState() == STATE_SELECTED) {
			sel = i;
			HaltGui();
			Subtitle.SetText(option_subtitles[i]);
			ResumeGui();
			break;
		}
	}

	int clicked = GetClicked();
	if (clicked>=0) {
		HaltGui();
		switch (clicked) {
			case OPTION_LEADERBOARDS:
				return new LeaderboardPopup(Parent);
			case OPTION_DEVICES:
				return new MenuDevices(Parent);
			case OPTION_ACTIVITY:
				return new ActivityPopup(Parent);
			default:
				return new MenuMain(Parent);
		}
	}

	if (default_mount<0) {
		if (Buttons[OPTION_DEVICES]->button->GetState() != STATE_DISABLED) {
			// move the highlight down if necessary
			if (Buttons[OPTION_DEVICES]->button->GetState() == STATE_SELECTED)
				Buttons[OPTION_LEADERBOARDS]->Select();

			Buttons[OPTION_DEVICES]->Disable();
			sel = OPTION_DEVICES; // make sure the right subtitle is shown
		}
	} else if (Buttons[OPTION_DEVICES]->button->GetState() == STATE_DISABLED)
		Buttons[OPTION_DEVICES]->Enable();

	return this;
}
