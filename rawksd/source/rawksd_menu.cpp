#include <gccore.h>
#include <ogcsys.h>
#include <network.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <time.h>
#include <sys/param.h>

#include "rawksd_menu.h"

#include "init.h"
#include "files.h"
#include "launcher.h"
#include "riivolution.h"

#define SD_X 19
#define SD_Y 381
#define USB_X 24
#define USB_Y 401
#define WIFI_X 37
#define WIFI_Y 401

#define SANTA_X 84
#define SANTA_Y 144

GuiTrigger Trigger[Triggers::Count];
GuiWindow* Window;

s32 sd_mounted = -1;
s32 usb_mounted = -1;
s32 wifi_mounted = -1;
s32 default_mount = -1;

s8 net_initted = 0;
config_t global_config ATTRIBUTE_ALIGN(32);

const char *basic_popup_options[] = { "NO", "YES", NULL};
const char *popup_error[] = {"CONTINUE", NULL};

extern "C"  {
	extern const u8 bg_png[];
	extern const u8 sd_sticker_png[];
	extern const u8 sd_sticker_def_png[];
	extern const u8 usb_sticker_png[];
	extern const u8 usb_sticker_def_png[];
	extern const u8 wifi_sticker_png[];
	extern const u8 wifi_sticker_def_png[];
	extern const u8 Rawk1_png[];
	extern const u8 santa_hat_png[];
}


static lwp_t guithread = LWP_THREAD_NULL;
static volatile bool guiHalt = true;
static volatile int exitRequested = 0;
float MenuImage::shine_pos = 0;

void ExitApp()
{
	ShutoffRumble();
	StopGX();
	exit(0);
}

void ResumeGui()
{
	guiHalt = false;
	LWP_ResumeThread(guithread);
}

void HaltGui()
{
	guiHalt = true;

	while(!LWP_ThreadIsSuspended(guithread))
		usleep(THREAD_SLEEP);
}

void RequestExit()
{
	exitRequested = 1;
}

static void* UpdateGUI(void*)
{
	int i;

	while (true) {
		if(guiHalt)
			LWP_SuspendThread(guithread);
		else {
			UpdatePads();
			Window->Draw();

			Menu_Render();

			for(i = 0; i < 4; i++)
				Window->Update(&userInput[i]);

			if (exitRequested) {
				for(i = 0; i < 255; i += 15) {
					Window->Draw();
					Menu_DrawRectangle(0, 0, screenwidth, screenheight, (GXColor){0, 0, 0, (u8)i}, 1);
					Menu_Render();
				}
				ExitApp();
			}
		}
	}

	return NULL;
}

void InitGUIThreads()
{
	LWP_CreateThread(&guithread, UpdateGUI, NULL, NULL, 0, 70);
}

// Just like base class except only scaled vertically and never tiled, top position is compensated for scale
void GuiImageGrower::Draw()
{
	if(!image || !IsVisible() || tile == 0)
		return;

	float currScale = GetScale();
	int currTop = GetTop();
	currTop -= (int)((height<<15)*(1.0-currScale)+(1<<15))>>16;

	Menu_DrawImg(GetLeft(), currTop, width, height, image, imageangle, 1.0, currScale, GetAlpha());

	UpdateEffects();
}

MenuImage::MenuImage(GuiImageData *imgData) :
GuiImageGrower(imgData),
original(imgData)
{
	image = (u8*)memalign(32, width*height*4);
}

MenuImage::~MenuImage()
{
	free(image);
	image = NULL;
}

