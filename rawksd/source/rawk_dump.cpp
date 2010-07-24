#include "rawksd_menu.h"
#include "fst.h"

#include "launcher.h"
#include "wdvd.h"
#include "files.h"

#include <unistd.h>
#include <time.h>
#include <dirent.h>

#include "rawk_dump.h"

#define printf(...)

#define MAX_PATH 128

#define RIP_FILE			0
#define RIP_DIR				1
#define RIP_OPTIONAL		2
#define RIP_CREATE_ONLY		4

struct dump_files {
	const char *name;
	int type;
};

struct disc_info {
	const char *disc_name;
	const char *dump_path;
	const struct dump_files *rip_files;
	const char **exclusions;
};

static const char *gh_exclusions[] = {
	"synctest",
	"tut",
	"whammytime",
	"_f.pak.ngc",
	"_g.pak.ngc",
	"_gfx.pak.ngc",
	"_i.pak.ngc",
	"_s.pak.ngc",
	"_sfx.pak.ngc",
	"boss",
	"mutetest",
	"debug",
	"dlc",
	"_song_scripts",
	".txt",
	".lst",
	".mid",
	NULL
};

static const char *gha_exclusions[] = {
	"synctest",
	"tut",
	"whammytime",
	"_f.pak.ngc",
	"_g.pak.ngc",
	"_gfx.pak.ngc",
	"_i.pak.ngc",
	"_s.pak.ngc",
	"_sfx.pak.ngc",
	"boss",
	"mutetest",
	"debug",
	"dlc",
	"_song_scripts",
	".txt",
	".lst",
	// needs *.mid_text.qb.ngc
	".mid.qb.ngc",
	NULL
};

static const struct dump_files dump_gh3[] = {
	{"music", RIP_DIR},
	{"songs", RIP_DIR},
	{"pak", RIP_DIR|RIP_CREATE_ONLY},
	{"pak/qb.pak.ngc", RIP_FILE},
	{NULL, 0}
};

static const struct dump_files dump_ghwt[] = {
	{"music", RIP_DIR},
	{"songs", RIP_DIR},
	{"pak", RIP_DIR|RIP_CREATE_ONLY},
	{"pak/qb.pak.ngc", RIP_FILE},
	{"pak/qs.pak.ngc", RIP_FILE},
	{NULL, 0}
};

static const struct dump_files dump_rb1[] = {
	{"gen", RIP_DIR|RIP_CREATE_ONLY},
	{"gen/main.hdr", RIP_FILE},
	{"gen/main_0.ark", RIP_FILE},
	{"gen/main_1.ark", RIP_FILE|RIP_OPTIONAL}, // probably not present
	{NULL, 0}
};

static const struct dump_files dump_rb_new[] = {
	{"gen", RIP_DIR|RIP_CREATE_ONLY},
	{"gen/main_wii.hdr", RIP_FILE},
	{"gen/main_wii_0.ark", RIP_FILE},
	{"gen/main_wii_1.ark", RIP_FILE},
	// LRB doesn't have these
	{"gen/main_wii_2.ark", RIP_FILE|RIP_OPTIONAL},
	{"gen/main_wii_3.ark", RIP_FILE|RIP_OPTIONAL},
	{"gen/main_wii_4.ark", RIP_FILE|RIP_OPTIONAL},
	{"gen/main_wii_5.ark", RIP_FILE|RIP_OPTIONAL},
	{"gen/main_wii_6.ark", RIP_FILE|RIP_OPTIONAL},
	{"gen/main_wii_7.ark", RIP_FILE|RIP_OPTIONAL},
	{"gen/main_wii_8.ark", RIP_FILE|RIP_OPTIONAL},
	{"gen/main_wii_9.ark", RIP_FILE|RIP_OPTIONAL},
	{NULL, 0}
};

