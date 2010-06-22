#include <gccore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ogcsys.h>
#include <unistd.h>
#include <wiiuse/wpad.h>
#include <files.h>

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

#if 0
// TODO: Clean this up
	s32 fd = File_Open("/mnt/isfs/title/00010001/52494956/data/disc.sys", O_RDONLY);
	if (fd>=0) {
		File_Close(fd);
		File_Delete("/mnt/isfs/title/00010001/52494956/data/launch.sys");
	}

	fd = File_Open("/mnt/isfs/title/00010001/52494956/data/launch.sys", O_RDONLY);
	if (fd >=0) {
		u64 ret_title=0;
		u64 *ret_title_buf = (u64*)memalign(32, 32);
		if (ret_title_buf) {
			memset(ret_title_buf, 0, sizeof(u64));
			File_Read(fd, ret_title_buf, sizeof(u64));
			ret_title = *ret_title_buf;
			free(ret_title_buf);
		}
		File_Close(fd);
		if (ret_title==0x0001000152494956llu)
			SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);
		File_Delete("/mnt/isfs/title/00010001/52494956/data/launch.sys");
	}
#endif

	InitFreeType((u8*)font_ttf, font_ttf_size);
	InitGUIThreads();

	MainMenu(Menus::Mount);

	exit(0);
}
