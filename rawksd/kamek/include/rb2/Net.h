#pragma once

struct Server;

struct Net
{
	u8 Unknown[0x34];
	Server* server;
};

extern Net TheNet;

