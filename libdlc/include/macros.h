#ifndef __MACROS_H__
#define __MACROS_H__

void LogPrintf(const char *fmt, ...);
#define printf LogPrintf

#define ASSERT(x) do { if(!(x)) { printf("Assert failure: %s in %s:%d\n", #x, __FILE__, __LINE__); return 0; } } while(0)
#define ASSERT_N(x) do { if(!(x)) { printf("Assert failure: %s in %s:%d\n", #x, __FILE__, __LINE__); return NULL; } } while(0)
#define ASSERT_JMP(x,y) do { if(!(x)) { printf("Assert failure: %s in %s:%d\n", #x, __FILE__, __LINE__); goto y; } } while(0)

#endif
