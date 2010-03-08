#include "riivolution_config.h"
#include "launcher.h"

using std::string;

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

bool ParseXML(const char* xmldata, int length, RiiDisc* disc, const char* rootpath, const char* rootfs)
{
	TRIM_XML();

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

					disc->Macros.Add(macro);
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
											choice.Patches.Add(patch);
										}
									}

									option.Choices.Add(choice);
								} // </choice>
							}

							section.Options.Add(option);
						} // </option>
					}

					disc->Sections.Add(section);
				} // </section>
			}
		} // </options>

		ELEMENT_START("patch") {
			RiiPatch patch;
			string id;

			ELEMENT_ATTRIBUTE("id", true)
				id = rootpath + attribute;
			ELEMENT_ATTRIBUTE("root", true)
				xmlroot = AbsolutePathCombine(xmlroot, attribute, rootfs);

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
						file.External = AbsolutePathCombine(xmlroot, attribute, rootfs);
					ELEMENT_ATTRIBUTE("fileoffset", true)
						file.FileOffset = ELEMENT_INT(attribute);
					ELEMENT_ATTRIBUTE("length", true)
						file.Length = ELEMENT_INT(attribute);

					patch.Files.Add(file);
				}

				ELEMENT_START("folder") {
					RiiFolderPatch folder;
					ELEMENT_ATTRIBUTE("create", true)
						folder.Create = ELEMENT_BOOL();
					ELEMENT_ATTRIBUTE("create", true)
						folder.Create = ELEMENT_BOOL();
					ELEMENT_ATTRIBUTE("recursive", true)
						folder.Recursive = ELEMENT_BOOL();
					ELEMENT_ATTRIBUTE("disc", true)
						folder.Disc = attribute;
					ELEMENT_ATTRIBUTE("external", true)
						folder.External = AbsolutePathCombine(xmlroot, attribute, rootfs);

					patch.Folders.Add(folder);
				}

				ELEMENT_START("shift") {
					RiiShiftPatch shift;
					ELEMENT_ATTRIBUTE("source", true)
						shift.Source = attribute;
					ELEMENT_ATTRIBUTE("destination", true)
						shift.Destination = attribute;

					patch.Shifts.Add(shift);
				}

				ELEMENT_START("savegame") {
					RiiSavegamePatch savegame;
					ELEMENT_ATTRIBUTE("external", true)
						savegame.External = AbsolutePathCombine(xmlroot, attribute, rootfs);

					patch.Savegames.Add(savegame);
				}

				ELEMENT_START("memory") {
					RiiMemoryPatch memory;
					ELEMENT_ATTRIBUTE("offset", true)
						memory.Offset = ELEMENT_INT(attribute);

					patch.Memory.Add(memory);
				}
			}

			disc->Patches[id] = patch;
		} // </patch>
	}

	return true;
}

bool ParseConfigXML(const char* xmldata, int length, RiiDisc* disc)
{
	TRIM_XML();

	TextReader reader((const u8*)xmldata, length);
	string attribute;
	reader.read();
	ELEMENT_START("riivolution")
		ELEMENT_ATTRIBUTE("version", ELEMENT_INT(attribute) != 1)
			goto versionisvalid;
	return false;

versionisvalid:
	RiiConfig config;

	ELEMENT_LOOP {
		ELEMENT_END("riivolution")
			break;

		ELEMENT_START("option") {
			string id;
			int choice = 0;

			ELEMENT_ATTRIBUTE("id", true)
				id = attribute;
			ELEMENT_ATTRIBUTE("default", true)
				choice = ELEMENT_INT(attribute);

			config.Defaults[id] = choice;
		}
	}

	for (RiiSection* section = disc->Sections.Data(); section < disc->Sections.End(); section++) {
		for (RiiOption* option = section->Options.Data(); option != section->Options.End(); option++) {
			for (Pair<string, int>* choice = config.Defaults.Data(); choice != config.Defaults.End(); choice++) {
				if (!option->ID.compare(choice->Key())) {
					option->Default = choice->Value();
					break;
				}
			}
		}
	}

	return true;
}

#define STD_APPEND(list, toadd) \
		(list).Add((toadd).Data(), (toadd).Size())

static RiiSection* CreateSectionByID(RiiDisc* disc, RiiSection* section)
{
	File_Open("createsectionbyname", 0);
	for (RiiSection* iter = disc->Sections.Data(); iter != disc->Sections.End(); iter++) {
		if (!iter->ID.compare(section->ID)) {
			File_Open("foundone", 0);
			return iter;
		}
	}

	RiiSection newsection;
	newsection.ID = section->ID;
	newsection.Name = section->Name;
	return disc->Sections.Add(newsection);
}

