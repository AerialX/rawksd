#pragma once

#include <gccore.h>

void Installer_Initialize();
void Installer_Deinitialize();
int Install(u64 titleid, int version, bool comexploit);
