#include "menu.h"
#include "launcher.h"
#include "haxx.h"
#include "riivolution_config.h"
#include "installer.h"

#include <unistd.h>
#include <files.h>

#include <vector>
using std::vector;


#define OPTIONS_PER_PAGE 13
#define OPTION_FONT_SIZE 18
#define OPTION_FONT_HEIGHT 22
#define OPTION_ARROW_OFFSET 0
/*
#define OPTIONS_PER_PAGE 12
#define OPTION_FONT_SIZE 20
#define OPTION_FONT_HEIGHT 28
#define OPTION_ARROW_OFFSET 0
*/

using std::string;

struct Page {
	string Name;
	vector<RiiOption*> Options;
};

RiiDisc Disc;
vector<int> Mounted;
vector<int> ToMount;

static vector<Page> Pages;

extern "C" {
	extern u8 arrow_left_png[];
	extern u8 arrow_active_left_png[];
	extern u8 arrow_right_png[];
	extern u8 arrow_active_right_png[];
}

#define UNSELECT_ALL() { \
	for (int unselect = Window->GetSelected(); unselect >= 0; unselect = Window->GetSelected()) \
		Window->GetGuiElementAt(unselect)->ResetState(); \
}

struct PageViewer {
	u32 PageNumber;
	Page* Current;

	GuiText** Title;
	GuiText** ChoiceText;
	GuiText** ChoiceOverText;
	GuiButton** Choice;
	GuiImage** LeftArrowImage;
	GuiImage** LeftArrowOverImage;
	GuiImage** RightArrowImage;
	GuiImage** RightArrowOverImage;
	GuiButton** LeftArrow;
	GuiButton** RightArrow;

	GuiImage* LeftButtonImage;
	GuiImage* LeftButtonOverImage;
	GuiImage* RightButtonImage;
	GuiImage* RightButtonOverImage;
	GuiButton* LeftButton;
	GuiButton* RightButton;

	GuiImageData* LeftArrowImageData;
	GuiImageData* LeftArrowOverImageData;
	GuiImageData* RightArrowImageData;
	GuiImageData* RightArrowOverImageData;

	PageViewer()
	{
		Current = NULL;

		Subtitle->SetText(RIIVOLUTION_TITLE);

		LeftArrowImageData = new GuiImageData(arrow_left_png);
		LeftArrowOverImageData = new GuiImageData(arrow_active_left_png);
		RightArrowImageData = new GuiImageData(arrow_right_png);
		RightArrowOverImageData = new GuiImageData(arrow_active_right_png);

		if (Pages.size() > 1) {
			LeftButtonImage = new GuiImage(LeftArrowImageData);
			LeftButtonOverImage = new GuiImage(LeftArrowOverImageData);
			RightButtonImage = new GuiImage(RightArrowImageData);
			RightButtonOverImage = new GuiImage(RightArrowOverImageData);

			LeftButton = new GuiButton(LeftArrowImageData->GetWidth(), LeftArrowImageData->GetHeight());
			LeftButton->SetImage(LeftButtonImage);
			LeftButton->SetImageOver(LeftButtonOverImage);
			LeftButton->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
			LeftButton->SetPosition(68, 49);
			LeftButton->SetTrigger(&Trigger[Triggers::Select]);
			LeftButton->SetTrigger(&Trigger[Triggers::PageLeft]);
			Window->Append(LeftButton);

			RightButton = new GuiButton(RightArrowImageData->GetWidth(), RightArrowImageData->GetHeight());
			RightButton->SetImage(RightButtonImage);
			RightButton->SetImageOver(RightButtonOverImage);
			RightButton->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
			RightButton->SetPosition(348, 49);
			RightButton->SetTrigger(&Trigger[Triggers::Select]);
			RightButton->SetTrigger(&Trigger[Triggers::PageRight]);
			Window->Append(RightButton);
		}

		if (Pages.size())
			SetPage(0);
	}

	~PageViewer()
	{
		if (Pages.size() > 1) {
			Window->Remove(RightButton);
			Window->Remove(LeftButton);

			delete RightButton;
			delete LeftButton;
			delete RightButtonOverImage;
			delete RightButtonImage;
			delete LeftButtonOverImage;
			delete LeftButtonImage;
			delete LeftArrowImageData;
			delete LeftArrowOverImageData;
			delete RightArrowImageData;
			delete RightArrowOverImageData;
		}

		CleanPage();
	}

	const char* GetChoiceText(RiiOption* option)
	{
		if (option->Default > option->Choices.size())
			option->Default = 0; // NOTE: Should this really happen here? Probably not

		if (option->Default == 0)
			return "Disabled";

		return option->Choices[option->Default - 1].Name.c_str();
	}

