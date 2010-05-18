#pragma once

#include "rb2.h"

namespace Hmx {
	struct Object
	{

	};
}

struct Symbol
{
	char* Name;
};

struct HxGuid
{
	int data[0x04];

	HxGuid();

	bool IsNull();
	int* Data();
	int Chunk32();

	void SetChunk32(int index, int value);
	void SetData(int, int, int, int);
	void Clear();
	void Generate();
};

