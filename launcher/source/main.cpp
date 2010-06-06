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
#include "menu.h"
#include "input.h"
#include "filelist.h"

#include "init.h"

int main(int argc, char *argv[])
{
	Initialise();

	InitFreeType((u8*)font_ttf, font_ttf_size);
	InitGUIThreads();

	MainMenu(Menus::Mount);

	return 0;
}
