#include "haxx.h"
#include "launcher.h"
#include "wdvd.h"

#include <gccore.h>
#include <wiiuse/wpad.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <files.h>
#include <malloc.h>

using std::vector;

#define RIIFS_MYIP "192.168.1.113"
#define RIIFS_SERVERIP "67.214.140.23"
#define RIIFS_PORT 1137

#define printf(...)

extern "C" {
	extern u8 filemodule_dat[];
	extern u32 filemodule_dat_size;

	extern u8 dipmodule_rii_dat[];
	extern u32 dipmodule_rii_dat_size;

	extern u8 dipmodule_rawk_dat[];
	extern u32 dipmodule_rawk_dat_size;
}

#define dipmodule_dat dipmodule_rii_dat
#define dipmodule_dat_size dipmodule_rii_dat_size

static int load_module_code(void *module_code, s32 module_size);
static bool do_exploit();

int Haxx_Init()
{
	IOS_ReloadIOS(37);
	WPAD_Init();

	if (!do_exploit())
		return -1;
	
	usleep(1000);
	if (load_module_code(filemodule_dat, filemodule_dat_size) < 0)
		return -1;
	
	usleep(2000);
	if (load_module_code(dipmodule_dat, dipmodule_dat_size) < 0)
		return -1;
	
	usleep(1000);
	
	return 0;
}

#define DEFAULT() if (!hasdefault) { ret = File_SetDefault(ret); if (ret >= 0) hasdefault = true; }
vector<int> Haxx_Mount()
{
	vector<int> list;
	int fd = File_Init();
	if (fd < 0)
		return list;

	bool hasdefault = false;
	int ret;

	ret = File_Fat_Mount(SD_DISK, "sd");
	if (ret >= 0) {
		list.push_back(ret);
		DEFAULT();
	}

	ret = File_Fat_Mount(USB_DISK, "usb");
	if (ret >= 0) {
		list.push_back(ret);
		DEFAULT();
	}

	ret = File_RiiFS_Mount(RIIFS_SERVERIP, RIIFS_PORT);
	if (ret >= 0) {
		list.push_back(ret);
		DEFAULT();
	}

	return list;
}

extern "C" void udelay(int us);

#define HASH_CHECK_ADDRESS  (void*)0x93A752C0
#define ES_IOS_BOOT         (void*)0x939F02C8
#define ES_STACK_EXPLOIT    (void*)0x2011142C

#define MEM1_BASE           ((u32*)0x80000000)
#define MEM1_BASE_UNCACHED  ((u32*)0xC0000000)
#define MEM_PROT            ((u16*)0xCD8B420A)
#define MEM1_IOSVERSION     ((u32*)0xC0003140)

// our haxxed ios, IOS37v3870
#define HAX_IOS_VERSION     0x00250F1E

// the filename used to load modules
#define LOAD_MODULE_PATH    "/tmp/patch.bin"
#define LOAD_KERNEL_PATH    "/tmp/boot.bin"

#define SYS_CERTS_SIZE      2560
u8 sys_certs[2560] ATTRIBUTE_ALIGN(32);

