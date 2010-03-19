#include "riivolution_config.h"
#include "launcher.h"

using std::string;
using std::vector;
using std::map;

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <files.h>

#include <libxml++/libxml++.h>
#include <libxml++/parsers/textreader.h>
using namespace xmlpp;

#define RIIVOLUTION_CONFIG_PATH (RIIVOLUTION_PATH "config/")

#define ELEMENT_START(str) if (reader.get_node_type() == TextReader::Element && !reader.get_name().compare(str))
#define ELEMENT_END(str) if (reader.get_node_type() == TextReader::EndElement && !reader.get_name().compare(str))
#define ELEMENT_ATTRIBUTE(str, condition) attribute = reader.get_attribute(str); if (!attribute.empty() && (condition))
#define ELEMENT_LOOP if (!reader.is_empty_element()) while (reader.read())
#define ELEMENT_VALUE() reader.get_value()
#define ELEMENT_BOOL() !strcasecmp(attribute.c_str(), "yes") || !strcasecmp(attribute.c_str(), "true")
static s64 ELEMENT_INT(string str)
{
	if (str.compare(0, 2, "0x"))
		return strtoll(str.c_str(), NULL, 10);
	else
		return strtoll(str.c_str() + 2, NULL, 16);
}

static void HexToBytes(void* mem, const char* source)
{
	u8* dest = (u8*)mem;
	bool mod = true;
	while (*source) {
    	u8 chr = tolower(*source);
    	chr = chr - ((chr > '9') ? ('a' - 10) : '0');
    	if (mod)
    		*dest = chr << 4;
    	else {
    		*dest = *dest | (chr & 0x0F);
    		dest++;
    	}
    	mod = !mod;
    	source++;
    }
}

string PathCombine(string path, string file)
{
	if (path.empty())
		return file;
	if (file.empty())
		return path;

	bool flag = true;
	while (flag) {
		flag = false;
		if (!strncmp(file.c_str(), "../", 3)) {
			string::size_type pos = path.find_last_of('/', path.size() - 1);
			if (pos != string::npos)
				path = path.substr(0, pos);

			file = file.substr(3);

			flag = true;
		} else if (!strncmp(file.c_str(), "./", 2)) {
			file = file.substr(2);

			flag = true;
		}
	}

	if (path[path.size() - 1] == '/' && file[0] == '/')
		return path + file.substr(1);

	if (path[path.size() - 1] == '/' || file[0] == '/')
		return path + file;

	return path + "/" + file;
}

// If 'file' starts with '/', it is treated as an absolute path
static string AbsolutePathCombine(string path, string file, string rootfs)
{
	if (file.empty())
		return path;

	if (file[0] == '/')
		return PathCombine(rootfs, file);

	return PathCombine(path, file);
}

#define ELEMENT_PARAM(params) \
	ELEMENT_START("param") { \
		string key; \
		string value; \
		ELEMENT_ATTRIBUTE("name", true) \
			key = attribute; \
		ELEMENT_ATTRIBUTE("value", true) \
			value = attribute; \
		(params)[key] = value; \
	}

// Round down for "extra content at end of document" by finding the last '>'
#define TRIM_XML() \
	while (length > 1 && xmldata[length - 1] != '>') \
		length--; \
	if (length == 0) \
		return false; // Not an xml document

