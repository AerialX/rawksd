#pragma once

#include "riivolution.h"

#include <string>
#include <map>
#include <vector>

#define RIIVOLUTION_PATH "/riivolution/"

struct RiiMacro {
	std::string Name;
	std::string ID;
	std::map<std::string, std::string> Params;
};

struct RiiChoice {
	struct Patch {
		std::string ID;
		std::map<std::string, std::string> Params;
	};
	std::string Name;
	std::string ID;
	std::map<std::string, std::string> Params;
	std::vector<Patch> Patches;
	std::string Filesystem;
};

struct RiiOption {
	std::string Name;
	std::string ID;
	u32 Default;
	std::map<std::string, std::string> Params;
	std::vector<RiiChoice> Choices;
};

struct RiiSection {
	std::string Name;
	std::string ID;
	std::vector<RiiOption> Options;
};

struct RiiFilePatch {
	RiiFilePatch()
	{
		Resize = true;
		Create = false;
		Offset = 0;
		FileOffset = 0;
		Length = 0;
	}

	bool Resize;
	bool Create;
	std::string Disc;
	int Offset;
	std::string External;
	u32 FileOffset;
	u32 Length;
};

struct RiiFolderPatch {
	RiiFolderPatch()
	{
		Resize = true;
		Create = false;
		Recursive = true;
	}

	bool Create;
	bool Resize;
	bool Recursive;
	std::string Disc;
	std::string External;
};

struct RiiShiftPatch {
	std::string Source;
	std::string Destination;
};

struct RiiSavegamePatch {
	std::string External;
};

struct RiiMemoryPatch {
	RiiMemoryPatch()
	{
		Offset = 0;
		Value = NULL;
		Original = NULL;
		Length = 0;
		Align = 1;
		Search = false;
		Ocarina = false;
	}
	int Offset;
	u8* Value;
	u8* Original;
	u32 Length;
	bool Search;
	bool Ocarina;
	u32 Align;

	std::string ValueFile;

	u8* GetValue();

	u32 GetLength()
	{
		return Length;
	}
};

struct RiiPatch {
	std::vector<RiiFilePatch> Files;
	std::vector<RiiFolderPatch> Folders;
	std::vector<RiiShiftPatch> Shifts;
	std::vector<RiiMemoryPatch> Memory;
	std::vector<RiiSavegamePatch> Savegames;
};

struct RiiDisc
{
	std::vector<RiiMacro> Macros;
	std::vector<RiiSection> Sections;
	std::map<std::string, RiiPatch> Patches;
};

void ParseXMLs(const char* rootpath, const char* rootfs, std::vector<RiiDisc>* discs);
bool ParseXML(const char* xmldata, int length, RiiDisc* disc, const char* rootpath, const char* rootfs);
RiiDisc CombineDiscs(std::vector<RiiDisc>* discs);
void ParseConfigXMLs(RiiDisc* disc);
bool ParseConfigXML(const char* xmldata, int length, RiiDisc* disc);
void SaveConfigXML(RiiDisc* disc);

std::string PathCombine(std::string path, std::string file);