static RiiOption* GetOptionByID(RiiSection* section, string id)
{
	File_Open("getoptionbyid", 0);
	for (RiiOption* option = section->Options.Data(); option != section->Options.End(); option++) {
		if (!option->ID.compare(id)) {
			File_Open("foundone", 0);
			return option;
		}
	}
	return NULL;
}

static void AddOption(RiiSection* section, RiiOption* option)
{
	File_Open("addoption", 0);
	RiiOption* found = GetOptionByID(section, option->ID);
	if (found)
		STD_APPEND(found->Choices, option->Choices);
	else
		section->Options.Add(*option);
}

RiiDisc CombineDiscs(List<RiiDisc>* discs)
{
	RiiDisc ret;

	for (RiiDisc* disc = discs->Data(); disc != discs->End(); disc++) {
		File_Open("disc", 0);
		for (RiiSection* section = disc->Sections.Data(); section != disc->Sections.End(); section++) {
			File_Open("section", 0);
			RiiSection* retsection = CreateSectionByID(&ret, section);
			for (RiiOption* option = section->Options.Data(); option != section->Options.End(); option++) {
				File_Open("option", 0);
				bool macroed = false;
				for (RiiMacro* macro = disc->Macros.Data(); macro != disc->Macros.End(); macro++) {
					File_Open("macro", 0);
					if (!option->ID.compare(macro->ID)) {
						macroed = true;
						RiiOption newoption = *option;
						newoption.Name = macro->Name;
						newoption.ID += macro->Name;
						STD_APPEND(newoption.Params, macro->Params);
						AddOption(retsection, &newoption);
					}
				}
				if (!macroed)
					AddOption(retsection, option);
			}
		}

		ret.Patches.Add(disc->Patches.Data(), disc->Patches.Size());

		// TODO: Expand folder patches into files now?
	}

	return ret;
}

void ParseXMLs(const char* rootpath, const char* rootfs, List<RiiDisc>* discs)
{
	File_CreateDir(rootpath);
	int dir = File_OpenDir(rootpath);
	if (dir < 0)
		return;

	static char filename[MAXPATHLEN];
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
			File_Read(fd, xmldata, st.Size);
			File_Close(fd);
			RiiDisc current;
			if (ParseXML(xmldata, st.Size, &current, rootpath, rootfs))
				discs->Add(current);
		}
	}
	File_CloseDir(dir);
}

static RiiConfig Config;

void ParseConfigXMLs(RiiDisc* disc)
{
	char filename[MAXPATHLEN];
	strcpy(filename, RIIVOLUTION_CONFIG_PATH);
	strncat(filename, (const char*)MEM_BASE, 4);
	strcat(filename, ".xml");

	Stats st;
	char* xmldata = (char*)SYS_GetArena2Lo(); // Borrowing MEM2, lulz
	if (File_Stat(filename, &st) < 0)
		return;
	int fd = File_Open(filename, O_RDONLY);
	if (fd < 0)
		return;
	File_Read(fd, xmldata, st.Size);
	File_Close(fd);
	ParseConfigXML(xmldata, st.Size, disc);
}

void SaveConfigXML(RiiDisc* disc)
{
	Element* root;

	Document document;

	root = document.create_root_node("riivolution");
	root->set_attribute("version", "1");
	char choice[0x10];

	for (RiiSection* section = disc->Sections.Data(); section != disc->Sections.End(); section++) {
		for (RiiOption* option = section->Options.Data(); option != section->Options.End(); option++) {
			Element* node = root->add_child("option");
			node->set_attribute("id", option->ID);
			sprintf(choice, "%d", option->Default);
			node->set_attribute("default", choice);
		}
	}

	string xml = document.write_to_string_formatted();

	char filename[MAXPATHLEN];
	strcpy(filename, RIIVOLUTION_CONFIG_PATH);
	strncat(filename, (const char*)MEM_BASE, 4);
	strcat(filename, ".xml");

	File_CreateDir(RIIVOLUTION_CONFIG_PATH);
	File_CreateFile(filename);
	int fd = File_Open(filename, O_WRONLY | O_TRUNC);
	if (fd < 0)
		return;

	File_Write(fd, xml.c_str(), xml.size());

	File_Close(fd);
}
