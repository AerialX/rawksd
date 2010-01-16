#pragma once

#include <time.h>

#ifdef __cplusplus
   extern "C" {
#endif /* __cplusplus */

/* Prototypes */
void RTC_Init(time_t epoch);
void RTC_Update();
time_t time(time_t *_timer);

#ifdef __cplusplus
   }
#endif /* __cplusplus */