void MenuImage::Draw()
{
	int x, y;
	GXColor color;
	int value;
	int shift = 180; // intensity add
	int b = 20; // light radius
	int a = (sin(shine_pos)+1)*120+15; // range: {15,235} sweeper
	int len = width*height*4;
	// this relies on only one MenuImage being displayed at once
	shine_pos += 0.01;
	if (shine_pos >= 2*M_PI)
		shine_pos = 0;

	// copy original image over the last shown
	memcpy(image, original->GetImage(), len);

	for (y=0; y < height; y++)
	{
		int halfheight = height/2;
		int shift2 = shift << 14;
		if (y <= halfheight)
			shift2 = (shift2*(y+b-halfheight))/b;
		else
			shift2 = (shift2*(halfheight-y+b))/b;

		for (x=0; x < b*2; x++)
		{
			value = shift2;
			if (x <= b)
				value = (value*x)/b;
			else
				value = (value*(b*2-x))/b;
			value >>= 14;

			color = GetPixel(x+a-b, y);

			// increase red and green 4x more than blue
			if (color.r < 255-value*2)
				color.r += value*2;
			else
				color.r = 255;
			if (color.g < 255-value*2)
				color.g += value*2;
			else
				color.g = 255;
			if (color.b < 255-value/2)
				color.b += value/2;
			else
				color.b = 255;

			SetPixel(x+a-b, y, color);
		}
	}

	len = (len+31)&~31;
	DCFlushRange(image, len);
	GuiImageGrower::Draw();
}

MenuButton::MenuButton(GuiWindow *_Parent, int x, int y, const u8 *normal_png, const u8 *select_png, const u8 *disabled_png, Triggers::Enum trigger, int _id) :
Parent(_Parent),
lbl_normal(NULL),
lbl_selected(NULL),
id(_id)
{
	imgdata_normal = new GuiImageData(normal_png);
	img_normal = new GuiImageGrower(imgdata_normal);
	imgdata_selected = new GuiImageData(select_png);
	img_selected = new MenuImage(imgdata_selected);
	if (disabled_png) {
		imgdata_disabled = new GuiImageData(disabled_png);
		img_disabled = new GuiImageGrower(imgdata_disabled);
	} else {
		imgdata_disabled = NULL;
		img_disabled = NULL;
	}
	button = new GuiButton(0,0);
	button->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	button->SetTrigger(&Trigger[trigger]);
	button->SetPosition(x, y);

	img_normal->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	img_normal->SetPosition(0,0);
	button->SetImage(img_normal);
	img_selected->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	img_selected->SetPosition(0,0);
	button->SetImageOver(img_selected);
	if (img_disabled) {
		img_disabled->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
		img_disabled->SetPosition(0,0);
	}
	button->SetEffect(EFFECT_SCALE, 5, 100);

	Parent->Append(button);
}

MenuButton::MenuButton(GuiWindow *_Parent, int x, int y, const char *label, Triggers::Enum trigger, int _id) :
Parent(_Parent),
img_normal(NULL),
img_selected(NULL),
img_disabled(NULL),
imgdata_normal(NULL),
imgdata_selected(NULL),
imgdata_disabled(NULL),
id(_id)
{
	button = new GuiButton(0,0);
	button->SetAlignment(ALIGN_CENTRE, ALIGN_BOTTOM);
	button->SetTrigger(&Trigger[trigger]);
	button->SetPosition(x, y);

	lbl_normal = new GuiText(label, 24, (GXColor){200,200,200,255});
	lbl_selected = new GuiText(label, 24, (GXColor){255,255,0,255}); // yellow
	button->SetLabel(lbl_normal);
	button->SetLabelOver(lbl_selected);

	Parent->Append(button);
}

MenuButton::~MenuButton() {
	Parent->Remove(button);
	delete button;
	delete img_normal;
	delete img_selected;
	delete img_disabled;
	delete imgdata_normal;
	delete imgdata_selected;
	delete imgdata_disabled;
	delete lbl_normal;
	delete lbl_selected;
}

void MenuButton::Select() {
	button->SetState(STATE_SELECTED, 0);
}

void MenuButton::Disable() {
	button->SetState(STATE_DISABLED);
	if (img_disabled)
		button->SetImage(img_disabled);
}

void MenuButton::Enable() {
	if (button->GetState() == STATE_CLICKED)
		button->SetState(STATE_SELECTED, 0);
	else {
		if (img_normal)
			button->SetImage(img_normal);
		button->SetState(STATE_DEFAULT, 0);
	}
}

int MenuButton::IsClicked() {
	if (button->GetState() == STATE_CLICKED)
		return id;
	return -1;
}

