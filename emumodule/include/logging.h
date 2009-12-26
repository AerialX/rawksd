#pragma once

void LogInit();
void LogPrintf(const char *fmt, ...);
void LogDeinit();
void LogMessage(ipcmessage* message);
void LogTimestamp();
