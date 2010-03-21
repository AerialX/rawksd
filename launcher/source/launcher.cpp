#include "launcher.h"
#include "menu.h"
#include "wdvd.h"
#include "riivolution.h"

#include <ogc/lwp_watchdog.h>
#include <ogc/lwp_threads.h>

#include <files.h>

#include <malloc.h>

typedef void (*AppReport) (const char*, ...);
typedef void (*AppEnter) (AppReport);
typedef int  (*AppLoad) (void**, s32*, s32*);
typedef void* (*AppExit) ();
typedef void (*AppStart) (AppEnter*, AppLoad*, AppExit*);

typedef struct {
	u8 revision[16];
	AppStart start;
	u32 loader_size;
	u32 data_size;
	u8 unused[4];
} app_info;

static union {
	u32 partition_info[24];
	app_info app;
} ATTRIBUTE_ALIGN(32);

static char GameName[0x40] ATTRIBUTE_ALIGN(32);
static s16 BannerName[42];

#define PART_OFFSET			0x00040000
#define APP_INFO_OFFSET		0x2440
#define APP_DATA_OFFSET		(APP_INFO_OFFSET + 0x20)

#define min(a,b) ((a)>(b) ? (b) : (a))
#define max(a,b) ((a)>(b) ? (a) : (b))

static int nullprintf(const char* fmt, ...)
{
	return 0;
}

typedef struct {
    u8 zeroes[64]; // padding
    u32 imet; // "IMET"
    u8 unk[8];  // 0x0000060000000003 fixed, unknown purpose
    u32 sizes[3]; // icon.bin, banner.bin, sound.bin
    u32 flag1; // unknown
    s16 names[10][42]; // Japanese, English, German, French, Spanish, Italian, Dutch, unknown, unknown, Korean
    u8 zeroes_2[588]; // padding
    u8 crypto[16]; // MD5 of 0x40 to 0x640 in header. crypto should be all 0's when calculating final MD5
} __attribute__((packed)) IMET;

const s16* Launcher_GetGameNameWide()
{
	if (RVL_GetFST()) {
		if (BannerName[0])
			return BannerName;
		DiscNode* node = RVL_FindNode("/opening.bnr");
		if (node != NULL) {
			static IMET imet ATTRIBUTE_ALIGN(32);
			if (!WDVD_LowRead(&imet, sizeof(imet), (u64)node->DataOffset << 2) && imet.imet == 0x494D4554) {
				memcpy(BannerName, imet.names[CONF_GetLanguage()], sizeof(BannerName));
				if (BannerName[0]) {
					return BannerName;
				}
			}
		}
	}

	for (int i = 0; i < 42; i++)
		BannerName[i] = GameName[i];
	return BannerName;
}

const char* Launcher_GetGameName()
{
	if (!RVL_GetFST())
		return GameName;

	static char gamename[42];
	const s16* name = Launcher_GetGameNameWide();
	// Find the last non-null char
	int end = 41;
	while (!name[end])
		end--;
	end = MIN(end, 40);
	for (int i = 0; i <= end; i++)
		gamename[i] = (char)(name[i] ? name[i] : ' ');
	gamename[end + 1] = '\0';

	return gamename;
}

LauncherStatus::Enum Launcher_Init()
{
	int fd = WDVD_Init();
	if (fd < 0)
		return LauncherStatus::IosError;

	if (!WDVD_Reset())
		return LauncherStatus::IosError;

	return LauncherStatus::OK;
}

bool Launcher_DiscInserted()
{
	bool cover;
	if (!WDVD_VerifyCover(&cover))
		return cover;
	return false;
}

static bool subsequent = false;
LauncherStatus::Enum Launcher_ReadDisc()
{
	u32 i;

	BannerName[0] = 0;

	if (!Launcher_DiscInserted())
		return LauncherStatus::NoDisc;

	if (subsequent && !WDVD_Reset()) // In case of re-inserted discs, needs to be called again
		return LauncherStatus::IosError;

	subsequent = true;

	if (WDVD_LowReadDiskId())
		return LauncherStatus::ReadError;

	memset(GameName, 0xFF, 0x40);
	if (WDVD_LowReadBCA((u8*)GameName, 0x40))
		return LauncherStatus::ReadError;

	for (i=0; i < 51; i++)
		if (GameName[i])
			return LauncherStatus::ReadError;

	memset(GameName, 0xFF, 0x40);
	if (WDVD_LowUnencryptedRead(GameName, 0x40, 0x100))
		return LauncherStatus::ReadError;

	for (i=0; i < 0x40; i++)
		if (GameName[i])
			return LauncherStatus::ReadError;

	// check if it returns a key when it shouldn't (001 error)
	if (WDVD_ReportKey(4, 0, GameName) != -2)
		return LauncherStatus::ReadError;

	// make sure LowUnenencryptedRead lba table hasn't been patched
	WDVD_SetDVDMode(0);
	if (WDVD_LowUnencryptedRead(0, 0, 0x2EE00000llu << 2) != -32)
		return LauncherStatus::ReadError;

	if (WDVD_LowUnencryptedRead(GameName, 0x40, 0x20))
		return LauncherStatus::ReadError;

	memset(partition_info, 0, sizeof(partition_info));
	// make sure there is at least one primary partition
	if (WDVD_LowUnencryptedRead(partition_info, 0x20, PART_OFFSET) || partition_info[0]==0)
		return LauncherStatus::ReadError;

	// read primary partition table
	if (WDVD_LowUnencryptedRead(partition_info + 8, max(4, min(8, partition_info[0])) * 8, (u64)partition_info[1] << 2))
		return LauncherStatus::ReadError;
	for (i = 0; i < partition_info[0]; i++)
		if (partition_info[i * 2 + 8 + 1] == 0)
			break;

	// make sure we found a game partition
	if (i >= partition_info[0])
		return LauncherStatus::ReadError;

	if (WDVD_LowOpenPartition((u64)partition_info[i * 2 + 8] << 2))
		return LauncherStatus::ReadError;

	return LauncherStatus::OK;
}

