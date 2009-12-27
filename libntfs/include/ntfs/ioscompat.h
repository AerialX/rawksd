#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <mntent.h>


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <mem.h>

#define malloc Alloc
#define memalign Memalign
#define free Dealloc

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

int _sprintf(char *buf, const char *fmt, ...);
int _vsprintf(char *buf, const char *fmt, va_list args);

#define memmove compat_memmove
#define strdup compat_strdup

void * compat_memmove(void *s1, const void *s2, size_t n);
char * compat_strdup(const char *s1);

// Disable printfs
#define printf(...)

#define _exit exit
#define vprintf(...) 0
#define vsprintf _vsprintf
#define vfprintf(...) 0
#define fprintf(...) 0
#define sprintf(...) _sprintf

#define snprintf(x, y, ...) _sprintf(x, __VA_ARGS__)
#define vsnprintf(x, a, y, z) _vsprintf(x, y, z)

#define calloc(x, y) Alloc(x*y)
#define realloc(x, y) ReAlloc(x, y, y)

// IOS compatibility stuff
#define LWP_MutexLock(...)
#define LWP_MutexUnlock(...)
#define LWP_MutexInit(...)
#define LWP_MutexDestroy(...)
#define mutex_t void*

#define rand(...) 0
#define time(...) 0

#define fork(...) 0
#define setsid(...) 0
#define open(...) 0
#define creat(...) 0
#define write(...) 0
#define read(...) 0
#define seek(...) 0
#define close(...) 0

#define endmntent(...) 0
#define setmntent(...) 0
#define getmntent(...) 0
#define addmntent(...) 0
#define hasmntopt(...) 0
