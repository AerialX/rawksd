#pragma once

#include <stdarg.h>

#ifdef __cplusplus
   extern "C" {
#endif

int _vsprintf(char *buf, const char *fmt, va_list args);
int _sprintf(char *buf, const char *fmt, ...);
int _vprintf(const char *fmt, va_list args);
int _printf(const char *fmt, ...);

#ifdef __cplusplus
   }
#endif
