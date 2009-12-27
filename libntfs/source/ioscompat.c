#include "config.h"

#include "ioscompat.h"

void* compat_memmove(void *s1, const void *s2, size_t n)
{
	void* temp = Alloc(n);
	memcpy(temp, s2, n);
	memcpy(s1, temp, n);
	Dealloc(temp);

	return s1;
}

char * compat_strdup(const char *s1)
{
	char* ret = Alloc(strlen(s1) + 1);
	strcpy(ret, s1);
	return ret;
}