static const disc_info discs[] = {
	{"Rock Band 1", "rawk/RB1", dump_rb1, NULL},
	{"Rock Band Track Pack 1", "rawk/RB_TP1", dump_rb1, NULL},
	{"Rock Band Track Pack 2", "rawk/RB_TP2", dump_rb1, NULL},
	{"Rock Band ACDC Track Pack", "rawk/RB_ACDC", dump_rb1, NULL},
	{"Rock Band Classic Track Pack", "rawk/RB_CLASSIC", dump_rb1, NULL},
	{"Rock Band Country Track Pack", "rawk/RB_COUNTRY", dump_rb1, NULL},
	{"Guitar Hero World Tour", "rawk/GHWT", dump_ghwt, gh_exclusions},
	{"Guitar Hero 3", "rawk/GH3", dump_gh3, gh_exclusions},
	{"Guitar Hero Metallica", "rawk/GHM", dump_ghwt, gh_exclusions},
	{"Guitar Hero Smash Hits", "rawk/GHSH", dump_ghwt, gh_exclusions},
	{"Guitar Hero Aerosmith", "rawk/GHA", dump_gh3, gha_exclusions},
	{"PS2 Guitar Hero 2", "rawk/GH2", dump_rb1, NULL},
	{"PS2 Guitar Hero", "rawk/GH", dump_rb1, NULL},
	{"PS2 Guitar Hero 80s", "rawk/GH80S", dump_rb1, NULL},
	{"Guitar Hero 5", "rawk/GH5", dump_ghwt, gh_exclusions},
	{"Rock Band Metal Track Pack", "rawk/RB_METAL", dump_rb1, NULL},
	{"Green Day Rock Band", "rawk/GDRB", dump_rb_new, NULL},
	{"Band Hero", "rawk/BH", dump_ghwt, gh_exclusions},
	{"Guitar Hero Van Halen", "rawk/GHVH", dump_ghwt, gh_exclusions},
	{"LEGO Rock Band", "rawk/LRB", dump_rb_new, NULL}
};

static const char *disc_prefix[] = {
	"fst:/1/",
	"fst:/0/",
	"wddvd:/",
	NULL
};

static s32 hId = -1;
static char default_path[MAX_PATH*2] = {0};
static void *copy_buf = NULL;

void File_Chdir(const char *path) {
	if (path==NULL)
		return;
	if (path[0]!='/')
		strcat(default_path, path);
	else
		strcpy(default_path, path);
	strcat(default_path, "/");
}

int File_CreateDirPath(const char *path) {
	char fullpath[MAX_PATH*2];
	if (path==NULL)
		return -1;
	if (path[0]=='/')
		return File_CreateDir(path);
	else {
		sprintf(fullpath, "%s%s", default_path, path);
		return File_CreateDir(fullpath);
	}
}

int File_OpenPath(const char *path, int mode) {
	char fullpath[MAX_PATH*2];
	if (path==NULL)
		return -1;
	if (path[0]=='/')
		return File_Open(path, mode);
	else {
		sprintf(fullpath, "%s%s", default_path, path);
		return File_Open(fullpath, mode);
	}
}

int get_disc()
{
	u64 disc_id;

	if ((disc_id=FST_Mount()))
	{
		switch ((u16)(disc_id>>40))
		{
			case 0x4B58:
				return DISC_RB1;
			case 0x3333:
				return DISC_ACDC;
			case 0x5245:
				return DISC_TP1;
			case 0x5244:
				return DISC_TP2;
			case 0x335A:
				return DISC_CLASSIC;
			case 0x3334:
				return DISC_COUNTRY;
			case 0x3337:
				return DISC_METAL;
			case 0x5841:
				return DISC_GHWT;
			case 0x4748:
				return DISC_GH3;
			case 0x5842:
				return DISC_GHM;
			case 0x5843:
				return DISC_GHSH;
			case 0x4756:
				return DISC_GHA;
			case 0x5845:
				return DISC_GH5;
			case 0x3336:
				return DISC_GDRB;
			case 0x5844:
				return DISC_GHVH;
			case 0x5846:
				return DISC_BH;
			case 0x364C:
				return DISC_LRB;
			case 0x5A41:
				FST_Unmount();
				return DISC_RB2;
			case 0x394A:
				FST_Unmount();
				return DISC_TBRB;
			default:
				FST_Unmount();
				return DISC_UNRECOGNIZED;
		}
	} else
		printf("FST_Mount failed\n");

	if (ISO9660_Mount())
	{
		struct stat st;

		if (stat("wddvd:/sles_548.59", &st)>=0 || stat("wddvd:/slus_215.86", &st)>=0)
			return DISC_GH80S;
		if (stat("wddvd:/sles_544.42", &st)>=0 || stat("wddvd:/slus_214.47", &st)>=0)
			return DISC_GH2;
		if (stat("wddvd:/sles_541.32", &st)>=0 || stat("wddvd:/slus_212.24", &st)>=0)
			return DISC_GH1;
		ISO9660_Unmount();
		return DISC_UNRECOGNIZED;
	} else
		printf("ISO_Mount failed\n");

	return -1;
}

void close_disc()
{
	ISO9660_Unmount();
	FST_Unmount();
}

