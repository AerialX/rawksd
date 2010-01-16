#pragma once

#include <stdint.h>

#include "gctypes.h"

#define FEATURE_MEDIUM_CANREAD      0x00000001
#define FEATURE_MEDIUM_CANWRITE     0x00000002
#define FEATURE_GAMECUBE_SLOTA      0x00000010
#define FEATURE_GAMECUBE_SLOTB      0x00000020
#define FEATURE_WII_SD              0x00000100
#define FEATURE_WII_USB             0x00000200

typedef uint32_t sec_t;

typedef bool (* FN_MEDIUM_STARTUP)(void);
typedef bool (* FN_MEDIUM_ISINSERTED)(void);
typedef bool (* FN_MEDIUM_READSECTORS)(sec_t sector, sec_t numSectors, void* buffer);
typedef bool (* FN_MEDIUM_WRITESECTORS)(sec_t sector, sec_t numSectors, const void* buffer);
typedef bool (* FN_MEDIUM_CLEARSTATUS)(void);
typedef bool (* FN_MEDIUM_SHUTDOWN)(void);

struct DISC_INTERFACE_STRUCT {
	unsigned long			ioType;
	unsigned long			features;
	FN_MEDIUM_STARTUP		startup;
	FN_MEDIUM_ISINSERTED	isInserted;
	FN_MEDIUM_READSECTORS	readSectors;
	FN_MEDIUM_WRITESECTORS	writeSectors;
	FN_MEDIUM_CLEARSTATUS	clearStatus;
	FN_MEDIUM_SHUTDOWN		shutdown;
} ;

typedef struct DISC_INTERFACE_STRUCT DISC_INTERFACE;
