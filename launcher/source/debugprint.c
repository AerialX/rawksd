#include <network.h>
#include <stdio.h>
#include <sys/iosupport.h>
#include <ogc/machine/processor.h>
#include "files.h"

#include <string.h>

static int socket = -1;

static void InitializeNetwork(const char *ip_str, const int port)
{
	int init;
	if (socket>=0)
		return;
	while ((init = net_init()) < 0)
		;

	// DEBUG: Connect to me
	socket = net_socket(PF_INET, SOCK_STREAM, 0);
	struct sockaddr_in address;
	memset(&address, 0, sizeof(address));

	address.sin_family = PF_INET;
	address.sin_port = htons(port);
	int ret = inet_aton(ip_str, &address.sin_addr);
	if (ret == 0)
		return;
	if (net_connect(socket, (struct sockaddr*)&address, sizeof(address)) < 0)
	{
		net_close(socket);
		socket = -1;
	}
}

static ssize_t DebugPrint(struct _reent *r, void *fd, const char *ptr, size_t len)
{
	File_Log(ptr, len);
	if (socket >= 0) {
		ssize_t res = net_send(socket, ptr, len, 0);
		while (res > 0 && res != len) {
			ptr += res;
			len -= res;
			res = net_send(socket, ptr, len, 0);
		}
		return len;
	}
	return 0;
}

static const devoptab_t dotab_netout = {
	"netout",
	0,
	NULL,
	NULL,
	DebugPrint, // device write
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	0,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};


void Init_DebugConsole(const char *ip_str, int port)
{
	unsigned int level;

	_CPU_ISR_Disable(level);

	if (ip_str)
		InitializeNetwork(ip_str, port);

	devoptab_list[STD_OUT] = &dotab_netout;
	devoptab_list[STD_ERR] = &dotab_netout;

	_CPU_ISR_Restore(level);

	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	printf("Debug Console Connected\n");
}

void Init_DebugConsole_Shutdown() {
	net_deinit();
	socket = -1;
}