void update_time(u64 total, u64 current, time_t start, int *h, int *m, int *s)
{
	time_t time_so_far = time(0) - start;
	time_t estimated = (u64)time_so_far*total / current;
	time_t est = (estimated>time_so_far) ? (estimated - time_so_far):1;
	*h = est / 3600;
	*m = (est/60)%60;
	*s = est % 60;
}

u64 space_needed(struct rip_state *rs, int disc)
{
	printf("space_needed %d\n", disc);
	u64 space=0;
	unsigned int i;
	int j;
	struct stat st;
	const char **excl = discs[disc].exclusions;
	const struct dump_files *files = discs[disc].rip_files;
	char full_filename[MAX_PATH];

	for (i=0;files[i].name;i++)
	{
		printf("Checking file %s\n", files[i].name);
		if ((files[i].type & RIP_DIR) && !(files[i].type & RIP_CREATE_ONLY))
		{
			DIR_ITER *d=NULL;
			for (j=0;disc_prefix[j]!=NULL;j++)
			{
				sprintf(full_filename, "%s%s/", disc_prefix[j], files[i].name);
				if ((d=diropen(full_filename)))
					break;
			}
			if (d==NULL)
			{
				if (!files[i].type&RIP_OPTIONAL)
				{
					printf("Couldn't open dir %s\n", files[i].name);
					return 0;
				}
			}
			else
			{
				while (!dirnext(d, full_filename, &st))
				{
					if (!(st.st_mode & S_IFDIR)) {
						if (excl)
						{
							for (j=0;excl[j];j++)
							{
								if (strcasestr(full_filename, excl[j]))
									break;
							}
							if (excl[j])
								continue;
						}
						space += st.st_size;
						if ((s64)rs->bytes_in_current_file < st.st_size)
							rs->bytes_in_current_file = rs->offset_in_current_file = st.st_size;
					}
				}
				dirclose(d);
			}
		}
		else if (!(files[i].type & RIP_CREATE_ONLY))
		{
			for (j=0; disc_prefix[j]!=NULL; j++)
			{
				sprintf(full_filename, "%s%s", disc_prefix[j], files[i].name);
				printf("attempting to stat %s\n", full_filename);
				if (stat(full_filename, &st)>=0 && (st.st_mode&S_IFREG))
				{
					printf("stat'd file %s\n", full_filename);
					break;
				}
				else
				{
					printf("failed\n");
					st.st_size=0;
				}
			}
			if (st.st_size==0 && !(files[i].type&RIP_OPTIONAL))
			{
				printf("Couldn't open file %s\n", full_filename);
				return 0;
			}
			printf("adding size %llu\n", st.st_size);
			space += st.st_size;
			if ((s64)rs->bytes_in_current_file<st.st_size)
				rs->bytes_in_current_file = rs->offset_in_current_file = st.st_size;
		}
	}

	printf("space needed: %llu\n", space);
	return space;
}

u64 get_space(int device, int init)
{
	printf("get_space %d %d\n", device, init);
	if (device==DEVICE_NONE)
		return -1;
	if (init && File_CheckPhysical(device)<0)
	{
		printf("Device isn't present\n");
		return -1;
	}
	u64 space=0;

	if (File_GetFreeSpace(device, &space)<0)
		return 0;

	printf("free space: %llu\n", space);
	return space;
}

bool prepare_device(struct rip_state *rs, int device)
{
	printf("preparing device %d\n", device);
	if (rs->out_device>0 && rs->out_device!=device)
	{
		printf("Closing old device %d\n", rs->out_device);
		if (rs->f_out>=0)
		{
			File_Close(rs->f_out);
			rs->f_out=-1;
		}
		// shutdown old device
		rs->switch_time = time(0);
		rs->out_device = DEVICE_NONE;
		rs->device_free_space = 0;
	}

	if (device>=0 && rs->out_device!=device)
	{
		// setup new device
		if (File_CheckPhysical(device)<0)
		{
			printf("New device not present\n");
			return false;
		}
		else
		{
			printf("mounted new device\n");
			File_SetDefault(device);
			default_path[0] = '\0';
			File_CreateDirPath("/rawk");
			if (File_CreateDirPath(discs[rs->disc].dump_path)>=0)
			{
				const struct dump_files *filelist = discs[rs->disc].rip_files;
				File_Chdir(discs[rs->disc].dump_path);
				int id = File_OpenPath("disc_id", O_CREAT|O_TRUNC|O_WRONLY);
				if (id>=0) {
					File_Write(id, discs[rs->disc].dump_path+5, strlen(discs[rs->disc].dump_path+5));
					File_Close(id);
				}
				for (int i=0;filelist[i].name!=NULL;i++)
				{
					if (filelist[i].type&RIP_DIR)
					{
						printf("creating dir %s\n", filelist[i].name);
						File_CreateDirPath(filelist[i].name);
					}
				}
				if ((rs->out_filename==NULL) || (rs->f_out=File_OpenPath(rs->out_filename, O_CREAT|O_TRUNC|O_WRONLY))>=0)
				{
					rs->out_device = device;
					rs->device_free_space = get_space(device, 0);
					// ignore the time taken to remount/prepare
					rs->start += time(0) - rs->switch_time;
					return true;
				}
			}
			printf("error setting up new device\n");
		}
	}

	return false;
}