bool ParseXML(const char* xmldata, int length, vector<RiiDisc>* discs, const char* rootpath, const char* rootfs)
{
	TRIM_XML();

	RiiDisc disc;

	string xmlroot = rootpath;
	TextReader reader((const u8*)xmldata, length);
	string attribute;
	reader.read();

	ELEMENT_START("wiidisc")
		ELEMENT_ATTRIBUTE("version", ELEMENT_INT(attribute) == 1)
			goto versionisvalid;
	return false;

versionisvalid:
	ELEMENT_ATTRIBUTE("root", true)
		xmlroot = AbsolutePathCombine(xmlroot, attribute, rootfs);
	ELEMENT_ATTRIBUTE("shiftfiles", true)
		RVL_SetAlwaysShift(ELEMENT_BOOL());

	ELEMENT_LOOP {
		ELEMENT_END("wiidisc")
			break;

		ELEMENT_START("id") {
			ELEMENT_ATTRIBUTE("game", memcmp(MEM_BASE, attribute.c_str(), attribute.length()))
				return false;
			ELEMENT_ATTRIBUTE("developer", memcmp(MEM_BASE + 4, attribute.c_str(), attribute.length()))
				return false;
			ELEMENT_ATTRIBUTE("disc", (u8)ELEMENT_INT(attribute) != *(MEM_BASE + 6))
				return false;
			ELEMENT_ATTRIBUTE("version", (u8)ELEMENT_INT(attribute) != *(MEM_BASE + 7))
				return false;

			bool foundregion = false;
			bool hasregion = false;
			ELEMENT_LOOP {
				ELEMENT_END("id")
					break;

				ELEMENT_START("region") {
					hasregion = true;
					ELEMENT_ATTRIBUTE("type", attribute[0] == *(MEM_BASE + 3))
						foundregion = true;
				}
			}
			if (hasregion && !foundregion)
				return false;
		}

		ELEMENT_START("network") {
			string ip;
			int port = 1137;
			string protocol = "riifs";
			ELEMENT_ATTRIBUTE("protocol", true)
				protocol = attribute;
			ELEMENT_ATTRIBUTE("address", true)
				ip = attribute;
			ELEMENT_ATTRIBUTE("port", true)
				port = ELEMENT_INT(attribute);

			int mnt = -1;
			if (protocol == "riifs")
				mnt = File_RiiFS_Mount(ip.c_str(), port);
			if (mnt >= 0) {
				char mountpoint[MAXPATHLEN];
				if (File_GetMountPoint(mnt, mountpoint, sizeof(mountpoint)) >= 0) {
					char mountpath[MAXPATHLEN];
					strcpy(mountpath, mountpoint);
					strcat(mountpath, RIIVOLUTION_PATH);

					ParseXMLs(mountpath, mountpoint, discs);
				}
			}
		}

		ELEMENT_START("options") {
			ELEMENT_LOOP {
				ELEMENT_END("options")
					break;

				ELEMENT_START("macro") {
					RiiMacro macro;
					ELEMENT_ATTRIBUTE("name", true)
						macro.Name = attribute;
					ELEMENT_ATTRIBUTE("id", true)
						macro.ID = attribute;

					ELEMENT_LOOP {
						ELEMENT_END("macro")
							break;

						ELEMENT_PARAM(macro.Params);
					}

					disc.Macros.push_back(macro);
				}

				ELEMENT_START("section") {
					RiiSection section;
					ELEMENT_ATTRIBUTE("name", true)
						section.Name = attribute;
					ELEMENT_ATTRIBUTE("id", true)
						section.ID = attribute;
					else
						section.ID = section.Name;

					ELEMENT_LOOP {
						ELEMENT_END("section")
							break;

						ELEMENT_START("option") {
							RiiOption option;
							option.Default = 0;

							ELEMENT_ATTRIBUTE("name", true)
								option.Name = attribute;
							ELEMENT_ATTRIBUTE("id", true)
								option.ID = attribute;
							else
								option.ID = section.ID + option.Name;
							ELEMENT_ATTRIBUTE("default", true)
								option.Default = ELEMENT_INT(attribute);

							ELEMENT_LOOP {
								ELEMENT_END("option")
									break;

								ELEMENT_PARAM(option.Params);

								ELEMENT_START("choice") {
									RiiChoice choice;
									choice.Filesystem = rootfs;
									ELEMENT_ATTRIBUTE("name", true)
										choice.Name = attribute;
									ELEMENT_ATTRIBUTE("id", true)
										choice.ID = rootpath + attribute;
									else
										choice.ID = rootpath + option.ID + choice.Name;

									ELEMENT_LOOP {
										ELEMENT_END("choice")
											break;

										ELEMENT_PARAM(choice.Params);

										ELEMENT_START("patch") {
											RiiChoice::Patch patch;
											ELEMENT_ATTRIBUTE("id", true)
												patch.ID = rootpath + attribute;

											ELEMENT_LOOP {
												ELEMENT_END("patch")
													break;

												ELEMENT_PARAM(patch.Params);
											}
											choice.Patches.push_back(patch);
										}
									}

									option.Choices.push_back(choice);
								} // </choice>
							}

							section.Options.push_back(option);
						} // </option>
					}

					disc.Sections.push_back(section);
				} // </section>
			}
		} // </options>

		ELEMENT_START("patch") {
			RiiPatch patch;
			string id;
			std::string patchroot = xmlroot;

			ELEMENT_ATTRIBUTE("id", true)
				id = rootpath + attribute;
			ELEMENT_ATTRIBUTE("root", true)
				patchroot = AbsolutePathCombine(xmlroot, attribute, rootfs);

			ELEMENT_LOOP {
				ELEMENT_END("patch")
					break;

				ELEMENT_START("file") {
					RiiFilePatch file;
					ELEMENT_ATTRIBUTE("resize", true)
						file.Resize = ELEMENT_BOOL();
					ELEMENT_ATTRIBUTE("create", true)
						file.Create = ELEMENT_BOOL();
					ELEMENT_ATTRIBUTE("disc", true)
						file.Disc = attribute;
					ELEMENT_ATTRIBUTE("offset", true)
						file.Offset = ELEMENT_INT(attribute);
					ELEMENT_ATTRIBUTE("external", true)
						file.External = AbsolutePathCombine(patchroot, attribute, rootfs);
					ELEMENT_ATTRIBUTE("fileoffset", true)
						file.FileOffset = ELEMENT_INT(attribute);
					ELEMENT_ATTRIBUTE("length", true)
						file.Length = ELEMENT_INT(attribute);

					patch.Files.push_back(file);
				}

				ELEMENT_START("folder") {
					RiiFolderPatch folder;
					ELEMENT_ATTRIBUTE("create", true)
						folder.Create = ELEMENT_BOOL();
					ELEMENT_ATTRIBUTE("resize", true)
						folder.Resize = ELEMENT_BOOL();
					ELEMENT_ATTRIBUTE("recursive", true)
						folder.Recursive = ELEMENT_BOOL();
					ELEMENT_ATTRIBUTE("disc", true)
						folder.Disc = attribute;
					ELEMENT_ATTRIBUTE("external", true)
						folder.External = AbsolutePathCombine(patchroot, attribute, rootfs);

					patch.Folders.push_back(folder);
				}

				ELEMENT_START("shift") {
					RiiShiftPatch shift;
					ELEMENT_ATTRIBUTE("source", true)
						shift.Source = attribute;
					ELEMENT_ATTRIBUTE("destination", true)
						shift.Destination = attribute;

					patch.Shifts.push_back(shift);
				}

				ELEMENT_START("savegame") {
					RiiSavegamePatch savegame;
					ELEMENT_ATTRIBUTE("external", true)
						savegame.External = AbsolutePathCombine(patchroot, attribute, rootfs);

					patch.Savegames.push_back(savegame);
				}

				ELEMENT_START("memory") {
					RiiMemoryPatch memory;
					ELEMENT_ATTRIBUTE("offset", true)
						memory.Offset = ELEMENT_INT(attribute);

					ELEMENT_ATTRIBUTE("search", true)
						memory.Search = ELEMENT_BOOL();
					ELEMENT_ATTRIBUTE("ocarina", true)
						memory.Ocarina = ELEMENT_BOOL();
					ELEMENT_ATTRIBUTE("align", true)
						memory.Align = ELEMENT_INT(attribute);
					ELEMENT_ATTRIBUTE("valuefile", true)
						memory.ValueFile = AbsolutePathCombine(patchroot, attribute, rootfs);
					ELEMENT_ATTRIBUTE("value", true) {
						if (!attribute.compare(0, 2, "0x"))
							attribute = attribute.substr(2);
						int length = attribute.size() / 2;
						memory.Value = new u8[length];
						if (memory.Value) {
							HexToBytes(memory.Value, attribute.c_str());
							memory.Length = length;
						}
					}
					ELEMENT_ATTRIBUTE("original", true) {
						if (!attribute.compare(0, 2, "0x"))
							attribute = attribute.substr(2);
						int length = attribute.size() / 2;
						memory.Original = new u8[length];
						if (memory.Original)
							HexToBytes(memory.Original, attribute.c_str());
					}

					patch.Memory.push_back(memory);
				}
			}

			disc.Patches[id] = patch;
		} // </patch>
	}

	discs->push_back(disc);

	return true;
}

