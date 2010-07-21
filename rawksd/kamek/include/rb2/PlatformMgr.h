#pragma once

struct PlatformMgr
{
	u8 Unknown[0x42];
	u8 IsConnected;
	const char* GetUsernameFull();
};

extern PlatformMgr ThePlatformMgr;

