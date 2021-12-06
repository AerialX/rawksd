#pragma once

#include <gctypes.h>
#include <stddef.h>

static constexpr size_t MEM1_SIZE = 0x01800000;
static constexpr size_t MEM1_APPLOADER_OFFSET = 0x01200000;
static constexpr size_t MEM1_EXE_OFFSET = 0x3F00;
extern "C" u32 MEM1_BASE[MEM1_EXE_OFFSET / sizeof(u32)];
extern "C" u32 MEM_APPLOADER[(MEM1_SIZE - MEM1_APPLOADER_OFFSET) / sizeof(u32)];

#define DEFINE_ARRAY(ty, len, sym, target) static constexpr ty (&sym)[len] = reinterpret_cast<ty(&)[len]>(target);
#define DEFINE_MEM(ty, sym, off) static constexpr ty& sym = reinterpret_cast<ty&>(MEM1_BASE[off / sizeof(u32)])
#define MEM1_BYTE(ty, off) (reinterpret_cast<ty&>(MEM1_BASE_BYTES[off]))

DEFINE_ARRAY(u8, MEM1_EXE_OFFSET, MEM1_BASE_BYTES, MEM1_BASE);
DEFINE_MEM(u32, MEM_GAMECODE, 0);
DEFINE_ARRAY(char, sizeof(u32), MEM_GAMECODE_CHARS, MEM1_BASE[0]);
#define MEM_GAMEREGION MEM1_BYTE(char, 3)
DEFINE_ARRAY(char, sizeof(u16), MEM_MAKERCODE_CHARS, MEM1_BASE[1]);
#define MEM_DISCNUMBER MEM1_BYTE(u8, 6)
#define MEM_DISCVERSION MEM1_BYTE(u8, 7)
DEFINE_MEM(u32, MEM_BOOTCODE, 0x20);
DEFINE_MEM(u32, MEM_VERSION, 0x24);
DEFINE_MEM(size_t, MEM_PHYSICALSIZE, 0x28);
DEFINE_MEM(size_t, MEM_ARENA1LOW, 0x30);
DEFINE_MEM(size_t, MEM_ARENA1HIGH, 0x34);
DEFINE_MEM(size_t, MEM_FSTADDRESS, 0x38);
DEFINE_MEM(size_t, MEM_FSTSIZE, 0x3C);
DEFINE_MEM(u32, MEM_VIDEOMODE, 0xCC);
DEFINE_MEM(size_t, MEM_VIRTUALSIZE, 0xF0);
DEFINE_MEM(u32, MEM_BI2, 0xF4);
DEFINE_MEM(u32, MEM_BUSSPEED, 0xF8);
DEFINE_MEM(u32, MEM_CPUSPEED, 0xFC);
DEFINE_MEM(u32, MEM_IOSVERSION, 0x3140);
DEFINE_MEM(u32, MEM_GAMEONLINE, 0x3180);
DEFINE_MEM(u32, MEM_TITLEFLAGS, 0x3184);
DEFINE_MEM(u32, MEM_IOSEXPECTED, 0x3188);

DEFINE_MEM(size_t, MEM_FSTADDRESS2, 0x3110);

#undef DEFINE_ARRAY
#undef DEFINE_MEM

namespace LauncherStatus { enum Enum {
	OK = 0,
	NoDisc = -0x100,
	OutOfMemory = -0x400,
	IosError = -0x200,
	ReadError = -0x201
}; }

extern bool disk_subsequent_reset;

LauncherStatus::Enum Launcher_Init();
LauncherStatus::Enum Launcher_ReadDisc();
LauncherStatus::Enum Launcher_RunApploader();
LauncherStatus::Enum Launcher_Launch();
LauncherStatus::Enum Launcher_RVL();
LauncherStatus::Enum Launcher_CommitRVL(bool dip);
LauncherStatus::Enum Launcher_AddPlaytimeEntry();
LauncherStatus::Enum Launcher_SetVideoMode();
LauncherStatus::Enum Launcher_ScrubPlaytimeEntry();
bool Launcher_DiscInserted();
const char* Launcher_GetGameName();
const s16* Launcher_GetGameNameWide();
const u32* Launcher_GetFstData();
