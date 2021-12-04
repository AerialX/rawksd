#include "dip.h"
#include "logging.h"

#include <files.h>

#include "print.h"

#ifdef LOGGING

void LogInit()
{
}

void LogPrintf(const char *fmt, ...)
{
	static char str[1024] __attribute__ ((aligned (32)));

	va_list arg;
	va_start(arg, fmt);
	int len = _vsprintf(str, fmt, arg);
	va_end(arg);
	File_Log(str, len);
}

void LogTimestamp()
{
	int heapfree = 0;
	//heapfree = HeapInfo();
	LogPrintf("%u (%X): ", *(u32*)0x0D800010, heapfree);
}

void LogDeinit()
{
}
#endif

namespace std {
	// aliased from std::__throw_length_error()
	void __logging_abort(const char* message) {
		LogPrintf("__throw_length_error: %s", message);
		os_crash();
	}
}
