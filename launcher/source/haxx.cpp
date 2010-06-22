#include "haxx.h"
#include "launcher.h"
#include "wdvd.h"
#include "sha1.h"

#include <gccore.h>
#include <wiiuse/wpad.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <files.h>
#include <malloc.h>

using std::vector;

#define printf(...)

extern "C" {
	extern u8 filemodule_dat[];
	extern u32 filemodule_dat_size;

	extern u8 dipmodule_dat[];
	extern u32 dipmodule_dat_size;

	extern const u8 root_dat[];
}

extern vector<int> ToMount;

// our haxxed ios, IOS37v3870
#ifndef YARR
#define HAXX_IOS_VERSION     0x00250F1E
#else
#undef HAXX_IOS
#define HAXX_IOS_VERSION     0x00890F1E
#define HAXX_IOS 0x0000000100000089ULL
#endif

static int load_module_code(void *module_code, s32 module_size);
static bool do_exploit();

int Haxx_Init()
{
	IOS_ReloadIOS((u32)HAXX_IOS);

	if (!do_exploit())
		return -1;

	usleep(4000);
	if (load_module_code(filemodule_dat, filemodule_dat_size) < 0)
		return -1;

	usleep(4000);
	if (load_module_code(dipmodule_dat, dipmodule_dat_size) < 0)
		return -1;

	usleep(4000);

#if 0
	WPAD_Init();
	printf("Press home to exit\n");
	while(1) {

		// Call WPAD_ScanPads each loop, this reads the latest controller states
		WPAD_ScanPads();

		// WPAD_ButtonsDown tells us which buttons were pressed in this loop
		// this is a "one shot" state which will not fire again until the button has been released
		u32 pressed = WPAD_ButtonsDown(0)|WPAD_ButtonsDown(1);

		// We return to the launcher application via exit
		if ( pressed & WPAD_BUTTON_HOME ) exit(0);

		// Wait for the next frame
		VIDEO_WaitVSync();
	}
#endif

	return 0;
}

// this function probably shouldn't be here
#define DEFAULT() if (!hasdefault) { ret = File_SetDefault(ret); if (ret >= 0) hasdefault = true; }
void Haxx_Mount(vector<int>* mounted)
{
	int fd = File_Init();
	if (fd < 0)
		return;

	bool hasdefault = false;
	int ret;

	ret = File_Fat_Mount(SD_DISK, "sd");
	if (ret >= 0) {
		mounted->push_back(ret);
		DEFAULT();
	}

	ret = File_Fat_Mount(USB_DISK, "usb");
	if (ret >= 0) {
		mounted->push_back(ret);
		DEFAULT();
	}

	ret = File_RiiFS_Mount("", 0);
	if (ret >= 0) {
		mounted->push_back(ret);
		ToMount.push_back(ret);
		if (!hasdefault)
			File_SetLogFS(ret);
		
		DEFAULT();
	}
}

extern "C" void udelay(int us);

#define HASH_CHECK_ADDRESS  (void*)0x93A752C0
#define ES_IOS_BOOT         (void*)0x939F02C8
#define ES_STACK_EXPLOIT    (void*)0x2011142C
#define ES_MODULE_START     ((u32*)0x939F0000)
#define ES_MODULE_SIZE      0x20000
#define SYSCALL_DEVICE_OPEN 0xE6000390

#define MEM1_BASE           ((u32*)0x80000000)
#define MEM1_BASE_UNCACHED  ((u32*)0xC0000000)
#define MEM_PROT            ((u16*)0xCD8B420A)
#define MEM1_IOSVERSION     ((u32*)0xC0003140)
#define OTP_COMMAND			((volatile u32*)0xCD8001EC)
#define OTP_DATA			((volatile u32*)0xCD8001F0)

// the filename used to load modules
static const char LOAD_MODULE_PATH[] ATTRIBUTE_ALIGN(32) = "/tmp/patch.bin";
static const char LOAD_KERNEL_PATH[] ATTRIBUTE_ALIGN(32) = "/tmp/boot.bin";

u8 sys_certs[SYS_CERTS_SIZE] ATTRIBUTE_ALIGN(32);

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

static const u8 old_hashcmp[] = {0x24, 0x01, 0x78, 0x3A, 0x37, 0x01, 0x78, 0x2B, 0x35, 0x01,
                    0x06, 0x1b, 0x06, 0x12, 0x42, 0x9a, 0xd1, 0x01, 0x40, 0x20, 0xe0, 0x00,
                    0x20, 0x00, 0x31, 0x01, 0x29, 0x13, 0xd9, 0xf1, 0x28, 0x00, 0xd0, 0x01};
