#pragma once

#include <ogcsys.h>
#include "libwiigui/gui.h"

#define THREAD_SLEEP 100

#define RIIVOLUTION_TITLE "Riivolution v1.02"

namespace Menus { enum Enum
{
	Exit = 0,
	Mount,
	Init,
	Main,
	Settings,
	Connect,
	Launch,
	Install,
	Uninstall
}; }

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
	Down
}; }

void InitGUIThreads();
void MainMenu(Menus::Enum menu);

void ResumeGui();
void HaltGui();

void CheckShutdown();

Menus::Enum MenuMount();
Menus::Enum MenuInit();
Menus::Enum MenuMain();
Menus::Enum MenuLaunch();
Menus::Enum MenuSettings();
Menus::Enum MenuConnect();
Menus::Enum MenuInstall();
Menus::Enum MenuUninstall();

extern GuiTrigger Trigger[];

extern GuiImageData* Pointers[];
extern GuiImageData* BackgroundImage;
extern GuiImage* Background;
extern GuiSound* Music;
extern GuiWindow* Window;
extern GuiText* Title;
extern GuiText* Subtitle;

class ButtonList
{
protected:
	static GuiImageData* ImageData[4];

	GuiWindow* Window;
	int Count;
	GuiText** Text;
	GuiText** TextOver;
	GuiButton** Buttons;
	GuiImage** Images;

public:
	enum {
		LaunchImage = 0,
		SettingsImage,
		InstallImage,
		ExitImage
	};

	static void InitImageData();

	ButtonList(GuiWindow* window, int items);
	~ButtonList();

	void SetButton(int index, const char* text, int imageindex);
	void SetButtonLaunch(GuiWindow* window, int index, const char* text);

	GuiButton* GetButton(int index);

	int Pressed();
};
