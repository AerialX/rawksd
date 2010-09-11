#include <ogcsys.h>
#include <string.h>
#include <ogc/lwp_threads.h>

#define MEM_BASE			((u8*)0x80000000)
#define MEM_STAGING         ((u8*)0x91000000)

int valid_elf_image(void *addr)
{
	u32 *header = addr;

	return header[0] == 0x7f454c46		// ELF
		&& header[1] == 0x01020100	// 32-bit, BE, ELF v1, SVR
		&& header[4] == 0x00020014	// executable, PowerPC
		&& header[5] == 1		// object file v1
		&& (header[10] & 0xffff) == 32;	// PHDR size
}


// Returns the entry point address.
void *load_elf_image(void *addr)
{
	u32 *header = addr;
	u32 *phdr = addr + header[7];
	u32 n = header[11] >> 16;
	u32 i;

	for (i = 0; i < n; i++, phdr += 8) {
		if (phdr[0] != 1)	// PT_LOAD
			continue;

		u32 off = phdr[1];
		void *dest = (void *)phdr[3];
		u32 filesz = phdr[4];
		u32 memsz = phdr[5];

		memcpy(dest, addr + off, filesz);
		memset(dest + filesz, 0, memsz - filesz);

		ICInvalidateRange(dest, memsz);
	}

	return (void *)header[6];
}

void jumpto(void *app_address)
{
	if (app_address) {
		DCFlushRange(MEM_BASE, 0x17FFFFFF);

		SYS_ResetSystem(SYS_SHUTDOWN, 0, 0);
		__lwp_thread_stopmultitasking((void(*)())app_address);
	}
}

int main(int argc, char **argv)
{
	u8 *buf = MEM_STAGING;
	s32 fd = ES_OpenContent(2);
	if (fd>=0) {
		while (ES_ReadContent(fd, buf, 0x8000)==0x8000)
			buf += 0x8000;

		ES_CloseContent(fd);

		if (valid_elf_image(MEM_STAGING))
			jumpto(load_elf_image(MEM_STAGING));
	}
	exit(0);
}