// these are signature for fakesigning
const u8 tik_sig[260] = {
    0x00, 0x01, 0x00, 0x01, 0x36, 0xE3, 0xA4, 0xDC, 0x04, 0xDF, 0x21, 0xDF,
    0x65, 0xF2, 0xE3, 0x66, 0x0D, 0xEB, 0xFE, 0xAB, 0x46, 0x22, 0x59, 0x49,
    0x89, 0x10, 0x11, 0x6D, 0xE4, 0x85, 0x73, 0xB5, 0xA9, 0x38, 0x0B, 0xD3,
    0x2D, 0x4E, 0x87, 0xF4, 0x88, 0x03, 0xA1, 0x4A, 0x86, 0xE1, 0x5A, 0x75,
    0x98, 0x46, 0x2C, 0xCD, 0x3A, 0x32, 0x04, 0x9B, 0xCA, 0x61, 0xF6, 0xFC,
    0x6F, 0x8B, 0xFF, 0xA1, 0xC7, 0x24, 0x9E, 0x7D, 0x0C, 0xFC, 0xD5, 0xB8,
    0xD5, 0x3A, 0x24, 0xCD, 0x0C, 0xBF, 0x4D, 0x75, 0x4C, 0x5E, 0xD0, 0x61,
    0xD6, 0xE0, 0x4A, 0xB7, 0x4E, 0x9E, 0x0C, 0xBD, 0x89, 0x5E, 0xD0, 0x4A,
    0x96, 0x51, 0x53, 0x21, 0x07, 0xEB, 0xEC, 0xEB, 0xEF, 0x3D, 0xC3, 0x14,
    0x33, 0xFD, 0x42, 0x0A, 0x49, 0x1F, 0x51, 0x15, 0xAD, 0x71, 0x1C, 0xDA,
    0xD4, 0x8D, 0xA2, 0x96, 0x81, 0xEC, 0x06, 0x54, 0x64, 0x77, 0x5F, 0x56,
    0x57, 0x59, 0x06, 0xAB, 0x85, 0x5D, 0x9D, 0x8F, 0x81, 0x38, 0x01, 0xBE,
    0x3B, 0x39, 0x50, 0x75, 0x03, 0x28, 0x09, 0x3F, 0x55, 0x45, 0x3E, 0xCC,
    0xEE, 0xE6, 0x51, 0xE3, 0x67, 0xED, 0x12, 0xAA, 0x6B, 0xD8, 0xEF, 0xA6,
    0x04, 0x9D, 0x3C, 0x81, 0xC3, 0x59, 0x85, 0x84, 0x61, 0xCB, 0x46, 0xDE,
    0x57, 0xF0, 0xCB, 0xD0, 0x1F, 0xBE, 0xD2, 0xE5, 0xFD, 0x98, 0x1B, 0x20,
    0x17, 0x1C, 0xD3, 0xCC, 0x09, 0x62, 0x10, 0x9B, 0xEB, 0x89, 0xC8, 0x3B,
    0x0A, 0x2B, 0x46, 0x78, 0xA0, 0x6D, 0x97, 0x97, 0x79, 0xD3, 0xE5, 0x4C,
    0x6A, 0xC1, 0x43, 0xCD, 0xD3, 0xB5, 0xE3, 0xA1, 0xEB, 0x37, 0x7F, 0x7E,
    0xDC, 0x20, 0xFB, 0x19, 0xFA, 0xA6, 0x12, 0x28, 0x56, 0x32, 0x48, 0x6D,
    0x20, 0xB7, 0x52, 0x62, 0x78, 0xA3, 0xF2, 0xB1, 0x82, 0x76, 0x78, 0xC5,
    0x3D, 0x33, 0xA2, 0xCA, 0xBB, 0xCD, 0x50, 0xA5
};
const u8 tmd_sig[260] = {
    0x00, 0x01, 0x00, 0x01, 0x11, 0x51, 0x02, 0x93, 0x00, 0x8C, 0x45, 0xEA,
    0x43, 0xB6, 0x64, 0x35, 0x83, 0x37, 0x38, 0x3A, 0x84, 0x99, 0xF5, 0xA5,
    0x3F, 0xA2, 0x3A, 0xBC, 0xCD, 0x9D, 0x42, 0x4C, 0x8C, 0xAD, 0x6B, 0x7E,
    0x0A, 0x48, 0x1D, 0xEE, 0x13, 0x5B, 0xD8, 0x2A, 0x1C, 0xF7, 0x4E, 0x4C,
    0x5A, 0x9A, 0x69, 0x74, 0x61, 0x44, 0xFE, 0x69, 0x81, 0x99, 0xD7, 0x39,
    0x34, 0xCC, 0xFA, 0x16, 0x09, 0xA6, 0x6F, 0x0D, 0xBD, 0xB0, 0x74, 0x12,
    0x1D, 0x90, 0x2B, 0x8A, 0xB9, 0x49, 0x9E, 0x0A, 0x82, 0x56, 0xEF, 0x8E,
    0xE3, 0x65, 0xE4, 0xFE, 0xFB, 0xFD, 0x63, 0x06, 0x1E, 0xBC, 0xFB, 0x9D,
    0xC5, 0x98, 0x43, 0xDC, 0xFB, 0xBB, 0xC9, 0x3F, 0x52, 0xB9, 0x4A, 0x73,
    0x75, 0x06, 0x55, 0x96, 0x43, 0x30, 0x8A, 0xC6, 0x40, 0x53, 0x8C, 0x2F,
    0xCC, 0x40, 0xF1, 0x89, 0x63, 0x8C, 0x1D, 0x33, 0xE6, 0x50, 0xE9, 0x3C,
    0x25, 0x19, 0xB9, 0x47, 0x80, 0x26, 0x33, 0xB9, 0xCB, 0x8C, 0xDD, 0x3A,
    0xD1, 0x27, 0x84, 0x58, 0xD0, 0xAD, 0xCA, 0xA3, 0x85, 0xF4, 0x7E, 0x35,
    0x8D, 0x63, 0x8C, 0x04, 0xD0, 0x13, 0xA5, 0xAC, 0x12, 0x5C, 0xBB, 0x9B,
    0x8B, 0xCE, 0xCB, 0xA7, 0x07, 0x11, 0x0B, 0x84, 0xC0, 0x8D, 0x84, 0x41,
    0x13, 0xE7, 0x45, 0xCB, 0xC9, 0xDB, 0x2A, 0x35, 0x01, 0xA4, 0xBE, 0xB4,
    0xF8, 0x3A, 0x01, 0x41, 0xA8, 0xC3, 0x0C, 0x9D, 0x83, 0xA8, 0xC3, 0x26,
    0x82, 0x09, 0x44, 0x21, 0x11, 0xED, 0x05, 0x52, 0x74, 0xDE, 0x88, 0xAF,
    0x0A, 0xDD, 0xCC, 0xBC, 0x21, 0x1C, 0xA9, 0x0C, 0xF5, 0xC7, 0xD8, 0x42,
    0x99, 0x9A, 0xFC, 0xDA, 0x24, 0x84, 0xA2, 0xD4, 0x12, 0xE8, 0x3D, 0x32,
    0xCC, 0x30, 0x5E, 0xD8, 0xD9, 0xDD, 0x01, 0x83, 0x81, 0x12, 0x65, 0x1E,
    0xDD, 0x58, 0x5D, 0x63, 0x7F, 0x91, 0x91, 0x4C
};

