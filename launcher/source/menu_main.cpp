#include "menu.h"
#include "launcher.h"
#include "haxx.h"
#include "riivolution_config.h"

#include <unistd.h>
#include <files.h>

#include "mystl.h"

/*
#define OPTIONS_PER_PAGE 15
#define OPTION_FONT_SIZE 18
#define OPTION_FONT_HEIGHT 22
#define OPTION_ARROW_OFFSET 0
*/
#define OPTIONS_PER_PAGE 15
#define OPTION_FONT_SIZE 20
#define OPTION_FONT_HEIGHT 28
#define OPTION_ARROW_OFFSET 0

using std::string;

struct Page {
	string Name;
	List<RiiOption*> Options;
};

static RiiDisc Disc;
static List<Page> Pages;

extern "C" {
	extern u8 arrow_left_png[];
	extern u8 arrow_active_left_png[];
	extern u8 arrow_right_png[];
	extern u8 arrow_active_right_png[];
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

	GuiText* PageText;
	GuiText* PageNumberText;
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

		Subtitle->SetText("Riivolution");

		LeftArrowImageData = new GuiImageData(arrow_left_png);
		LeftArrowOverImageData = new GuiImageData(arrow_active_left_png);
		RightArrowImageData = new GuiImageData(arrow_right_png);
		RightArrowOverImageData = new GuiImageData(arrow_active_right_png);

		if (Pages.Size() > 1) {
			PageText = new GuiText("Page", 18, (GXColor){0, 0, 0, 255});
			PageText->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
			PageText->SetPosition(352, 48);
			Window->Append(PageText);

			PageNumberText = new GuiText("", 18, (GXColor){0, 0, 0, 255});
			PageNumberText->SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
			PageNumberText->SetPosition(400 + 48 / 2, 48);
			Window->Append(PageNumberText);

			LeftButtonImage = new GuiImage(LeftArrowImageData);
			LeftButtonOverImage = new GuiImage(LeftArrowOverImageData);
			RightButtonImage = new GuiImage(RightArrowImageData);
			RightButtonOverImage = new GuiImage(RightArrowOverImageData);

			LeftButton = new GuiButton(LeftArrowImageData->GetWidth(), LeftArrowImageData->GetHeight());
			LeftButton->SetImage(LeftButtonImage);
			LeftButton->SetImageOver(LeftButtonOverImage);
			LeftButton->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
			LeftButton->SetPosition(400, 48);
			LeftButton->SetTrigger(&Trigger[Triggers::Select]);
			LeftButton->SetTrigger(&Trigger[Triggers::PageLeft]);
			Window->Append(LeftButton);

			RightButton = new GuiButton(RightArrowImageData->GetWidth(), RightArrowImageData->GetHeight());
			RightButton->SetImage(RightButtonImage);
			RightButton->SetImageOver(RightButtonOverImage);
			RightButton->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
			RightButton->SetPosition(448, 48);
			RightButton->SetTrigger(&Trigger[Triggers::Select]);
			RightButton->SetTrigger(&Trigger[Triggers::PageRight]);
			Window->Append(RightButton);
		}

		if (Pages.Size())
			SetPage(0);
	}

	~PageViewer()
	{
		if (Pages.Size() > 1) {
			Window->Remove(RightButton);
			Window->Remove(LeftButton);
			Window->Remove(PageText);
			Window->Remove(PageNumberText);

			delete RightButton;
			delete LeftButton;
			delete RightButtonOverImage;
			delete RightButtonImage;
			delete LeftButtonOverImage;
			delete LeftButtonImage;
			delete PageNumberText;
			delete PageText;
			delete LeftArrowImageData;
			delete LeftArrowOverImageData;
			delete RightArrowImageData;
			delete RightArrowOverImageData;
		}

		CleanPage();
	}

	const char* GetChoiceText(RiiOption* option)
	{
		if (option->Default > option->Choices.Size())
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

		for (u32 i = 0; i < Current->Options.Size(); i++) {
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

		if (page >= Pages.Size())
			return;

		HaltGui();

		PageNumber = page;
		Current = &Pages[PageNumber];

		Subtitle->SetText(Current->Name.c_str());

		if (Pages.Size() > 1) {
			char pagenum[0x10];
			sprintf(pagenum, "%d", PageNumber + 1);
			PageNumberText->SetText(pagenum);
		}

		int options = Current->Options.Size();
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
		for (RiiOption** iter = Current->Options.Data(); iter != Current->Options.End(); iter++, i++) {
			int y = 112 + i * OPTION_FONT_HEIGHT;
			RiiOption* option = *iter;

			GuiText* title = new GuiText(option->Name.c_str(), OPTION_FONT_SIZE, (GXColor){0, 0, 0, 255}); Title[i] = title;
			title->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
			title->SetPosition(24, y);
			Window->Append(title);

			GuiText* choiceText = new GuiText(GetChoiceText(option), OPTION_FONT_SIZE, (GXColor){0, 0, 0, 255}); ChoiceText[i] = choiceText;
			choiceText->SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
			choiceText->SetPosition(0, 0);
			GuiText* choiceOverText = new GuiText(GetChoiceText(option), OPTION_FONT_SIZE, (GXColor){128, 0, 0, 255}); ChoiceOverText[i] = choiceOverText;
			choiceOverText->SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
			choiceOverText->SetPosition(0, 0);

			GuiButton* choice = new GuiButton(152, OPTION_FONT_HEIGHT); Choice[i] = choice;
			choice->SetLabel(choiceText);
			choice->SetLabelOver(choiceOverText);
			choice->SetTrigger(&Trigger[Triggers::Select]);
			choice->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
			choice->SetPosition(352, y);
			Window->Append(choice);

			GuiImage* leftArrowImage = new GuiImage(LeftArrowImageData); LeftArrowImage[i] = leftArrowImage;
			GuiImage* leftArrowOverImage = new GuiImage(LeftArrowOverImageData); LeftArrowOverImage[i] = leftArrowOverImage;
			GuiImage* rightArrowImage = new GuiImage(RightArrowImageData); RightArrowImage[i] = rightArrowImage;
			GuiImage* rightArrowOverImage = new GuiImage(RightArrowOverImageData); RightArrowOverImage[i] = rightArrowOverImage;

			GuiButton* leftArrow = new GuiButton(LeftArrowImageData->GetWidth(), LeftArrowImageData->GetHeight()); LeftArrow[i] = leftArrow;
			leftArrow->SetImage(leftArrowImage);
			leftArrow->SetImageOver(leftArrowOverImage);
			leftArrow->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
			leftArrow->SetPosition(336, y + OPTION_ARROW_OFFSET);
			leftArrow->SetTrigger(&Trigger[Triggers::Select]);
			Window->Append(leftArrow);

			GuiButton* rightArrow = new GuiButton(RightArrowImageData->GetWidth(), RightArrowImageData->GetHeight()); RightArrow[i] = rightArrow;
			rightArrow->SetImage(rightArrowImage);
			rightArrow->SetImageOver(rightArrowOverImage);
			rightArrow->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
			rightArrow->SetPosition(504, y + OPTION_ARROW_OFFSET);
			rightArrow->SetTrigger(&Trigger[Triggers::Select]);
			Window->Append(rightArrow);
		}

		ResumeGui();
	}

	void Update()
	{
		if (Pages.Size() > 1) {
			if (LeftButton->GetState() == STATE_CLICKED) {
				LeftButton->ResetState();
				SetPage(Wrap(PageNumber - 1, Pages.Size()));
			}
			if (RightButton->GetState() == STATE_CLICKED) {
				RightButton->ResetState();
				SetPage(Wrap(PageNumber + 1, Pages.Size()));
			}
		}

		if (!Current)
			return;

		for (u32 i = 0; i < Current->Options.Size(); i++) {
			if (RightArrow[i]->GetState() == STATE_CLICKED || Choice[i]->GetState() == STATE_CLICKED) {
				RightArrow[i]->ResetState();
				Choice[i]->ResetState();
				SetOption(i, Current->Options[i], Wrap(Current->Options[i]->Default + 1, Current->Options[i]->Choices.Size() + 1));
			}
			if (LeftArrow[i]->GetState() == STATE_CLICKED) {
				LeftArrow[i]->ResetState();
				SetOption(i, Current->Options[i], Wrap(Current->Options[i]->Default - 1, Current->Options[i]->Choices.Size() + 1));
			}
		}
	}

	void SetOption(int index, RiiOption* option, u32 choice)
	{
		if (choice >= option->Choices.Size() + 1)
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
}
Menus::Enum MenuInit()
{
	HaltGui();
	ButtonList buttons(Window, 1);
	buttons.SetButton(0, "Exit", ButtonList::ExitImage);
	buttons.GetButton(0)->SetTrigger(&Trigger[Triggers::Home]);
	ResumeGui();

	List<int> mounted;
	do {
		mounted = Haxx_Mount();
		if (!mounted.Size())
			Subtitle->SetText("No SD/USB Mounted");

		MENUINIT_CHECKBUTTONS();
	} while (!mounted.Size());

	RVL_Initialize();
	RVL_SetClusters(true);

	LauncherStatus::Enum status;

	do {
		status = Launcher_Init();
		switch (status) {
			case LauncherStatus::IosError:
				Subtitle->SetText("IOS Error!");
				break;
			default:
				break;
		}

		MENUINIT_CHECKBUTTONS();
	} while (status != LauncherStatus::OK);

	do {
		status = Launcher_ReadDisc();
		switch (status) {
			case LauncherStatus::NoDisc:
				Subtitle->SetText("Insert Disc...");
				//TODO: Launcher_WaitForDisc() maybe, also MENUINIT_CHECKBUTTONS during it >.>
				break;
			case LauncherStatus::ReadError:
				Subtitle->SetText("Disc Read Error!");
				sleep(3); // TODO: Repeatedly MENUINIT_CHECKBUTTONS during this sleep
				break;
			default:
				break;
		}

		MENUINIT_CHECKBUTTONS();
	} while (status != LauncherStatus::OK);

	Title->SetText(Launcher_GetGameName());

	List<RiiDisc> discs;
	for (int* mount = mounted.Data(); mount != mounted.End(); mount++) {
		char mountpoint[MAXPATHLEN];
		char mountpath[MAXPATHLEN];
		if (File_GetMountPoint(*mount, mountpoint, sizeof(mountpoint)) < 0)
			continue;
		strcpy(mountpath, mountpoint);
		strcat(mountpath, RIIVOLUTION_PATH);
		ParseXMLs(mountpath, mountpoint, &discs);
	}
	Disc = CombineDiscs(&discs);
	ParseConfigXMLs(&Disc);
	MENUINIT_CHECKBUTTONS();

	return Menus::Main;
}

#define PAGENUM(page, pagenum) { \
	char num[0x10]; sprintf(num, "%d", pagenum); \
	page.Name = section->Name + " [" + num + "]"; \
}

