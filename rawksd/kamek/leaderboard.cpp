#include "rb2.h"
#include <ogc/machine/processor.h>
#include <ogc/es.h>
#include <network.h>

#define LEADERBOARD_ENABLED() \
	(*(u32*)0x80002FFC == 0x1337BAAD)

const char asciitohex[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

static char* ToHttpString(const char* str, int length = 256)
{
	int newlength=1;
	const char *s = str;
	char *dest, *d;
	int c;

	if (str)
	{
		while (*s && length--)
		{
			c = *s;
			if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
				newlength++;
			else
				newlength+=3;
			s++;
		}
	}

	length = s - str;

	dest = (char*)malloc(newlength);
	if (dest)
	{
		for (d = dest; length--; str++)
		{
			c = *str;
			if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
				*d++ = *str;
			else
			{
				*d++ = '%';
				*d++ = asciitohex[c>>4];
				*d++ = asciitohex[c&0xF];
			}
		}
		*d = '\0';
	}

	return dest;
}

#define HOSTNAME     "rvlution.net"

static bool SubmitLeaderboardRawkSD(Symbol* symbol, int instrument, int difficulty, int score)
{
	if (!LEADERBOARD_ENABLED())
		return false;
	if (!ThePlatformMgr.IsConnected)
		return false;
	if (symbol==NULL || symbol->name==NULL)
		return false;

	char* username;
	char* songname;
	char* fullsongname;
	char* artistname;
	char* message = NULL;
	u32 console;
	int socket;
	int ret=0;
	struct sockaddr_in address;
	hostent* host;
	int size;
	u32 checksum;
	const char* song = symbol->name;
	const char* originalsongname = gSongMgrWii.SongName(symbol);
	const char* originalartistname = gSongMgrWii.SongArtist(symbol);
	PassiveMessagesPanel* panel = GetPMPanel();

	address.sin_family = PF_INET;
	address.sin_len = 8;
	address.sin_port = 80;

	// FIXME: should use the friend code instead of console ID to handle wii->wiiu migration
	const char* fullusername = ThePlatformMgr.GetUsernameFull(); // Full username is in the format "RB2OnlineName;consolefriendcode"
	if (fullusername==NULL)
	{
		OSReport("RawkSD: Failed to get fullusername\n");
		goto onerror;
	}
	const char* colon = strrchr(fullusername, ';');

	if (ES_GetDeviceID(&console) < 0) {
		OSReport("RawkSD: Error getting ID\n");
		goto onerror;
	}

	// any of these may return NULL (out of memory)
	username = ToHttpString(fullusername, colon ? (colon - fullusername) : 256);
	songname = ToHttpString(song);
	if (username==NULL || songname==NULL) {
		OSReport("RawkSD: failed to http-convert username or songname\n");
		free(username);
		free(songname);
		goto onerror;
	}
	// fullsongname and/or artistname are allowed to be NULL
	fullsongname = ToHttpString(originalsongname);
	artistname = ToHttpString(originalartistname);

	checksum = instrument << 3;
	checksum |= difficulty;
	checksum ^= console >> 4;
	checksum ^= ~score << 6;
	for (u32 i = 0; i < strlen(song); i++)
		checksum ^= (u32)song[i] << 24;

	size = 0x100 + strlen(username) + strlen(songname) + (fullsongname ? strlen(fullsongname):0) + (artistname ? strlen(artistname):0) + strlen(HOSTNAME);
	message = (char*)malloc(size);
	if (message) {
		sprintf(message, "GET /rb2/post?inst=%d&diff=%d&score=%d&console=%d&nickname=%s&songid=%s&valid=%d", instrument, difficulty, score, console, username, songname, checksum);
		if (fullsongname) {
			strcat(message, "&name=");
			strcat(message, fullsongname);
		}
		if (artistname) {
			strcat(message, "&artist=");
			strcat(message, artistname);
		}
		strcat(message, " HTTP/1.1\r\nHost: " HOSTNAME "\r\n\r\n\r\n");
	}

	free(username);
	free(songname);
	free(fullsongname);
	free(artistname);

	if (message==NULL) {
		OSReport("RawkSD: out of memory for http buffer\n");
		goto onerror;
	}

	host = net_gethostbyname(HOSTNAME);
	if (!host || host->h_length != sizeof(address.sin_addr) || host->h_addrtype != PF_INET || host->h_addr_list == NULL || host->h_addr_list[0] == NULL) {
		OSReport("RawkSD: Host lookup failed.\n");
		goto onerror;
	}
	memcpy(&address.sin_addr, host->h_addr_list[0], host->h_length);

	socket = net_socket(PF_INET, SOCK_STREAM, 0);
	if (socket < 0) {
		OSReport("RawkSD: Socket creation failed. %d\n", socket);
		goto onerror;
	}
	ret = net_connect(socket, (struct sockaddr*)&address, sizeof(address));
	if (ret < 0) {
		OSReport("RawkSD: Connection failed. %d\n", ret);
		net_close(socket);
		goto onerror;
	}

	size = strlen(message);
	ret = net_send(socket, message, size, 0);
	net_close(socket);
	if (ret != size) {
		OSReport("RawkSD: Send failed. %d\n", ret);
		goto onerror;
	}

	OSReport("RawkSD: Leaderboard submission for \"%s\" succeeded.\n", song);
	if (panel) {
		sprintf(message, "Your new high score%s%s been sent to the RawkSD Leaderboards!", originalsongname ? " for " : "", originalsongname ? originalsongname : "");
		panel->QueueMessage(message);
	}
	free(message);
	return true;

onerror:
	OSReport("RawkSD: Leaderboard submission for \"%s\" failed! :(\n", song);
	if (!message)
		message = (char*)malloc(0x100);
	if (message)
	{
		if (panel) {
			sprintf(message, "An error occurred uploading your high score%s%s to RawkSD.", originalsongname ? " for " : "", originalsongname ? originalsongname : "");
			GetPMPanel()->QueueMessage(message);
		}
		free(message);
	}

	return false;
}

extern "C" void SoloLeaderboardHack(RockCentralGateway* gateway, Symbol* symbol, int instrument, int difficulty, int score1, int score2, int player, Hmx::Object* object)
{
	gateway->SubmitPlayerScore(symbol, instrument, difficulty, score1, score2, player, object);
	SubmitLeaderboardRawkSD(symbol, instrument, difficulty, score1);
}

extern "C" void BandLeaderboardHack(RockCentralGateway* gateway, Symbol* symbol, int score1, int score2, int fans, HxGuid* guid, Hmx::Object* object)
{
	gateway->SubmitBandScore(symbol, score1, score2, fans, guid, object);
	SubmitLeaderboardRawkSD(symbol, 4, 0, score1);
}