u64 begin_rip(struct rip_state *rs,int disc)
{
	printf("begin_rip\n");
	if (rs==NULL)
		return 0;

	memset(rs, 0, sizeof(struct rip_state));
	rs->f_out = -1;
	rs->out_device = -1;
	rs->file_index = -1;
	rs->disc = disc;
	rs->disc_name = discs[disc].disc_name;
	rs->total = space_needed(rs, disc);
	rs->start = rs->switch_time = time(NULL);

	return rs->total;
}

bool get_next(struct rip_state *rs, int dirs_only=0)
{
	printf("get_next\n");
	int j;
	struct stat st;
	const char **excl = discs[rs->disc].exclusions;
	const struct dump_files *files = discs[rs->disc].rip_files;
	static char full_filename[MAX_PATH];
	char short_filename[MAX_PATH];

	if (rs->f_in)
	{
		fclose(rs->f_in);
		rs->f_in = NULL;
	}

	if (rs->f_out>=0)
	{
		File_Close(rs->f_out);
		rs->f_out = -1;
	}


	while (rs->d && !dirnext(rs->d, short_filename, &st))
	{
		printf("found %s\n", short_filename);
		if ((st.st_mode&S_IFREG) && !(st.st_mode&S_IFDIR)) {
			if (excl)
			{
				for (j=0;excl[j];j++)
				{
					if (strcasestr(short_filename, excl[j]))
						break;
				}
				if (excl[j])
				{
					printf("matched exclusion %s, skipping\n", excl[j]);
					continue;
				}
			}
			rs->bytes_in_current_file = st.st_size;
			rs->offset_in_current_file = 0;
			strncpy(rs->file_name, short_filename, 31);
			full_filename[MAX_PATH-1] = 0;
			snprintf(full_filename, MAX_PATH-1, "%s%s", rs->dir_name, short_filename);
			printf("opening %s for reading\n", full_filename);
			rs->f_in = fopen(full_filename, "rb");
			snprintf(full_filename, MAX_PATH-1, "%s/%s", files[rs->file_index].name, short_filename);
			printf("opening %s for writing\n", full_filename);
			rs->f_out = File_OpenPath(full_filename, O_CREAT|O_TRUNC|O_WRONLY);
			if (rs->f_in==NULL || rs->f_out<0)
				return false;
			rs->out_filename = full_filename;
			return true;
		}
	}

	if (rs->d)
	{
		dirclose(rs->d);
		rs->d = NULL;
	}

	rs->file_index++;

	if (files[rs->file_index].name==NULL)
		return false;

	printf("get_next %s\n", files[rs->file_index].name);

	if (files[rs->file_index].type&RIP_DIR)
	{
		rs->d = NULL;
		for(j=0;disc_prefix[j]!=NULL;j++)
		{
			sprintf(full_filename, "%s%s/", disc_prefix[j], files[rs->file_index].name);
			if ((rs->d=diropen(full_filename)))
				break;
		}
		if (rs->d==NULL)
		{
			if (!(files[rs->file_index].type&RIP_OPTIONAL))
				return false;
		}
		else
		{
			File_CreateDirPath(files[rs->file_index].name);
			if (files[rs->file_index].type&RIP_CREATE_ONLY || dirs_only)
			{
				printf("create directory\n");
				dirclose(rs->d);
				rs->d = NULL;
			}
			else
			{
				rs->dir_name[31] = 0;
				strncpy(rs->dir_name, full_filename, 31);
			}
		}
		return get_next(rs);
	}
	else if (dirs_only)
		return get_next(rs);

	for (j=0; disc_prefix[j]!=NULL; j++)
	{
		sprintf(full_filename, "%s%s", disc_prefix[j], files[rs->file_index].name);
		printf("attempting to stat %s\n", full_filename);
		if (stat(full_filename, &st)>=0 && (st.st_mode&S_IFREG))
		{
			printf("succeeded\n");
			break;
		}
		else
		{
			printf("failed\n");
			st.st_size=0;
		}
	}
	if (st.st_size==0)
	{
		if (files[rs->file_index].type&RIP_OPTIONAL)
			return get_next(rs);
		else
			return false;
	}
	rs->bytes_in_current_file = st.st_size;
	rs->offset_in_current_file = 0;
	rs->f_in = fopen(full_filename, "rb");
	rs->f_out = File_OpenPath(files[rs->file_index].name, O_CREAT|O_TRUNC|O_WRONLY);
	rs->out_filename = files[rs->file_index].name;
	strncpy(rs->file_name, strrchr(full_filename, '/')+1, 31);
	if (rs->f_in==NULL)
	{
		printf("Couldn't open input file\n");
		return false;
	}
	if (rs->f_out<0)
	{
		printf("Couldn't open output file\n");
		return false;
	}

	printf("Got next output file\n");
	return true;
}

