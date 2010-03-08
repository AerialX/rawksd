#pragma once

#include <translatios.h>

#include <string>
#include <vector>
#include <map>

int LoadDisc_Init();
int LoadDisc_Begin();
void LoadDisc_ParseXMLs();
const char* LoadDisc_Error(int ret);
void LoadDisc();

extern char GameTitle[0x40];

struct PatchMemory
{
	int* Offset;
	int Value;
	int Original;
	bool Verify;
};

struct PatchFs
{
	std::string Source;
	std::string Destination;
};

struct PatchPatch
{
	std::string ID;
	std::vector<PatchShift> Shifts;
	std::vector<PatchFile> Files;
	std::vector<PatchMemory> Memory;
	std::vector<PatchFs> Fs;
};

struct PatchOption;

struct PatchChoice
{
	PatchOption* Parent;
	std::string Name;
	std::string Description;
	std::vector<std::string> Patches;
};

struct PatchOption
{
	std::string Name;
	std::string Description;
	std::string ID;
	int Enabled;
	std::vector<PatchChoice> Choices;
	std::map<std::string, std::string> Params;
};

struct PatchSection
{
	std::string Name;
	std::vector<int> options;
	PatchOption* Options(int i);
	int Options() { return options.size(); }
};


extern std::map<std::string, int> Defaults;


extern std::vector<PatchSection> Sections;
extern std::vector<PatchOption> Options;

#define ERROR_SUCCESS		 0
#define ERROR_MLOAD			-1
#define ERROR_MOUNT			-2
#define ERROR_NODISC		-3
#define ERROR_DISCREAD		-4

void InitializeNetwork();
void ShutdownNetwork();
void DebugPrint(const char* data);
void DebugPrintf(const char *fmt, ...);