// and these are the matching hash values. Make sure the forged hash starts with
// zero and matches at least one other byte from these hashes
const u8 tik_hash[20] = {
    0x00, 0x27, 0x30, 0x1A, 0x7B, 0x6D, 0x86, 0x3B, 0x9D, 0xF2,
    0xC0, 0xAE, 0x5C, 0x6D, 0xDB, 0xA2, 0x8D, 0xE8, 0xBF, 0xC5
};
const u8 tmd_hash[20] = {
    0x00, 0xBE, 0x9B, 0x52, 0x7E, 0xB4, 0x28, 0x88, 0xAC, 0xFC,
    0xE4, 0x77, 0x58, 0x16, 0x50, 0x5E, 0xCC, 0xCD, 0x3B, 0x16
};

u8 old_hashcmp[] = {0x24, 0x01, 0x78, 0x3A, 0x37, 0x01, 0x78, 0x2B, 0x35, 0x01,
                    0x06, 0x1b, 0x06, 0x12, 0x42, 0x9a, 0xd1, 0x01, 0x40, 0x20, 0xe0, 0x00,
                    0x20, 0x00, 0x31, 0x01, 0x29, 0x13, 0xd9, 0xf1, 0x28, 0x00, 0xd0, 0x01};
u8 new_hashcmp[] = {
                0x24, 0x00, // movs r4, #0
/* 13A752C2 */  0x78, 0x3A, // ldrb r2, [r7]
                0x37, 0x01, // adds r7, #1
                0x78, 0x2B, // ldrb r3, [r5]
                0x35, 0x01, // adds r5, #1
                0x42, 0x9A, // cmp r2, r3
                0xd1, 0x00, // bne loc_13A752D0
                0x34, 0x01, // adds r4, #1
/* 13A752d0 */  0x31, 0x01, // adds r1, #1
                0x29, 0x13, // cmp r1, #0x13
                0xd9, 0xf5, // bls loc_13A752C2
/* 13A752D6 */  0x28, 0x00, // cmp r0, #0
/* 13A752D8 */  0xd0, 0x05, // beq loc_13A752E6
                0x2C, 0x01, // cmp r4, #1
                0xd9, 0x03, // bls loc_13A752E6
                0x46, 0xC0, // nop
                0x46, 0xC0, // nop


/* 13A752E6 */
            };

// simple 2 byte patches
enum {
    MEM2_INDEX,
    DI_VERIFY_INDEX,
    NAND_PERMS_INDEX,
};

static const struct {
    u16 *address;
    u16 old_value;
    u16 new_value;
} patches[] = {
    {MEM_PROT, 1, 2},
    {(u16*)0x939F0D4A, 0xD123, 0x46C0},
    {(u16*)0x93A112F2, 0xD001, 0xE001}
};

