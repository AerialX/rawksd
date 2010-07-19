#include "rawksd_menu.h"
#include "launcher.h"
#include "riivolution.h"
#include "riivolution_config.h"
#include "haxx.h"
#include "init.h"
#include "wdvd.h"
#include "motd.h"

#include <files.h>

#include <math.h>
#include <unistd.h>

extern "C" {
	extern const u8 menu_play_png[];
	extern const u8 menu_play_sel_png[];
	extern const u8 menu_rip_png[];
	extern const u8 menu_rip_sel_png[];
	extern const u8 menu_scores_png[];
	extern const u8 menu_scores_sel_png[];
	extern const u8 menu_save_png[];
	extern const u8 menu_save_sel_png[];
	extern const u8 menu_exit_png[];
	extern const u8 menu_exit_sel_png[];
}

const u8* MenuMain::raw_images[OPTION_COUNT*3+1] = {
		menu_play_png,
		menu_play_sel_png,
		NULL,
		menu_rip_png,
		menu_rip_sel_png,
		NULL,
		menu_scores_png,
		menu_scores_sel_png,
		NULL,
		menu_save_png,
		menu_save_sel_png,
		NULL,
		menu_exit_png,
		menu_exit_sel_png,
		NULL,
		NULL
};

MenuMain::MenuMain(GuiWindow *Parent) : RawkMenu(Parent, raw_images, 238, 188),
Subtitle(NULL)
{
	Buttons[OPTION_EXIT]->button->SetTrigger(1, &Trigger[Triggers::Home]);
}

MenuMain::~MenuMain()
{
	if (Subtitle) {
		Parent->Remove(Subtitle);
		delete Subtitle;
	}
}

RawkMenu* MenuMain::Process()
{
	int clicked;
	RawkMenu *next = this;

	if (Subtitle==NULL) {
		const char *motd = GetMotd();
		if (motd && motd[0]) {
			Subtitle = new GuiText(motd, 18, (GXColor){255, 255, 255, 255});
			Subtitle->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
			Subtitle->SetPosition(264, 338);
			Parent->Append(Subtitle);
		}
	}

	clicked = GetClicked();
	if (clicked>=0) {
		HaltGui();
		switch(clicked) {
			case OPTION_EXIT:
				next = NULL;
				break;
			case OPTION_PLAY:
				next = new MenuPlay(Parent);
				break;
			default:
				Buttons[clicked]->Enable();
		}
	}

	return next;
}