static void PreparePages()
{
	Pages.Clear();
	for (RiiSection* section = Disc.Sections.Data(); section != Disc.Sections.End(); section++) {
		Page page;
		int pagenum = 1;
		for (RiiOption* option = section->Options.Data(); option != section->Options.End(); option++) {
			if (page.Options.Size() == OPTIONS_PER_PAGE) {
				PAGENUM(page, pagenum++);
				Pages.Add(page);
				page = Page();
			}

			page.Options.Add(option);
		}

		if (pagenum > 1) {
			PAGENUM(page, pagenum);
		} else
			page.Name = section->Name;
		Pages.Add(page);
	}
}

Menus::Enum MenuMain()
{
	HaltGui();
	ButtonList buttons(Window, 2);
	buttons.SetButton(0, "Exit", ButtonList::ExitImage);
	buttons.GetButton(0)->SetTrigger(&Trigger[Triggers::Home]);
	buttons.SetButton(1, "Launch", ButtonList::LaunchImage);
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
			case 1:
				return Menus::Launch;
		}
	}
}

Menus::Enum MenuLaunch()
{
	Subtitle->SetText("Loading...");

	ShutoffRumble();

	SaveConfigXML(&Disc);

	if (Launcher_RVL() < 0)
		return Menus::Exit;
	RVL_Patch(&Disc);

	// Launcher_CommitRVL(true); // TODO: CommitRVL properly?

	HaltGui();

	Launcher_RunApploader();

	Launcher_CommitRVL(false);

	RVL_PatchMemory(&Disc);

	Launcher_Launch();

	return Menus::Exit;
}
