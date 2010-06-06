#pragma once

#include <ogcsys.h>
#include "libwiigui/gui.h"

#define THREAD_SLEEP 100

namespace Menus { enum Enum
{
	Exit = 0,
	Mount,
	Init,
	Main,
	Rip,
	Scores,
	Save,
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
	GuiWindow* Window;
	int Count;
	GuiButton** Buttons;
	GuiImage** Images;
	GuiImage** ImagesOver;

public:
	ButtonList(GuiWindow* window, int items);
	~ButtonList();

	void SetButton(int index, GuiImageData* image, GuiImageData* imageSelected);
	GuiButton* GetButton(int index);

	int Pressed();
};