RawkMenu::RawkMenu(GuiWindow *_Parent, const u8 **option_images, int x, int y) :
is_popup(0),
Parent(_Parent)
{
	int i=0;
	if (option_images) {
		while (option_images[0]) {
			MenuButton* x_button = new MenuButton(Parent, x, y+27*i, option_images[0], option_images[1], option_images[2], Triggers::Select, i);
			Buttons.push_back(x_button);
			i++;
			option_images += 3;
		}
	}
	if (i)
		Buttons[0]->Select();
}

RawkMenu::RawkMenu(const char **option_strings, const char *info_string, const char *title_string) :
is_popup(1)
{
	int i=0;
	Parent = new GuiWindow(640, 308);
	Parent->SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
	Parent->SetPosition(0, 0);
	popup_imagedata = new GuiImageData(Rawk1_png);
	popup_image = new GuiImage(popup_imagedata);
	Parent->Append(popup_image);

	if (option_strings) {
		while (option_strings[0]) {
			MenuButton *x_button = new MenuButton(Parent, 0, -45-i*26, option_strings[0], Triggers::Select, i);
			Buttons.push_back(x_button);
			i++;
			option_strings++;
		}
	}

	if (title_string)
	{
		popup_text[0] = new GuiText(title_string, 28, (GXColor){220, 220, 220, 255});
		popup_text[0]->SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
		popup_text[0]->SetPosition(0, 28);
		Parent->Append(popup_text[0]);
	}
	else popup_text[0] = NULL;

	if (info_string)
	{
		popup_text[1] = new GuiText(info_string, 18, (GXColor){220, 220, 220, 255});
		popup_text[1]->SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
		popup_text[1]->SetPosition(0, 64);
		popup_text[1]->SetWrap(true, 520);
		Parent->Append(popup_text[1]);
	}
	else popup_text[1] = NULL;

	Parent->SetEffect(EFFECT_SLIDE_BOTTOM | EFFECT_SLIDE_IN, 20);
	//Window->SetState(STATE_DISABLED);
	Window->Append(Parent);
	Window->ChangeFocus(Parent);
	if (i!=1) { // don't slide error windows in before the previous window has slid out
		ResumeGui();
		while (Parent->GetEffect() > 0) usleep(THREAD_SLEEP);
		HaltGui();
	}
}

RawkMenu::~RawkMenu()
{
	if (is_popup) {
		// put ourselves back on top for this
		Window->Append(Parent);
		ResumeGui();
		Parent->SetEffect(EFFECT_SLIDE_BOTTOM | EFFECT_SLIDE_OUT, 20);
		while (Parent->GetEffect() > 0) usleep(THREAD_SLEEP);
		HaltGui();
	}

	std::vector<MenuButton*>::iterator iter = Buttons.begin();
	for(;iter!=Buttons.end();iter++)
		delete *iter;

	if (is_popup) {
		if (popup_text[0]) {
			Parent->Remove(popup_text[0]);
			delete popup_text[0];
		}
		if (popup_text[1]) {
			Parent->Remove(popup_text[1]);
			delete popup_text[1];
		}
		Parent->Remove(popup_image);
		delete popup_image;
		delete popup_imagedata;
		Window->Remove(Parent);
		delete Parent;
	}
}

int RawkMenu::GetClicked()
{
	int ret = -1;
	std::vector<MenuButton*>::iterator iter = Buttons.begin();
	for(;iter!=Buttons.end();iter++) {
		ret = (*iter)->IsClicked();
		if (ret>=0)
			break;
	}
	return ret;
}

static u8 wifi_check_now = 0;