static u32 fstdata[0x40] ATTRIBUTE_ALIGN(32);

const u32* Launcher_GetFstData()
{
	return fstdata;
}

LauncherStatus::Enum Launcher_RVL()
{
	if (WDVD_LowRead(fstdata, 0x40, 0x420))
		return LauncherStatus::ReadError;

	fstdata[2] <<= 2;
	u8* fstbuffer = (u8*)memalign(32, fstdata[2]);
	if (!fstbuffer)
		return LauncherStatus::OutOfMemory;

	if (WDVD_LowRead(fstbuffer, fstdata[2], (u64)fstdata[1] << 2))
		return LauncherStatus::ReadError;

	RVL_SetFST(fstbuffer, fstdata[2]);
	free(fstbuffer);
	return LauncherStatus::OK;
}

LauncherStatus::Enum Launcher_CommitRVL(bool dip)
{
	if (dip) {
		File_CreateDir("/riivolution/temp");
		File_CreateFile("/riivolution/temp/fst");
		int tmpfd = File_Open("/riivolution/temp/fst", O_WRONLY | O_TRUNC);
		if (tmpfd < 0)
			return LauncherStatus::IosError;
		File_Write(tmpfd, RVL_GetFST(), RVL_GetFSTSize());
		File_Close(tmpfd);
		fstdata[1] = (u32)(RVL_GetShiftOffset(fstdata[2]) >> 2);
		RVL_AddPatch(RVL_AddFile("/riivolution/temp/fst"), 0, (u64)fstdata[1] << 2, fstdata[2]);
		fstdata[2] >>= 2;
		fstdata[3] = fstdata[2]; // NOTE: Not really sure why
		File_CreateFile("/riivolution/temp/fst.header");
		tmpfd = File_Open("/riivolution/temp/fst.header", O_WRONLY | O_TRUNC);
		if (tmpfd < 0)
			return LauncherStatus::IosError;
		File_Write(tmpfd, fstdata, 0x40);
		File_Close(tmpfd);
		RVL_AddPatch(RVL_AddFile("/riivolution/temp/fst.header"), 0, 0x420, 0x40);
	} else
		memcpy(*MEM_FSTADDRESS, RVL_GetFST(), RVL_GetFSTSize()); // TODO: Account for fstdata changing

	return LauncherStatus::OK;
}

typedef struct
{
	u32 checksum;
	union
	{
		u32 data[0x1f];
		struct {
			s16 name[42];
			u64 ticks_boot;
			u64 ticks_last;
			u32 title_id;
			u16 title_gid;
			//u8 padding[18];
		} ATTRIBUTE_PACKED;
	};
} playtime_buf_t;

#define PLAYLOG_FILE "/mnt/isfs/title/00000001/00000002/data/play_rec.dat"
LauncherStatus::Enum Launcher_AddPlaytimeEntry()
{
	u32 titleid = WDVD_GetTMD()->title_id;
	u16 groupid = WDVD_GetTMD()->group_id;

	static playtime_buf_t playtime_buf ATTRIBUTE_ALIGN(32);
	File_CreateFile(PLAYLOG_FILE);
	int d = File_Open(PLAYLOG_FILE, O_WRONLY);
	if (d >= 0) {
		u64 tick_now = gettime();
		memset(&playtime_buf, 0, sizeof(playtime_buf_t));
		s32 i = 0;
		memcpy(playtime_buf.name, Launcher_GetGameNameWide(), sizeof(playtime_buf.name));
		playtime_buf.ticks_boot = tick_now;
		playtime_buf.ticks_last = tick_now;
		playtime_buf.title_id = titleid;
		playtime_buf.title_gid = groupid;
		for (i = 0; i < 0x1f; i++)
			playtime_buf.checksum += playtime_buf.data[i];

		bool ret = sizeof(playtime_buf_t) == File_Write(d, (u8*)&playtime_buf, sizeof(playtime_buf_t));
		File_Close(d);
		if (ret)
			return LauncherStatus::OK;
		return LauncherStatus::IosError;
	}

	return LauncherStatus::IosError;
}

static void *app_address = NULL;

