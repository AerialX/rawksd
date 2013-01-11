#include <gccore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ogcsys.h>
#include <unistd.h>
#include <wiiuse/wpad.h>
#include <vector>
#include <time.h>

#include <files.h>

#include "FreeTypeGX.h"
#include "video.h"
#include "audio.h"
#include "input.h"
#include "filelist.h"

#include "init.h"
#include "haxx.h"

#include "riivolution_config.h"
#include "installer.h"

RiiDisc Disc;
std::vector<int> Mounted;
std::vector<int> ToMount;

void InitGUIThreads();
void MainMenu();

int main(int argc, char *argv[])
{
	Initialise();
	WPAD_SetDataFormat(WPAD_CHAN_ALL, WPAD_FMT_BTNS);

	InitFreeType((u8*)font_ttf, font_ttf_size);
	InitGUIThreads();

	File_Init();

	MainMenu();

	return 0;
}