void UpdateDevice(s32 *mount, GuiImage *image, GuiImageData *regular_image, GuiImageData *default_image, disk_phys disk, const char *dev)
{
	if (*mount<0) {
		// ugh
		if (disk == DISK_NONE) {
			if (wifi_check_now) {
				*mount = File_RiiFS_Mount("", 5256);
				wifi_check_now = 0;
			}
			else
				return;
		} else
			*mount = File_Fat_Mount(disk, dev);
		if (*mount>=0) {
			STACK_ALIGN(config_t,new_config,2,32);
			char filepath[MAXPATHLEN];

			image->SetVisible(true);
			*new_config = global_config;
			filepath[0] = '\0';
			File_GetMountPoint(*mount, filepath, sizeof(filepath));
			strcat(filepath, "/rawk/rb2/config");
			s32 in_fd = File_Open(filepath, O_RDONLY);

			if (in_fd>=0) {
				int filesize = File_Read(in_fd, new_config, sizeof(*new_config));
				File_Close(in_fd);

				if ((filesize>=9 && new_config->version==1) || (filesize>=10 && new_config->version==2)) {
					if (new_config->version==1)
						new_config->slot_led = global_config.slot_led;

					if (new_config->version > global_config.version || \
						(new_config->version == global_config.version && new_config->timestamp > global_config.timestamp)) {
							default_mount = -1;
							global_config = *new_config;
							File_SetSlotLED(global_config.slot_led);
					}
				}
			}

			if (default_mount<0)
				default_mount = *mount;

		} else
			return;
	}

	if (disk==DISK_NONE && wifi_check_now) {
		wifi_check_now = 0;
		disk = SD_DISK; // hack to trigger CheckPhysical
	}

	if (disk!=DISK_NONE && File_CheckPhysical(*mount)<0) {
		int old_mount = *mount;
		File_Unmount(*mount);
		*mount = -1;
		image->SetVisible(false);
		if (default_mount == old_mount) {
			global_config.timestamp = 0;
			global_config.version = 0;
			if (sd_mounted>=0)
				default_mount = sd_mounted;
			else if (usb_mounted>=0)
				default_mount = usb_mounted;
			else if (wifi_mounted>=0)
				default_mount = wifi_mounted;
			else {
				default_mount = -1;
				return;
			}
		}
	}

	HaltGui();
	if (default_mount == *mount)
		image->SetImage(default_image);
	else
		image->SetImage(regular_image);
	ResumeGui();
}

static void wifi_check(syswd_t alarm, void *_check_now)
{
	u8 *check_now = (u8*)_check_now;
	*check_now = 1;
}

static s32 net_init_cb(s32 result, void *data)
{
	syswd_t *timer = (syswd_t*)data;

	if (result>=0) {
		net_initted = 1;
		if (!SYS_CreateAlarm(timer)) {
			struct timespec tm = {2, 0};
			SYS_SetPeriodicAlarm(*timer, &tm, &tm, wifi_check, &wifi_check_now);
		}
	} else
		net_initted = -1;
	return 0;
}

