#include "http.h"

#include <network.h>
#include <malloc.h>
#include <string.h>

#define MOTD_URL "http://rvlution.net/rb2/static/motd.txt"

static char MotdMessage[0x1000];
static bool Motd = false;

const char* GetMotd()
{
	if (Motd)
		return MotdMessage;

	if (net_init() < 0 || !http_request(MOTD_URL, 0x0FFFFFFF))
		return "";

	u8* content;
	u32 size;
	u32 status;
	http_get_result(&status, &content, &size);
	memcpy(MotdMessage, content, size);
	MotdMessage[size] = '\0';
	free(content);

	Motd = true;

	return MotdMessage;
}

