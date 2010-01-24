#include "syscalls.h"
#include "gctypes.h"
#include "gcutil.h"
#include "rtc.h"
#include "starlet.h"

#define TICKS_TO_SEC(a) ((((u64)(a))*2390)>>32)
#define TIMER_REG (*(u32*)HW_TIMER)

typedef struct ios_timer
{
	time_t epoch;
	u64 count;
	u32 prev;
} ios_timer;

static ios_timer starlet_time;
static s32 initted = 0;

void RTC_Init(time_t epoch)
{
	u32 stime = TIMER_REG;
	starlet_time.epoch = epoch-TICKS_TO_SEC(stime);
	starlet_time.count = 0;
	starlet_time.prev = stime;
	initted = 1;
}

void RTC_Update()
{
	u32 stime = TIMER_REG;
	if (!initted)
		return;
	if (stime < starlet_time.prev)
		starlet_time.count++;
	starlet_time.prev = stime;
}

time_t time(time_t *_timer)
{
	time_t x = -1;

	RTC_Update();
	if (initted)
		x = starlet_time.epoch + TICKS_TO_SEC((starlet_time.count<<32)+TIMER_REG);

	if (_timer)
		*_timer = x;
	return x;
}
