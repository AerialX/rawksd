#pragma once

#include "rb2.h"

struct ContextWrapperPool;

namespace Hmx {
	struct Object
	{
		u8 unknown[0x1C];
		ContextWrapperPool* ContextPool; // 0x1C
	};
}

struct Symbol
{
	Symbol(const char*);

	char* name;

	static Symbol* Alloc()
	{
		return (Symbol*)malloc(0x04);
	}
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

