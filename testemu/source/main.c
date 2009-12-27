#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <mload.h>
#include <malloc.h>

#include "emumodule_dat.h"
#include "ehcmodule_dat.h"
#include "sslmodule_dat.h"
#include "filemodule_dat.h"

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

#define MOUNT_USB

bool MloadInit();

void HexDump(u8* buffer, u32 num)
{
    u32 i;
    for(i = 0; i < num; i++)
    {
        printf("0x%02X ", *(buffer+i));
        if ((i + 1) % 0x10 == 0)
            printf("\n");
    }
    printf("\n\n");
}

int main(int argc, char **argv) {
	IOS_ReloadIOS(242);
	VIDEO_Init();
	WPAD_Init();
	rmode = VIDEO_GetPreferredMode(NULL);
	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);
	VIDEO_Configure(rmode);
	VIDEO_SetNextFramebuffer(xfb);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if (rmode->viTVMode & VI_NON_INTERLACE)
		VIDEO_WaitVSync();

	printf("\x1b[2;0H");
	printf("y halo thar\n");

	printf("\nLet's start mloading stuff.\n");

	if (MloadInit()) {
		int ssl = IOS_Open("/dev/net/ssi", 0);
		printf("SSL open %d\n", ssl);
		printf("IOCTL %d\n", IOS_Ioctl(ssl, 25, &ssl, 4, &ssl, 4));

		// Try some ISFS calls on RB2 DLC apps now and see what you get
		char* path = "/title/00010005/735a4145/content/00000005.app";
		//char* path = "/private/wii/data/sZAE/000.bin";
		ISFS_Initialize();
		int fd = ISFS_Open(path, ISFS_OPEN_READ);
		printf("ISFS_Open(\"%s\", READ) = 0x%X\n", path, fd);
		if (fd) {
			u8* buffer = memalign(32, 0x100);
			memset(buffer, 0, 0x10);
			printf("ISFS_Read(0x10) = 0x%X\n", ISFS_Read(fd, buffer, 0x10));
			printf("I'mma let you finish, but first, a hexdump.\n");
			HexDump(buffer, 0x10);
			printf("ISFS_Seek(0, 0) = 0x%X\n", ISFS_Seek(fd, 0, 0));
			printf("ISFS_Read(0xA0) = 0x%X\n", ISFS_Read(fd, buffer, 0xA0));
			HexDump(buffer, 0x10);
			ISFS_Close(fd);
		} else
			printf("Uh... fuck?\n");
	}

	while (true) {
		WPAD_ScanPads();
		u32 pressed = WPAD_ButtonsDown(0);
		if (pressed & WPAD_BUTTON_HOME)
			return 0;

		
		VIDEO_WaitVSync();
	}

	return 0;
}

bool MloadInit()
{

printf("Press A...\n");
while (true) {
	WPAD_ScanPads();
if (WPAD_ButtonsDown(0) & WPAD_BUTTON_A)
break;
}

	s32 fd = IOS_Open("/dev/fs", 0);
	if (fd >= 0) {
		// should return 1
		printf("Enabling hook... %08X\n", IOS_Ioctl(fd, 100, NULL, 0, NULL, 0));
		IOS_Close(fd);
	}

	if (mload_init() >= 0) {
		data_elf elf;
		printf("Mload opened\n");
		if (mload_elf((void*)ehcmodule_elf, &elf)==0)
		{
			printf("ehcmodule loaded\n");
			printf("Run thread returned %d\n", mload_run_thread(elf.start, elf.stack, elf.size_stack, elf.prio));
			usleep(2000);
		}
		if (mload_elf((void*)filemodule_elf, &elf)==0)
		{
			printf("FileModule loaded\n");
			printf("Run thread returned %d\n", mload_run_thread(elf.start, elf.stack, elf.size_stack, elf.prio));
			usleep(200);
		}
		int ffd = IOS_Open("file", 0);
		if (ffd >= 0) {
			printf("Mounting...\n");
			int disk = 0;
#ifdef MOUNT_USB
			disk = 0x02;
#else
			disk = 0x01;
#endif
			while (IOS_Ioctl(ffd, 0x30, &disk, 4, NULL, 0) == -0x90) {
				printf("lolfailed\n");
				usleep(8000);
			}
			disk = 0x01; // FAT
			IOS_Ioctl(ffd, 0x31, &disk, 4, NULL, 0);
			IOS_Close(ffd);
		} else
			printf("FileModule fail\n");
		/*
		if (mload_elf((void*)sslmodule_elf, &elf)==0) {
			printf("sslmodule loaded\n");
			printf("Run thread returned %d\n", mload_run_thread(elf.start, elf.stack, elf.size_stack, elf.prio));
			usleep(2000);
		} return true; */
		if (mload_elf((void*)emumodule_elf, &elf)==0)
		{
			printf("RawkEMU loaded\n");
			printf("Run thread returned %d\n", mload_run_thread(elf.start, elf.stack, elf.size_stack, elf.prio));
			usleep(200);
		}
		mload_close();
	}

	fd = IOS_Open("emu", 0);
	if (fd >= 0) {
		// get address of emu hook function
		// should be 0x13700635 at the moment
		int fshook = IOS_Ioctl(fd, 0x100, NULL, 0, NULL, 0);
		if (fshook > 0) {
			printf("Hook is at %08X\n", fshook);
			// should return 0
			printf("Set hook returned %08X\n", mload_set_FS_ioctl_vector((void*)fshook));
			// close mload because it got reopened
			mload_close();
		}
		IOS_Close(fd);
	}
	else
		printf("Couldn't open RawkEMU (%08X)\n", fd);

	return true;
}
