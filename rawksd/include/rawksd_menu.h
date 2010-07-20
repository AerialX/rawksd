#pragma once

#include <vector>
#include <time.h>

#include <ogcsys.h>
#include "libwiigui/gui.h"

#define THREAD_SLEEP 100

namespace Triggers { enum Enum
{
	Select = 0,
	Back,
	Home,
	PageLeft,
	PageRight,
	Left,
	Right,
	Up,
	Down,
	Count
}; }

void ResumeGui();
void HaltGui();

extern GuiTrigger Trigger[];

extern s32 sd_mounted;
extern s32 usb_mounted;
extern s32 wifi_mounted;
extern s32 default_mount;

extern s8 net_initted;

extern const char *basic_popup_options[];

typedef struct {
	u32 version;
	time_t timestamp;
	u8 leaderboards;
} config_t;

class MenuImage : public GuiImage
{
private:
	static float shine_pos;
	GuiImageData *original;
public:
	MenuImage(GuiImageData *imgData);
	~MenuImage();
	void Draw();
};

class MenuButton
{
friend class MenuMain;
friend class MenuSettings;
friend class MenuDevices;
private:
	GuiButton *button;
	GuiWindow *Parent;
	GuiImage *img_normal;
	MenuImage *img_selected;
	GuiImage *img_disabled;
	GuiImageData *imgdata_normal;
	GuiImageData *imgdata_selected;
	GuiImageData *imgdata_disabled;
	GuiText *lbl_normal;
	GuiText *lbl_selected;
	int id;
public:
	MenuButton(GuiWindow *_Parent, int x, int y, const u8 *normal_png, const u8 *select_png, const u8 *disabled_png, Triggers::Enum trigger, int _id);
	MenuButton(GuiWindow *_Parent, int x, int y, const char *label, Triggers::Enum trigger, int _id);
	~MenuButton();
	void Select();
	void Disable();
	void Enable();
	int IsClicked();
};

class RawkMenu
{
protected:
	int is_popup;
	std::vector<MenuButton*> Buttons;
	GuiWindow *Parent;
	GuiImage *popup_image;
	GuiImageData *popup_imagedata;
	GuiText *popup_text[2];
public:
	RawkMenu(GuiWindow *_Parent, const u8 **option_images, int x, int y);
	RawkMenu(const char **option_strings, const char *info_string, const char *title_string);
	virtual ~RawkMenu();
	virtual RawkMenu *Process() = 0;
	int GetClicked();
};

class MenuMain : public RawkMenu
{
private:
	enum {OPTION_PLAY, OPTION_RIP, OPTION_SETTINGS, OPTION_SAVES, OPTION_EXIT, OPTION_MAIN_COUNT};
	static const u8 *main_images[OPTION_MAIN_COUNT*3+1];
	GuiText *Subtitle;
public:
	MenuMain(GuiWindow *Parent);
	virtual ~MenuMain();
	RawkMenu* Process();
};

class MenuSettings : public RawkMenu
{
private:
	enum {OPTION_DEVICES, OPTION_LEADERBOARDS, OPTION_SETTINGS_COUNT};
	static const u8 *settings_images[OPTION_SETTINGS_COUNT*3+1];
	GuiText Subtitle;
	int sel;
public:
	MenuSettings(GuiWindow *Parent);
	virtual ~MenuSettings();
	RawkMenu* Process();
};

class MenuPlay : public RawkMenu
{
private:
	GuiWindow *Main;
public:
	MenuPlay(GuiWindow *_Main);
	RawkMenu *Process();
};