s64 process_rip(struct rip_state *rs)
{
	//printf("process_rip\n");
	s64 readed;
	if (rs==NULL)
		return BEGIN_ERROR;

	if (rs->out_device==DEVICE_NONE)
	{
		printf("no output device!\n");
		return DEVICE_FULL;
	}
	else if (rs->f_out<0 && rs->out_filename)
	{
		printf("opening output file on new disc\n");
		rs->f_out = File_OpenPath(rs->out_filename, O_CREAT|O_TRUNC|O_WRONLY);
		if (rs->f_out<0)
			return WRITE_ERROR;
	}

	if (rs->offset_in_current_file >= rs->bytes_in_current_file)
	{
		if (!get_next(rs))
			return READ_ERROR;

#if 0 // can't happen, the device has already been checked to make sure it is big enough for everything
		rs->device_free_space = get_space(rs->out_device, 0);
		printf("free space: %llu, needed: %llu\n", rs->device_free_space, rs->bytes_in_current_file);

		if ((rs->bytes_in_current_file+MIN_FREE_SPACE) >= rs->device_free_space)
		{
			printf("Not enough space on current disc\n");
			if (rs->f_out>=0)
			{
				File_Close(rs->f_out);
				rs->f_out = -1;
				File_Delete(rs->out_filename);
			}
			prepare_device(rs, DEVICE_NONE);
			return DEVICE_FULL;
		}
#endif
	}

	readed=fread(copy_buf, 1, MIN(BUFFER_SIZE*2,rs->bytes_in_current_file-rs->offset_in_current_file), rs->f_in);
	if (readed>0)
	{
		int h, m, s;
		// modify .bik version
		if (rs->offset_in_current_file==0 && strcasestr(rs->file_name, ".bik") && !memcmp(copy_buf, "BIKi", 4))
			memcpy(copy_buf, "RAWK", 4);
		if (readed != File_Write(rs->f_out, copy_buf, readed))
		{
			printf("Write error\n");
			return WRITE_ERROR;
		}
		rs->current += readed;
		rs->offset_in_current_file += readed;
		rs->device_free_space -= readed;
		update_time(rs->total, rs->current, rs->start, &h, &m, &s);
		sprintf(rs->status_text, "\n\n%.29s %02d%%\n", rs->file_name, (u32)((u64)100*rs->offset_in_current_file/rs->bytes_in_current_file));
		sprintf(rs->status_text+strlen(rs->status_text), "%llu of %llu bytes copied.\n\nApproximately ", \
			rs->offset_in_current_file, rs->bytes_in_current_file);
		if (h>1)
			sprintf(rs->status_text+strlen(rs->status_text), "%d hours, ", h);
		else if (h)
			sprintf(rs->status_text+strlen(rs->status_text), "1 hour, ");
		if (m>1 || (h&&m!=1))
			sprintf(rs->status_text+strlen(rs->status_text), "%d minutes ", m);
		else if (m)
			sprintf(rs->status_text+strlen(rs->status_text), "1 minute ");
		else if (s!=1)
			sprintf(rs->status_text+strlen(rs->status_text), "%d seconds ", s);
		else
			sprintf(rs->status_text+strlen(rs->status_text), "1 second ");
		strcat(rs->status_text, "remaining.");
		return readed;
	}
	else
		return READ_ERROR;
}

