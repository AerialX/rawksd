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
#include "dipmodule_dat.h"
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

const char* LoadDisc_Error(int ret)
{
	switch (ret) {
		case ERROR_MLOAD:
			return "Unable to startup; if this problem persists you may need to reinstall.";
		case ERROR_MOUNT:
			return "Please insert an SD card or USB drive...";
		case ERROR_NODISC:
			return "Please insert a game disc.";
		case ERROR_DISCREAD:
			return "The inserted disc does not look like a valid Wii disc.";
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

PatchSection* GetSectionByName(string name)
{
	for (int i = 0; i < Sections.size(); i++)
		if (!Sections[i].Name.compare(name))
			return &Sections[i];
	
	return NULL;
}

PatchOption* GetOptionByID(string id)
{
	for (int i = 0; i < Options.size(); i++)
		if (!Options[i].ID.compare(id))
			return &Options[i];
	
	return NULL;
}

PatchPatch* GetPatchByID(string id)
{
	for (int i = 0; i < Patches.size(); i++) {
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
			
			//u8* xmldata = (u8*)malloc(st.Size); // >.>
			//u8* xmldata = new u8[st.Size]; // >.>
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
				//free(xmldata);
				continue; // Not an xml document
			}
			
			string xmlroot = "/riivolution/";
			
			TextReader reader(xmldata, size);
			
			string attribute;
			reader.read();
			if (reader.get_node_type() != TextReader::Element || reader.get_name().compare("wiidisc") || ELEMENT_INT(reader.get_attribute("version")) != 1)
				goto nextloop;
			
			ELEMENT_ATTRIBUTE("root", true)
				xmlroot = AbsolutePathCombine(xmlroot, attribute);
			
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
									
									// Defaults
									for (map<string, int>::iterator odef = Defaults.begin(); odef != Defaults.end(); odef++) {
										if (!odef->first.compare(option.ID)) {
											option.Enabled = odef->second;
											break;
										}
									}
									
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
		Options.push_back(optionp); \
		section->options.push_back(Options.size() - 1); \
	} else { \
		options->Choices.insert(options->Choices.end(), optionp.Choices.begin(), optionp.Choices.end()); \
		options->Params.insert(optionp.Params.begin(), optionp.Params.end()); \
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
								file.Resize = !strcasecmp(attribute.c_str(), "yes") || !strcasecmp(attribute.c_str(), "true");
							
							ELEMENT_ATTRIBUTE("create", true)
								file.Create = !strcasecmp(attribute.c_str(), "yes") || !strcasecmp(attribute.c_str(), "true");
							
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
								create = !strcasecmp(attribute.c_str(), "yes") || !strcasecmp(attribute.c_str(), "true");
							
							RecurseFolderPatch(&patch, discfolder, externalfolder, create);
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
							memory.Verify = false;;
							
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
			
		nextloop:
			//free(xmldata);
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
		for (int i = 0; i < patch.Files.size(); i++) \
			patches.push_back(patch.Files[i]); \
		for (int i = 0; i < patch.Shifts.size(); i++) \
			shifts.push_back(patch.Shifts[i]); \
	} \
	if (memory) { \
		for (int i = 0; i < patch.Memory.size(); i++) { \
			PatchMemory* memory = &patch.Memory[i]; \
			if (memory->Verify && *memory->Offset != memory->Original) \
				continue; \
			*memory->Offset = memory->Value; \
		} \
	} \
}

#define STRING_REPLACE(str) { \
	while ((pos = str->find("{$")) != string::npos) { \
		string::size_type pend = str->find("}", pos); \
		string param = str->substr(pos + 2, pend - pos - 2); \
		*str = str->substr(0, pos) + option->Params[param] + str->substr(pend + 1); \
	} \
}

void DipReplacement(bool disc, bool memory, u32* fstsize)
{
	vector<PatchShift> shifts;
	vector<PatchFile> patches;
	
	for (vector<PatchOption>::iterator option = Options.begin(); option != Options.end(); option++) {
		if (option->Enabled == 0)
			continue;
		
		for (vector<string>::iterator patchid = option->Choices[option->Enabled - 1].Patches.begin(); patchid != option->Choices[option->Enabled - 1].Patches.end(); patchid++) {
			PatchPatch* pth = GetPatchByID(*patchid);
			if (pth != NULL) {
				PatchPatch patch = *pth;
				for (vector<PatchFile>::iterator file = patch.Files.begin(); file != patch.Files.end(); file++) {
					string::size_type pos;
					string* str = &file->DiscFile;
					
					STRING_REPLACE(str);
					
					str = &file->External;
					
					STRING_REPLACE(str);
				}
				ADD_PATCH(patch);
			}
		}
	}
	
	if (disc)
		ExecuteQueue(&shifts, &patches, fstsize);
}

int FindInBuffer(const u8* buffer, const int bufferlength, const char* pattern, const int patternlength)
{
	for (int i = 0; i < bufferlength - patternlength; i++) {
		if (!memcmp(buffer + i, pattern, patternlength))
			return i;
	}
	
	return -1;
}

int patch_di_dol(u8 *Address, int Size)
{
	int ret = 0;
	int pos;
	while (Size > 0 && (pos = FindInBuffer(Address, Size, "/dev/di", 7)) >= 0) {
		ret++;
		Address[pos + 6] = 'o';
		pos += 7;
		Address += pos;
		Size -= pos;
	}
	
	return ret;
}