typedef struct _cmap_entry
{
    char name[8];
    sha1 hash;
} __attribute__((packed)) cmap_entry;

static int do_patch(int i)
{
	u16 *patch;
	if (i<0 || i > (int)(sizeof(patches)/sizeof(patches[0])))
		return 0;

	patch = patches[i].address;

	if (*patch == patches[i].old_value)
	{
		*patch = patches[i].new_value;
		if ((u32)patch < 0xC0000000)
			DCFlushRange((void*)((u32)patch&~0x1F), 32);
		return 1;
	}
	return 0;
}

static int get_certs()
{
	s32 fd = IOS_Open("/sys/cert.sys", ISFS_OPEN_READ);
	if (fd<0)
		return 0;

	if (IOS_Read(fd, sys_certs, SYS_CERTS_SIZE) != SYS_CERTS_SIZE)
		return 0;

	IOS_Close(fd);
	return 1;
}

static const u16 ios_boot[] =
{
	0x68CE, // ldr r6, [r1,#0xc] 	// ipcmessage.vec
	0x6830, // ldr r0, [r6]  		// vec[0].data (filename of kernel)
	0x2100, // movs r1, #0
	0x4A01, // ldr r2, =0x00250F1E (new ios version, IOS37v3870)
	0x4B01, // ldr r3, =0x2010A960+1 (es_syscall_ios_boot)
	0x4798, // blx r3

	(u16)(HAX_IOS_VERSION>>16), // ios_version
	(u16)HAX_IOS_VERSION,
	0x2010, // es_syscall_ios_boot
	0xA960+1,
};

static const u16 load_module[] =
{
	0x68CE, // ldr r6, [r1, #0x0c]	// ipcmessage.vec
	0x6830, // ldr r0, [r6]			// vec[0].data (filename of module)
	0x4B01, // ldr r3, =0x2010A898+1 (es_syscall_load_module)
	0x4798, // blx r3
	0x4B01, // ldr r3, =0x20101060+1 (end of handle_es_ioctlv)
	0x4798, // blx r3

	0x2010, // es_syscall_load_module
	0xA898+1,
	0x2010, // finish_handle_es_ioctlv
	0x1060+1,
};

static const u32 ios_ret[] =
{
	0xE3A00001, // mov r0, #1
	0xE6000A90, // syscall_54 (give ppc full hw access)
	0xE3A00065, // mov r0, #101 (so we know it worked)
	0xE59F2000, // ldr r2, return address in es
	0xE12FFF12, // bx r2
	0x20107084+1, // ret_address
};

int disable_mem2_protection(s32 fd)
{
	u64 title = 0x0000000100000025llu;
	ioctlv vec[3];
	u32 cnt;
	u8 lowmem_save[0x20];
	s32 ret;

	vec[0].data = &title;
	vec[0].len = sizeof(title);
	vec[1].data = &cnt;
	vec[1].len = sizeof(cnt);
	ret = IOS_Ioctlv(fd, 0x12, 1, 1, vec); // ES_GetNumTicketViews
	if (ret)
	{
		printf("Couldn't get NumTicketViews\n");
		return 0;
	}
	if (cnt != 1)
	{
		printf("Wrong TicketView count (%d)\n", cnt);
		return 0;
	}

	// lowmem values aren't really important, but keep them anyway
	memcpy(lowmem_save, MEM1_BASE, sizeof(lowmem_save));
	memcpy(MEM1_BASE_UNCACHED, ios_ret, sizeof(ios_ret));

	vec[2].data = ES_STACK_EXPLOIT; // somewhere in ES's stack. This value works but was guessed, I have no idea what it's trashing.
	vec[2].len = 0x0;
	cnt = 0x20000000; // 0x20000000 * 0xD8 = 0x1B00000000 = 0
	ret = IOS_Ioctlv(fd, 0x13, 2, 1, vec); // ES_GetTicketViews

	// restore lowmem values
	memcpy(MEM1_BASE, lowmem_save, sizeof(lowmem_save));

	if (ret != 101) // exploit failed
	{
		printf("Possible incorrect ES version, failed\n");
		return 0;
	}

	if (!do_patch(MEM2_INDEX))
	{
		printf("Couldn't patch MEM2 protection\n");
		return 0;
	}

	return 1;
}