void MainMenu()
{
	syswd_t riifs_timer;
	u8 i;

	*(u32*)0xCD006C00 = 0; // magic audio fix

	// TODO: Confirm this actually scrubs the playlog
	Launcher_ScrubPlaytimeEntry();
	RVL_Initialize();
	Launcher_Init();

	global_config.version = 0;
	global_config.leaderboards = 1;
	global_config.timestamp = 0;
	global_config.slot_led = 1;

	// clear out old rawksd custom titles if they exist
	for(i='A'; i <= 'Z'; i++) {
		char path[64];
		u64 titleid = 0x0001000563524200llu | i;
		ES_DeleteTitle(titleid);
		sprintf(path, "/mnt/isfs/ticket/00010005/635242%02x.tik", i);
		File_Delete(path);
		// DIE!
		sprintf(path, "/mnt/isfs/title/00010005/635242%02x/content/title.tmd", i);
		File_Delete(path);
	}

	net_init_async(net_init_cb, &riifs_timer);
	// use this instead if wifi debugging is on
	//net_init_cb(1, &riifs_timer);

	RawkMenu *menu;
	GuiImageData BackgroundImage(bg_png);
	GuiImageData SDstickerImage(sd_sticker_png);
	GuiImageData USBstickerImage(usb_sticker_png);
	GuiImageData WIFIstickerImage(wifi_sticker_png);
	GuiImageData SDstickerDefaultImage(sd_sticker_def_png);
	GuiImageData USBstickerDefaultImage(usb_sticker_def_png);
	GuiImageData WIFIstickerDefaultImage(wifi_sticker_def_png);
	GuiImageData SantaImage(santa_hat_png);

	Window = new GuiWindow(screenwidth, screenheight);
	Window->SetPosition(0, 0);
	Window->SetAlignment(ALIGN_LEFT, ALIGN_TOP);

	GuiImage Background(&BackgroundImage);
	Background.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	Background.SetPosition(0, 0);
	Window->Append(&Background);

//	GuiText buildText("RawkSD3 BETA " __DATE__ " " __TIME__, 14, (GXColor){255, 255, 255, 255});
//	buildText.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
//	buildText.SetPosition(40, 40);
//	Window->Append(&buildText);

	GuiImage SDsticker(&SDstickerImage);
	SDsticker.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	SDsticker.SetPosition(SD_X, SD_Y);
	SDsticker.SetVisible(false);
	Window->Append(&SDsticker);

	GuiImage USBsticker(&USBstickerImage);
	USBsticker.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	USBsticker.SetPosition(USB_X, USB_Y);
	USBsticker.SetVisible(false);
	Window->Append(&USBsticker);

	GuiImage WIFIsticker(&WIFIstickerImage);
	WIFIsticker.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	WIFIsticker.SetPosition(WIFI_X, WIFI_Y);
	WIFIsticker.SetVisible(false);
	Window->Append(&WIFIsticker);

	GuiImage SantaHat(&SantaImage);
	time_t now = time(NULL);
	struct tm *tm_now = localtime(&now);
	if (tm_now && tm_now->tm_mon==11 && tm_now->tm_mday==25) {
		SantaHat.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
		SantaHat.SetPosition(SANTA_X, SANTA_Y);
		SantaHat.SetVisible(true);
		Window->Append(&SantaHat);
	}

	Window->SetFocus(true);

	Trigger[Triggers::Select].SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);
	Trigger[Triggers::Back].SetButtonOnlyTrigger(-1, WPAD_BUTTON_B | WPAD_CLASSIC_BUTTON_B, PAD_BUTTON_B);
	Trigger[Triggers::Home].SetButtonOnlyTrigger(-1, WPAD_BUTTON_HOME | WPAD_CLASSIC_BUTTON_HOME | WPAD_GUITAR_HERO_3_BUTTON_ORANGE, PAD_BUTTON_START);
	Trigger[Triggers::PageLeft].SetButtonOnlyTrigger(-1, WPAD_BUTTON_MINUS | WPAD_CLASSIC_BUTTON_FULL_L | WPAD_CLASSIC_BUTTON_MINUS, PAD_TRIGGER_L);
	Trigger[Triggers::PageRight].SetButtonOnlyTrigger(-1, WPAD_BUTTON_PLUS | WPAD_CLASSIC_BUTTON_FULL_R | WPAD_CLASSIC_BUTTON_PLUS, PAD_TRIGGER_R);

	/*
	Music = new GuiSound(bg_music_ogg, bg_music_ogg_size, SOUND_OGG);
	Music->SetVolume(50);
	Music->Play();
	*/

	menu = new MenuMain(Window);

	do {
		ResumeGui();
		RawkMenu *next = menu->Process();
		if (next != menu) {
			delete menu;
			menu = next;
		} else {
			UpdateDevice(&sd_mounted, &SDsticker, &SDstickerImage, &SDstickerDefaultImage, SD_DISK, "sd");
			UpdateDevice(&usb_mounted, &USBsticker, &USBstickerImage, &USBstickerDefaultImage, USB_DISK, "usb");
			if (net_initted>0)
				UpdateDevice(&wifi_mounted, &WIFIsticker, &WIFIstickerImage, &WIFIstickerDefaultImage, DISK_NONE, NULL);
			usleep(THREAD_SLEEP);
		}
		CheckShutdown();
	} while (menu);

	ShutoffRumble();
	RequestExit();
	ResumeGui();

	while (true)
		usleep(THREAD_SLEEP);
}