bool Mount(int disk, int filesystem)
{
	return File_Mount(disk, filesystem) >= 0;
}

static u32 partition_info[24] ATTRIBUTE_ALIGN(32);
static int mounted;
static int fd;

static bool preminited = false;

int LoadDisc_Init()
{
	InitializeNetwork();
	
	Sections = vector<PatchSection>();
	Options = vector<PatchOption>();
	Patches = vector<PatchPatch>();
	
	if (!preminited) {
		if (mload_init() < 0)
			return ERROR_MLOAD;
		
		if (mload_module((void*)filemodule_dat, filemodule_dat_size) >= 0)
			usleep(200);
		else
			return ERROR_MLOAD;
#ifdef EHCUSB
		if (mload_module((void*)ehcmodule_dat, ehcmodule_dat_size) >= 0)
			usleep(350*1000);
		else
			return ERROR_MLOAD;
#endif
	}
	preminited = true;
	
	fd = File_Init();
	if (fd >= 0) {
#ifdef EHCUSB
		if (!Mount(2, 1)) {
#endif
			if (!Mount(1, 1)) {
				File_Deinit();
				return ERROR_MOUNT;
			} else
				mounted = 0;
#ifdef EHCUSB
		} else
			mounted = 1;
#endif
	} else
		return ERROR_MOUNT;
	
	if (mload_module((void*)dipmodule_dat, dipmodule_dat_size) >= 0)
		usleep(200);
	else
		return ERROR_MLOAD;
	
	fd = WDVD_Init();
	if (fd < 0)
		return ERROR_MLOAD;
	
#ifdef RAWKHAXX
	if (mload_module((void*)emumodule_dat, emumodule_dat_size) >= 0)
		usleep(200);
	else
		return ERROR_MLOAD;
	
	fd = IOS_Open("emu", 0);
	if (fd >= 0) {
		int fshook = IOS_Ioctl(fd, 0x100, NULL, 0, NULL, 0);
		IOS_Close(fd);
		
		if (fshook > 0) {
			fshook = mload_set_FS_ioctl_vector((void*)fshook);
			mload_close();
			if (fshook < 0)
				return ERROR_MLOAD;
		} else
			return ERROR_MLOAD;
	} else
		return ERROR_MLOAD;
	
	fd = IOS_Open("/dev/fs", 0);
	if (fd >= 0) {
		s32 ret = IOS_Ioctl(fd, 100, NULL, 0, NULL, 0);
		IOS_Close(fd);
		if (ret != 1)
			return ERROR_MLOAD;
	} else
		return ERROR_MLOAD;
#endif
	
	return ERROR_SUCCESS;
}

char GameTitle[0x40] ATTRIBUTE_ALIGN(0x20);

int LoadDisc_Begin()
{
	STACK_ALIGN(u8, bca_data, 0x40, 32);
	u32 i;
	
	//WDVD_StartLog();
	
	WDVD_Reset();
	
	if (!WDVD_CheckCover())
		return ERROR_NODISC;
	
	WDVD_LowReadDiskId();
	
	// Piracy detection
	if (WDVD_LowReadBCA(bca_data, 0x40) != 1) {
		WDVD_StopMotor(true, false);
		return ERROR_NODISC;
	}
	
	WDVD_LowUnencryptedRead(GameTitle, 0x40, 0x20);
	
	settime(secs_to_ticks(time(NULL) - 946684800));
	
	WDVD_LowUnencryptedRead(partition_info, 0x20, PART_OFFSET);
	// make sure there is at least one primary partition
	if (partition_info[0] == 0) {
		WDVD_StopMotor(true, false);
		return ERROR_DISCREAD;
	}
	
	// read primary partition table
	WDVD_LowUnencryptedRead(partition_info + 8, max(4, min(8, partition_info[0])) << 3, partition_info[1] << 2);
	for (i = 0; i < partition_info[0]; i++) {
		if (partition_info[(i << 1) + 1 + 8] == 0)
			break;
	}
	// make sure we found a game partition
	if (i >= partition_info[0]) {
		WDVD_StopMotor(true, false);
		return ERROR_DISCREAD;
	}
	
	if (WDVD_LowOpenPartition((u64)partition_info[(i << 1) + 8] << 2) != 1) {
	   WDVD_StopMotor(true, false);
	   return ERROR_DISCREAD;
	}
	
	InitializePatcher(fd);
#ifdef RAWKHAXX
	SetClusters(false);
#else
	SetClusters(true); // Save space for ProxiIOS
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

static u32 fstdata[0x10];

void LoadDisc()
{
	// Avoid a flash of green
	VIDEO_SetBlack(true);
	VIDEO_Flush();
	VIDEO_WaitVSync(); VIDEO_WaitVSync();
	
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
	fstdata[1] = (SHIFT_BASE + GetShiftOffset()) >> 2;
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
	WDVD_LowRead(MEM_APPLOADER, app->loader_size+app->data_size, APP_DATA_OFFSET);
	DCFlushRange(MEM_APPLOADER, app->loader_size+app->data_size);
	app->start(&app_enter, &app_loader, &app_exit);
	app_enter((AppReport)nullprintf);
	while (app_loader(&app_address, &app_section_size, &app_disc_offset)) {
		WDVD_LowRead(app_address, app_section_size, ((u64)app_disc_offset) << 2);
		DCFlushRange(app_address, app_section_size);
		patch_di_dol((u8*)app_address, app_section_size);
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
	DipReplacement(false, true, NULL);
#endif
	
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

static int socket;

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