#define STD_APPEND(list, toadd) \
		(list).insert((list).end(), (toadd).begin(), (toadd).end())
#define STD_APPEND_MAP(list, toadd) \
		(list).insert((toadd).begin(), (toadd).end())

static RiiSection* CreateSectionByID(RiiDisc* disc, RiiSection* section)
{
	for (vector<RiiSection>::iterator iter = disc->Sections.begin(); iter != disc->Sections.end(); iter++) {
		if (!iter->ID.compare(section->ID))
			return &*iter;
	}

	RiiSection newsection;
	newsection.ID = section->ID;
	newsection.Name = section->Name;
	disc->Sections.push_back(newsection);
	return &*disc->Sections.rbegin();
}

static RiiOption* GetOptionByID(RiiSection* section, string id)
{
	for (vector<RiiOption>::iterator option = section->Options.begin(); option != section->Options.end(); option++) {
		if (id == option->ID)
			return &*option;
	}
	return NULL;
}

static void AddOption(RiiSection* section, RiiOption* option)
{
	RiiOption* found = GetOptionByID(section, option->ID);
	if (found)
		STD_APPEND(found->Choices, option->Choices);
	else
		section->Options.push_back(*option);
}

RiiDisc CombineDiscs(vector<RiiDisc>* discs)
{
	RiiDisc ret;

	for (vector<RiiDisc>::iterator disc = discs->begin(); disc != discs->end(); disc++) {
		for (vector<RiiSection>::iterator section = disc->Sections.begin(); section != disc->Sections.end(); section++) {
			RiiSection* retsection = CreateSectionByID(&ret, &*section);
			for (vector<RiiOption>::iterator option = section->Options.begin(); option != section->Options.end(); option++) {
				bool macroed = false;
				for (vector<RiiMacro>::iterator macro = disc->Macros.begin(); macro != disc->Macros.end(); macro++) {
					if (!option->ID.compare(macro->ID)) {
						macroed = true;
						RiiOption newoption = *option;
						newoption.Name = macro->Name;
						newoption.ID += macro->Name;
						STD_APPEND_MAP(newoption.Params, macro->Params);
						AddOption(retsection, &newoption);
					}
				}
				if (!macroed)
					AddOption(retsection, &*option);
			}
		}

		STD_APPEND_MAP(ret.Patches, disc->Patches);

		// TODO: Expand folder patches into files now?
	}

	return ret;
}