	void CleanPage()
	{
		if (!Current)
			return;

		HaltGui();

		for (u32 i = 0; i < Current->Options.size(); i++) {
			Window->Remove(Title[i]);
			Window->Remove(Choice[i]);
			Window->Remove(LeftArrow[i]);
			Window->Remove(RightArrow[i]);

			delete Title[i];
			delete Choice[i];
			delete ChoiceText[i];
			delete ChoiceOverText[i];
			delete LeftArrowImage[i];
			delete LeftArrowOverImage[i];
			delete RightArrowImage[i];
			delete RightArrowOverImage[i];
			delete RightArrow[i];
			delete LeftArrow[i];
		}

		delete[] Title;
		delete[] ChoiceText;
		delete[] ChoiceOverText;
		delete[] Choice;
		delete[] LeftArrowImage;
		delete[] LeftArrowOverImage;
		delete[] RightArrowImage;
		delete[] RightArrowOverImage;
		delete[] LeftArrow;
		delete[] RightArrow;

		Current = NULL;

		ResumeGui();
	}

	void SetPage(u32 page)
	{
		CleanPage();

		if (page >= Pages.size())
			return;

		HaltGui();

		PageNumber = page;
		Current = &Pages[PageNumber];

		Subtitle->SetText(Current->Name.c_str());

		int options = Current->Options.size();
		Title = new GuiText*[options];
		ChoiceText = new GuiText*[options];
		ChoiceOverText = new GuiText*[options];
		Choice = new GuiButton*[options];
		LeftArrowImage = new GuiImage*[options];
		LeftArrowOverImage = new GuiImage*[options];
		RightArrowImage = new GuiImage*[options];
		RightArrowOverImage = new GuiImage*[options];
		LeftArrow = new GuiButton*[options];
		RightArrow = new GuiButton*[options];


		u32 i = 0;
		for (vector<RiiOption*>::iterator iter = Current->Options.begin(); iter != Current->Options.end(); iter++, i++) {
			int y = 82 + i * OPTION_FONT_HEIGHT;
			RiiOption* option = *iter;

			GuiText* title = new GuiText(option->Name.c_str(), OPTION_FONT_SIZE, (GXColor){0, 0, 0, 255}); Title[i] = title;
			title->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
			title->SetPosition(24, y);
			Window->Append(title);

			GuiText* choiceText = new GuiText(GetChoiceText(option), OPTION_FONT_SIZE, (GXColor){0, 0, 0, 255}); ChoiceText[i] = choiceText;
			choiceText->SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
			choiceText->SetPosition(0, 0);
			GuiText* choiceOverText = new GuiText(GetChoiceText(option), OPTION_FONT_SIZE, (GXColor){128, 128, 128, 255}); ChoiceOverText[i] = choiceOverText;
			choiceOverText->SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
			choiceOverText->SetPosition(0, 0);

			GuiButton* choice = new GuiButton(196, OPTION_FONT_HEIGHT); Choice[i] = choice;
			choice->SetLabel(choiceText);
			choice->SetLabelOver(choiceOverText);
			choice->SetTrigger(&Trigger[Triggers::Select]);
			choice->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
			choice->SetPosition(392, y);
			Window->Append(choice);

			GuiImage* leftArrowImage = new GuiImage(LeftArrowImageData); LeftArrowImage[i] = leftArrowImage;
			GuiImage* leftArrowOverImage = new GuiImage(LeftArrowOverImageData); LeftArrowOverImage[i] = leftArrowOverImage;
			GuiImage* rightArrowImage = new GuiImage(RightArrowImageData); RightArrowImage[i] = rightArrowImage;
			GuiImage* rightArrowOverImage = new GuiImage(RightArrowOverImageData); RightArrowOverImage[i] = rightArrowOverImage;

			GuiButton* leftArrow = new GuiButton(LeftArrowImageData->GetWidth(), LeftArrowImageData->GetHeight()); LeftArrow[i] = leftArrow;
			leftArrow->SetImage(leftArrowImage);
			leftArrow->SetImageOver(leftArrowOverImage);
			leftArrow->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
			leftArrow->SetPosition(368, y + OPTION_ARROW_OFFSET);
			leftArrow->SetTrigger(&Trigger[Triggers::Select]);
			Window->Append(leftArrow);

			GuiButton* rightArrow = new GuiButton(RightArrowImageData->GetWidth(), RightArrowImageData->GetHeight()); RightArrow[i] = rightArrow;
			rightArrow->SetImage(rightArrowImage);
			rightArrow->SetImageOver(rightArrowOverImage);
			rightArrow->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
			rightArrow->SetPosition(594, y + OPTION_ARROW_OFFSET);
			rightArrow->SetTrigger(&Trigger[Triggers::Select]);
			Window->Append(rightArrow);
		}

		ResumeGui();
	}

