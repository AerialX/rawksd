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
#include <ogc/es.h>

#include "libwiigui/gui.h"
#include "menu.h"
#include "demo.h"
#include "input.h"
#include "filelist.h"
#include "filebrowser.h"

#include "launcher.h"

#include "patchmii_core.h"

#define THREAD_SLEEP 100

#include <string>
using std::string;

#define printf DebugPrintf

using std::vector;

static GuiImageData * pointer[4];
static GuiImage * bgImg = NULL;
static GuiSound * bgMusic = NULL;
static GuiWindow * mainWindow = NULL;
static lwp_t guithread = LWP_THREAD_NULL;
static volatile bool guiHalt = true;

s32 Uninstall_RemoveTicket(u64 tid);
s32 Uninstall_DeleteTitle(u32 title_u, u32 title_l);
s32 Uninstall_DeleteTicket(u32 title_u, u32 title_l);

s32 Uninstall_FromTitle(const u64 tid)
{
	s32 contents_ret, tik_ret, title_ret, ret;
	u32 id = (u32)tid;
	u32 kind = (u32)(tid >> 32);
	contents_ret = tik_ret = title_ret = ret = 0;
	tik_ret		= Uninstall_DeleteTicket(kind, id);
	title_ret	= Uninstall_DeleteTitle(kind, id);
	contents_ret = title_ret;
	if (tik_ret < 0 && contents_ret < 0 && title_ret < 0)
		ret = -1;
	else if (tik_ret < 0 || contents_ret < 0 || title_ret < 0)
		ret =  1;
	else
		ret =  0;
	
	return ret;
}

/****************************************************************************
 * ResumeGui
 *
 * Signals the GUI thread to start, and resumes the thread. This is called
 * after finishing the removal/insertion of new elements, and after initial
 * GUI setup.
 ***************************************************************************/
static void
ResumeGui()
{
	guiHalt = false;
	LWP_ResumeThread (guithread);
}

/****************************************************************************
 * HaltGui
 *
 * Signals the GUI thread to stop, and waits for GUI thread to stop
 * This is necessary whenever removing/inserting new elements into the GUI.
 * This eliminates the possibility that the GUI is in the middle of accessing
 * an element that is being changed.
 ***************************************************************************/
static void
HaltGui()
{
	guiHalt = true;

	// wait for thread to finish
	while(!LWP_ThreadIsSuspended(guithread))
		usleep(THREAD_SLEEP);
}


/****************************************************************************
 * WindowPrompt
 *
 * Displays a prompt window to user, with information, an error message, or
 * presenting a user with a choice
 ***************************************************************************/