void end_rip(struct rip_state *rs)
{
	printf("end_rip\n");
	if (rs==NULL)
		return;

	if (rs->d)
		dirclose(rs->d);

	if (rs->f_in)
		fclose(rs->f_in);

	if (rs->f_out>=0)
		File_Close(rs->f_out);
}

static struct rip_state rs;

MenuDump::MenuDump(GuiWindow *_Main) :
RawkMenu(NULL, "\nInitializing.....", "Reading Disc"),
Main(_Main)
{
	if (hId <0)
		hId = iosCreateHeap(BUFFER_SIZE*2+64);

	if (hId>=0 && copy_buf==NULL)
		copy_buf = iosAlloc(hId, BUFFER_SIZE*2);

	if (disk_subsequent_reset && default_mount>=0)
		WDVD_Reset();

	old_net_initted = net_initted;
	net_initted = 0;

	memset(&rs, 0, sizeof(rs));
	rs.f_out = -1;

	state = RIP_BEGIN;
}

static char space_error[150];
static const char *abort_messages[] = {
	"\nCan't rip discs: Some weird memory error has happened.",
	"\nThis disc isn't a recognized music disc or no disc is inserted.",
	"\nYou can't use songs from Rock Band 2 as DLC for Rock Band 2, that would be silly.",
	"\nThe Beatles: Rock Band is not supported by RawkSD.",
	"\nAn error occurred reading the disc.",
	"\nAn error occurred writing to the device.",
	"\nNo storage device was detected.",
	space_error
};

RawkMenu* MenuDump::Process()
{
	RawkMenu *next = NULL;
	switch (state) {
		case RIP_BEGIN: {
			int disc;
			if (copy_buf==NULL) {
				state = RIP_ABORT;
				msg_index = ABORT_MEM;
				break;
			}
			if (default_mount<0) {
				state = RIP_ABORT;
				msg_index = ABORT_NO_DEVICE;
				break;
			}
			disk_subsequent_reset = true;
			disc = get_disc();
			switch (disc) {
				case DISC_TBRB:
					state = RIP_ABORT;
					msg_index = ABORT_DISC_TBRB;
					break;
				case DISC_RB2:
					state = RIP_ABORT;
					msg_index = ABORT_DISC_RB2;
					break;
				case DISC_UNRECOGNIZED:
				case -1:
					state = RIP_ABORT;
					msg_index = ABORT_DISC_UNKNOWN;
					break;
				default:
					space = begin_rip(&rs, disc);
					if (space<=0) {
						state = RIP_ABORT;
						msg_index = ABORT_READ;
						break;
					}
					HaltGui();
					popup_text[0]->SetText(rs.disc_name);
					ResumeGui();
					state = RIP_NEED_DEVICE;
			}
			break;
		}
		case RIP_NEED_DEVICE:
			HaltGui();
			popup_text[1]->SetText("\nCalculating free space.....");
			ResumeGui();
			prepare_device(&rs, default_mount);
			if (rs.device_free_space < rs.total+MIN_FREE_SPACE) {
				sprintf(space_error, "\nThe current storage device only has %llu bytes free space, this disc requires %llu bytes free.", rs.device_free_space, rs.total+MIN_FREE_SPACE);
				state = RIP_ABORT;
				msg_index = ABORT_NO_SPACE;
			} else
				state = RIP_RIPPING;
			break;
		case RIP_RIPPING: {
			s64 bytes_read = process_rip(&rs);
			if (bytes_read>0) {
				space -= bytes_read;
				HaltGui();
				popup_text[1]->SetText(rs.status_text);
				ResumeGui();
			} else if (bytes_read<0) {
				state = RIP_ABORT;
				switch(bytes_read) {
					// device full shouldn't happen since the free space has been checked already...
					case DEVICE_FULL:
					case WRITE_ERROR:
						msg_index = ABORT_WRITE;
						break;
					//case BEGIN_ERROR:
					//case READ_ERROR:
					default:
						msg_index = ABORT_READ;
						break;
				}
				break;
			}
			if (space<=0)
				state = RIP_DONE;
			break;
		}
		case RIP_DONE:
			HaltGui();
			next = new MenuSaves(Main, "Ripping Completed", "\nAll files copied successfully.");
			// fallthrough
		case RIP_ABORT:
		default:
			if (next == NULL) {
				HaltGui();
				next = new MenuSaves(Main, "Ripping Aborted", abort_messages[msg_index]);
			}
		
			end_rip(&rs);
			close_disc();
			net_initted = old_net_initted;
			return next;
	}

	return this;
}