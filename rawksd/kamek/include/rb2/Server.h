#pragma once

struct Server
{
	u8 unknown[0x2c];
	u8 isConnected;
	u8 unknown2[];

	bool IsConnected();
};