static void* FindInBuffer(void* buffer, s32 size, const void* tofind, s32 findsize)
{
	for (int i = 0; i < size - findsize; i++, buffer = (u8*)buffer + 1) {
		if (!memcmp(buffer, tofind, findsize))
			return buffer;
	}

	return NULL;
}

extern RiiDisc Disc;
static void ApplyBinaryPatches(s32 app_section_size)
{
	void* found;

	// DIP
	while ((found = FindInBuffer(app_address, app_section_size, "/dev/di", 7))) {
		((u8*)found)[6] = 'o'; // "/dev/di" to "/dev/do"
	}

	RVL_PatchMemory(&Disc, app_address, app_section_size);
}

LauncherStatus::Enum Launcher_SetVideoMode()
{
	GXRModeObj* vmode;
	u32 tvmode = CONF_GetVideo();
	if (tvmode==CONF_VIDEO_PAL) {
		*MEM_VIDEOMODE = VI_EURGB60;
		if (CONF_GetProgressiveScan() > 0 && VIDEO_HaveComponentCable()) {
			// wtf, why does this cause a DSI?
			//vmode = &TVEurgb60Hz480Prog;
			vmode = &TVNtsc480Prog;
		} else if (CONF_GetEuRGB60() > 0)
			vmode = &TVEurgb60Hz480IntDf;
		else {
			vmode = &TVPal528IntDf;
			*MEM_VIDEOMODE = VI_PAL;
		}
	} else {
		if (CONF_GetProgressiveScan() > 0 && VIDEO_HaveComponentCable())
			vmode = &TVNtsc480Prog;
		else
			vmode = &TVNtsc480IntDf;
		if (tvmode == CONF_VIDEO_NTSC)
			*MEM_VIDEOMODE = VI_NTSC;
		else
			*MEM_VIDEOMODE = VI_MPAL;
	}

	VIDEO_Configure(vmode);
	VIDEO_SetBlack(true);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	VIDEO_WaitVSync();

	return LauncherStatus::OK;
}

LauncherStatus::Enum Launcher_RunApploader()
{
	AppEnter app_enter = NULL;
	AppLoad app_loader = NULL;
	AppExit app_exit = NULL;
	s32 app_section_size = 0;
	s32 app_disc_offset = 0;

	settime(secs_to_ticks(time(NULL) - 946684800));

	// Avoid a flash of green
	VIDEO_SetBlack(true); VIDEO_Flush(); VIDEO_WaitVSync(); VIDEO_WaitVSync();

	// put crap in memory to keep the apploader/dol happy
	*MEM_VIRTUALSIZE = 0x01800000;
	*MEM_PHYSICALSIZE = 0x01800000;
	*MEM_BI2 = 0;
	*MEM_BOOTCODE = 0x0D15EA5E;
	*MEM_VERSION = 1;
	*MEM_ARENA1LOW = 0;
	*MEM_BUSSPEED = 0x0E7BE2C0;
	*MEM_CPUSPEED = 0x2B73A840;
	*MEM_TITLEFLAGS = 0x80000000;
	memcpy(MEM_GAMEONLINE, MEM_BASE, 4);
	DCFlushRange(MEM_BASE, 0x3F00);

	// read the apploader info
	if (WDVD_LowRead(&app, sizeof(app_info), APP_INFO_OFFSET))
		return LauncherStatus::ReadError;
	DCFlushRange(&app, sizeof(app_info));
	// read the apploader into memory
	if (WDVD_LowRead(MEM_APPLOADER, app.loader_size + app.data_size, APP_DATA_OFFSET))
		return LauncherStatus::ReadError;
	DCFlushRange(MEM_APPLOADER, app.loader_size + app.data_size);
	app.start(&app_enter, &app_loader, &app_exit);
	app_enter((AppReport)nullprintf);

	while (app_loader(&app_address, &app_section_size, &app_disc_offset)) {
		if (WDVD_LowRead(app_address, app_section_size, (u64)app_disc_offset << 2))
			return LauncherStatus::ReadError;
		ApplyBinaryPatches(app_section_size);
		DCFlushRange(app_address, app_section_size);
		app_address = NULL;
		app_section_size = 0;
		app_disc_offset = 0;
	}

	// copy the IOS version over the expected IOS version
	memcpy(MEM_IOSEXPECTED, MEM_IOSVERSION, 4);

	*(u32*)0xCD006C00 = 0x00000000;	// deinit audio due to libogc fail

	app_address = app_exit();

	return LauncherStatus::OK;
}

LauncherStatus::Enum Launcher_Launch()
{
	if (app_address) {
		WPAD_Shutdown();
		WDVD_Close();
		__ES_Close();
		DCFlushRange(MEM_BASE, 0x17FFFFFF);

		SYS_ResetSystem(SYS_SHUTDOWN, 0, 0);
		__lwp_thread_stopmultitasking((void(*)())app_address);
	}

	// We shouldn't get to here
	VIDEO_SetBlack(false);
	VIDEO_Flush();
	VIDEO_WaitVSync(); VIDEO_WaitVSync();
	return LauncherStatus::ReadError;
}
