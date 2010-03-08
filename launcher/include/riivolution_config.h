#pragma once

#include "riivolution.h"
#include "mystl.h"

#include <string>

#define RIIVOLUTION_PATH "/riivolution/"

struct RiiMacro {
	std::string Name;
	std::string ID;
	Map<std::string, std::string> Params;
};

struct RiiChoice {
	struct Patch {
		std::string ID;
		Map<std::string, std::string> Params;
	};
	std::string Name;
	std::string ID;
	Map<std::string, std::string> Params;
	List<Patch> Patches;
	std::string Filesystem;
};

struct RiiOption {
	std::string Name;
	std::string ID;
	u32 Default;
	Map<std::string, std::string> Params;
	List<RiiChoice> Choices;
};

struct RiiSection {
	std::string Name;
	std::string ID;
	List<RiiOption> Options;
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
	int Offset;
	u8* Value;
	u8* Original;
	u32 Length;
};

struct RiiPatch {
	List<RiiFilePatch> Files;
	List<RiiFolderPatch> Folders;
	List<RiiShiftPatch> Shifts;
	List<RiiMemoryPatch> Memory;
	List<RiiSavegamePatch> Savegames;
};

struct RiiDisc
{
	List<RiiMacro> Macros;
	List<RiiSection> Sections;
	Map<std::string, RiiPatch> Patches;
};

struct RiiConfig
{
	Map<std::string, int> Defaults;
};

void ParseXMLs(const char* rootpath, const char* rootfs, List<RiiDisc>* discs);
bool ParseXML(const char* xmldata, int length, RiiDisc* disc, const char* rootpath, const char* rootfs);
RiiDisc CombineDiscs(List<RiiDisc>* discs);
void ParseConfigXMLs(RiiDisc* disc);
bool ParseConfigXML(const char* xmldata, int length, RiiDisc* disc);
void SaveConfigXML(RiiDisc* disc);

std::string PathCombine(std::string path, std::string file);