static const u8 new_hashcmp[] = {
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

// 1st word = value to search, 2nd word = mask of bits to disregard
static const s16 old_dev_fs_main[] = {
	0xD009, 0,
	0x68A0, 0,
	0xF7FF, 0x07FF, 0xF9CF, 0xFFFF,
	0x2800, 0,
	0xD104, 0,
	0x1C20, 0,
	0xF7FF, 0x07FF, 0xFA84, 0xFFFF,
	0x1C01, 0,
	0xE038, 0x00FF,
	0x6823, 0,
	0x2B01, 0,
	0xD009, 0x00FF,
	0x68A0, 0,
	0xF7FA, 0x07FF, 0xFBEA, 0xFFFF,
	0x2800, 0,
	0xD104, 0
};

// not used directly, here for reference purposes
static const u16 new_dev_fs_main[] = {
/* 20005E82 */ 0xD10C,     // bne 20005E9E (+3)
               0x6823,     // ldr r3, [r4] (ipcmsg)
/* 20005E86 */ 0xF7FF,     // bl open_dev_flash    (unmodified)
               0xF9B9,     // check_fd_is_flash-0x16 (-22)
               0x2800,     // cmp r0, #0           (unmodified)
/* 20005E8C */ 0xD103,     // bne 20005E96 (-1)
/* 20005E8E */ 0xE005,     // b 200005E9C
               0x0000,     // unreachable          (unmodified)
               0x0000,     // unreachable          (unmodified)
               0x1C01,     // unreachable          (unmodified)
/* 20005E96 */ 0xE038,     // b 20005F0A           (unmodified)
               0x6823,     // unreachable          (unmodified)
               0x2B01,     // unreachable          (unmodified)
/* 20005E9C */ 0xD009,     // beq(b) 20005EB2      (unmodified)
/* 20005E9E */ 0x68A0,     // ldr r0, [r4,#8]      (unmodified)
/* 20005EA0 */ 0x0000,     // bl check_fd_is_boot2 (unmodified)
               0x0000,     //                      (unmodified)
               0x2800,     // cmp r0, #0           (unmodified)
/* 20005EA6 */ 0xD1ED      // bne 20005E84 (-23)
};

static const s16 old_dev_fs_open_flash[] = {
	0xB510, 0,
	0x2005, 0,
	0x4240, 0,
	0x2100, 0,
	0x4C07, 0,
	0x00CB, 0,
	0x191A, 0,
	0x6813, 0,
	0x2B00, 0,
	0xD103, 0,
	0x2301, 0,
	0x6013, 0,
	0x1C10, 0,
	0xE002, 0,
	0x3101, 0,
	0x2901, 0,
	0xD9F3, 0,
	0xBC10, 0,
	0xBC02, 0,
	0x4708, 0,
	0x2004, 0xFFFF, 0x9C44, 0xFFFF,
	0xB500, 0,
	0x1C02, 0
};

// assume this will be placed at a mod 4 address
static const s16 new_dev_fs_open_flash[] = {
/* 200051FC */ 0xB500,     // push {lr}
               0x4809,     // ldr r0, =dev_flash_fds
               0x2200,     // movs r2, #0
               0x2B06,     // cmp r3, #6 (ipc_ioctl)
/* 20005204 */ 0xD105,     // bne 20005212
               0x68E1,     // ldr r1, [r4, 0xC] (ipcmsg.ioctl.cmd)
               0x296F,     // cmp r1, #0x6F (111)
/* 2000520A */ 0xD007,     // beq 2000521C
               0x2201,     // movs r2, #1
               0x2964,     // cmp r1, #0x64 (100)
/* 20005210 */ 0xD004,     // beq 2000521C
/* 20005212 */ 0x6800,     // ldr r0, [r0]
               0x2801,     // cmp r0, #1
/* 20005216 */ 0xD103,     // bne 2000521E
/* 20005218 */ 0x4803,     // ldr r0, =_FS_ioctl_ret+1
               0x4700,     // bx r0
/* 2000521C */ 0x6002,     // str r2, [r0]
               0x2001,     // movs r0, #1
/* 20005220 */ 0xBC04,     // pop {r2}
               0x4710,     // bx r2
/* 20005224 */ 0x0000,     // dev_flash_fds  (unmodified)
               0x0000,     //                (unmodified)
/* 20005228 */ 0x1374,     // _FS_ioctl_ret_
               0x001C+1    // _FS_ioctl_ret_+1 (thumb)
};

otp_t otp ATTRIBUTE_ALIGN(4);
//seeprom_t seeprom ATTRIBUTE_ALIGN(4);

// simple 2 byte patches
enum {
    MEM2_INDEX,
    DI_VERIFY_INDEX,
    NAND_PERMS_INDEX,
    DVD_SWITCH_INDEX
};

static const struct {
    u16 *address;
    u16 old_value;
    u16 new_value;
} patches[] = {
    {MEM_PROT, 1, 2},
    {(u16*)0x939F0D4A, 0xD123, 0x46C0},
    {(u16*)0x93A112F2, 0xD001, 0xE001},
    {(u16*)0x939B0528, 0x4651, 0x2100}
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

u8 *fetch_file(const char *filename, u32 filesize)
{
	u8 *filebuf = NULL;
	s32 fd = IOS_Open(filename, IPC_OPEN_READ);
	if (fd>=0)
	{
		u32 real_size = IOS_Seek(fd, 0, SEEK_END);
		if (!filesize)
			filesize = real_size;
		if (real_size >= filesize)
		{
			IOS_Seek(fd, SEEK_SET, 0);
			filebuf = (u8*)memalign(32, filesize+32);
			real_size = IOS_Read(fd, filebuf, filesize);
			if (real_size < filesize)
			{
				free(filebuf);
				filebuf = NULL;
			}
		}
		IOS_Close(fd);
	}
	return filebuf;
}

int get_certs()
{
	u32 readed;
	s32 fd = IOS_Open("/sys/cert.sys", ISFS_OPEN_READ);
	if (fd<0)
		return 0;

	readed = IOS_Read(fd, sys_certs, SYS_CERTS_SIZE);
	IOS_Close(fd);

	return (readed==SYS_CERTS_SIZE);
}

static const u16 ios_boot[] =
{
	0x4804, // ldr r0, =kernel_name
	0x2100, // movs r1, #0
	0x4A01, // ldr r2, =0x00250F1E
	0x4B02, // ldr r3, os_ios_boot
	0x4798, // blx r3
	0x46C0,

	(u16)(HAXX_IOS_VERSION>>16),
	(u16)HAXX_IOS_VERSION,
	0x0000, // es_syscall_ios_boot
	0x0000,
	0x0000, // kernel_name
	0x0000
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

bool forge_sig(u8 *data, u32 length)
{
	const u8* fixed_hash;
	u8 hash[20];
	u32 payload_len = length - SIGNATURE_SIZE((u32*)data);
	u8 *payload = data + length - payload_len;
	u16 *payload_junk;
	int i;
	u32 j;

	if (STD_SIGNED_TIK_SIZE == length)
	{
		memcpy(data, tik_sig, sizeof(tik_sig));
		// if the console id is set, use the end of the ticket id for payload
		if (*(u32*)(data+0x1D8))
			payload_junk = (u16*)(data+0x1D6);
		else
			payload_junk = (u16*)(data+0x262); // padding
		fixed_hash = tik_hash;
	}
	else // TMD
	{
		if (length < SIGNED_TMD_SIZE((u32*)data))
		{
			printf("Bad payload length (%08X)\n", length);
			return false;
		}
		memcpy(data, tmd_sig, sizeof(tmd_sig));
		payload_junk = (u16*)(data+0x1E2); // fill 3
		fixed_hash = tmd_hash;
	}

	for (i=0; i < 65536; i++)
	{
		*payload_junk = i;
		SHA1(payload, payload_len, hash);
		if (hash[0]==0)
		{
			for (j=1; j < sizeof(hash); j++)
			{
				if (hash[j]==fixed_hash[j])
				{
					printf("Fakesigned ok, junk %04X\n", i);
					return true;
				}
			}
		}
	}
	printf("Fakesigning failed\n");
	return false;
}

u8 *make_ticket(u64 title)
{
	u32 *ticket = (u32*)memalign(32, STD_SIGNED_TIK_SIZE);
	if (ticket)
	{
		tik *fake_ticket;
		memset(ticket, 0, STD_SIGNED_TIK_SIZE);
		*ticket = ES_SIG_RSA2048;

		fake_ticket = (tik*)(SIGNATURE_PAYLOAD(ticket));

		strcpy(fake_ticket->issuer, "Root-CA00000001-XS00000003");
		fake_ticket->ticketid = 0x0123456789ABCDEFllu;
		fake_ticket->titleid = title;
		fake_ticket->access_mask = 0xFFFF;
		fake_ticket->reserved[0x3B] = 1;
		memset(fake_ticket->cidx_mask, 0xFF, 32);

		forge_sig((u8*)ticket, STD_SIGNED_TIK_SIZE);
	}

	return (u8*)ticket;
}

u8 *make_tmd(u64 title)
{
	u32 *TMD = (u32*)memalign(32, 0x208); // TMD with 1 content
	if (TMD)
	{
		tmd *fake_tmd;
		memset(TMD, 0, 0x208);
		*TMD = ES_SIG_RSA2048;

		fake_tmd = (tmd*)(SIGNATURE_PAYLOAD(TMD));

		strcpy(fake_tmd->issuer, "Root-CA00000001-CP00000004");
		fake_tmd->num_contents = 1;
		fake_tmd->access_rights = -1;
		fake_tmd->title_version = -1;
		fake_tmd->title_id = title;
		fake_tmd->sys_version = title;
		fake_tmd->title_type = 1;
		fake_tmd->group_id = 1;

		forge_sig((u8*)TMD, SIGNED_TMD_SIZE(TMD));
	}

	return (u8*)TMD;
}

int check_for_sneek(s32 fd)
{
	u32 bversion;

	if (ES_GetBoot2Version(&bversion)==0 && bversion >= 5)
	{
		printf("Smells like sneek...");
		if (!IOS_Ioctlv(fd, 0x1F, 0, 0, NULL))
		{
			printf("Yep.\n");
			return 1;
		}
		printf("Nope.\n");
	}

	return 0;
}

int disable_mem2_protection(s32 fd)
{
	// FIXME: find which one of these needs manual aligning
	static u64 title ATTRIBUTE_ALIGN(32) = HAXX_IOS;
	static ioctlv vec[3] ATTRIBUTE_ALIGN(32);
	static u32 cnt ATTRIBUTE_ALIGN(32);
	u8 lowmem_save[0x20];
	s32 ret;

	if (check_for_sneek(fd))
	{
		u8 *fake_tik;
		u8 *fake_tmd;

		if (!get_certs())
		{
			printf("Couldn't get certs\n");
			return 0;
		}

		fake_tik = make_ticket(0x0000000100000001llu);
		if (fake_tik==NULL)
		{
			printf("Couldn't make fake ticket\n");
			return 0;
		}
		fake_tmd = make_tmd(0x0000000100000001llu);
		if (fake_tmd==NULL)
		{
			free(fake_tik);
			printf("Couldn't make fake TMD\n");
			return 0;
		}

		ret = ES_Identify((signed_blob*)sys_certs, SYS_CERTS_SIZE, (signed_blob*)fake_tmd, SIGNED_TMD_SIZE((u32*)fake_tmd), (signed_blob*)fake_tik, SIGNED_TIK_SIZE((u32*)fake_tik), &cnt);

		free(fake_tik);
		free(fake_tmd);

		if (ret==0)
		{

			if (!do_patch(MEM2_INDEX))
			{
				printf("Couldn't patch MEM2 protection\n");
				return 0;
			}
			else
			{
				u8 *i;
				const char ESVersion[] = "$IOSVersion: ES:";

				for (i = (u8*)0x939F0000; i < (u8*)0x93B00000 - strlen(ESVersion); i++)
				{
					if (!memcmp(i, ESVersion, strlen(ESVersion)))
					{
						printf("Found: %.40s\n", i);
						return 1;
					}
				}

			}
		}

		return 0;
	}

	vec[0].data = &title;
	vec[0].len = sizeof(title);
	vec[1].data = &cnt;
	vec[1].len = sizeof(cnt);
	// do this myself because libogc fails
	DCFlushRange(&title, 4);
	DCFlushRange(((u8*)&title)+4, 4);
	ret = IOS_Ioctlv(fd, 0x12, 1, 1, vec); // ES_GetNumTicketViews
	if (ret)
	{
		printf("Couldn't get NumTicketViews (%d)\n", ret);
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

	vec[2].data = ES_STACK_EXPLOIT;
	vec[2].len = 0x0;
	cnt = 0x20000000; // 0x20000000 * 0xD8 = 0x1B00000000 = 0
	ret = IOS_Ioctlv(fd, 0x13, 2, 1, vec); // ES_GetTicketViews

	// restore lowmem values
	memcpy(MEM1_BASE, lowmem_save, sizeof(lowmem_save));

	if (ret != 101) // exploit failed
	{
		printf("Possible incorrect ES version, failed (%d)\n", ret);
		return 0;
	}

	if (!do_patch(MEM2_INDEX))
	{
		printf("Couldn't patch MEM2 protection\n");
		return 0;
	}

	return 1;
}

// this function will only work once after reloading IOS, so use sparingly. Make sure ES has been patched before calling!
int identify_as_title(u64 title)
{
	s32 ret;
	u32 *tmd_blob = NULL;
	tmd *title_tmd;
	u32 *tik_blob = NULL;
	char filename[65];
	u32 junk;

	if (!get_certs())
		return 0;

	sprintf(filename, "/title/%08x/%08x/content/title.tmd", (u32)(title>>32), (u32)title);
	tmd_blob = (u32*)fetch_file(filename, 0);
	if (tmd_blob==NULL)
		return 0;
	title_tmd = (tmd*)SIGNATURE_PAYLOAD(tmd_blob);

	sprintf(filename, "/ticket/%08x/%08x.tik", (u32)(title>>32), (u32)title);
	tik_blob = (u32*)fetch_file(filename, 0);
	if (tik_blob==NULL)
	{
		// Disc based games don't have a stored ticket
		tik_blob = (u32*)make_ticket(title);
		if (tik_blob==NULL)
		{
			free(tmd_blob);
			return 0;
		}
	}

	ret = ES_Identify((signed_blob*)sys_certs, SYS_CERTS_SIZE, (signed_blob*)tmd_blob, SIGNED_TMD_SIZE(tmd_blob), (signed_blob*)tik_blob, SIGNED_TIK_SIZE(tik_blob), &junk);

	free(tmd_blob);
	free(tik_blob);

	return !ret;
}

static int patch_mem2(void* buf, s32 size)
{
	u32 i, count = 0;
	u8 *kernel = (u8*)buf;
	const u8 old_table[] = {0xB5,0x00,0x4B,0x09,0x22,0x01,0x80,0x1A,0x22,0xF0};

	for (i=0; i < size - sizeof(old_table); i++)
	{
		if (!memcmp(kernel+i, old_table, sizeof(old_table)))
		{
			kernel[i+5] = 0;
			i += sizeof(old_table);
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

	for (i=0; i < size - sizeof(sd_old); i++)
	{
		if (!memcmp(kernel+i, sd_old, sizeof(sd_old)))
		{
			kernel[i+16] = 0x2A;
			kernel[i+17] = 0x04;
			i += sizeof(sd_old);
			count++;
		}
	}

	return count;
}

static int patch_gpio_stm(void* buf, s32 size)
{
	u32 i;
	u8 *kernel = (u8*)buf;
	const u8 gpio_orig[8] = {0xD1, 0x0F, 0x28, 0xFC, 0xD0, 0x33, 0x28, 0xFC};

	for (i=0; i < size - sizeof(gpio_orig); i++)
	{
		if (!memcmp(kernel+i, gpio_orig, sizeof(gpio_orig)))
		{
			kernel[i]   = 0x46;
			kernel[i+1] = 0xC0;
			return 1;
		}
	}
	return 0;
}

static int patch_fs_redirect(void* buf, s32 size)
{
	u32 i, j;
	s16 *kernel = (s16*)buf;
	for (i=0; i < size - sizeof(old_dev_fs_main)/4; i++)
	{
		s16 *open_dev_flash;
		for (j=0; j < sizeof(old_dev_fs_main)/4; j++) {
			s16 match_needed = old_dev_fs_main[j*2] | old_dev_fs_main[j*2+1];
			s16 match_masked = kernel[i+j]          | old_dev_fs_main[j*2+1];
			if (match_needed != match_masked)
				break;
		}
		if (j < sizeof(old_dev_fs_main)/4)
			continue;

		//printf("Found /dev/fs main() %08X\n", i*2);

		open_dev_flash = kernel+i+4+kernel[i+3]-0x16;
		printf("open_dev_flash offset %04X%04X\n", open_dev_flash[0], open_dev_flash[1]);
		for (j=0; j < sizeof(old_dev_fs_open_flash)/4; j++) {
			s16 match_needed = old_dev_fs_open_flash[j*2] | old_dev_fs_open_flash[j*2+1];
			s16 match_masked = open_dev_flash[j]          | old_dev_fs_open_flash[j*2+1];
			if (match_needed != match_masked)
			{
				//printf("j %d mask %04X needed %04X\n", j, match_masked, match_needed);
				break;
			}
		}
		if (j < sizeof(old_dev_fs_open_flash)/4)
			continue;

		// do all the patching
		kernel[i] +=   0x0103;
		kernel[i+1] =  0x6823;
		kernel[i+3] -= 0x0016;
		kernel[i+5] -= 0x0001;
		kernel[i+6] =  0xE005;
		kernel[i+18] = 0xD1ED; // lol 0xdied

		// TODO: Check this isn't overrunning
		for (j=0; j < sizeof(new_dev_fs_open_flash)/2; j++) {
			open_dev_flash[j] = (open_dev_flash[j] & old_dev_fs_open_flash[j*2+1]) | new_dev_fs_open_flash[j];
		}

		return 1;
	}
	return 0;
}

static u8 *load_tmd_content(tmd* title_tmd, u16 index)
{
	u8 *content = NULL;
	char filename[65];
	u32 size;

	filename[0] = '\0';

	if (title_tmd->contents[index].type & 0x8000) // shared content
	{
		// make sure it's at least 32 bytes long for IOS_read
		static union
		{
			cmap_entry cmap;
			u8 unused[32];
		} shared ATTRIBUTE_ALIGN(32);

		s32 fd = IOS_Open("/shared1/content.map", ISFS_OPEN_READ);

		if (fd<0)
			return NULL;

		while (IOS_Read(fd, &shared, sizeof(cmap_entry))==sizeof(cmap_entry))
		{
			if (!memcmp(shared.cmap.hash, title_tmd->contents[index].hash, sizeof(sha1)))
			{
				strcpy(filename, "/shared1/00000000.app");
				memcpy(filename+9, shared.cmap.name, 8);
				break;
			}
		}

		IOS_Close(fd);
	}
	else
		sprintf(filename, "/title/%08x/%08x/content/%08x.app", (u32)(title_tmd->title_id>>32), (u32)(title_tmd->title_id), title_tmd->contents[index].cid);

	size = title_tmd->contents[index].size;
	content = fetch_file(filename, size);

	if (content)
	{
#ifndef YARR
		u8 real_hash[20];
		SHA1(content, size, real_hash);
		if (memcmp(title_tmd->contents[index].hash, real_hash, sizeof(sha1)))
		{
			printf("Content file %s had bad hash\n", filename);
			free(content);
			content = NULL;
		}
#endif
	}
	else
		printf("Couldn't fetch content %s\n", filename);

	return content;
}

static int prepare_new_kernel(u64 title)
{
	s32 fd;
	s32 size=0, readed;
	u32 *tmd_blob = NULL;
	tmd *title_tmd;
	char filename[65];
	void *kernel_blob = NULL;
	int i;

	sprintf(filename, "/title/%08x/%08x/content/title.tmd", (u32)(title>>32), (u32)title);
	tmd_blob = (u32*)fetch_file(filename, 0);
	if (tmd_blob==NULL)
	{
		printf("Couldn't allocate TMD blob\n");
		return 0;
	}
	title_tmd = (tmd*)SIGNATURE_PAYLOAD(tmd_blob);

	if (check_cert_chain((u8*)tmd_blob, SIGNED_TMD_SIZE(tmd_blob)))
	{
		free(tmd_blob);
		printf("IOS %d TMD failed sig check\n", (u32)title);
		return 0;
	}

	for (i=0; i < title_tmd->num_contents; i++)
	{
		// check SHA1 hash of each content against the TMD
		u8 *blob = load_tmd_content(title_tmd, i);
		if (blob==NULL)
		{
			printf("Couldn't load IOS content %d\n", i);
			free(tmd_blob);
			return 0;
		}
		if (title_tmd->contents[i].index == title_tmd->boot_index)
		{
			size = title_tmd->contents[i].size;
			kernel_blob = blob;
		}
		else
			free(blob);
	}

	free(tmd_blob);

	if (kernel_blob==NULL)
	{
		printf("Couldn't fetch new kernel\n");
		return 0;
	}

	if (!patch_mem2(kernel_blob, size) || !patch_ios37_sd_load(kernel_blob, size) ||
		!patch_gpio_stm(kernel_blob, size) || !patch_fs_redirect(kernel_blob, size))
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
	*MEM1_IOSVERSION = 0x00020000;
}

static void load_patched_ios(s32 fd, const char* filename)
{
	ioctlv vec;
	u32* addr;
	for (addr=(u32*)ES_MODULE_START;addr < (u32*)(ES_MODULE_START+ES_MODULE_SIZE);addr++)
	{
		if (*addr == SYSCALL_DEVICE_OPEN)
		{
			u32 junk;
			//printf("Found ES_SYSCALL_DEVICE_OPEN at %p\n", addr);
			memcpy(MEM1_BASE_UNCACHED, ios_boot, sizeof(ios_boot));
			MEM1_BASE_UNCACHED[4] = (u32)addr + 0x130 - 0x939F0000 + 0x20100000;
			MEM1_BASE_UNCACHED[5] = (u32)filename & 0x7FFFFFFF;
			DCFlushRange((void*)filename, strlen(filename+1));
			//printf("%08X %08X\n", MEM1_BASE_UNCACHED[4], MEM1_BASE_UNCACHED[5]);

			addr[0] = 0xE3A02001; // mov r2, #1
			addr[1] = 0xE12FFF12; // bx r2
			DCFlushRange(addr, 8);
			vec.data = &junk;
			vec.len = sizeof(junk);
			//printf("Taking the plunge...\n");
			//printf("IOS returned %d\n", IOS_Ioctlv(fd, 0x0c, 0, 1, &vec));
			//printf("Owned titles: %d\n", junk);
			IOS_IoctlvRebootBackground(fd, 0x0C, 0, 1, &vec);
			return;
		}
	}
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
			udelay(6000);
		else
			break;
	}

	// Catch erroneous IPC signal
	for (retries = 0; retries < 10; retries++)
	{
		if(IPC_ReadReg(1) & 2)
			break;
		udelay(6000);
	}

	IRQ_Request(IRQ_PI_ACR, irq_handler, NULL);
	__UnmaskIrq(IRQ_PI_ACR);

	__IPC_Reinitialize();

	__IOS_InitializeSubsystems();
	__ES_Init();
	__STM_Init();
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

static int load_sdhc_module(u64 title_ios)
{
	s32 ret=0;
	s32 size;
	u32 *tmd_blob = NULL;
	tmd *title_tmd;
	char filename[65];
	u8 *module_buf = NULL;
	int i, found=0;
	u32 j;

	//printf("Attempting to load SDHC module from IOS %d\n", (u32)title_ios);

	sprintf(filename, "/title/%08x/%08x/content/title.tmd", (u32)(title_ios>>32), (u32)title_ios);
	tmd_blob = (u32*)fetch_file(filename, 0);
	if (tmd_blob==NULL)
	{
		//printf("Couldn't get TMD blob for SDHC\n");
		return 0;
	}
	title_tmd = (tmd*)SIGNATURE_PAYLOAD(tmd_blob);

	if (check_cert_chain((u8*)tmd_blob, SIGNED_TMD_SIZE(tmd_blob)))
	{
		free(tmd_blob);
		printf("IOS %d TMD failed sig check\n", (u32)title_ios);
		return 0;
	}

	for (i=0; i < title_tmd->num_contents && !found; i++)
	{
		free(module_buf); // lazy
		module_buf = load_tmd_content(title_tmd, i);
		size = title_tmd->contents[i].size;
		if (module_buf)
		{
			const char sd_version[] = "$IOSVersion: SDI:";
			for (j=0; j < size - strlen(sd_version); j++)
			{
				if (!memcmp(module_buf + j, sd_version, strlen(sd_version)))
				{
					found = 1;
					break;
				}
			}
		}
	}

	if (found)
		ret = load_module_code(module_buf, size);

	free(tmd_blob);
	free(module_buf);
	return ret;
}

static int do_sig_check_patch()
{
	if (memcmp(HASH_CHECK_ADDRESS, old_hashcmp, sizeof(old_hashcmp)) && memcmp(HASH_CHECK_ADDRESS, new_hashcmp, sizeof(new_hashcmp)))
		return 0;

	memcpy(HASH_CHECK_ADDRESS, new_hashcmp, sizeof(new_hashcmp));
	DCFlushRange(HASH_CHECK_ADDRESS, 64);

	return 1;
}

static void read_otp()
{
	u32 *otpd = (u32*)&otp;
	for (unsigned int i=0; i < sizeof(otp)/sizeof(u32); i++)
	{
		*OTP_COMMAND = 0x80000000|i;
		*otpd++ = *OTP_DATA;
	}
}

static bool do_exploit()
{
	s32 es_fd = -1;
	int patch_failed = 0;
	u32 sneek=0;

	printf("Grabbin' HAXX\n");

	if (IOS_GetVersion() != (u32)HAXX_IOS || IOS_GetRevision() != HAXX_IOS_REVISION)
	{
		printf("Wrong IOS version. Update IOS%d to the latest version.\n", (u32)HAXX_IOS);
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
		u32 ng_id;
		printf("MEM2 protection disabled\n");

		if (!check_for_sneek(es_fd))
		{
			// make sure signature check isn't patched
			*(u16*)0x93A752E6 = 0x2007;
			DCFlushRange((void*)0x93A752E0, 32);

			if (!do_patch(NAND_PERMS_INDEX))
			{
				patch_failed = 1;
				printf("NAND Permissions patch failed\n");
			}
		} else
			sneek = 1;

		read_otp();
		if (ES_GetDeviceID(&ng_id) || ng_id != otp.ng_id) // wtf how
			patch_failed = 1;


		if (!patch_failed)
		{
			patch_failed = !prepare_new_kernel(HAXX_IOS);
			if (patch_failed)
				printf("Failed to prepare new kernel\n");
		}

		if (!patch_failed)
		{
			shutdown_for_reload();
			load_patched_ios(es_fd, LOAD_KERNEL_PATH);
			es_fd = 0;
			recover_from_reload(HAXX_IOS_VERSION>>16);
			if (IOS_GetVersion() != (HAXX_IOS_VERSION>>16) || IOS_GetRevision() != (HAXX_IOS_VERSION&0xFFFF))
				patch_failed = 1;
		}

		if (!patch_failed)
			patch_failed = !do_patch(NAND_PERMS_INDEX);

		patch_failed |= sneek;

		if (!patch_failed)
		{
			patch_failed = !(load_sdhc_module(0x0000000100000038llu) || load_sdhc_module(0x000000010000003Cllu) || \
				load_sdhc_module(0x000000010000003Dllu) || load_sdhc_module(0x0000000100000046llu));
			if (patch_failed)
			{
				printf("SDHC module couldn't be loaded, trying IOS37 standard SD.\n");
				patch_failed = !load_sdhc_module(0x0000000100000025llu);
			}
		}

#ifndef YARR
		if (!patch_failed)
			patch_failed = !do_sig_check_patch();
		if (!patch_failed)
			patch_failed = !do_patch(DVD_SWITCH_INDEX);
#else
		// kill sig check
		*(u16*)0x93A752E6 = 0x2000;
		DCFlushRange((void*)0x93A752E0, 32);
		// disable key permissions check in os_calc_ecdh_shared
		//*(u16*)0x93A72C62 = 0x46C0;
		//DCFlushRange((void*)0x93A72C60, 32);
#endif

		if (sneek) {
			printf("SNEEK found, have to reboot again *sigh*\n");
			if (es_fd >=0 )
				IOS_Close(es_fd);
			IOS_ReloadIOS((u32)HAXX_IOS);
			return do_exploit();
		}

		if (!patch_failed)
			printf("All patching done!\n");

	}

	if (es_fd >= 0)
		IOS_Close(es_fd);

	return !patch_failed;
}

/******* BEGIN SIGNATURE CHECKING STUF ******/
static inline u32 be32(const u8 *p)
{
	return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}

static inline void bn_zero(u8 *d, const u32 n)
{
	memset(d, 0, n);
}

static inline void bn_copy(u8 *d, const u8 *a, const u32 n)
{
	memcpy(d, a, n);
}

static inline int bn_compare(const u8 *a, const u8 *b, const u32 n)
{
	u32 i;

	for (i = 0; i < n; i++) {
		if (a[i] < b[i])
			return -1;
		if (a[i] > b[i])
			return 1;
	}

	return 0;
}

static inline void bn_sub_modulus(u8 *a, const u8 *N, const u32 n)
{
	u32 i;
	u32 dig;
	u8 c;

	c = 0;
	for (i = n - 1; i < n; i--) {
		dig = N[i] + c;
		c = (a[i] < dig);
		a[i] -= dig;
	}
}

static inline void bn_add(u8 *d, const u8 *a, const u8 *b, const u8 *N, const u32 n)
{
	u32 i;
	u32 dig;
	u8 c;

	c = 0;
	for (i = n - 1; i < n; i--) {
		dig = a[i] + b[i] + c;
		c = (dig >= 0x100);
		d[i] = dig;
	}

	if (c)
		bn_sub_modulus(d, N, n);

	if (bn_compare(d, N, n) >= 0)
		bn_sub_modulus(d, N, n);
}

static void bn_mul(u8 *d, const u8 *a, const u8 *b, const u8 *N, const u32 n)
{
	u32 i;
	u8 mask;

	bn_zero(d, n);

	for (i = 0; i < n; i++)
		for (mask = 0x80; mask != 0; mask >>= 1) {
			bn_add(d, d, d, N, n);
			if ((a[i] & mask) != 0)
				bn_add(d, d, b, N, n);
		}
}

static void bn_exp(u8 *d, const u8 *a, const u8 *N, const u32 n, const u8 *e, const u32 en)
{
	u8 t[512];
	u32 i;
	u8 mask;

	bn_zero(d, n);
	d[n-1] = 1;
	for (i = 0; i < en; i++)
		for (mask = 0x80; mask != 0; mask >>= 1) {
			bn_mul(t, d, d, N, n);
			if ((e[i] & mask) != 0)
				bn_mul(d, t, a, N, n);
			else
				bn_copy(d, t, n);
		}
}

static u32 get_sig_len(const u8 *sig)
{
	u32 type;

	type = be32(sig);
	switch (type - 0x10000) {
	case 0:
		return 0x240;

	case 1:
		return 0x140;

	case 2:
		return 0x80;
	}

	return 0;
}

static u32 get_sub_len(const u8 *sub)
{
	u32 type;
	type = be32(sub + 0x40);
	switch (type) {
	case 0:
		return 0x2c0;

	case 1:
		return 0x1c0;

	case 2:
		return 0x100;
	}

	return 0;
}

static int check_rsa(const u8 *h, const u8 *sig, const u8 *key, const u32 n)
{
	u8 correct[0x200], x[0x200];
	static const u8 ber[16] = {0x00,0x30,0x21,0x30,0x09,0x06,0x05,0x2b,
		0x0e,0x03,0x02,0x1a,0x05,0x00,0x04,0x14};

	correct[0] = 0;
	correct[1] = 1;
	memset(correct + 2, 0xff, n - 38);
	memcpy(correct + n - 36, ber, 16);
	memcpy(correct + n - 20, h, 20);

	bn_exp(x, sig, key, n, key + n, 4);

	if (memcmp(correct, x, n) == 0)
		return 0;

	// replicate the strncmp bug
	if (strncmp((char*)h, (char*)x+n-20, 20)==0)
		return -100;

	return -5;
}

static int check_hash(const u8 *h, const u8 *sig, const u8 *key)
{
	u32 type;

	//if (h[0])
	//	return -400;
	type = be32(sig) - 0x10000;
	if (type != be32(key + 0x40))
		return -6;

	switch (type)
	{
	case 1:
		return check_rsa(h, sig+4, key+0x88, 0x100);
	}

	return -7;
}

static const u8* find_cert_in_chain(const u8 *sub, const u8 *cert, const u32 cert_len)
{
	char parent[64], *child;
	u32 sig_len, sub_len;
	const u8 *p, *issuer;

	strncpy(parent, (char*)sub, sizeof parent);
	parent[sizeof parent - 1] = 0;
	child = strrchr(parent, '-');
	if (child)
		*child++ = 0;
	else
	{
		*parent = 0;
		child = (char*)sub;
	}

	for (p=cert;p<cert+cert_len;p+=sig_len+sub_len)
	{
		sig_len = get_sig_len(p);
		if (sig_len==0)
			return 0;
		issuer = p + sig_len;
		sub_len = get_sub_len(issuer);
		if (sub_len==0)
			return 0;

		if (strcmp(parent, (char*)issuer)==0 && strcmp(child, (char*)issuer+0x44)==0)
			return p;
	}

	return 0;
}

int check_cert_chain(const u8 *data, const u32 data_len)
{
#ifndef YARR
	const u8* key;
	const u8 *sig, *sub, *key_cert;
	u32 sig_len, sub_len;
	u8 h[20];
	int ret;

	if (!get_certs())
		return -1;

	sig = data;
	sig_len = get_sig_len(sig);
	if (sig_len==0)
		return -1;

	sub = data + sig_len;
	sub_len = data_len - sig_len;
	if (sub_len==0)
		return -2;

	for (;;)
	{
		if (strcmp((char*)sub, "Root")==0)
		{
			key = root_dat;
			SHA1(sub, sub_len, h);
			if (be32(sig) != 0x10000)
				return -8;
			return check_rsa(h, sig+4, key, 0x200);
		}

		key_cert = find_cert_in_chain(sub, sys_certs, SYS_CERTS_SIZE);
		if (key_cert==0)
			return -3;

		key = key_cert + get_sig_len(key_cert);

		SHA1(sub, sub_len, h);
		ret = check_hash(h, sig, key);
		// uncomment this if you want to check the whole chain's integrity
		// rather than just the tail certificate
		//if (ret)
			return ret;

		sig = key_cert;
		sig_len = get_sig_len(sig);
		if (sig_len==0)
			return -4;
		sub = sig + sig_len;
		sub_len = get_sub_len(sub);
		if (sub_len==0)
			return -5;
	}
#else
	return 0;
#endif
}
/*********** SIGNATURE CHECKING STUFF ENDS */