/*
int
WindowPrompt(const char *title, const char *msg, const char *btn2a)
{
	int choice = -1;

	GuiWindow promptWindow(380,260);
	promptWindow.SetAlignment(ALIGN_CENTRE, ALIGN_MIDDLE);
	promptWindow.SetPosition(0, 20);

	GuiSound btnSoundOver(button_over_ogg, button_over_ogg_size, SOUND_OGG);
	GuiSound btnSoundClick(button_click_ogg, button_click_ogg_size, SOUND_OGG);
	
	GuiTrigger trigA;
	trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);

	GuiImageData dialogBox(DialogueBox_png);
	GuiImage dialogBoxImg(&dialogBox);

	GuiText titleTxt(title, 18, (GXColor){255, 230, 150, 255});
	titleTxt.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
	titleTxt.SetPosition(0,30);
	titleTxt.SetMaxWidth(340);
	titleTxt.SetWrap(true, 340);
	GuiText msgTxt(msg, 16, (GXColor){255, 230, 150, 255});
	msgTxt.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
	msgTxt.SetPosition(0,80);
	msgTxt.SetMaxWidth(340);
	msgTxt.SetWrap(true, 340);


	GuiImageData btnYes(ButtonYes_png);
	GuiImageData btnYesOver(ButtonYes_over_png);
	GuiImage btn1Img(&btnYes);
	GuiImage btn1ImgOver(&btnYesOver);
	GuiButton btn1(btnYes.GetWidth(), btnYes.GetHeight());

	if(btn2a)
	{
		btn1.SetAlignment(ALIGN_CENTRE, ALIGN_BOTTOM);
		btn1.SetPosition(-75, -55);
	}
	else
	{
		btn1.SetAlignment(ALIGN_CENTRE, ALIGN_BOTTOM);
		btn1.SetPosition(0, -55);
	}

	btn1.SetImage(&btn1Img);
	btn1.SetImageOver(&btn1ImgOver);
	btn1.SetSoundOver(&btnSoundOver);
	btn1.SetSoundClick(&btnSoundClick);
	btn1.SetTrigger(&trigA);
	btn1.SetState(STATE_SELECTED);
	btn1.SetEffectGrow();
	
	GuiImageData btnNo(ButtonNo_png);
	GuiImageData btnNoOver(ButtonNo_over_png);
	GuiImage btn2Img(&btnNo);
	GuiImage btn2ImgOver(&btnNoOver);
	GuiButton btn2(btnNo.GetWidth(), btnNo.GetHeight());
	btn2.SetAlignment(ALIGN_CENTRE, ALIGN_BOTTOM);
	btn2.SetPosition(75, -60);
	btn2.SetImage(&btn2Img);
	btn2.SetImageOver(&btn2ImgOver);
	btn2.SetSoundOver(&btnSoundOver);
	btn2.SetSoundClick(&btnSoundClick);
	btn2.SetTrigger(&trigA);
	btn2.SetEffectGrow();
	
	promptWindow.Append(&dialogBoxImg);
	promptWindow.Append(&titleTxt);
	promptWindow.Append(&msgTxt);
	promptWindow.Append(&btn1);

	if(btn2a)
		promptWindow.Append(&btn2);

	promptWindow.SetEffect(EFFECT_SLIDE_TOP | EFFECT_SLIDE_IN, 50);
	HaltGui();
	mainWindow->SetState(STATE_DISABLED);
	mainWindow->Append(&promptWindow);
	mainWindow->ChangeFocus(&promptWindow);
	ResumeGui();

	while(choice == -1)
	{
		usleep(THREAD_SLEEP);
			
		if(btn1.GetState() == STATE_CLICKED)
			choice = 1;
		else if(btn2.GetState() == STATE_CLICKED)
			choice = 0;
	}

	promptWindow.SetEffect(EFFECT_SLIDE_TOP | EFFECT_SLIDE_OUT, 50);
	while(promptWindow.GetEffect() > 0) usleep(THREAD_SLEEP);
	HaltGui();
	mainWindow->Remove(&promptWindow);
	mainWindow->SetState(STATE_DEFAULT);
	ResumeGui();
	return choice;
}*/