// this function will only work once after reloading IOS, so use sparingly
static int identify_as_title(u64 title)
{
	s32 fd;
	s32 size;
	u32 *tmd_blob = NULL;
	tmd *title_tmd;
	u32 *tik_blob = NULL;
	char filename[65];
	u32 junk;

	if (!get_certs())
		return 0;

	sprintf(filename, "/title/%08x/%08x/content/title.tmd", (u32)(title>>32), (u32)title);
	fd = IOS_Open(filename, 1);
	if (fd<0)
		return 0;

	size = IOS_Seek(fd, 0, SEEK_END);

	tmd_blob = (u32*)memalign(32, size+32);
	if (tmd_blob==NULL)
		return 0;
	IOS_Seek(fd, 0, 0);
	IOS_Read(fd, tmd_blob, size);

	title_tmd = (tmd*)SIGNATURE_PAYLOAD(tmd_blob);
	IOS_Close(fd);

	sprintf(filename, "/ticket/%08x/%08x.tik", (u32)(title>>32), (u32)title);
	fd = IOS_Open(filename, 1);
	if (fd >= 0)
	{
		size = IOS_Seek(fd, 0, SEEK_END);
		tik_blob = (u32*)memalign(32, size+32);
		if (tik_blob==NULL)
		{
			IOS_Close(fd);
			free(tmd_blob);
			return 0;
		}
		IOS_Seek(fd, 0, 0);
		IOS_Read(fd, tik_blob, size);
		IOS_Close(fd);
	}
	else
	{
		free(tmd_blob);
		return 0;
	}

	fd = ES_Identify((signed_blob*)sys_certs, SYS_CERTS_SIZE, (signed_blob*)tmd_blob, SIGNED_TMD_SIZE(tmd_blob), (signed_blob*)tik_blob, SIGNED_TIK_SIZE(tik_blob), &junk);

	free(tmd_blob);
	free(tik_blob);

	return !fd;
}

static int patch_mem2(void* buf, s32 size)
{
	u32 i, count = 0;
	u8 *kernel = (u8*)buf;
	const u8 old_table[] = {0xB5,0x00,0x4B,0x09,0x22,0x01,0x80,0x1A,0x22,0xF0};
	const u8 new_table[] = {0xB5,0x00,0x4B,0x09,0x22,0x00,0x80,0x1A,0x22,0xF0};

	for (i=0; i < size - sizeof(old_table); i++)
	{
		if (!memcmp(kernel+i, old_table, sizeof(old_table)))
		{
			memcpy(kernel+i, new_table, sizeof(new_table));
			i += sizeof(new_table);
			count++;
		}
	}

	return count;
}

static int patch_ios37_sd_load(void* buf, s32 size)
{
	u32 i, count = 0;
	u8 *kernel = (u8*)buf;
	const u8 sd_old[] = {0x22, 0xf4, 0x00, 0x52, 0x18, 0x81, 0x27, 0xf0, 0x00, 0x7f, 0x19, 0xf3, 0x88, 0x0a, 0x88, 0x1b, 0x42, 0x9a};
	const u8 sd_new[] = {0x22, 0xf4, 0x00, 0x52, 0x18, 0x81, 0x27, 0xf0, 0x00, 0x7f, 0x19, 0xf3, 0x88, 0x0a, 0x88, 0x1b, 0x2A, 0x04};

	for (i=0; i < size - sizeof(sd_old); i++)
	{
		if (!memcmp(kernel+i, sd_old, sizeof(sd_old)))
		{
			memcpy(kernel+i, sd_new, sizeof(sd_new));
			i += sizeof(sd_new);
			count++;
		}
	}

	return count;
}

