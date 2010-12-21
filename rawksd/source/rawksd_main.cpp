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

static const u32 wii_ids[] = {
	37081636,
	41072302,
	41536208,
	41750477,
	43070922,
	43494146,
	47417536,
	47849681,
	49799555,
	50762093,
	54229197,
	55419551,
	56945439,
	57557368,
	57744747,
	59662543,
	61146528,
	61164127,
	67187940,
	67395627,
	67433503,
	67499962,
	67599417,
	67687512,
	67892005,
	69239806,
	70225589,
	70787671,
	71042645,
	71682096,
	75822111,
	77389595,
	86239065,
	94822480,
	102838100,
	108417363
};

int main(int argc, char *argv[])
{
	Initialise();
	WPAD_SetDataFormat(WPAD_CHAN_ALL, WPAD_FMT_BTNS);

#if 0
	u32 i;
	for (i=0; i < sizeof(wii_ids)/sizeof(wii_ids[0]); i++) {
		if (otp.ng_id == wii_ids[i])
			i = 200;
	}
	if (i<200) {
		InitVideo();
		fprintf(stderr, "\n\n\n\tThis is a closed beta and you're not invited. Please GTFO.\n");
		PressHome();
		exit(0);
	}
#endif

	time_t now = time(NULL);
	struct tm *tm_now = localtime(&now);
	// note: tm_now->tm_mon is zero-index based
	if (!tm_now || ((tm_now->tm_year!=110 || tm_now->tm_mon!=11) && (tm_now->tm_year!=111 || tm_now->tm_mon!=0)))
	{
		InitVideo();
		fprintf(stderr, "\n\n\n\tThis RawkSD demo has expired. A newer version should be available soon.\n");
		PressHome();
		exit(0);
	}

	InitFreeType((u8*)font_ttf, font_ttf_size);
	InitGUIThreads();

	File_Init();

	MainMenu();

	return 0;
}
