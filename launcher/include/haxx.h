#pragma once

#include <vector>

#include <gctypes.h>

int Haxx_Init();
void Haxx_Mount(std::vector<int>* mounted);
bool forge_sig(u8 *data, u32 length);

#define HAXX_IOS 0x0000000100000025ULL
#define HAXX_IOS_REVISION 3869