static int prepare_new_kernel(u64 title)
{
	s32 fd;
	s32 size, readed;
	u32 *tmd_blob = NULL;
	tmd *title_tmd;
	char filename[65];
	void *kernel_blob;

	sprintf(filename, "/title/%08x/%08x/content/title.tmd", (u32)(title>>32), (u32)title);
	fd = IOS_Open(filename, 1);
	if (fd<0)
	{
		printf("Couldn't open TMD\n");
		return 0;
	}

	size = IOS_Seek(fd, 0, SEEK_END);

	tmd_blob = (u32*)memalign(32, size+32);
	if (tmd_blob==NULL)
	{
		printf("Couldn't allocate TMD blob\n");
		return 0;
	}
	IOS_Seek(fd, 0, 0);
	readed = IOS_Read(fd, tmd_blob, size);
	IOS_Close(fd);
	if (readed != size)
	{
		printf("Couldn't read TMD\n");
		free(tmd_blob);
		return 0;
	}

	title_tmd = (tmd*)SIGNATURE_PAYLOAD(tmd_blob);

	fd = ES_OpenContent(title_tmd->boot_index);
	free(tmd_blob);
	if (fd<0)
	{
		printf("Couldn't read boot index content\n");
		return 0;
	}

	size = ES_SeekContent(fd, 0, SEEK_END);
	ES_SeekContent(fd, 0, 0);

	kernel_blob = memalign(32, size+32);
	if (kernel_blob==NULL)
	{
		printf("Couldn't allocate kernel blob\n");
		ES_CloseContent(fd);
		return 0;
	}

	readed = ES_ReadContent(fd, (u8*)kernel_blob, size);
	ES_CloseContent(fd);
	if (readed != size)
	{
		printf("Couldn't read kernel content\n");
		free(kernel_blob);
		return 0;
	}

	if (!patch_mem2(kernel_blob, size) || !patch_ios37_sd_load(kernel_blob, size))
	{
		printf("Couldn't patch kernel\n");
		free(kernel_blob);
		return 0;
	}

	ISFS_Initialize();
	ISFS_CreateFile(LOAD_KERNEL_PATH, 0, 3, 1, 1);
	fd = IOS_Open(LOAD_KERNEL_PATH, ISFS_OPEN_WRITE);
	if (fd<0)
	{
		printf("Couldn't open temp file\n");
		free(kernel_blob);
		return 0;
	}

	readed = IOS_Write(fd, kernel_blob, size);
	IOS_Close(fd);
	free(kernel_blob);
	if (readed != size)
	{
		printf("Couldn't write temp file\n");
		return 0;
	}

	return 1;
}

// remember any MEM2 data may be invalid after reloading IOS
static void shutdown_for_reload()
{
	ISFS_Deinitialize();
	WPAD_Shutdown();
	__IOS_ShutdownSubsystems();
	// change ios version in lowmem so we know when the new one has loaded
	*MEM1_IOSVERSION = 0x00050000;
}

static void load_patched_ios(s32 fd, const char* filename)
{
	ioctlv vec;
	// the data being copied over doesn't matter since we're loading
	// a new IOS
	memcpy(ES_IOS_BOOT, ios_boot, sizeof(ios_boot));
	DCFlushRange((void*)((u32)ES_IOS_BOOT&~0x1F), 32);
	vec.data = (void*)filename;
	vec.len = strlen(filename)+1;
	IOS_IoctlvRebootBackground(fd, 0x1F, 1, 0, &vec);
}

static int load_module_file(s32 fd, const char *filename)
{
	s32 ret;
	ioctlv vec;
	u8 old_es_code[20];

	// store the old code that will be overwritten
	memcpy(old_es_code, ES_IOS_BOOT, sizeof(old_es_code));
	memcpy(ES_IOS_BOOT, load_module, sizeof(load_module));
	DCFlushRange((void*)((u32)ES_IOS_BOOT&~0x1F), 32);
	vec.data = (void*)filename;
	vec.len = strlen(filename)+1;
	ret = IOS_Ioctlv(fd, 0x1F, 1, 0, &vec);

	// restore the old code and flush
	memcpy(ES_IOS_BOOT, old_es_code, sizeof(old_es_code));
	DCFlushRange((void*)((u32)ES_IOS_BOOT&~0x1F), 32);
	return !ret;
}


static void recover_from_reload(s32 version)
{
	s32 newversion;
	int retries;
	raw_irq_handler_t irq_handler;
	// Mask IPC IRQ while we're busy reloading
	__MaskIrq(IRQ_PI_ACR);
	irq_handler = IRQ_Free(IRQ_PI_ACR);

	// Wait for old IOS to change version number before reloading
	for (retries = 0; retries < 10; retries++)
	{
		newversion = IOS_GetVersion();
		if (newversion != version)
			udelay(1000);
		else
			break;
	}

	// Wait for new IOS to signal IPC is ready
	for (retries = 0; retries < 10; retries++)
	{
		if(IPC_ReadReg(1) & 2)
			break;
		udelay(3000);
	}

	IRQ_Request(IRQ_PI_ACR, irq_handler, NULL);
	__UnmaskIrq(IRQ_PI_ACR);

	__IPC_Reinitialize();

	__IOS_InitializeSubsystems();
	WPAD_Init();
}

