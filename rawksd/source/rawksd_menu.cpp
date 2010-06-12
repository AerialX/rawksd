#include <gccore.h>
#include <ogcsys.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <wiiuse/wpad.h>

#include "rawksd_menu.h"
#include "input.h"
#include "filelist.h"
#include "filebrowser.h"
#include "launcher.h"
#include "motd.h"

GuiTrigger Trigger[Triggers::Down + 1];

GuiImage* Background;
GuiImageData* BackgroundImage;
GuiSound* Music;
GuiWindow* Window;
GuiText* Title;
GuiText* Subtitle;

extern "C" u8 bg_png[];

static lwp_t guithread = LWP_THREAD_NULL;
static volatile bool guiHalt = true;
static volatile int exitRequested = 0;

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
static void* UpdateGUI(void* arg);
void InitGUIThreads()
{
	LWP_CreateThread(&guithread, UpdateGUI, NULL, NULL, 0, 70);
}
void RequestExit()
{
	exitRequested = 1;
}

static void* UpdateGUI(void* arg)
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
					Menu_DrawRectangle(0, 0, screenwidth, screenheight, (GXColor){0, 0, 0, i}, 1);
					Menu_Render();
				}
				ExitApp();
			}
		}
	}

	return NULL;
}

void MainMenu(Menus::Enum menu)
{
	BackgroundImage = new GuiImageData(bg_png);

	Window = new GuiWindow(screenwidth, screenheight);
	Window->SetPosition(0, 0);
	Window->SetAlignment(ALIGN_LEFT, ALIGN_TOP);

	Background = new GuiImage(BackgroundImage);
	Background->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	Background->SetPosition(0, 0);
	Window->SetFocus(true);
	Window->Append(Background);

	Subtitle = new GuiText(GetMotd(), 18, (GXColor){255, 255, 255, 255});
	Subtitle->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	Subtitle->SetPosition(264, 338);
	Window->Append(Subtitle);

	Trigger[Triggers::Select].SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);
	Trigger[Triggers::Back].SetButtonOnlyTrigger(-1, WPAD_BUTTON_B | WPAD_CLASSIC_BUTTON_B, PAD_BUTTON_B);
	Trigger[Triggers::Home].SetButtonOnlyTrigger(-1, WPAD_BUTTON_HOME | WPAD_CLASSIC_BUTTON_HOME | WPAD_GUITAR_HERO_3_BUTTON_ORANGE, PAD_BUTTON_START);
	Trigger[Triggers::PageLeft].SetButtonOnlyTrigger(-1, WPAD_BUTTON_MINUS | WPAD_CLASSIC_BUTTON_FULL_L | WPAD_CLASSIC_BUTTON_MINUS, PAD_TRIGGER_L);
	Trigger[Triggers::PageRight].SetButtonOnlyTrigger(-1, WPAD_BUTTON_PLUS | WPAD_CLASSIC_BUTTON_FULL_R | WPAD_CLASSIC_BUTTON_PLUS, PAD_TRIGGER_R);

	ResumeGui();
	/*
	Music = new GuiSound(bg_music_ogg, bg_music_ogg_size, SOUND_OGG);
	Music->SetVolume(50);
	Music->Play();
	*/
	while (menu != Menus::Exit) {
		switch (menu) {
			case Menus::Mount:
				menu = MenuMount();
				break;
			case Menus::Init:
				menu = MenuInit();
				break;
			case Menus::Main:
				menu = MenuMain();
				break;
			case Menus::Settings:
				menu = MenuSettings();
				break;
			case Menus::Connect:
				menu = MenuConnect();
				break;
			case Menus::Launch:
				menu = MenuLaunch();
				break;
			case Menus::Install:
				menu = MenuInstall();
				break;
			case Menus::Uninstall:
				menu = MenuUninstall();
				break;
			default:
				break;
		}
	}

	ShutoffRumble();

	if (*(vu32*)0x80001804 != 0x53545542) // "STUB" - Check for whether the HBC (or other loader) reload stub is in place or not
		SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);

	ResumeGui();
	RequestExit();
	while (true)
		usleep(THREAD_SLEEP);

	HaltGui();

//	Music->Stop();
//	delete Music;
	delete Background;
	delete Window;
}

ButtonList::ButtonList(GuiWindow* window, int items)
{
	Window = window;
	Count = items;

	Images = new GuiImage*[Count];
	ImagesOver = new GuiImage*[Count];
	Buttons = new GuiButton*[Count];

	for (int i = 0; i < Count; i++) {
		Buttons[i] = new GuiButton(295, 21);
		Buttons[i]->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
		Buttons[i]->SetTrigger(&Trigger[Triggers::Select]);
		Buttons[i]->SetRumble(true);
		Buttons[i]->SetPosition(238, 188 + (21 + 6) * i);
		Window->Append(Buttons[i]);

		Images[i] = NULL;
		ImagesOver[i] = NULL;
	}
}

void ButtonList::SetButton(int index, GuiImageData* image, GuiImageData* selected)
{
	delete Images[index];
	delete ImagesOver[index];

	if (image) {
		Images[index] = new GuiImage(image);
		Images[index]->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
		Images[index]->SetPosition(0, 0);
		Buttons[index]->SetImage(Images[index]);
	} else
		Images[index] = NULL;
	if (selected) {
		ImagesOver[index] = new GuiImage(selected);
		ImagesOver[index]->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
		ImagesOver[index]->SetPosition(0, 0);
		Buttons[index]->SetImageOver(ImagesOver[index]);
	} else
		ImagesOver[index] = NULL;
}

int ButtonList::Pressed()
{
	for (int i = 0; i < Count; i++) {
		if (Buttons[i]->GetState() == STATE_CLICKED) {
			Buttons[i]->ResetState();
			return i;
		}
	}

	return -1;
}

GuiButton* ButtonList::GetButton(int index)
{
	return Buttons[index];
}

ButtonList::~ButtonList()
{
	for (int i = 0; i < Count; i++) {
		Window->Remove(Buttons[i]);
		delete Buttons[i];
		delete Images[i];
		delete ImagesOver[i];
	}
	delete[] Buttons;
	delete[] Images;
	delete[] ImagesOver;
}

Menus::Enum MenuInstall() { return Menus::Main; }
Menus::Enum MenuUninstall() { return Menus::Main; }
Menus::Enum MenuSettings() { return Menus::Main; }
Menus::Enum MenuConnect() { return Menus::Main; }
Menus::Enum MenuMount() { return Menus::Main; }
Menus::Enum MenuInit() { return Menus::Main; }

