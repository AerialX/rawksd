/****************************************************************************
 * libwiigui Template
 * Tantric 2009
 *
 * menu.cpp
 * Menu flow routines - handles all menu logic
 ***************************************************************************/

#include <gccore.h>
#include <ogcsys.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <wiiuse/wpad.h>

#include "menu.h"
#include "input.h"
#include "filelist.h"
#include "filebrowser.h"
#include "launcher.h"

GuiTrigger Trigger[Triggers::Down + 1];

GuiImageData* Pointers[4];
GuiImage* Background;
GuiImageData* BackgroundImage;
GuiSound* Music;
GuiWindow* Window;
GuiText* Title;
GuiText* Subtitle;

extern "C" {
	extern u8 background_png[];
	extern u8 launch_over_png[];
	extern u8 exit_over_png[];
	extern u8 settings_over_png[];
	extern u8 uninstall_over_png[];
}

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

			for (i = 3; i >= 0; i--) {
				if(userInput[i].wpad->ir.valid)
					Menu_DrawImg(userInput[i].wpad->ir.x - 48, userInput[i].wpad->ir.y - 48, 96, 96, Pointers[i]->GetImage(), userInput[i].wpad->ir.angle, 1, 1, 255);
				DoRumble(i);
			}

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

void MainMenu(Menus::Enum menu)
{
	Pointers[0] = new GuiImageData(player1_point_png);
	Pointers[1] = new GuiImageData(player2_point_png);
	Pointers[2] = new GuiImageData(player3_point_png);
	Pointers[3] = new GuiImageData(player4_point_png);

	BackgroundImage = new GuiImageData(background_png);

	Window = new GuiWindow(screenwidth, screenheight);
	Window->SetPosition(0, 0);
	Window->SetAlignment(ALIGN_LEFT, ALIGN_TOP);

	Background = new GuiImage(BackgroundImage);
	Background->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	Background->SetPosition(0, 0);
	Window->SetFocus(true);
	Window->Append(Background);

	Title = new GuiText(RIIVOLUTION_TITLE, 32, (GXColor){255, 255, 255, 255});
	Title->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	Title->SetPosition(56, 32);
	// TODO: Title->SetItalic(true);
	Window->Append(Title);

	Subtitle = new GuiText("Loading...", 18, (GXColor){255, 255, 255, 255});
	Subtitle->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	Subtitle->SetPosition(74, 78);
	Window->Append(Subtitle);

	Trigger[Triggers::Select].SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);
	Trigger[Triggers::Back].SetButtonOnlyTrigger(-1, WPAD_BUTTON_B | WPAD_CLASSIC_BUTTON_B, PAD_BUTTON_B);
	Trigger[Triggers::Home].SetButtonOnlyTrigger(-1, WPAD_BUTTON_HOME | WPAD_CLASSIC_BUTTON_HOME | WPAD_GUITAR_HERO_3_BUTTON_ORANGE, PAD_BUTTON_START);
	Trigger[Triggers::PageLeft].SetButtonOnlyTrigger(-1, WPAD_BUTTON_MINUS | WPAD_CLASSIC_BUTTON_FULL_L | WPAD_CLASSIC_BUTTON_MINUS, PAD_TRIGGER_L);
	Trigger[Triggers::PageRight].SetButtonOnlyTrigger(-1, WPAD_BUTTON_PLUS | WPAD_CLASSIC_BUTTON_FULL_R | WPAD_CLASSIC_BUTTON_PLUS, PAD_TRIGGER_R);

	ButtonList::InitImageData();

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

	ResumeGui();
	RequestExit();
	while (true)
		usleep(THREAD_SLEEP);

	HaltGui();

	Music->Stop();
	delete Music;
	delete Background;
	delete Window;

	delete Pointers[0];
	delete Pointers[1];
	delete Pointers[2];
	delete Pointers[3];
}

GuiImageData* ButtonList::ImageData[4];

void ButtonList::InitImageData()
{
	ImageData[LaunchImage] = new GuiImageData(launch_over_png);
	ImageData[SettingsImage] = new GuiImageData(settings_over_png);
	ImageData[UninstallImage] = new GuiImageData(uninstall_over_png);
	ImageData[ExitImage] = new GuiImageData(exit_over_png);
}

ButtonList::ButtonList(GuiWindow* window, int items)
{
	Window = window;
	Count = items;

	Text = new GuiText*[Count];
	TextOver = new GuiText*[Count];
	Buttons = new GuiButton*[Count];
	Images = new GuiImage*[Count];

	for (int i = 0; i < Count; i++) {
		Text[i] = new GuiText("", 24, (GXColor){0, 0, 0, 255});
		TextOver[i] = new GuiText("", 24, (GXColor){128, 0, 0, 255});
		Text[i]->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
		Text[i]->SetPosition(0, 0);
		TextOver[i]->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
		TextOver[i]->SetPosition(0, 0);
		Buttons[i] = new GuiButton(128, 24);
		Buttons[i]->SetLabel(Text[i]);
		Buttons[i]->SetLabelOver(TextOver[i]);
		Buttons[i]->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
		Buttons[i]->SetTrigger(&Trigger[Triggers::Select]);
		Buttons[i]->SetRumble(true);
		Buttons[i]->SetEffectGrow();
		Buttons[i]->SetPosition(536, 192 + (18 + 32) * (3 - i)); // First button is at the bottom
		Window->Append(Buttons[i]);

		Images[i] = NULL;
	}
}

void ButtonList::SetButton(int index, const char* text, int imageindex)
{
	if (Images[index])
		delete Images[index];

	Text[index]->SetText(text);
	TextOver[index]->SetText(text);

	Images[index] = new GuiImage(ImageData[imageindex]);
	Images[index]->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	Images[index]->SetPosition(536, 192 + (18 + 32) * (3 - index));
	Window->Append(Images[index]);
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
		delete Text[i];
		delete TextOver[i];
		if (Images[i]) {
			Window->Remove(Images[i]);
			delete Images[i];
		}
	}
	delete[] Buttons;
	delete[] Images;
}
