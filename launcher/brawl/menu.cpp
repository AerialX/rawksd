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
	extern u8 p_png[];
	extern u8 exit_png[];
	extern u8 s_png[];
	extern u8 i_png[];
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
	
	Title = new GuiText(RIIVOLUTION_TITLE, 32, (GXColor){0, 0, 0, 255});
	Title->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	Title->SetPosition(100, 424);
	// TODO: Title->SetItalic(true);
	Window->Append(Title);

	Subtitle = new GuiText("Loading...", 24, (GXColor){215, 215, 215, 255});
	Subtitle->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	Subtitle->SetPosition(92, 50);
	Window->Append(Subtitle);

	Trigger[Triggers::Select].SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);
	Trigger[Triggers::Back].SetButtonOnlyTrigger(-1, WPAD_BUTTON_B | WPAD_CLASSIC_BUTTON_B, PAD_BUTTON_B);
	Trigger[Triggers::Home].SetButtonOnlyTrigger(-1, WPAD_BUTTON_HOME | WPAD_CLASSIC_BUTTON_HOME, PAD_BUTTON_START);
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

	if (*(vu32*)0x80001804 != 0x53545542) // "STUB" - Check for whether the HBC (or other loader) reload stub is in place or not
		SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);

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
	ImageData[LaunchImage] = new GuiImageData(p_png);
	ImageData[SettingsImage] = new GuiImageData(s_png);
	ImageData[InstallImage] = new GuiImageData(i_png);
	ImageData[ExitImage] = new GuiImageData(exit_png);
}

ButtonList::ButtonList(GuiWindow* window, int items)
{
	Window = window;
	Count = items;

	Text = new GuiText*[Count + 1];
	TextOver = new GuiText*[Count + 1];
	Buttons = new GuiButton*[Count + 1];
	Images = new GuiImage*[Count + 1];

	for (int i = 0; i < items; i++) {
		Text[i] = new GuiText("", 16, (GXColor){255, 255, 255, 255});
		TextOver[i] = new GuiText("", 16, (GXColor){210, 210, 210, 255});
		Text[i]->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
		Text[i]->SetPosition(12, 2);
		TextOver[i]->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
		TextOver[i]->SetPosition(12, 2);
		Buttons[i] = new GuiButton(72, 48);
		Buttons[i]->SetLabel(Text[i]);
		Buttons[i]->SetLabelOver(TextOver[i]);
		Buttons[i]->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
		Buttons[i]->SetTrigger(&Trigger[Triggers::Select]);
		Buttons[i]->SetRumble(true);
//    	Buttons[i]->SetEffectOnOver(EFFECT_FADE, 2, 50);
		Buttons[i]->SetPosition(472 - i*72 + i*6, 27); // First button is at the right - add 72 to 472 if you want a third button
		Window->Append(Buttons[i]);

		Images[i] = NULL;
	}
}

void ButtonList::SetButtonLaunch(GuiWindow* window, int index, const char* text)
{
	Window = window;

	Text[index] = new GuiText("", 16, (GXColor){0, 0, 0, 255});
	TextOver[index] = new GuiText("", 16, (GXColor){80, 80, 80, 255});
	Text[index]->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	Text[index]->SetPosition(16, 26);
	TextOver[index]->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	TextOver[index]->SetPosition(16, 26);
	Buttons[index] = new GuiButton(106, 106);
	Buttons[index]->SetLabel(Text[index]);
	Buttons[index]->SetLabelOver(TextOver[index]);
	Buttons[index]->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	Buttons[index]->SetTrigger(&Trigger[Triggers::Select]);
	Buttons[index]->SetRumble(true);
//	Buttons[index]->SetEffectOnOver(EFFECT_FADE, 2, 50);
	Buttons[index]->SetPosition(490, 358);
	Window->Append(Buttons[index]);

	Images[index] = NULL;

	Text[index]->SetText(text);
	TextOver[index]->SetText(text);

	Images[index] = new GuiImage(ImageData[LaunchImage]);
	Buttons[index]->SetImage(Images[index]);

	Count = MAX(Count, index + 1);
}

void ButtonList::SetButton(int index, const char* text, int imageindex)
{

	Text[index]->SetText(text);
	TextOver[index]->SetText(text);    

	Images[index] = new GuiImage(ImageData[imageindex]);
	Buttons[index]->SetImage(Images[index]);

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
