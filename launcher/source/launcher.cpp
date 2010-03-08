#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>

#include <ogc/lwp_watchdog.h>
#include <ogc/isfs.h>
#include <files.h>

#include "launcher.h"
#include "video.h"

#include "wdvd.h"
#include <mload.h>

#include <translatios.h>

#include "patchmii_core.h"

#include "ehcmodule_dat.h"
#include "filemodule_dat.h"

#ifdef RAWKHAXX
#include "dipmodule_rawk_dat.h"
#define dipmodule_dat dipmodule_rawk_dat
#define dipmodule_dat_size dipmodule_rawk_dat_size
#endif

#ifdef RIIVOLUTION
#include "dipmodule_rii_dat.h"
#define dipmodule_dat dipmodule_rii_dat
#define dipmodule_dat_size dipmodule_rii_dat_size
#endif

#ifdef RAWKHAXX
#include "emumodule_dat.h"
#endif

#include <launcher.h>
using std::string;
using std::vector;
using std::map;

#include <libxml++/libxml++.h>
#include <libxml++/parsers/textreader.h>
using namespace xmlpp;

#include <sys/stat.h>
#include <fcntl.h>

#include <network.h>
#include <stdarg.h>

#define printf DebugPrintf
extern "C" void __exception_closeall();

/* USAGE
* Make sure the makefile has something like this:
* LDFLAGS		=	$(MACHDEP) -Wl,-Map,$(notdir $@).map,--section-start,.init=0x80900100
* so the app is positioned above the memory where the .dol will be loaded
* (the .init address may need to be raised to 0x80A00000 for some games, depends on .dol size)
* also check the .map file after building to make sure no code is in the apploader area (0x81200000)
* if it is, you need to trim your program (remove globals, unused functions/resources etc.
* You will also need the fst library for the DVD read functions (or implement your own)
*/