/****************************************************************************
* WindowPrompt
*
* Displays a prompt window to user, with information, an error message, or
* presenting a user with a choice
***************************************************************************/
int
WindowPrompt(const char *title, const char *msg, const char *btn1Label, const char *btn2Label)
{
	int choice = -1;
	
	GuiWindow promptWindow(448,288);
	promptWindow.SetAlignment(ALIGN_CENTRE, ALIGN_MIDDLE);
	promptWindow.SetPosition(0, -10);
	GuiSound btnSoundOver(button_over_pcm, button_over_pcm_size, SOUND_PCM);
	GuiImageData btnOutline(button_png);
	GuiImageData btnOutlineOver(button_over_png);
	GuiTrigger trigA;
	trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);
	
	GuiImageData dialogBox(dialogue_box_png);
	GuiImage dialogBoxImg(&dialogBox);
	
	GuiText titleTxt(title, 26, (GXColor){255, 229, 149, 255});
	titleTxt.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
	titleTxt.SetPosition(0,40);
	GuiText msgTxt(msg, 22, (GXColor){255, 229, 149, 255});
	msgTxt.SetAlignment(ALIGN_CENTRE, ALIGN_MIDDLE);
	msgTxt.SetPosition(0,-20);
	msgTxt.SetWrap(true, 400);
	
	GuiText btn1Txt(btn1Label, 22, (GXColor){255, 229, 149, 255});
	GuiText btn1Over(btn1Label, 22, (GXColor){226, 182, 46, 255});
	
	GuiImage btn1Img(&btnOutline);
	GuiImage btn1ImgOver(&btnOutlineOver);
	GuiButton btn1(btnOutline.GetWidth(), btnOutline.GetHeight());
	
	if(btn2Label)
	{
		btn1.SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
		btn1.SetPosition(20, -25);
	}
	else
	{
		btn1.SetAlignment(ALIGN_CENTRE, ALIGN_BOTTOM);
		btn1.SetPosition(0, -25);
	}
	
	btn1.SetLabel(&btn1Txt);
	btn1.SetLabelOver(&btn1Over);
	btn1.SetImage(&btn1Img);
	btn1.SetImageOver(&btn1ImgOver);
	btn1.SetSoundOver(&btnSoundOver);
	btn1.SetTrigger(&trigA);
	btn1.SetState(STATE_SELECTED);
	btn1.SetEffectGrow();
	
	GuiText btn2Txt(btn2Label, 22, (GXColor){255, 229, 149, 255});
	GuiText btn2Over(btn2Label, 22, (GXColor){226, 182, 46, 255});
	
	GuiImage btn2Img(&btnOutline);
	GuiImage btn2ImgOver(&btnOutlineOver);
	GuiButton btn2(btnOutline.GetWidth(), btnOutline.GetHeight());
	btn2.SetAlignment(ALIGN_RIGHT, ALIGN_BOTTOM);
	btn2.SetPosition(-20, -25);
	btn2.SetLabel(&btn2Txt);
	btn2.SetLabelOver(&btn2Over);
	btn2.SetImage(&btn2Img);
	btn2.SetImageOver(&btn2ImgOver);
	btn2.SetSoundOver(&btnSoundOver);
	btn2.SetTrigger(&trigA);
	btn2.SetEffectGrow();
	
	promptWindow.Append(&dialogBoxImg);
	promptWindow.Append(&titleTxt);
	promptWindow.Append(&msgTxt);
	promptWindow.Append(&btn1);
	
	if(btn2Label)
		promptWindow.Append(&btn2);
	
	promptWindow.SetEffect(EFFECT_SLIDE_TOP | EFFECT_SLIDE_IN, 50);
	HaltGui();
	mainWindow->SetState(STATE_DISABLED);
	mainWindow->Append(&promptWindow);
	mainWindow->ChangeFocus(&promptWindow);
	ResumeGui();
	
	while(choice == -1)
	{
		usleep(THREAD_SLEEP);
		
		if(btn1.GetState() == STATE_CLICKED)
			choice = 1;
		else if(btn2.GetState() == STATE_CLICKED)
			choice = 0;
	}
	
	promptWindow.SetEffect(EFFECT_SLIDE_TOP | EFFECT_SLIDE_OUT, 50);
	while(promptWindow.GetEffect() > 0) usleep(THREAD_SLEEP);
	HaltGui();
	mainWindow->Remove(&promptWindow);
	mainWindow->SetState(STATE_DEFAULT);
	ResumeGui();
	return choice;
}

/****************************************************************************
 * UpdateGUI
 *
 * Primary thread to allow GUI to respond to state changes, and draws GUI
 ***************************************************************************/

static void *
UpdateGUI (void *arg)
{
	int i;

	while(1)
	{
		if(guiHalt)
		{
			LWP_SuspendThread(guithread);
		}
		else
		{
			mainWindow->Draw();

			#ifdef HW_RVL
			for(i=3; i >= 0; i--) // so that player 1's cursor appears on top!
			{
				if(userInput[i].wpad.ir.valid)
					Menu_DrawImg(userInput[i].wpad.ir.x-48, userInput[i].wpad.ir.y-48,
						96, 96, pointer[i]->GetImage(), userInput[i].wpad.ir.angle, 1, 1, 255);
				DoRumble(i);
			}
			#endif

			Menu_Render();

			for(i=0; i < 4; i++)
				mainWindow->Update(&userInput[i]);

			if (ExitRequested) {
				for (i = 0; i < 255; i += 15) {
					mainWindow->Draw();
					Menu_DrawRectangle(0,0,screenwidth,screenheight,(GXColor){0, 0, 0, i},1);
					Menu_Render();
				}
				VIDEO_SetBlack(true); VIDEO_Flush(); VIDEO_WaitVSync(); VIDEO_WaitVSync();
				ExitApp();
				return NULL;
			}
		}
	}
	return NULL;
}

/****************************************************************************
 * InitGUIThread
 *
 * Startup GUI threads
 ***************************************************************************/
void
InitGUIThreads()
{
	LWP_CreateThread (&guithread, UpdateGUI, NULL, NULL, 0, 70);
}

 /****************************************************************************
 * MainLoader
 ****************************************************************************/