	void Update()
	{
		if (Pages.size() > 1) {
			if (LeftButton->GetState() == STATE_CLICKED) {
				UNSELECT_ALL();
				LeftButton->SetState(STATE_SELECTED, LeftButton->GetStateChan());
				SetPage(Wrap(PageNumber - 1, Pages.size()));
			}
			if (RightButton->GetState() == STATE_CLICKED) {
				UNSELECT_ALL();
				RightButton->SetState(STATE_SELECTED, RightButton->GetStateChan());
				SetPage(Wrap(PageNumber + 1, Pages.size()));
			}
		}

		if (!Current)
			return;

		for (u32 i = 0; i < Current->Options.size(); i++) {
			if (RightArrow[i]->GetState() == STATE_CLICKED || Choice[i]->GetState() == STATE_CLICKED) {
				UNSELECT_ALL();
				if (RightArrow[i]->GetState() == STATE_CLICKED)
					RightArrow[i]->SetState(STATE_SELECTED, RightArrow[i]->GetStateChan());
				if (Choice[i]->GetState() == STATE_CLICKED)
					Choice[i]->SetState(STATE_SELECTED, Choice[i]->GetStateChan());
				SetOption(i, Current->Options[i], Wrap(Current->Options[i]->Default + 1, Current->Options[i]->Choices.size() + 1));
			}
			if (LeftArrow[i]->GetState() == STATE_CLICKED) {
				UNSELECT_ALL();
				LeftArrow[i]->SetState(STATE_SELECTED, LeftArrow[i]->GetStateChan());
				SetOption(i, Current->Options[i], Wrap(Current->Options[i]->Default - 1, Current->Options[i]->Choices.size() + 1));
			}
		}
	}

	void SetOption(int index, RiiOption* option, u32 choice)
	{
		if (choice >= option->Choices.size() + 1)
			return;

		option->Default = choice;

		ChoiceText[index]->SetText(GetChoiceText(option));
		ChoiceOverText[index]->SetText(GetChoiceText(option));
	}

	int Wrap(int page, int pages)
	{
		if (page < 0)
			page = pages - 1;
		if (page >= pages)
			page = 0;

		return page;
	}
};

#define MENUINIT_CHECKBUTTONS() { \
	switch (buttons.Pressed()) { \
		case 0: \
			return Menus::Exit; \
		case 1: \
			return Menus::Settings; \
	} \
	CheckShutdown(); \
}

Menus::Enum MenuMount()
{
	HaltGui();
	ButtonList buttons(Window, 1);
	buttons.SetButton(0, "Exit", ButtonList::ExitImage);
	buttons.GetButton(0)->SetTrigger(&Trigger[Triggers::Home]);
	ResumeGui();

	do {
		Haxx_Mount(&Mounted);
		if (!Mounted.size()) {
			HaltGui(); Subtitle->SetText("Insert SD/USB..."); ResumeGui();
		}

		MENUINIT_CHECKBUTTONS();
	} while (!Mounted.size());
	HaltGui(); Subtitle->SetText("Loading..."); ResumeGui();

	LauncherStatus::Enum status;

	do {
		if (RVL_Initialize() < 0 || (status = Launcher_Init()) == LauncherStatus::IosError) {
				HaltGui(); Subtitle->SetText("IOS Error!"); ResumeGui();
				status = LauncherStatus::IosError;
		}

		MENUINIT_CHECKBUTTONS();
	} while (status != LauncherStatus::OK);

	return Menus::Init;
}

Menus::Enum MenuInit()
{
	HaltGui();
	ButtonList buttons(Window, 1);
	buttons.SetButton(0, "Exit", ButtonList::ExitImage);
	buttons.GetButton(0)->SetTrigger(&Trigger[Triggers::Home]);
	Title->SetText(RIIVOLUTION_TITLE);
	ResumeGui();

	Launcher_ScrubPlaytimeEntry();

	RVL_SetFST(NULL, 0);

	LauncherStatus::Enum status = LauncherStatus::NoDisc;
	HaltGui(); Subtitle->SetText("Loading..."); ResumeGui();

	do {
		status = Launcher_ReadDisc();
		switch (status) {
			case LauncherStatus::NoDisc:
				HaltGui(); Subtitle->SetText("Insert Disc..."); ResumeGui();
				break;
			case LauncherStatus::ReadError:
				HaltGui(); Subtitle->SetText("Disc Read Error!"); ResumeGui();
				sleep(3); // TODO: Repeatedly MENUINIT_CHECKBUTTONS during this sleep
				HaltGui(); Subtitle->SetText("Loading..."); ResumeGui();
				break;
			default:
				break;
		}

		MENUINIT_CHECKBUTTONS();
	} while (status != LauncherStatus::OK);

	HaltGui(); Title->SetText(Launcher_GetGameName()); ResumeGui();

	vector<RiiDisc> discs;
	for (vector<int>::iterator mount = Mounted.begin(); mount != Mounted.end(); mount++) {
		ParseXMLs(*mount, &discs);
	}
	Mounted.insert(Mounted.end(), ToMount.begin(), ToMount.end());
	Disc = CombineDiscs(&discs);
	ParseConfigXMLs(&Disc);

	Launcher_RVL();
	HaltGui(); Title->SetText(Launcher_GetGameName()); ResumeGui();

	MENUINIT_CHECKBUTTONS();

	return Menus::Main;
}

