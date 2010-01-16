#pragma once 

#define usleep(t)	Timer_Sleep(t)
#define msleep(t)	Timer_Sleep(t*1000)

#ifdef __cplusplus
   extern "C" {
#endif

void Timer_Init(void);
void Timer_Sleep(u32 time);

#ifdef __cplusplus
   }
#endif
