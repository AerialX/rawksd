#include <gccore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ogcsys.h>
#include <unistd.h>
#include <wiiuse/wpad.h>

#include "FreeTypeGX.h"
#include "video.h"
#include "audio.h"
#include "input.h"
#include "filelist.h"

#include "init.h"

#include "riivolution_config.h"
#include "installer.h"

#include "rawksd_menu.h"

#include <vector>
using std::vector;

RiiDisc Disc;
vector<int> Mounted;
vector<int> ToMount;

int main(int argc, char *argv[])
{
	Initialise();
	WPAD_SetDataFormat(WPAD_CHAN_ALL, WPAD_FMT_BTNS);

	InitFreeType((u8*)font_ttf, font_ttf_size);
	InitGUIThreads();

	MainMenu(Menus::Mount);

	return 0;
}