void ResizeSections(int size)
{
	vector<PatchSection> sections;
	for (int i = 0; i < Sections.size(); i++) {
		PatchSection newsection;
		int total = (Sections[i].Options() + size - 1) / size;
		int l = 0;
		for (int k = 0; k < Sections[i].Options(); k++) {
			if (k % size == 0) {
				if (k != 0)
					sections.push_back(newsection);
				newsection = PatchSection();
				newsection.Name = Sections[i].Name;
				
				if (total > 1) {
					char num[0x20];
					char num2[0x20];
					sprintf(num, "%d", l + 1); l++;
					sprintf(num2, "%d", total);
					newsection.Name = newsection.Name + "(" + num + " of " + num2 + ")";
				}
			}
			newsection.options.push_back(Sections[i].options[k]);
		}
		
		sections.push_back(newsection);
	}
	
	Sections = sections;
}

string disabledstr = "Disabled";

struct PageView
{
	PageView(PatchSection* page, GuiImageData* arrowRightd, GuiImageData* arrowLeftd, GuiTrigger* trigA, GuiSound* btnSoundOver, GuiSound* btnSoundClick) : Window(640, 480)
	{
		Page = page;
		
		for (int i = 0; i < Page->Options(); i++) {
			OptionsChoices.push_back(new string*[Page->Options(i)->Choices.size() + 1]);
			OptionsChoices[i][0] = &disabledstr;
			for (int k = 0; k < Page->Options(i)->Choices.size(); k++)
				OptionsChoices[i][k + 1] = &Page->Options(i)->Choices[k].Name;
			Options.push_back(new GuiText(Page->Options(i)->Name.c_str(), 20, (GXColor){255, 229, 149, 255}));
			Choices.push_back(new GuiText(OptionsChoices[i][Page->Options(i)->Enabled]->c_str(), 20, (GXColor){255, 229, 149, 255}));
			ChoicesOver.push_back(new GuiText(OptionsChoices[i][Page->Options(i)->Enabled]->c_str(), 20, (GXColor){226, 182, 46, 255}));
			
			int y = i * 20 + 118;
			
			Options[i]->SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
			Options[i]->SetPosition(-96, y);
			Window.Append(Options[i]);
			
			ArrowImages.push_back(new GuiImage(arrowRightd));
			ArrowImages.push_back(new GuiImage(arrowLeftd));
			
			OptionsButton.push_back(new GuiButton(140 - 32, 20));
			OptionsButton[i]->SetLabel(Choices[i]);
			OptionsButton[i]->SetLabelOver(ChoicesOver[i]);
			OptionsButton[i]->SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
			OptionsButton[i]->SetPosition(32 + (140 - 32) / 2, y);
			OptionsButton[i]->SetTrigger(trigA);
			
			OptionsRight.push_back(new GuiButton(ArrowImages[i * 2]->GetWidth(), ArrowImages[i * 2]->GetHeight()));
			OptionsRight[i]->SetImage(ArrowImages[i * 2]);
			//OptRight[p][o]->SetImageOver(SmallArrowRightOver);
			OptionsRight[i]->SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
			OptionsRight[i]->SetPosition(140, y + 10 - ArrowImages[i * 2]->GetHeight() / 2);
			OptionsRight[i]->SetSelectable(false);
			OptionsRight[i]->SetTrigger(trigA);
			OptionsRight[i]->SetSoundOver(btnSoundOver);
			OptionsRight[i]->SetSoundClick(btnSoundClick);
			OptionsRight[i]->SetEffectGrow();
			
			OptionsLeft.push_back(new GuiButton(ArrowImages[i * 2 + 1]->GetWidth(), ArrowImages[i * 2 + 1]->GetHeight()));
			OptionsLeft[i]->SetImage(ArrowImages[i * 2 + 1]);
			//OptLeft[p][o]->SetImageOver(SmallArrowLeftOver);
			OptionsLeft[i]->SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
			OptionsLeft[i]->SetPosition(24, y + 10 - ArrowImages[i * 2 + 1]->GetHeight() / 2);
			OptionsLeft[i]->SetSelectable(false);
			OptionsLeft[i]->SetTrigger(trigA);
			OptionsLeft[i]->SetSoundOver(btnSoundOver);
			OptionsLeft[i]->SetSoundClick(btnSoundClick);
			OptionsLeft[i]->SetEffectGrow();
			
			Window.Append(OptionsRight[i]);
			Window.Append(OptionsLeft[i]);
			Window.Append(OptionsButton[i]);
		}
	}
	
