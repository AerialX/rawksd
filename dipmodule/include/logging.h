#pragma once

//#define LOGGING

#ifdef LOGGING
void LogInit();
void LogPrintf(const char *fmt, ...);
void LogDeinit();
void LogMessage(ipcmessage* message);
void LogTimestamp();
#else
#define LogInit(...)
#define LogPrintf(...)
#define LogDeinit(...)
#define LogMessage(...)
#define LogTimestamp(...)
#endif
