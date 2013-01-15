#include "http.h"
#include "rawksd_menu.h"
#include "ssl.h"
#include <network.h>
#include <string.h>

#define MOTD_URL "http://rvlution.net/rb2/static/motd.txt"

static char MotdMessage[0x1000];
static bool Motd = false;

const char* GetMotd()
{
	u8* content;
	u32 size;
	u32 status;

	if (Motd)
		return MotdMessage;

	if (net_initted <= 0 || !http_request(MOTD_URL, 0x0FFFFFFF))
		return "";

	http_get_result(&status, &content, &size);

	// remove leading newlines
	for (status=0; size && content[status]=='\n'; status++, size--);

	if (size >= sizeof(MotdMessage)) size = sizeof(MotdMessage)-1;

	memcpy(MotdMessage, content+status, size);
	MotdMessage[size] = '\0';
	free(content);

	Motd = true;

	return MotdMessage;
}