	GuiWindow Window;
	PatchSection* Page;
	vector<GuiImage*> ArrowImages;
	vector<GuiText*> Options;
	vector<GuiText*> Choices;
	vector<GuiText*> ChoicesOver;
	vector<GuiButton*> OptionsButton;
	vector<GuiButton*> OptionsRight;
	vector<GuiButton*> OptionsLeft;
	vector<string**> OptionsChoices;
};
int GetPreferredIOS();
static bool Uninstall()
{
	if (WindowPrompt("Uninstall", "Are you sure that you want to uninstall?", "Yes", "No") == 1) {
		if (Uninstall_FromTitle(HAXXED_NEW_TITLEID) == 0) {
			WindowPrompt("Uninstall", "The patch has been successfully uninstalled.", "OK", NULL);
			SYS_ResetSystem(SYS_RESTART, 0, 0);
			IOS_ReloadIOS(GetPreferredIOS());
			return true;
		} else {
			WindowPrompt("Uninstall", "An error occurred while uninstalling the patch.", "OK", NULL);
			return false;
		}
	}
	
	return false;
}

#define DoWaitForInput() \
	if (ExitBtn.GetState() == STATE_CLICKED) { \
		ExitBtn.ResetState(); \
		HaltGui(); \
		return; \
	} else if (DisableBtn.GetState() == STATE_CLICKED) { \
		DisableBtn.ResetState(); \
		if (Uninstall()) \
			return; \
	}

#define WaitForInput() \
	while (true) { \
		DoWaitForInput(); \
	}
	
#define SET_PAGE(pg) { \
	HaltGui(); \
	PageInfo.Remove(&page[CurrentPage]->Window); \
	CurrentPage = pg; \
	if (CurrentPage < 0) \
		CurrentPage = Sections.size() - 1; \
	else \
		CurrentPage %= Sections.size(); \
	pageTitle.SetText(Sections[CurrentPage].Name.c_str()); \
	PageInfo.Append(&page[CurrentPage]->Window); \
	ResumeGui(); \
	}