void ParseXMLs(const char* rootpath, const char* rootfs, vector<RiiDisc>* discs)
{
	int dir = File_OpenDir(rootpath);
	if (dir < 0)
		return;

	char filename[MAXPATHLEN];
	char path[MAXPATHLEN];
	Stats st;
	while (!File_NextDir(dir, filename, &st)) {
		if (!(st.Mode & S_IFDIR) && strcasestr(filename, ".xml")) {
			strcpy(path, rootpath);
			strcat(path, filename);

			char* xmldata = (char*)SYS_GetArena2Lo(); // Borrowing MEM2, lulz
			int fd = File_Open(path, O_RDONLY);
			if (fd < 0)
				continue;
			int read = File_Read(fd, xmldata, st.Size);
			File_Close(fd);
			ParseXML(xmldata, read, discs, rootpath, rootfs);
		}
	}
	File_CloseDir(dir);
}

struct RiiConfig { string ID; int Default; };
bool ParseConfigXML(const char* xmldata, int length, RiiDisc* disc)
{
	TRIM_XML();

	vector<RiiConfig> config;
	TextReader reader((const u8*)xmldata, length);
	string attribute;

	reader.read();
	ELEMENT_START("riivolution")
		ELEMENT_ATTRIBUTE("version", ELEMENT_INT(attribute) == 2)
			goto versionisvalid;
	return false;

versionisvalid:
	ELEMENT_LOOP {
		ELEMENT_END("riivolution")
			break;

		ELEMENT_START("option") {
			RiiConfig conf;

			ELEMENT_ATTRIBUTE("id", true)
				conf.ID = attribute;
			ELEMENT_ATTRIBUTE("default", true)
				conf.Default = ELEMENT_INT(attribute);

			config.push_back(conf);
		}
	}

	for (vector<RiiSection>::iterator section = disc->Sections.begin(); section < disc->Sections.end(); section++) {
		for (vector<RiiOption>::iterator option = section->Options.begin(); option != section->Options.end(); option++) {
			for (vector<RiiConfig>::iterator choice = config.begin(); choice != config.end(); choice++) {
				if (option->ID == choice->ID) {
					if ((u32)choice->Default <= option->Choices.size())
						option->Default = choice->Default;
					break;
				}
			}
		}
	}

	return true;
}