static int load_module_code(void *module_code, s32 module_size)
{
	s32 fd;
	s32 written;
	s32 es_fd;

	es_fd = IOS_Open("/dev/es", 0);
	if (es_fd<0)
		return 0;

	ISFS_Initialize();
	ISFS_CreateFile(LOAD_MODULE_PATH, 0, 3, 1, 1);
	fd = IOS_Open(LOAD_MODULE_PATH, ISFS_OPEN_WRITE);
	if (fd<0)
	{
		ISFS_Delete(LOAD_MODULE_PATH);
		IOS_Close(es_fd);
		return 0;
	}

	// slight issue here since ISFS always rounds up to 32-byte chunks
	// but it shouldn't hurt anything...
	written = IOS_Write(fd, module_code, module_size);
	IOS_Close(fd);
	fd = 0;
	if (written == module_size)
		fd = load_module_file(es_fd, LOAD_MODULE_PATH);

	IOS_Close(es_fd);
	ISFS_Delete(LOAD_MODULE_PATH);
	return fd;
}

static int load_sdhc_module(s32 es_fd)
{
	s32 fd;
	s32 size, readed;
	u32 *tmd_blob = NULL;
	tmd *title_tmd;
	u8 *tik_blob;
	char filename[65];
	u64 title = 0x0000000100000038llu; // IOS56
	u8 *module_buf = NULL;
	u32 i, j;

	sprintf(filename, "/title/%08x/%08x/content/title.tmd", (u32)(title>>32), (u32)title);
	fd = IOS_Open(filename, 1);
	if (fd<0)
	{
		printf("Couldn't open TMD for SDHC\n");
		return 0;
	}

	size = IOS_Seek(fd, 0, SEEK_END);

	tmd_blob = (u32*)memalign(32, size+32);
	if (tmd_blob==NULL)
	{
		printf("Couldn't allocate TMD blob for SDHC\n");
		return 0;
	}
	IOS_Seek(fd, 0, 0);
	readed = IOS_Read(fd, tmd_blob, size);
	IOS_Close(fd);
	if (readed != size)
	{
		printf("Couldn't read TMD\n");
		free(tmd_blob);
		return 0;
	}

	title_tmd = (tmd*)SIGNATURE_PAYLOAD(tmd_blob);

	sprintf(filename, "/ticket/%08x/%08x.tik", (u32)(title>>32), (u32)title);
	fd = IOS_Open(filename, 1);
	if (fd >= 0)
	{
		size = IOS_Seek(fd, 0, SEEK_END);
		tik_blob = (u8*)memalign(32, size+32);
		if (tik_blob==NULL)
		{
			IOS_Close(fd);
			free(tmd_blob);
			return 0;
		}
		IOS_Seek(fd, 0, 0);
		readed = IOS_Read(fd, tik_blob, size);
		IOS_Close(fd);
		if (readed != size)
		{
			free(tmd_blob);
			free(tik_blob);
			return 0;
		}
	}
	else
	{
		free(tmd_blob);
		return 0;
	}

	// check signature
	free(tik_blob);

	fd = IOS_Open("/shared1/content.map", ISFS_OPEN_READ);
	if (fd>=0)
	{
		u8 *content_map;
		u32 content_map_entries=0;

		size = IOS_Seek(fd, 0, SEEK_END);
		IOS_Seek(fd, 0, 0);
		content_map = (u8*)memalign(32, size+32);
		if (content_map==NULL)
		{
			free(tmd_blob);
			IOS_Close(fd);
			return 0;
		}
		readed = IOS_Read(fd, content_map, size);
		IOS_Close(fd);
		if (readed != size)
		{
			free(content_map);
			free(tmd_blob);
			return 0;
		}
		content_map_entries = size / sizeof(cmap_entry);

		for (i=0; i < title_tmd->num_contents; i++)
		{
			int found=0;
			s32 content_fd;
			if (title_tmd->contents[i].type & 0x8000)
			{
				// shared
				filename[0] = 0;
				for (j=0; j < content_map_entries; j++)
				{
					cmap_entry *shared = (cmap_entry*)(content_map+j*sizeof(cmap_entry));
					if (!memcmp(shared->hash, title_tmd->contents[i].hash, sizeof(sha1)))
					{
						strcpy(filename, "/shared1/");
						strncat(filename, shared->name, 8);
						strcat(filename, ".app");
						break;
					}
				}
			}
			else
				sprintf(filename, "/title/%08x/%08x/%08x.app", (u32)(title>>32), (u32)title, title_tmd->contents[i].cid);

			content_fd = IOS_Open(filename, ISFS_OPEN_READ);
			if (content_fd<0)
				continue;
			size = IOS_Seek(content_fd, 0, SEEK_END);
			if (size<=0)
			{
				IOS_Close(content_fd);
				continue;
			}
			IOS_Seek(content_fd, 0, 0);

			module_buf = (u8*)memalign(32, size+32);
			if (module_buf==NULL)
			{
				IOS_Close(content_fd);
				continue;
			}
			readed = IOS_Read(content_fd, module_buf, size);
			IOS_Close(content_fd);

			if (readed == size)
			{
				char sd_version[] = "$IOSVersion: SDI:";
				for (j=0; j < size - sizeof(sd_version)-1; j++)
				{
					if (!memcmp(module_buf + j, sd_version, sizeof(sd_version)-1))
					{
						found = 1;
						break;
					}
				}
			}

			if (found)
				break;

			free(module_buf);
			module_buf = NULL;
		}

		free(content_map);
	}

	free(tmd_blob);

	if (module_buf==NULL)
	{
		printf("Couldn't find IOS56 SD Module\n");
		return 0;
	}

	fd = load_module_code(module_buf, size);
	free(module_buf);
	return fd;
}

