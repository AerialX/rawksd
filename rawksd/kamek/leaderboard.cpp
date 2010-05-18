#include "rb2.h"
#include <ogc/machine/processor.h>
#include <ogc/es.h>
#include <network.h>

static char* ToHttpString(char* dest, const char* str, int length)
{
	*dest = '\0';
	for (int i = 0; i < length; i++) {
		char chr = str[i];
		char temp[0x08];
		if (strchr(" <>#%{}|\\^~[]`;/?:@=&$", chr)) {
			sprintf(temp, "%d", (int)chr);
		} else {
			temp[0] = chr;
			temp[1] = '\0';
		}
		strcat(dest, temp);	
	}

	return dest;
}

static int SubmitLeaderboardRawkSD(const char* song, int instrument, int difficulty, int score)
{
	const char* hostname = "rvlution.net";
	char username[0x100];
	char songname[0x100];
	u32 console = 0;
	int socket = 0;
	int ret = 0;
	struct sockaddr_in address;
	hostent* host;
	char message[0x400];
	int size = 0;
	int checksum = 0;

	memset(&address, 0, sizeof(address));
	address.sin_family = PF_INET;
	address.sin_len = 8;
	
	const char* fullusername = ThePlatformMgr.GetUsernameFull();
	char* colon = strrchr(fullusername, ';');
	ToHttpString(username, fullusername, colon ? (colon - fullusername) : strlen(fullusername));
	
	ToHttpString(songname, song, strlen(song));

	if (ES_GetDeviceID(&console) < 0) {
		OSReport("RawkSD: Error getting ID\n");
		goto onerror;
	}
	
	host = net_gethostbyname(hostname);
	if (!host || host->h_length != sizeof(address.sin_addr) || host->h_addrtype != PF_INET || host->h_addr_list == NULL || host->h_addr_list[0] == NULL) {
		OSReport("RawkSD: Host lookup failed.\n");
		goto onerror;
	}
	memcpy(&address.sin_addr, host->h_addr_list[0], host->h_length);
	OSReport("RawkSD: %s %s\n", hostname, inet_ntoa(address.sin_addr));
	address.sin_port = htons(80);

	socket = net_socket(PF_INET, SOCK_STREAM, 0);
	if (socket <= 0) {
		OSReport("RawkSD: Socket creation failed. %d\n", socket);
		goto onerror;
	}
	ret = net_connect(socket, (struct sockaddr*)&address, sizeof(address));
	if (ret < 0) {
		OSReport("RawkSD: Connection failed. %d\n", ret);
		goto onerror;
	}

	sprintf(message, "GET /rb2/post?inst=%d&diff=%d&score=%d&console=%d&nickname=%s&songid=%s&valid=%d HTTP/1.1\r\nHost: %s\r\nUser-Agent: RawkSD RB2 Patch\r\n\r\n\r\n", instrument, difficulty, score, console, username, song, checksum, hostname);
	size = strlen(message);
	ret = net_send(socket, message, size, 0);
	if (ret != size) {
		OSReport("RawkSD: Send failed. %d\n", ret);
		goto onerror;
	}

	net_close(socket);

	OSReport("RawkSD: Leaderboard submission for \"%s\" succeeded.\n", song);
	GetPMPanel()->QueueMessage("Score has been uploaded to the RawkSD Leaderboards");
	return 1;

onerror:
	OSReport("RawkSD: Leaderboard submission for \"%s\" failed! :(\n", song);
	GetPMPanel()->QueueMessage("An error occurred uploading the score to RawkSD.");
	return -1;
}

extern "C" int SoloLeaderboardHack(RockCentralGateway* gateway, Symbol* symbol, int instrument, int difficulty, int score1, int score2, int player, Hmx::Object* object)
{
	SubmitLeaderboardRawkSD(symbol->Name, instrument, difficulty, score1);
/*	if (!strncmp(symbol->Name, "rwk", 3)) {
		return SubmitLeaderboardRawkSD(symbol->Name + 3, instrument, difficulty, score1);
	} else */
		return gateway->SubmitPlayerScore(symbol, instrument, difficulty, score1, score2, player, object);
}

extern "C" int BandLeaderboardHack(RockCentralGateway* gateway, Symbol* symbol, int score1, int score2, int unknown, HxGuid* guid, Hmx::Object* object)
{
	SubmitLeaderboardRawkSD(symbol->Name, 4, unknown, score1);
/*	if (!strncmp(symbol->Name, "rwk", 3))
		return SubmitLeaderboardRawkSD(symbol->Name + 3, 4, unknown, score1);
	else */
		return gateway->SubmitBandScore(symbol, score1, score2, unknown, guid, object);
}