#define PAGENUM(page, pagenum) { \
	char num[0x10]; sprintf(num, "%d", pagenum); \
	page.Name = section->Name + " [" + num + "]"; \
}

static void PreparePages()
{
	Pages.clear();
	for (vector<RiiSection>::iterator section = Disc.Sections.begin(); section != Disc.Sections.end(); section++) {
		Page page;
		int pagenum = 1;
		for (vector<RiiOption>::iterator option = section->Options.begin(); option != section->Options.end(); option++) {
			if (page.Options.size() == OPTIONS_PER_PAGE) {
				PAGENUM(page, pagenum++);
				Pages.push_back(page);
				page = Page();
			}

			page.Options.push_back(&*option);
		}

		if (pagenum > 1) {
			PAGENUM(page, pagenum);
		} else
			page.Name = section->Name;
		Pages.push_back(page);
	}
}

#define BANNER_TICKET_PATH "/ticket/00010001/52494956.tik"
#define BANNER_CONTENT_PATH "/title/00010001/52494956"

Menus::Enum MenuMain()
{
	bool installed = false;

	int fd = ISFS_Open(BANNER_TICKET_PATH, 0);
	if (fd >= 0) {
		installed = true;
		ISFS_Close(fd);
	}

	HaltGui();
	Title->SetText(Launcher_GetGameName());
	ButtonList buttons(Window, 2);
	buttons.SetButton(0, "Exit", ButtonList::ExitImage);
	buttons.GetButton(0)->SetTrigger(&Trigger[Triggers::Home]);
	buttons.SetButton(1, installed ? "Remove" : "Install", ButtonList::InstallImage);
	buttons.SetButtonLaunch(Window, 2, "Launch");
	buttons.GetButton(2)->SetState(STATE_SELECTED, 0);
	ResumeGui();

	PreparePages();

	HaltGui();
	PageViewer viewer;
	ResumeGui();

	while (true) {
		viewer.Update();

		switch (buttons.Pressed()) {
			case 0:
				return Menus::Exit;
			case 2:
				return Menus::Launch;
			case 1:
				if (installed)
					return Menus::Uninstall;
				else
					return Menus::Install;
		}

		CheckShutdown();

		if (!Launcher_DiscInserted())
			return Menus::Init;
	}
}

Menus::Enum MenuInstall()
{
	HaltGui(); Subtitle->SetText("Installing..."); ResumeGui();
	int ret = InstallChannel();
	HaltGui();
	if (ret == 0)
		Subtitle->SetText("Installed");
	else {
		char failed[0x20];
		sprintf(failed, "Failed (%d)", ret);
		Subtitle->SetText(failed);
	}
	ResumeGui();

	sleep(3);

	return Menus::Main;
}

Menus::Enum MenuUninstall()
{
	HaltGui(); Subtitle->SetText("Uninstalling..."); ResumeGui();

	bool fail = false;
	if (ISFS_Delete(BANNER_CONTENT_PATH) < 0)
		fail = true;
	if (ISFS_Delete(BANNER_TICKET_PATH) < 0)
		fail = true;

	if (fail) {
		HaltGui();
		Subtitle->SetText("Failed");
		ResumeGui();
		sleep(3);
	}

	return Menus::Main;
}

Menus::Enum MenuLaunch()
{
	HaltGui(); Subtitle->SetText("Loading..."); ResumeGui();

	usleep(1000);
	HaltGui();

	ShutoffRumble();

	SaveConfigXML(&Disc);

	RVL_Patch(&Disc);

	// Launcher_CommitRVL(true); // TODO: CommitRVL properly?

	Launcher_RunApploader();

	Launcher_CommitRVL(false);

	Launcher_AddPlaytimeEntry();

	Launcher_SetVideoMode();

	RVL_PatchMemory(&Disc);

	RVL_Unmount();

	File_Deinit();
	Launcher_Launch();

	return Menus::Exit;
}