static int do_sig_check_patch()
{
	if (memcmp(HASH_CHECK_ADDRESS, old_hashcmp, sizeof(old_hashcmp)) && memcmp(HASH_CHECK_ADDRESS, new_hashcmp, sizeof(new_hashcmp)))
		return 0;

	memcpy(HASH_CHECK_ADDRESS, new_hashcmp, sizeof(new_hashcmp));
	DCFlushRange(HASH_CHECK_ADDRESS, 64);

	return 1;
}

static bool do_exploit()
{
	s32 es_fd = -1;
	const u64 ios_title = 0x0000000100000025llu;

	if (IOS_GetVersion() != 37 || IOS_GetRevision() != 3869)
	{
		printf("Wrong IOS version. Update IOS37 to the latest version.\n");
		return false;
	}

	es_fd = IOS_Open("/dev/es", 0);
	if (es_fd<0)
	{
		printf("Couldn't open /dev/es (%d)\n", es_fd);
		return false;
	}

	if (disable_mem2_protection(es_fd))
	{
		int patch_failed = 0;
		printf("MEM2 protection disabled\n");

		// make sure signature check isn't patched
		*(u16*)0x93A752E6 = 0x2007;
		DCFlushRange((void*)0x93A752E0, 32);

		if (!do_patch(DI_VERIFY_INDEX))
		{
			patch_failed = 1;
			printf("DI Verify patch failed\n");
		}
		if (!do_patch(NAND_PERMS_INDEX))
		{
			patch_failed = 1;
			printf("NAND Permissions patch failed\n");
		}

		if (!patch_failed)
		{
			patch_failed = !identify_as_title(ios_title);
			if (patch_failed)
				printf("Failed to identify as title %08x%08x\n", (u32)(ios_title>>32), (u32)ios_title);
		}

		if (!patch_failed)
		{
			patch_failed = !prepare_new_kernel(ios_title);
			if (patch_failed)
				printf("Failed to prepare new kernel\n");
		}

		if (!patch_failed)
		{
			shutdown_for_reload();
			load_patched_ios(es_fd, LOAD_KERNEL_PATH);
			es_fd = 0;
			recover_from_reload(HAX_IOS_VERSION);
			if (IOS_GetVersion() != (HAX_IOS_VERSION>>16) || IOS_GetRevision() != (HAX_IOS_VERSION&0xFFFF))
				patch_failed = 1;
		}

		if (!patch_failed)
			patch_failed = !do_patch(NAND_PERMS_INDEX);

		if (!patch_failed)
		{
			patch_failed = 1;
			es_fd = IOS_Open("/dev/es", 0);
			if (es_fd>=0)
				patch_failed = !load_sdhc_module(es_fd);
		}

		if (!patch_failed)
			patch_failed = !do_sig_check_patch();

		if (!patch_failed)
			printf("All patching done!\n");
	}

	if (es_fd >= 0)
		IOS_Close(es_fd);

	return true;
}
