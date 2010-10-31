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
extern const char *popup_error[];

#define BUFFER_SIZE 0x8000
extern u8 save_copy_buffer[BUFFER_SIZE] ATTRIBUTE_ALIGN(32);

#define STACK_ALIGN(type, name, cnt, alignment)		u8 _al__##name[((sizeof(type)*(cnt)) + (alignment) + (((sizeof(type)*(cnt))%(alignment)) > 0 ? ((alignment) - ((sizeof(type)*(cnt))%(alignment))) : 0))]; \
													type *name = (type*)(((u32)(_al__##name)) + ((alignment) - (((u32)(_al__##name))&((alignment)-1))))

typedef struct {
	u32 version;
	time_t timestamp;
	u8 leaderboards;
	// version 2
	u8 slot_led;
} config_t;

extern config_t global_config;

class GuiImageGrower : public GuiImage
{
public:
	GuiImageGrower(GuiImageData *imgData) : GuiImage(imgData) {}
	void Draw();
};

class MenuImage : public GuiImageGrower
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
friend class MenuSaves;
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
	enum {OPTION_DEVICES, OPTION_LEADERBOARDS, OPTION_ACTIVITY, OPTION_SETTINGS_COUNT};
	static const u8 *settings_images[OPTION_SETTINGS_COUNT*3+1];
	GuiText Subtitle;
	int sel;
public:
	MenuSettings(GuiWindow *Parent);
	virtual ~MenuSettings();
	RawkMenu* Process();
};

class MenuSaves : public RawkMenu
{
private:
	GuiWindow *Main;
	enum {OPTION_BACKUP, OPTION_RESTORE, OPTION_SAVES_COUNT};
	static const u8 *saves_images[OPTION_SAVES_COUNT*3+1];
public:
	MenuSaves(GuiWindow *Parent);
	// this can be used as a generic error or pause dialog window
	MenuSaves(GuiWindow *Parent, const char *title, const char *text, int cont=1);
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

class MenuDump : public RawkMenu
{
private:
	enum {RIP_BEGIN, RIP_NEED_DEVICE, RIP_RIPPING, RIP_DONE, RIP_ABORT};
	enum {ABORT_MEM, ABORT_DISC_UNKNOWN, ABORT_DISC_RB2, ABORT_DISC_TBRB, ABORT_READ, ABORT_WRITE, ABORT_NO_DEVICE, ABORT_NO_SPACE};
	GuiWindow *Main;
	int state;
	int msg_index;
	s64 space;
	int old_net_initted;
public:
	MenuDump(GuiWindow *_Main);
	RawkMenu *Process();
};