static void MenuExecute()
{
	//Setup all the Triggers
	GuiTrigger trigA;
	trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);
	GuiTrigger trigHome;
	trigHome.SetButtonOnlyTrigger(-1, WPAD_BUTTON_HOME | WPAD_CLASSIC_BUTTON_HOME, PAD_BUTTON_START);
	GuiTrigger trigPlus;
	trigPlus.SetButtonOnlyTrigger(-1, WPAD_BUTTON_PLUS | WPAD_CLASSIC_BUTTON_MINUS | WPAD_CLASSIC_BUTTON_FULL_L, PAD_TRIGGER_L);
	GuiTrigger trigMinus;
	trigMinus.SetButtonOnlyTrigger(-1, WPAD_BUTTON_MINUS | WPAD_CLASSIC_BUTTON_PLUS | WPAD_CLASSIC_BUTTON_FULL_R, PAD_TRIGGER_R);
	GuiTrigger trigLeft;
	trigLeft.SetButtonOnlyTrigger(-1, WPAD_BUTTON_LEFT | WPAD_CLASSIC_BUTTON_LEFT | PAD_BUTTON_LEFT, PAD_BUTTON_LEFT);
	GuiTrigger trigRight;
	trigRight.SetButtonOnlyTrigger(-1, WPAD_BUTTON_RIGHT | WPAD_CLASSIC_BUTTON_RIGHT | PAD_BUTTON_RIGHT, PAD_BUTTON_RIGHT);
	
	//Setup Sounds
	GuiSound btnSoundOver(button_over_pcm, button_over_pcm_size, SOUND_PCM);
	btnSoundOver.SetVolume(50);
	GuiSound btnSoundClick(button_click_pcm, button_click_pcm_size, SOUND_PCM);
	
	//Setup common UI buttons
	GuiImageData Loaderbtn(launch_png);
	GuiImageData LoaderbtnOver(launch_over_png);
	GuiImage LoaderImg(&Loaderbtn);
	GuiImage LoaderImgOver(&LoaderbtnOver);
	GuiButton LoaderBtn(Loaderbtn.GetWidth(), Loaderbtn.GetHeight());
	LoaderBtn.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	LoaderBtn.SetPosition(0, 30);
	LoaderBtn.SetImage(&LoaderImg);
	LoaderBtn.SetImageOver(&LoaderImgOver);
	LoaderBtn.SetSoundOver(&btnSoundOver);
	LoaderBtn.SetSoundClick(&btnSoundClick);
	LoaderBtn.SetTrigger(&trigA);
	LoaderBtn.SetEffectGrow();
	
	GuiText DisableTxt("Uninstall", 18, (GXColor){255, 229, 149, 255});
	GuiText DisableTxtOver("Uninstall", 18, (GXColor){226, 182, 46, 255});
	DisableTxt.SetAlignment(ALIGN_RIGHT, ALIGN_TOP);
	DisableTxtOver.SetAlignment(ALIGN_RIGHT, ALIGN_TOP);
	GuiButton DisableBtn(60, 24);
	DisableBtn.SetAlignment(ALIGN_RIGHT, ALIGN_BOTTOM);
	DisableBtn.SetPosition(-35, -40);
	DisableBtn.SetLabel(&DisableTxt);
	DisableBtn.SetLabelOver(&DisableTxtOver);
	DisableBtn.SetSoundOver(&btnSoundOver);
	DisableBtn.SetSoundClick(&btnSoundClick);
	DisableBtn.SetTrigger(&trigA);
	DisableBtn.SetEffectGrow();	
	
	GuiText ExitTxt("Exit", 18, (GXColor){255, 229, 149, 255});
	GuiText ExitTxtOver("Exit", 18, (GXColor){226, 182, 46, 255});
	ExitTxt.SetAlignment(ALIGN_RIGHT, ALIGN_TOP);
	ExitTxtOver.SetAlignment(ALIGN_RIGHT, ALIGN_TOP);
	GuiButton ExitBtn(40, 24);
	ExitBtn.SetAlignment(ALIGN_RIGHT, ALIGN_BOTTOM);
	ExitBtn.SetPosition(-35, -64);
	ExitBtn.SetLabel(&ExitTxt);
	ExitBtn.SetLabelOver(&ExitTxtOver);
	ExitBtn.SetSoundOver(&btnSoundOver);
	ExitBtn.SetSoundClick(&btnSoundClick);
	ExitBtn.SetTrigger(&trigA);
	ExitBtn.SetTrigger(&trigHome);
	ExitBtn.SetEffectGrow();
	
	//Begin the Creation of the Options Menu
	
	//Raw Image Data
	GuiImageData OptionsBGr(options_bg_png);
	GuiImageData InnerFramer(options_innerframe_png);
	GuiImageData OuterFramer(options_outerframe_png);
	GuiImageData PageActiver(page_active_png);
	GuiImageData PageInactiver(page_inactive_png);
	GuiImageData PageOverr(page_over_png);
	GuiImageData BigArrowRightr(bigarrow_right_png);
	GuiImageData BigArrowLeftr(bigarrow_left_png);
	GuiImageData SmallArrowRight(smallarrow_right_png);
	GuiImageData SmallArrowLeft(smallarrow_left_png);
	
	//Linked Images
	GuiImage OptionsBG(&OptionsBGr);
	GuiImage InnerFrame(&InnerFramer);
	GuiImage OuterFrame(&OuterFramer);
	GuiImage PageActive(&PageActiver);
	GuiImage PageInactive(&PageInactiver);
	GuiImage PageOver(&PageOverr);
	GuiImage BigArrowRight(&BigArrowRightr);
	GuiImage BigArrowLeft(&BigArrowLeftr);
	
	//Static Setup of alignment and positioning
	OptionsBG.SetAlignment(ALIGN_CENTRE, ALIGN_MIDDLE);
	OptionsBG.SetPosition(0, 0);
	InnerFrame.SetAlignment(ALIGN_CENTRE, ALIGN_MIDDLE);
	InnerFrame.SetPosition(0, 0);
	OuterFrame.SetAlignment(ALIGN_CENTRE, ALIGN_MIDDLE);
	OuterFrame.SetPosition(0, 0);
	PageActive.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	PageInactive.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	
	//Button Setup    ....   I'm misssing mouse over graphics
	GuiButton PageRight(BigArrowRight.GetWidth(), BigArrowRight.GetHeight());
	PageRight.SetImage(&BigArrowRight);
	//PageRight->SetImageOver(BigArrowRightOver);
	PageRight.SetAlignment(ALIGN_CENTRE, ALIGN_MIDDLE);
	PageRight.SetPosition(200, 0);
	PageRight.SetSelectable(false);
	PageRight.SetTrigger(&trigA);
	PageRight.SetTrigger(&trigPlus);
	PageRight.SetSoundOver(&btnSoundOver);
	PageRight.SetSoundClick(&btnSoundClick);
	
	GuiButton PageLeft(BigArrowLeft.GetWidth(), BigArrowLeft.GetHeight());
	PageLeft.SetImage(&BigArrowLeft);
	//PageLeft->SetImageOver(BigArrowLeftOver);
	PageLeft.SetAlignment(ALIGN_CENTRE, ALIGN_MIDDLE);
	PageLeft.SetPosition(-200, 0);
	PageLeft.SetSelectable(false);
	PageLeft.SetTrigger(&trigA);
	PageLeft.SetTrigger(&trigMinus);
	PageLeft.SetSoundOver(&btnSoundOver);
	PageLeft.SetSoundClick(&btnSoundClick);
	
	
	//Append the Common UI Buttons onto a Window named win. It's full of win.
	HaltGui();
	GuiWindow win(640, 480);
	win.Append(&OptionsBG);
	win.Append(&InnerFrame);
	win.Append(&OuterFrame);
	win.Append(&DisableBtn);
	win.Append(&ExitBtn);
	
	GuiText InfoText("Mounting SD...", 14, (GXColor){255, 255, 255, 255});
	InfoText.SetAlignment(ALIGN_CENTRE, ALIGN_MIDDLE);
	InfoText.SetPosition(0, 0);
	win.Append(&InfoText);
	
	mainWindow->Append(&win);
	ResumeGui();
	
	int ret = 0;
	
	ret = LoadDisc_Init();
	if (ret < 0)
		InfoText.SetText(LoadDisc_Error(ret));
	
	while (ret == ERROR_MOUNT) {
		sleep(1);
		DoWaitForInput();
		ret = LoadDisc_Init();
	}
	
	if (ret < 0)
		WaitForInput();
	
	InfoText.SetText("Reading Game Disc...");
	
	ret = LoadDisc_Begin();
	
	if (ret < 0)
		InfoText.SetText(LoadDisc_Error(ret));
	
	while (ret == ERROR_NODISC) {
		sleep(1);
		DoWaitForInput();
		ret = LoadDisc_Begin();
	}
	
	if (ret < 0)
		WaitForInput();
	
	// Stuff happens here... Options and shit
	HaltGui();
	win.Remove(&InfoText);

	win.Append(&LoaderBtn);
	
	//Create a page title we can assign the page name to
	GuiText pageTitle(NULL, 18, (GXColor){255, 229, 149, 255});
	pageTitle.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
	pageTitle.SetPosition(0, 74);	
	
	//Oh, and the Current Page Text
	/*
	GuiText curpageTxt(GameTitle, 14, (GXColor){255, 229, 149, 255});
	curpageTxt.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
	curpageTxt.SetPosition(0, 400);
	*/
	vector<PageView*> page;
	//vector<GuiButton*> pageMarker;
	
	int CurrentPage = 0;
	GuiWindow PageInfo(640, 480);
	
	ResizeSections(10);
	
	for (int p = 0; p < Sections.size(); p++) {
		if (Sections.size() > 1) {
			/*
			pageMarker.push_back(new GuiButton(12, 12));
			pageMarker[p]->SetImage(&PageInactive);
			pageMarker[p]->SetImageOver(&PageOver);
			pageMarker[p]->SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
			pageMarker[p]->SetPosition(p * 22 - (Sections.size() * 22) / 2, 390);
			pageMarker[p]->SetTrigger(&trigA);
			pageMarker[p]->SetSoundClick(&btnSoundClick);
			PageInfo.Append(pageMarker[p]);
			*/
		}
		
		page.push_back(new PageView(&Sections[p], &SmallArrowRight, &SmallArrowLeft, &trigA, &btnSoundOver, &btnSoundClick));
	}
	
	//PageInfo.Append(&curpageTxt);
	if (Sections.size() > 0) {
		pageTitle.SetText(Sections[CurrentPage].Name.c_str());
		PageInfo.Append(&pageTitle);
		PageInfo.Append(&page[CurrentPage]->Window);
	}
	if (Sections.size() > 1) {
		win.Append(&PageRight);
		win.Append(&PageLeft);
	}
	mainWindow->Append(&PageInfo);
	ResumeGui();
	
	while (true)
	{
		usleep(THREAD_SLEEP);
		/*
		for (int ap = 0; ap < pageMarker.size(); ap++) {
			if (pageMarker[ap]->GetState() == STATE_CLICKED) {
				pageMarker[ap]->ResetState();
				
				SET_PAGE(ap);
			}
		}
		*/
		if (Sections.size() > 0) {
			//Check to see if the option left/right buttons have been pressed, and change the choice if it has
			for (int o = 0; o < Sections[CurrentPage].Options(); o++) {
				if (page[CurrentPage]->OptionsRight[o]->GetState() == STATE_CLICKED || page[CurrentPage]->OptionsButton[o]->GetState() == STATE_CLICKED) {
					page[CurrentPage]->OptionsRight[o]->ResetState();
					page[CurrentPage]->OptionsButton[o]->ResetState();
					
					page[CurrentPage]->Page->Options(o)->Enabled += 1;
					page[CurrentPage]->Page->Options(o)->Enabled = page[CurrentPage]->Page->Options(o)->Enabled % (page[CurrentPage]->Page->Options(o)->Choices.size() + 1);
				}
				else if (page[CurrentPage]->OptionsLeft[o]->GetState() == STATE_CLICKED) {
					page[CurrentPage]->OptionsLeft[o]->ResetState();
					
					page[CurrentPage]->Page->Options(o)->Enabled -= 1;
					if (page[CurrentPage]->Page->Options(o)->Enabled < 0)
						page[CurrentPage]->Page->Options(o)->Enabled = page[CurrentPage]->Page->Options(o)->Choices.size();
				} else
					continue;
				
				page[CurrentPage]->Choices[o]->SetText(page[CurrentPage]->OptionsChoices[o][page[CurrentPage]->Page->Options(o)->Enabled]->c_str());
				page[CurrentPage]->ChoicesOver[o]->SetText(page[CurrentPage]->OptionsChoices[o][page[CurrentPage]->Page->Options(o)->Enabled]->c_str());
			}
		}
		
		if (PageRight.GetState() == STATE_CLICKED) {
			PageRight.ResetState();
			
			SET_PAGE(CurrentPage + 1);
		} else if (PageLeft.GetState() == STATE_CLICKED) {
			PageLeft.ResetState();
			
			SET_PAGE(CurrentPage - 1);
		}
		
		//Normal buttons
		if (LoaderBtn.GetState() == STATE_CLICKED) {
			HaltGui();
			LoadDisc();
		}
		
		DoWaitForInput();
	}
}