#define STACK_ALIGN(type, name, cnt, alignment)	\
u8 _al__##name[((sizeof(type)*(cnt)) + (alignment) + (((sizeof(type)*(cnt))%(alignment)) > 0 ? ((alignment) - ((sizeof(type)*(cnt))%(alignment))) : 0))]; \
type *name = (type*)(((u32)(_al__##name)) + ((alignment) - (((u32)(_al__##name))&((alignment)-1))))

typedef void (*AppReport) (const char*, ...);
typedef void (*AppEnter) (AppReport);
typedef int  (*AppLoad) (void**, s32*, s32*);
typedef void* (*AppExit) ();
typedef void (*AppStart) (AppEnter*, AppLoad*, AppExit*);

typedef struct {
	u8 revision[16];
	AppStart start;
	u32 loader_size;
	u32 data_size;
	u8 unused[4];
} app_info;

#define PART_OFFSET			0x00040000
#define APP_INFO_OFFSET		0x2440
#define APP_DATA_OFFSET		(APP_INFO_OFFSET + 0x20)

#define min(a,b) ((a)>(b) ? (b) : (a))
#define max(a,b) ((a)>(b) ? (a) : (b))

typedef struct
{
	u32 checksum;
	union
	{
		u32 data[0x1f];
		struct {
			s16 name[42];
			u64 ticks_boot;
			u64 ticks_last;
			u32 title_id;
			u16 title_gid;
			//u8 unknown[18];
		} ATTRIBUTE_PACKED;
	};
} playtime_buf_t;

int LoadDisc_WritePlayLog(u32 titleid, u16 groupid, char* title) {
	static playtime_buf_t playtime_buf ATTRIBUTE_ALIGN(32);
	int d = ISFS_Open("/title/00000001/00000002/data/play_rec.dat", ISFS_OPEN_WRITE);
	if (d >= 0) {
		u64 tick_now = gettime();
		memset(&playtime_buf, 0, sizeof(playtime_buf_t));
		s32 i = 0;
		while ((playtime_buf.name[i] = title[i++]))
			;
		playtime_buf.ticks_boot = tick_now;
		playtime_buf.ticks_last = tick_now;
		playtime_buf.title_id = titleid;
		playtime_buf.title_gid = groupid;
		for (i = 0; i < 0x1f; i++)
			playtime_buf.checksum += playtime_buf.data[i];
		
		bool ret = sizeof(playtime_buf_t) == ISFS_Write(d, (u8*)&playtime_buf, sizeof(playtime_buf_t));
		ISFS_Close(d);
		if (ret)
			return 1;
		else
			return -1;
	}
	
	return -1;
}


const char* LoadDisc_Error(int ret)
{
	switch (ret) {
		case ERROR_MLOAD:
			return "Unable to startup; if this problem persists you may need to reinstall.";
		case ERROR_MOUNT:
			return "Please insert an SD card...";
		case ERROR_NODISC:
#ifdef ZERO4
			return "Please insert the Fatal Frame 4 game disc.";
#else
			return "Please insert a game disc.";
#endif
		case ERROR_DISCREAD:
			return "An error occurred while reading the Wii disc.";
		default:
			return "An error has occurred.";
	}
}

extern "C" {
	extern const u32 loading_dat_size;
	extern const u8 loading_dat[];
	extern const u32 rawksdhc_dat_size;
	extern const u8 rawksdhc_dat[];
	extern const u32 rawkusb_dat_size;
	extern const u8 rawkusb_dat[];
	extern const u32 store_dat_size;
	extern const u8 store_dat[];
}

vector<PatchSection> Sections;
vector<PatchPatch> Patches;
vector<PatchOption> Options;
map<string, int> Defaults;
bool ShiftFileReplacements;

PatchSection* GetSectionByName(string name)
{
	for (u32 i = 0; i < Sections.size(); i++)
		if (!Sections[i].Name.compare(name))
			return &Sections[i];
	
	return NULL;
}

PatchOption* GetOptionByID(string id)
{
	for (u32 i = 0; i < Options.size(); i++)
		if (!Options[i].ID.compare(id))
			return &Options[i];
	
	return NULL;
}

PatchPatch* GetPatchByID(string id)
{
	for (u32 i = 0; i < Patches.size(); i++) {
		if (!Patches[i].ID.compare(id))
			return &Patches[i];
	}
	
	return NULL;
}

PatchOption* PatchSection::Options(int i)
{
	return &::Options[options[i]];
}

#define ELEMENT_START(str) if (reader.get_node_type() == TextReader::Element && !reader.get_name().compare(str))
#define ELEMENT_END(str) if (reader.get_node_type() == TextReader::EndElement && !reader.get_name().compare(str))
#define ELEMENT_ATTRIBUTE(str, condition) attribute = reader.get_attribute(str); if (!attribute.empty() && (condition))
#define ELEMENT_LOOP while (reader.read())
#define ELEMENT_VALUE() reader.get_value()
#define ELEMENT_BOOL() !strcasecmp(attribute.c_str(), "yes") || !strcasecmp(attribute.c_str(), "true")

s64 ELEMENT_INT(string str)
{
	if (str.find("0x") == string::npos)
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
string AbsolutePathCombine(string path, string file)
{
	if (file.empty())
		return path;
	
	if (file[0] == '/')
		return file;
	
	return PathCombine(path, file);
}

void RecurseFolderPatch(PatchPatch* patch, string discfolder, string externalfolder, bool create)
{
	char fdirname[MAXPATHLEN];
	int fdir = File_OpenDir(externalfolder.c_str());
	Stats stats;
	while (!File_NextDir(fdir, fdirname, &stats)) {
		if (fdirname[0] == '.')
			continue;
		if (stats.Mode & S_IFDIR) {
			RecurseFolderPatch(patch, PathCombine(discfolder, fdirname), PathCombine(externalfolder, fdirname), create);
		} else {
			PatchFile file;
			file.Create = create;
			file.Stated = true;
			file.Resize = true;
			file.Offset = 0;
			file.FileOffset = 0;
			file.Length = stats.Size;
			file.DiscFile = PathCombine(discfolder, fdirname);
			file.External = PathCombine(externalfolder, fdirname);
			file.ExternalID = stats.Identifier;
			patch->Files.push_back(file);
		}
	}
	File_CloseDir(fdir);
}

void LoadDisc_ParseXMLs()
{
	File_CreateDir("/riivolution");
	int dir = File_OpenDir("/riivolution/");
	static char filename[MAXPATHLEN] ATTRIBUTE_ALIGN(32);
	static char path[MAXPATHLEN] ATTRIBUTE_ALIGN(32);
	static Stats st ATTRIBUTE_ALIGN(32);
	char xmlid = 'a';
	char optid = 'a';
	while (!File_NextDir(dir, filename, &st)) {
		if (!(st.Mode & S_IFDIR) && strcasestr(filename, ".xml")) {
			xmlid++;
			
			strcpy(path, "/riivolution/");
			strcat(path, filename);
			
			u8* xmldata = (u8*)SYS_GetArena2Lo(); // Borrowing MEM2, lulz
			int fd = File_Open(path, O_RDONLY);
			if (fd < 0)
				continue;
			File_Read(fd, xmldata, st.Size);
			File_Close(fd);
			
			int size = st.Size;
			// Round down for "extra content at end of document"
			while (size > 1 && xmldata[size - 1] != '>') // Find the last '>'
				size--;
			if (size == 0) {
				continue; // Not an xml document
			}
			
			string xmlroot = "/riivolution/";
			try {
				TextReader reader(xmldata, size);
				
				string attribute;
				reader.read();
				if (reader.get_node_type() != TextReader::Element || reader.get_name().compare("wiidisc") || ELEMENT_INT(reader.get_attribute("version")) != 1)
					goto nextloop;
				
				ELEMENT_ATTRIBUTE("root", true)
					xmlroot = AbsolutePathCombine(xmlroot, attribute);
				ELEMENT_ATTRIBUTE("shiftfiles", true)
					ShiftFileReplacements = ELEMENT_BOOL();
				
				ELEMENT_LOOP {
					ELEMENT_END("wiidisc")
						break;
					
					ELEMENT_START("id") {
						ELEMENT_ATTRIBUTE("game", memcmp(MEM_BASE, attribute.c_str(), attribute.length()))
							goto nextloop;
						
						ELEMENT_ATTRIBUTE("developer", memcmp(MEM_BASE + 4, attribute.c_str(), attribute.length()))
							goto nextloop;
						
						ELEMENT_ATTRIBUTE("disc", (u8)ELEMENT_INT(attribute) != *(MEM_BASE + 6))
							goto nextloop;
						
						ELEMENT_ATTRIBUTE("version", (u8)ELEMENT_INT(attribute) != *(MEM_BASE + 7))
							goto nextloop;
						
						bool foundregion = false;
						ELEMENT_LOOP {
							ELEMENT_END("id")
								break;
							
							ELEMENT_START("region") {
								ELEMENT_ATTRIBUTE("type", attribute[0] == *(MEM_BASE + 3))
									foundregion = true;
							}
						}
						if (!foundregion)
							goto nextloop;
					}
					
					vector<PatchOption> macros;
					
					ELEMENT_START("options") {
						ELEMENT_LOOP {
							ELEMENT_END("options")
								break;
							
							ELEMENT_START("macro") {
								PatchOption macro;
								
								ELEMENT_ATTRIBUTE("name", true)
									macro.Name = attribute;
								
								ELEMENT_ATTRIBUTE("id", true)
									macro.ID = attribute;
								
								ELEMENT_LOOP {
									ELEMENT_END("macro")
										break;
									
									ELEMENT_START("param") {
										string key;
										string value;
										ELEMENT_ATTRIBUTE("name", true)
											key = attribute;
										ELEMENT_ATTRIBUTE("value", true)
											value = attribute;
										
										macro.Params[key] = value;
									}
								}
								
								macros.push_back(macro);
							}
							
							ELEMENT_START("section") {
								PatchSection sect;
								PatchSection* section = &sect;
								
								ELEMENT_ATTRIBUTE("name", true) {
									section = GetSectionByName(attribute);
									if (section == NULL) {
										section = &sect;
										sect.Name = attribute;
									}
								}
								
								ELEMENT_LOOP {
									ELEMENT_END("section")
										break;
									ELEMENT_START("option") {
										PatchOption option;
										option.ID = optid++;
										option.Enabled = 0;
										
										ELEMENT_ATTRIBUTE("name", true)
											option.Name = attribute;
										
										ELEMENT_ATTRIBUTE("id", true)
											option.ID = attribute;
										
										ELEMENT_ATTRIBUTE("default", true)
											option.Enabled = ELEMENT_INT(attribute);
										
										ELEMENT_LOOP {
											ELEMENT_END("option")
												break;
											
											ELEMENT_START("param") {
												string key;
												string value;
												ELEMENT_ATTRIBUTE("name", true)
													key = attribute;
												ELEMENT_ATTRIBUTE("value", true)
													value = attribute;
												
												option.Params[key] = value;
											}
											
											ELEMENT_START("description") {
												option.Description = ELEMENT_VALUE();
												ELEMENT_LOOP { ELEMENT_END("description") break; }
											}
											
											ELEMENT_START("choice") {
												PatchChoice choice;
												
												ELEMENT_ATTRIBUTE("name", true)
													choice.Name = attribute;
													
												ELEMENT_START("description") {
													choice.Description = ELEMENT_VALUE();
													ELEMENT_LOOP { ELEMENT_END("description") break; }
												}
													
												ELEMENT_LOOP {
													ELEMENT_END("choice")
														break;
													
													ELEMENT_START("patch") {
														ELEMENT_ATTRIBUTE("id", true)
															choice.Patches.push_back(xmlid + attribute);
													}
												}
												option.Choices.push_back(choice);
											}
										}
										
#define ADD_OPTION(optionp) \
	PatchOption* options = GetOptionByID(optionp.ID); \
	if (options == NULL) { \
		OPTION_DEFAULT(&optionp); \
		Options.push_back(optionp); \
		section->options.push_back(Options.size() - 1); \
	} else { \
		options->Choices.insert(options->Choices.end(), optionp.Choices.begin(), optionp.Choices.end()); \
		options->Params.insert(optionp.Params.begin(), optionp.Params.end()); \
		OPTION_DEFAULT(options); \
	}

#define OPTION_DEFAULT(optionp) \
	for (map<string, int>::iterator odef = Defaults.begin(); odef != Defaults.end(); odef++) { \
		if (!odef->first.compare((optionp)->ID) && (u32)odef->second <= (optionp)->Choices.size()) { \
			(optionp)->Enabled = odef->second; \
			break; \
		} \
	}

										
										bool macroed = false;;
										for (vector<PatchOption>::iterator optiter = macros.begin(); optiter != macros.end(); optiter++) {
											if (!option.ID.compare(optiter->ID)) {
												macroed = true;
												
												PatchOption opt = option;
												opt.ID = opt.ID + optiter->Name;
												opt.Name = optiter->Name;
												opt.Params.insert(optiter->Params.begin(), optiter->Params.end());
												
												ADD_OPTION(opt);
											}
										}
										
										if (!macroed) {
											ADD_OPTION(option);
										}
									}
								}
								
								if (section == &sect)
									Sections.push_back(sect);
							}
						}
					}
					
					ELEMENT_START("patch") {
						PatchPatch patch;
						string root = xmlroot;
						ELEMENT_ATTRIBUTE("id", true)
							patch.ID = xmlid + attribute;
						ELEMENT_ATTRIBUTE("root", true)
							root = AbsolutePathCombine(xmlroot, attribute);
						
						ELEMENT_LOOP {
							ELEMENT_END("patch")
								break;
								
							ELEMENT_START("file") {
								PatchFile file;
								file.Create = false;
								file.Stated = false;
								file.Resize = true;
								file.Offset = 0;
								file.FileOffset = 0;
								file.Length = 0;
								
								ELEMENT_ATTRIBUTE("resize", true)
									file.Resize = ELEMENT_BOOL();
								
								ELEMENT_ATTRIBUTE("create", true)
									file.Create = ELEMENT_BOOL();
								
								ELEMENT_ATTRIBUTE("disc", true)
									file.DiscFile = attribute;
								
								ELEMENT_ATTRIBUTE("offset", true)
									file.Offset = ELEMENT_INT(attribute);
									
								ELEMENT_ATTRIBUTE("fileoffset", true)
									file.FileOffset = ELEMENT_INT(attribute);
								
								ELEMENT_ATTRIBUTE("external", true)
									file.External = AbsolutePathCombine(root, attribute);
								
								ELEMENT_ATTRIBUTE("length", true)
									file.Length = ELEMENT_INT(attribute);
								
								patch.Files.push_back(file);
							}
							
							ELEMENT_START("folder") {
								string discfolder;
								string externalfolder;
								bool create = false;
								
								ELEMENT_ATTRIBUTE("disc", true)
									discfolder = attribute;
								
								ELEMENT_ATTRIBUTE("external", true)
									externalfolder = AbsolutePathCombine(root, attribute);
									
								ELEMENT_ATTRIBUTE("create", true)
									create = ELEMENT_BOOL();
								
								RecurseFolderPatch(&patch, discfolder, externalfolder, create);
							}
							
							ELEMENT_START("savefile") {
								PatchFs fs;
								
								ELEMENT_ATTRIBUTE("external", true)
									fs.Destination = AbsolutePathCombine(root, attribute);
								
								char folder[0x40];
								u64 titleid = WDVD_GetTMD()->title_id;
								sprintf(folder, "/title/%08x/%08x/data/", (u32)(titleid >> 32), (u32)titleid);
								
								fs.Source = string(folder);
								if (fs.Destination.size() > 0) {
									if (fs.Destination[fs.Destination.size() - 1] != '/')
										fs.Destination += '/';
									patch.Fs.push_back(fs);
								}
							}
							
							ELEMENT_START("shift") {
								PatchShift shift;
								
								ELEMENT_ATTRIBUTE("source", true)
									shift.Source = attribute;
								
								ELEMENT_ATTRIBUTE("destination", true)
									shift.Destination = attribute;
								
								patch.Shifts.push_back(shift);
							}
							
							ELEMENT_START("memory") {
								PatchMemory memory;
								memory.Value = 0;
								memory.Offset = 0;
								memory.Original = 0;
								memory.Verify = false;
								
								ELEMENT_ATTRIBUTE("offset", true)
									memory.Offset = (int*)ELEMENT_INT(attribute);
								
								ELEMENT_ATTRIBUTE("value", true)
									memory.Value = ELEMENT_INT(attribute);
								
								ELEMENT_ATTRIBUTE("original", true) {
									memory.Original = ELEMENT_INT(attribute);
									memory.Verify = true;
								}
								
								patch.Memory.push_back(memory);
							}
						}
						
						Patches.push_back(patch);
					}
				}
			} catch (...) { }
		nextloop:
			;
		}
	}
	
	File_CloseDir(dir);
}

void LoadDisc_LoadConfig()
{
	u8* xmldata = (u8*)SYS_GetArena2Lo(); // Borrowing MEM2, lulz
	
	static Stats st ATTRIBUTE_ALIGN(32);
	
	char filename[40];
	sprintf(filename, "/riivolution/config/%.4s.xml", (char*)MEM_BASE);
	
	File_Stat(filename, &st);
	
	int fd = File_Open(filename, O_RDONLY);
	if (fd < 0)
		return;
	File_Read(fd, xmldata, st.Size);
	File_Close(fd);
	
	int size = st.Size;
	// Round down for "extra content at end of document"
	while (size > 1 && xmldata[size - 1] != '>') // Find the last '>'
		size--;
	if (size == 0)
		return;
	
	try {
		TextReader reader(xmldata, size);
		
		string attribute;
		bool rooted = false;
		
		ELEMENT_LOOP {
			ELEMENT_START("riivolution") {
				ELEMENT_ATTRIBUTE("version", ELEMENT_INT(attribute) == 1)
					rooted = true;
				else
					break;
			}
			
			ELEMENT_END("riivolution")
				break;
			
			if (rooted) {
				ELEMENT_START("option") {
					string id;
					int defaul;
					ELEMENT_ATTRIBUTE("id", true)
						id = attribute;
					else
						continue;
					
					ELEMENT_ATTRIBUTE("default", true)
						defaul = ELEMENT_INT(attribute);
					else
						continue;
					
					Defaults[id] = defaul;
				}
			}
		}
	} catch(...) { }
}

void LoadDisc_SaveConfig()
{
	Element* root;
	
	Document document;
	
	root = document.create_root_node("riivolution");
	root->set_attribute("version", "1");
	char defaul[0x10];
	
	for (vector<PatchOption>::iterator iter = Options.begin(); iter != Options.end(); iter++) {
		Element* option = root->add_child("option");
		option->set_attribute("id", iter->ID);
		sprintf(defaul, "%d", iter->Enabled);
		option->set_attribute("default", defaul);
	}
	
	string xml = document.write_to_string_formatted();
	
	char filename[40];
	sprintf(filename, "/riivolution/config/%.4s.xml", (char*)MEM_BASE);
	
	File_CreateDir("/riivolution/config");
	
	File_Delete(filename);
	File_CreateFile(filename);
	
	int fd = File_Open(filename, O_WRONLY | O_TRUNC);
	
	if (fd < 0)
		return;
	
	File_Write(fd, (const u8*)xml.c_str(), xml.size());
	
	File_Close(fd);
}

void RB2DipReplacement(int device)
{
#ifdef RAWKHAXX
	// RB2 DIP Haxx
	//PatchFST("StrapUsage_eng_16_9.tpl", 0, "/rawk/rb2/strap.tpl", 0, 0);
	PatchFST("StrapUsage_eng_16_9.tpl", 0, "isfs\\/tmp/strap.tpl", 0, 0);
	FindNode("StrapUsage_deu_16_9.tpl")->DataOffset = 0xFE8921C8 >> 2;
	FindNode("StrapUsage_esl_16_9.tpl")->DataOffset = 0xFE8921C8 >> 2;
	FindNode("StrapUsage_fre_16_9.tpl")->DataOffset = 0xFE8921C8 >> 2;
	FindNode("StrapUsage_ita_16_9.tpl")->DataOffset = 0xFE8921C8 >> 2;
	FindNode("StrapUsage_nld_16_9.tpl")->DataOffset = 0xFE8921C8 >> 2;
	
	FindNode("StrapUsage_eng_4_3.tpl")->DataOffset = 0xFE8921C8 >> 2;
	FindNode("StrapUsage_deu_4_3.tpl")->DataOffset = 0xFE8921C8 >> 2;
	FindNode("StrapUsage_esl_4_3.tpl")->DataOffset = 0xFE8921C8 >> 2;
	FindNode("StrapUsage_fre_4_3.tpl")->DataOffset = 0xFE8921C8 >> 2;
	FindNode("StrapUsage_ita_4_3.tpl")->DataOffset = 0xFE8921C8 >> 2;
	FindNode("StrapUsage_nld_4_3.tpl")->DataOffset = 0xFE8921C8 >> 2;
	
	//eng_4_3 offset: 0xFE8B7A08 >> 2;
	
	//PatchFST("Loading.tpl", 0, "isfs\\/tmp/rawk.tpl", 0, 0);
	
	//if (device == 0)
	//	PatchFST("Loading.tpl", 0, "/rawk/rb2/rawksdhc.tpl", 0, 0);
	//else if (device == 1)
	//	PatchFST("Loading.tpl", 0, "/rawk/rb2/rawkusb.tpl", 0, 0);
	
	//PatchFST("main_wii_1.ark", 0x21900DF0, "/rawk/rb2/store.dtb", 0, 10645);
	//PatchFST("main_wii_1.ark", 0x21900DF0, "isfs\\/tmp/rawk.dtb", 0, 10645);
#endif
}

#define ADD_PATCH(patch) { \
	if (disc) { \
		for (size_t i = 0; i < (patch)->Files.size(); i++) \
			patches.push_back(&(patch)->Files[i]); \
		for (size_t i = 0; i < (patch)->Shifts.size(); i++) \
			shifts.push_back(&(patch)->Shifts[i]); \
	} \
	if (memory) { \
		for (size_t i = 0; i < (patch)->Memory.size(); i++) { \
			PatchMemory* memory = &(patch)->Memory[i]; \
			if (memory->Verify && *memory->Offset != memory->Original) \
				continue; \
			*memory->Offset = memory->Value; \
		} \
	} \
	for (size_t i = 0; i < (patch)->Fs.size(); i++) { \
		PatchFs* fs = &(patch)->Fs[i]; \
		AddPatchFs(fs->Source.c_str(), fs->Destination.c_str()); \
	} \
}

bool STRING_REPLACE(string* str, vector<PatchOption>::iterator* option)
{
	bool found = false;
	string::size_type pos;
	while ((pos = str->find("{$")) != string::npos) {
		string::size_type pend = str->find("}", pos);
		string param = str->substr(pos + 2, pend - pos - 2);
		*str = str->substr(0, pos) + (*option)->Params[param] + str->substr(pend + 1);
		found = true;
	}
	
	return found;
}

void DipReplacement(bool disc, bool memory, u32* fstsize)
{
	vector<PatchShift*> shifts;
	vector<PatchFile*> patches;
	
	vector<PatchPatch> patchcache;
	
	for (vector<PatchOption>::iterator option = Options.begin(); option != Options.end(); option++) {
		if (option->Enabled == 0)
			continue;
		
		for (vector<string>::iterator patchid = option->Choices[option->Enabled - 1].Patches.begin(); patchid != option->Choices[option->Enabled - 1].Patches.end(); patchid++) {
			PatchPatch* patch = GetPatchByID(*patchid);
			if (patch != NULL) {
				PatchPatch newpatch = *patch;
				bool found = false;
				for (vector<PatchFile>::iterator file = newpatch.Files.begin(); file != newpatch.Files.end(); file++) {
					found |= STRING_REPLACE(&file->DiscFile, &option);
					
					found |= STRING_REPLACE(&file->External, &option);
				}
				
				if (found) {
					patchcache.push_back(newpatch);
					ADD_PATCH(&patchcache[patchcache.size() - 1]); // WARNING: Taking address of a vector that will resize... However only storing the address of its subvector entries; should be okay.
				} else
					ADD_PATCH(patch);
			}
		}
	}
	
	if (disc)
		ExecuteQueue(&shifts, &patches, ShiftFileReplacements, fstsize);
}

bool Mount(int disk, int filesystem)
{
	return File_Mount(disk, filesystem) >= 0;
}

static u32 partition_info[24] ATTRIBUTE_ALIGN(32);
static int mounted;
static int fd;

static int loadstate = 0;

int LoadDisc_Init()
{
	InitializeNetwork();
	
	Sections = vector<PatchSection>();
	Options = vector<PatchOption>();
	Patches = vector<PatchPatch>();
	ShiftFileReplacements = true;
	
	if (loadstate < 1) {
		if (mload_init() < 0)
			return ERROR_MLOAD;
		
		usleep(1000);
		if (mload_module((void*)filemodule_dat, filemodule_dat_size) < 0)
			return ERROR_MLOAD;
		
		usleep(1000);
		
#ifdef EHCUSB
		if (mload_module((void*)ehcmodule_dat, ehcmodule_dat_size) < 0)
			return ERROR_MLOAD;
		
		usleep(350 * 1000);
#endif
		loadstate = 1;
	}
	
	if (loadstate < 2) {
		fd = File_Init();
		loadstate = 2;
	}
	
	usleep(1000);
	
	if (fd >= 0) {
#ifdef EHCUSB
		if (!Mount(2, 1)) {
#endif
			if (!Mount(1, 1)) {
				//File_Deinit();
				return ERROR_MOUNT;
			} else
				mounted = 0;
#ifdef EHCUSB
		} else
			mounted = 1;
#endif
	} else
		return ERROR_MOUNT;
	
	usleep(1000);
	
	if (loadstate < 3) {
		if (mload_module((void*)dipmodule_dat, dipmodule_dat_size) < 0)
			return ERROR_MLOAD;
		
		usleep(1000);
		
		loadstate = 3;
	}
	
	if (loadstate < 4) {
		fd = WDVD_Init();
		if (fd < 0)
			return ERROR_MLOAD;
		loadstate = 4;
		/*
#ifdef RIIVOLUTION
		int fshook = IOS_Ioctl(fd, 0xC8, NULL, 0, NULL, 0);
		if (fshook > 0) {
			fshook = mload_set_FS_ioctl_vector((void*)fshook);
			if (fshook < 0)
				return ERROR_MLOAD;
		}
#endif
*/
	}
#ifdef RAWKHAXX
	if (mload_module((void*)emumodule_dat, emumodule_dat_size) < 0)
		return ERROR_MLOAD;
	
	usleep(1000);
	
	int tfd = IOS_Open("emu", 0);
	if (tfd >= 0) {
		int fshook = IOS_Ioctl(tfd, 0x100, NULL, 0, NULL, 0);
		IOS_Close(tfd);
		
		if (fshook > 0) {
			fshook = mload_set_FS_ioctl_vector((void*)fshook);
			mload_close();
			if (fshook < 0)
				return ERROR_MLOAD;
		} else
			return ERROR_MLOAD;
	} else
		return ERROR_MLOAD;
	
	tfd = IOS_Open("/dev/fs", 0);
	if (fd >= 0) {
		s32 ret = IOS_Ioctl(tfd, 100, NULL, 0, NULL, 0);
		IOS_Close(tfd);
		if (ret != 1)
			return ERROR_MLOAD;
	} else
		return ERROR_MLOAD;
#endif
	
	WDVD_Reset();
	
	return ERROR_SUCCESS;
}

char GameTitle[0x40] ATTRIBUTE_ALIGN(0x20);

int LoadDisc_Begin()
{
	u32 i;
	
	//WDVD_StartLog();
	
	//bool disc = false;
	//WDVD_VerifyCover(&disc));
	//if (!disc)
	if (!WDVD_CheckCover())
		return ERROR_NODISC;
	
	WDVD_Reset();
	
	WDVD_LowReadDiskId();
	WDVD_LowUnencryptedRead(MEM_BASE, 0x20, 0x00000000); // Just to make sure...
	
	/* Piracy detection
	STACK_ALIGN(u8, bca_data, 0x40, 32);
	if (WDVD_LowReadBCA(bca_data, 0x40) != 1) {
		WDVD_StopMotor(true, false);
		return ERROR_NODISC;
	}
	*/
	
#ifdef ZERO4 
	if (~(*(s32*)MEM_BASE) != ~0x52345a4a) {
		WDVD_StopMotor(true, false);
		return ERROR_NODISC;
	}
#endif
	
	WDVD_LowUnencryptedRead(GameTitle, 0x40, 0x20);
	
	WDVD_LowUnencryptedRead(partition_info, 0x20, PART_OFFSET);
	// make sure there is at least one primary partition
	if (partition_info[0] == 0)
		return ERROR_DISCREAD;
	
	// read primary partition table
	WDVD_LowUnencryptedRead(partition_info + 8, max(4, min(8, partition_info[0])) << 3, partition_info[1] << 2);
	for (i = 0; i < partition_info[0]; i++)
		if (partition_info[(i << 1) + 1 + 8] == 0)
			break;
	
	// make sure we found a game partition
	if (i >= partition_info[0])
		return ERROR_DISCREAD;
	
	if (WDVD_LowOpenPartition((u64)partition_info[(i << 1) + 8] << 2) != 1)
	   return ERROR_DISCREAD;
	
#ifdef ZERO4 
	if (~(u32)WDVD_GetTMD()->title_id != ~0x52345a4a)
		return ERROR_NODISC;
#endif
	
	InitializePatcher(fd);
#ifdef RAWKHAXX
	SetClusters(false);
#else
	SetClusters(true); // Save space
	LoadDisc_LoadConfig();
	LoadDisc_ParseXMLs();
#endif
	
	return ERROR_SUCCESS;
}

int nullprintf(const char* fmt, ...)
{
	return 0;
}

#define RAWK_TEMP_FILE(filename, data, size) { \
	File_CreateFile(filename); \
	int tmpfd = File_Open(filename, O_WRONLY); \
	if (tmpfd >= 0) { \
		File_Write(tmpfd, data, size); \
		File_Close(tmpfd); \
	} \
}

static GXRModeObj* graphicsobjects[34] = {
	&TVNtsc240Ds,
	&TVNtsc240DsAa,
	&TVNtsc240Int,
	&TVNtsc240IntAa,
	&TVNtsc480Int,
	&TVNtsc480IntDf,
	&TVNtsc480IntAa,
	&TVNtsc480Prog,
	&TVNtsc480ProgSoft,
	&TVNtsc480ProgAa,
	&TVMpal480IntDf,
	&TVMpal480IntAa,
	&TVMpal240Ds,
	&TVMpal240DsAa,
	&TVPal264Ds,
	&TVPal264DsAa,
	&TVPal264Int,
	&TVPal264IntAa,
	&TVPal524IntAa,
	&TVPal528Int,
	&TVPal528IntDf,
	&TVPal574IntDfScale,
	&TVEurgb60Hz240Ds,
	&TVEurgb60Hz240DsAa,
	&TVEurgb60Hz240Int,
	&TVEurgb60Hz240IntAa,
	&TVEurgb60Hz480Int,
	&TVEurgb60Hz480IntDf,
	&TVEurgb60Hz480IntAa,
	&TVEurgb60Hz480Prog,
	&TVEurgb60Hz480ProgSoft,
	&TVEurgb60Hz480ProgAa,
	NULL
};

static GXRModeObj* graphicsmodes[0x100];
static int numgraphicsmodes = 0;

bool isviobject(GXRModeObj* mode1, GXRModeObj* mode2)
{
	if (mode1->viTVMode != mode2->viTVMode || mode1->fbWidth != mode2->fbWidth ||	mode1->efbHeight != mode2->efbHeight || mode1->xfbHeight != mode2->xfbHeight ||
		mode1->viXOrigin != mode2->viXOrigin || mode1->viYOrigin != mode2->viYOrigin || mode1->viWidth != mode2->viWidth || mode1->viHeight != mode2->viHeight ||
		mode1->xfbMode != mode2->xfbMode)
	{
		return false;
	} else
	{
		return true;
	}
}

void LoadDisc_PatchVideoMode(GXRModeObj* mode)
{
	for (int i = 0; i < numgraphicsmodes; i++) {
		memcpy(graphicsmodes[i], mode, sizeof(GXRModeObj));
	}
}

void LoadDisc_FindVideoModes(void* memory, u32 length, GXRModeObj** modes)
{
	if (length < sizeof(GXRModeObj))
		return;
	length -= sizeof(GXRModeObj);
	
	for (u32 num = 0; num < length; ) {
		int obj = 0;
		bool found = false;
		GXRModeObj* mode = (GXRModeObj*)(((u8*)memory) + num);
		do {
			//if (!memcmp(mode, modes[obj], sizeof(GXRModeObj))) {
			//if (!memcmp(mode->sample_pattern, modes[obj]->sample_pattern, 12*2) && !memcmp(mode->vfilter, modes[obj]->vfilter, 7)) {
			if (isviobject(mode, modes[obj])) {
				graphicsmodes[numgraphicsmodes] = mode;
				numgraphicsmodes++;
				found = true;
				break;
			}
			obj++;
		} while (modes[obj]);
		
		if (found)
			num += sizeof(GXRModeObj);
		else
			num += 4;
	}
}

#define REGION_PAL			'P'
#define REGION_PAL_FRANCE	'D'
#define REGION_PAL_GERMANY	'F'
#define REGION_EUR_X	'X'
#define REGION_EUR_Y	'Y'

/*
#define VIDEO_MODE_NTSC		0x00
#define VIDEO_MODE_PAL		0x01
#define VIDEO_MODE_PAL60	0x04
#define VIDEO_MODE_MPAL		0x05
*/
#define VIDEO_MODE_NTSC VI_NTSC
#define VIDEO_MODE_PAL VI_PAL
#define VIDEO_MODE_MPAL VI_MPAL
#define VIDEO_MODE_PAL60 VI_EURGB60
void LoadDisc_SetVideoMode()
{
	u32 tvmode = CONF_GetVideo();
	GXRModeObj* vmode = VIDEO_GetPreferredMode(0);
	u32 videomode = 0;
	switch (tvmode) {
		case CONF_VIDEO_PAL:
			if (CONF_GetEuRGB60() > 0)
				videomode = VIDEO_MODE_PAL60;
			else
				videomode = VIDEO_MODE_PAL;
			break;
		case CONF_VIDEO_MPAL:
			videomode = VIDEO_MODE_MPAL;
			break;
		default:
			videomode = VIDEO_MODE_NTSC;
			break;
	}
	
	/* Force system region video mode */
	if (CONF_GetProgressiveScan() > 0 && VIDEO_HaveComponentCable())
		vmode = &TVNtsc480Prog;
	else {
		switch (tvmode) {
			case CONF_VIDEO_PAL: case CONF_VIDEO_MPAL:
				if (videomode == VIDEO_MODE_PAL60)
					vmode = &TVEurgb60Hz480IntDf;
				else if (videomode == VIDEO_MODE_MPAL)
					vmode = &TVMpal480IntDf;
				else
					vmode = &TVPal528IntDf;
				break;
			case CONF_VIDEO_NTSC:
				vmode = &TVNtsc480IntDf;
			break;
		}
	}
	/* Force game region video mode
	switch (*((char*)MEM_BASE + 3)) {
		case REGION_PAL:
		case REGION_PAL_FRANCE:
		case REGION_PAL_GERMANY:
		case REGION_EUR_X:
		case REGION_EUR_Y:
			if (tvmode != CONF_VIDEO_PAL) {
				videomode = VIDEO_MODE_PAL60;
				if (CONF_GetProgressiveScan() > 0 && VIDEO_HaveComponentCable())
					vmode = &TVNtsc480Prog;
				else
					vmode = &TVEurgb60Hz480IntDf;
			}
			break;
		default:
			if (tvmode != CONF_VIDEO_NTSC) {
				videomode = VIDEO_MODE_NTSC;
				if (CONF_GetProgressiveScan() > 0 && VIDEO_HaveComponentCable())
					vmode = &TVNtsc480Prog;
				else
					vmode = &TVNtsc480IntDf;
			}
			break;
	}
	*/
	//vmode = &TVNtsc480IntDf;
	VIDEO_Configure(vmode); VIDEO_SetBlack(true); VIDEO_Flush(); VIDEO_WaitVSync(); VIDEO_WaitVSync();
	*(u32*)MEM_VIDEOMODE = vmode->viTVMode >> 2;
	
	/* Video mode trick using disc region video mode
	switch (*((char*)MEM_BASE + 3)) {
		case REGION_PAL:
		case REGION_PAL_FRANCE:
		case REGION_PAL_GERMANY:
		case REGION_EUR_X:
		case REGION_EUR_Y:
			if (tvmode != CONF_VIDEO_PAL || CONF_GetEuRGB60() > 0)
				videomode = VIDEO_MODE_PAL60;
			else
				videomode = VIDEO_MODE_PAL;
			break;
		default:
			videomode = VIDEO_MODE_NTSC;
			break;
	}
	*/
	LoadDisc_PatchVideoMode(vmode);
	//*(u32*)MEM_VIDEOMODE = videomode;
}


static u32 fstdata[0x10] ATTRIBUTE_ALIGN(32);

void LoadDisc()
{
	settime(secs_to_ticks(time(NULL) - 946684800));
	
#ifdef ZERO4
	LoadDisc_WritePlayLog((u32)WDVD_GetTMD()->title_id, WDVD_GetTMD()->group_id, (char*)"Fatal Frame 4");
#else
	LoadDisc_WritePlayLog((u32)WDVD_GetTMD()->title_id, WDVD_GetTMD()->group_id, GameTitle);
#endif
	
	// Avoid a flash of green
	VIDEO_SetBlack(true); VIDEO_Flush(); VIDEO_WaitVSync(); VIDEO_WaitVSync();
	
#ifndef RAWKHAXX
	LoadDisc_SaveConfig();
#endif
	
	// At 0x420: uint dolOffset, uint fstOffset, uint fstSize
	WDVD_LowRead(fstdata, 0x40, 0x420);
	fstdata[2] <<= 2;
	WDVD_LowRead(SYS_GetArena2Lo(), fstdata[2], (u64)fstdata[1] << 2); // MEM2
	SetFST(SYS_GetArena2Lo());
	
#ifdef RAWKHAXX
	if (!memcmp(MEM_BASE, "SZAE", 4)) {
		RB2DipReplacement(mounted);
		
		RAWK_TEMP_FILE("isfs\\/tmp/strap.tpl", loading_dat, loading_dat_size);
		RAWK_TEMP_FILE("isfs\\/tmp/store.dtb", store_dat, store_dat_size); // TODO: Generate this instead and make it dynamic (PAL vs SZAE)
		if (mounted == 0) {
			RAWK_TEMP_FILE("isfs\\/tmp/rawk.tpl", rawksdhc_dat, rawksdhc_dat_size);
		} else if (mounted == 1)
			RAWK_TEMP_FILE("isfs\\/tmp/rawk.tpl", rawkusb_dat, rawkusb_dat_size);
	}
#else
	DipReplacement(true, false, &fstdata[2]);
	
	File_CreateDir("/riivolution/temp");
	File_CreateFile("/riivolution/temp/fst");
	int tmpfd = File_Open("/riivolution/temp/fst", O_WRONLY | O_TRUNC);
	File_Write(tmpfd, (u8*)SYS_GetArena2Lo(), fstdata[2]);
	File_Close(tmpfd);
	fstdata[1] = (u32)((SHIFT_BASE + (u64)GetShiftOffset()) >> 2);
	AddPatch(AddPatchFile("/riivolution/temp/fst"), 0, ((u64)fstdata[1]) << 2, fstdata[2]);
	fstdata[2] >>= 2;
	// TODO: fstdata[3] = fstdata[2]
	File_CreateFile("/riivolution/temp/fst.header");
	tmpfd = File_Open("/riivolution/temp/fst.header", O_WRONLY | O_TRUNC);
	File_Write(tmpfd, (u8*)fstdata, 0x40);
	File_Close(tmpfd);
	AddPatch(AddPatchFile("/riivolution/temp/fst.header"), 0, 0x420, 0x40);
#endif
	
	AppEnter app_enter = NULL;
	AppLoad app_loader = NULL;
	AppExit app_exit = NULL;
	void *app_address = NULL;
	s32 app_section_size = 0;
	s32 app_disc_offset = 0;
	// reused memory
	app_info *app = (app_info*)partition_info;
	
	// put crap in memory to keep the apploader/dol happy
	*(u32*)0x800000F0 = 0x01800000; // Simulated memory size
	*(u32*)0x800000F4 = 0; // BI2
	*(u32*)0x80000028 = 0x01800000; // Physical mem size
	
	*MEM_BOOTCODE = 0x0D15EA5E;
	*MEM_VERSION = 1;
	*MEM_ARENA1LOW = 0;
	*MEM_BUSSPEED = 0x0E7BE2C0;
	*MEM_CPUSPEED = 0x2B73A840;
	*MEM_GAMEIDADDRESS = MEM_BASE;
	memcpy(MEM_GAMEONLINE, MEM_BASE, 4);
	//*MEM_GAMEONLINE = (u32)MEM_BASE;
	DCFlushRange(MEM_BASE, 0x3F00);
	
	// read the apploader info
	WDVD_LowRead(app, sizeof(app_info), APP_INFO_OFFSET);
	DCFlushRange(app, sizeof(app_info));
	// read the apploader into memory
	WDVD_LowRead(MEM_APPLOADER, app->loader_size + app->data_size, APP_DATA_OFFSET);
	DCFlushRange(MEM_APPLOADER, app->loader_size + app->data_size);
	app->start(&app_enter, &app_loader, &app_exit);
	app_enter((AppReport)nullprintf);
	
	while (app_loader(&app_address, &app_section_size, &app_disc_offset)) {
		WDVD_LowRead(app_address, app_section_size, (u64)app_disc_offset << 2);
		DCFlushRange(app_address, app_section_size);
		//patch_di_dol((u8*)app_address, app_section_size);
		LoadDisc_FindVideoModes(app_address, app_section_size, graphicsobjects);
		app_address = NULL;
		app_section_size = 0;
		app_disc_offset = 0;
	}
	
	// copy the IOS version over the expected IOS version
	memcpy(MEM_IOSEXPECTED, MEM_IOSVERSION, 4);
	
#ifdef RAWKHAXX
	if (!memcmp(MEM_BASE, "SZAE", 4)) {
		// RB2 patches
		//printf("Applying RB2 haxx:\n");
		
		//printf("\tSkip splash screens\n");
		*(char*)0x8000C3A0 = 0x40; // Speedy splash screens
		
		//printf("\tEnable customs\n");
		*(char*)0x8051A617 = 0; // Disable VerifyChecksum
		
		//printf("\tSD card redirect\n");
		*(u32*)0x8072E144 = 0x3860FFFF; // Disable SD Mode
		
		//printf("\tHopo Adjustment\n");
		// GuitarTrackWatcherImpl::CanHopo
		//*(u32*)0x802726C4 = 0x38600001; // li %r3, 1
		//*(u32*)0x802726C8 = 0x4E800020; // blr
		// GuitarTrackWatcherImpl::HarmlessFretDown
		*(u32*)0x8027273C = 0x38600001; // li %r3, 1
		*(u32*)0x80272740 = 0x4E800020; // blr
		
		//printf("\tPatching screens\n");
	}
#else
	// Just in case the writing failed, give it one shot
	memcpy((void*)*MEM_FSTADDRESS, SYS_GetArena2Lo(), fstdata[2]);

	DipReplacement(false, true, NULL);
#endif
	
	LoadDisc_SetVideoMode();
	
	app_address = app_exit();
	if (app_address) {
		WDVD_Close();
		DCFlushRange(MEM_BASE, 0x17FFFFFF);
		
		SYS_ResetSystem(SYS_SHUTDOWN, 0, 0);
		__asm__ __volatile__ (
			"mtlr %0;"
			"blr"
			:
			: "r" (app_address)
		);
	}
	VIDEO_SetBlack(false);
	VIDEO_Flush();
	VIDEO_WaitVSync(); VIDEO_WaitVSync();
	// shouldn't get to here
}

//#define DEBUG_NET

#define DEBUG_PORT 1100
#define DEBUG_IPADDRESS "192.168.1.8"

#ifdef DEBUG_NET
static int socket;
#endif

void InitializeNetwork()
{
#ifdef DEBUG_NET
	int init;
	while ((init = net_init()) < 0)
		;
	
	// DEBUG: Connect to me
	socket = net_socket(PF_INET, SOCK_STREAM, 0);
	struct sockaddr_in address;
	memset(&address, 0, sizeof(address));
	
	address.sin_family = PF_INET;
	address.sin_port = htons(DEBUG_PORT);
	int ret = inet_aton(DEBUG_IPADDRESS, &address.sin_addr);
	if (ret <= 0)
		return;
	if (net_connect(socket, (struct sockaddr*)(const void *)&address, sizeof(address)) == -1)
		return;
	DebugPrint("ohai thar\n");
	//net_send(socket, "lolhi\n", 6, 0);
	//net_close(socket);
	
	// DebugPrintf("%d\n", init); // 0
#endif
}

void ShutdownNetwork()
{
#ifdef DEBUG_NET
	if (socket != -1)
		net_close(socket);
	
	net_deinit();
#endif
}

void DebugPrint(const char* data)
{
#ifdef DEBUG_NET
	if (socket != -1) {
		net_send(socket, data, strlen(data), 0);
	}
#endif
}

void DebugPrintf(const char *fmt, ...)
{
	DebugPrint(".");
#ifdef DEBUG_NET
	if (socket != -1) {
		va_list arg;
		va_start(arg, fmt);
		char str[5 * 1024];
		//char* str = new char[strlen(fmt) + 5 * 1024];
		vsprintf(str, fmt, arg);
		va_end(arg);
		
		DebugPrint(str);
		
		//delete[] str;
	}
#endif
}
