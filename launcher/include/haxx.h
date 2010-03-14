#pragma once

#include <vector>

int Haxx_Init();
void Haxx_Mount(std::vector<int>* mounted);

#define HAXX_IOS 0x0000000100000025ULL
#define HAXX_IOS_VERSION 3869