void ParseConfigXMLs(RiiDisc* disc)
{
	char filename[MAXPATHLEN];
	strcpy(filename, RIIVOLUTION_CONFIG_PATH);
	strncat(filename, (const char*)MEM_BASE, 4);
	strcat(filename, ".xml");

	Stats st;
	char* xmldata = (char*)SYS_GetArena2Lo(); // Borrowing MEM2, lulz
	if (File_Stat(filename, &st) != 0)
		return;
	int fd = File_Open(filename, O_RDONLY);
	if (fd < 0)
		return;
	int read = File_Read(fd, xmldata, st.Size);
	File_Close(fd);
	ParseConfigXML(xmldata, read, disc);
}

void SaveConfigXML(RiiDisc* disc)
{
	char filename[MAXPATHLEN];

	strcpy(filename, RIIVOLUTION_PATH);
	filename[strlen(filename)-1]=0;
	File_CreateDir(filename);

	strcpy(filename, RIIVOLUTION_CONFIG_PATH);
	filename[strlen(filename)-1]=0;
	File_CreateDir(filename);

	strcpy(filename, RIIVOLUTION_CONFIG_PATH);
	strncat(filename, (const char*)MEM_BASE, 4);
	strcat(filename, ".xml");
	int fd = File_Open(filename, O_CREAT | O_WRONLY | O_TRUNC);
	if (fd < 0)
		return;

	Element* root;

	Document document;

	root = document.create_root_node("riivolution");
	root->set_attribute("version", "2");
	char choice[0x10];

	for (vector<RiiSection>::iterator section = disc->Sections.begin(); section != disc->Sections.end(); section++) {
		for (vector<RiiOption>::iterator option = section->Options.begin(); option != section->Options.end(); option++) {
			Element* node = root->add_child("option");
			node->set_attribute("id", option->ID); // TODO: Save section as well
			sprintf(choice, "%d", option->Default);
			node->set_attribute("default", choice);
		}
	}

	string xml = document.write_to_string_formatted();
	// <? crap
	if (xml.size() > 0x17)
		File_Write(fd, xml.c_str() + 0x16, xml.size() - 0x17);

	File_Close(fd);
}

u8* RiiMemoryPatch::GetValue()
{
	Stats st;
	if (!Value && ValueFile.size() && !File_Stat(ValueFile.c_str(), &st)) {
		int fd = File_Open(ValueFile.c_str(), O_RDONLY);
		if (fd >= 0) {
			Value = new u8[st.Size];
			if (Value) {
				File_Read(fd, Value, st.Size);
				Length = st.Size;
			}
			File_Close(fd);
		}
	}

	return Value;
}