/****************************************************************************
 * MainMenu
 ***************************************************************************/
void MainMenu(int menu)
{
	int currentMenu = menu;

	#ifdef HW_RVL
	pointer[0] = new GuiImageData(player1_point_png);
	pointer[1] = new GuiImageData(player2_point_png);
	pointer[2] = new GuiImageData(player3_point_png);
	pointer[3] = new GuiImageData(player4_point_png);
	#endif

	mainWindow = new GuiWindow(screenwidth, screenheight);

	GuiImageData Background(background_png);
	GuiImage BackgroundImg(&Background);
	mainWindow->Append(&BackgroundImg);

	GuiTrigger trigA;
	trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);

	ResumeGui();

	bgMusic = new GuiSound(bg_music_ogg, bg_music_ogg_size, SOUND_OGG);
	bgMusic->SetVolume(50);
	bgMusic->SetLoop(true);
	bgMusic->Play(); // startup music

	MenuExecute();

	ResumeGui();
	ExitRequested = true;
	while (true)
		usleep(THREAD_SLEEP);

	HaltGui();

	bgMusic->Stop();
	delete bgMusic;
	delete bgImg;
	delete mainWindow;

	delete pointer[0];
	delete pointer[1];
	delete pointer[2];
	delete pointer[3];

	mainWindow = NULL;
}
