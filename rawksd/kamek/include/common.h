#pragma once

#include <stdio.h>
#include <string.h>

#include <gccore.h>

extern "C" void __OSReport(const char *format, ...);
#ifdef DEBUG
#define OSReport(...) __OSReport(__VA_ARGS__)
#else
#define OSReport(str, ...) __OSReport("", ##__VA_ARGS__)
#endif

