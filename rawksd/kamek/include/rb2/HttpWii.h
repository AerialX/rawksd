#pragma once

#include "rb2/Main.h"

struct HttpWii
{
	int CancelAsync(int id);
	int CompleteAsync(int id, int unknown);
	int GetFileAsync(const char* url, void* buffer, int size);

	HttpWii();
	~HttpWii();
	int Init();
	int Terminate();
};

extern HttpWii TheHttpWii;

