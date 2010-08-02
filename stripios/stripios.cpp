/*
	IOS ELF stripper, converts traditional ELF files into the format IOS wants.
    Copyright (C) 2008 neimod.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define ELF_NIDENT 16

#ifdef _MSC_VER
typedef char s8;
typedef unsigned char u8;
typedef short s16;
typedef unsigned short u16;
typedef int s32;
typedef unsigned int u32;
typedef __int64 s64;
typedef unsigned __int64 u64;
#else
#include <stdint.h>
typedef int8_t s8;
typedef uint8_t u8;
typedef int16_t s16;
typedef uint16_t u16;
typedef int32_t s32;
typedef uint32_t u32;
typedef int64_t s64;
typedef uint64_t u64;
#endif

typedef struct
{
        u32		ident0;
		u32		ident1;
		u32		ident2;
		u32		ident3;
        u32		machinetype;
        u32		version;
        u32		entry;
        u32       phoff;
        u32       shoff;
        u32		flags;
        u16      ehsize;
        u16      phentsize;
        u16      phnum;
        u16      shentsize;
        u16      shnum;
        u16      shtrndx;
} elfheader;

typedef struct
{
       u32      type;
       u32      offset;
       u32      vaddr;
       u32      paddr;
       u32      filesz;
       u32      memsz;
       u32      flags;
       u32      align;
} elfphentry;

typedef struct
{
	u32 offset;
	u32 size;
} offset_size_pair;

u16 getbe16(void* pvoid)
{
	unsigned char* p = (unsigned char*)pvoid;

	return (p[0] << 8) | (p[1] << 0);
}

u32 getbe32(void* pvoid)
{
	unsigned char* p = (unsigned char*)pvoid;

	return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | (p[3] << 0);
}

void putbe16(void* pvoid, u16 val)
{
	unsigned char* p = (unsigned char*)pvoid;

	p[0] = val >> 8;
	p[1] = val >> 0;
}

void putbe32(void* pvoid, u32 val)
{
	unsigned char* p = (unsigned char*)pvoid;

	p[0] = val >> 24;
	p[1] = val >> 16;
	p[2] = val >> 8;
	p[3] = val >> 0;
}

void aes_encrypt(u8 *iv, const u8 *inbuf, u8 *outbuf, u32 len);
void gkey(int nb,int nk,const u8 *key);
void gentables(void);

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

static inline void bn_add(u8 *d, const u8 *a, const u8 *b, const u32 n)
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
}

static void bn_mul(u8 *d, const u8 *a, const u8 *b, const u32 n)
{
	u32 i;
	u8 mask;

	memset(d, 0, n);

	for (i = 0; i < n; i++)
		for (mask = 0x80; mask != 0; mask >>= 1) {
			bn_add(d, d, d, n);
			if ((a[i] & mask) != 0)
				bn_add(d, d, b, n);
		}
}

static u8 aes_key[32];
static u8 aes_iv[32];
static u8 aes_buffer[16];
static u8 aes_pos = 0;

size_t write_out(u8 *buf, size_t length, size_t count, FILE *out)
{
	length *= count;
	while (length) {
		size_t to_write = (length + aes_pos > 16) ? 16-aes_pos : length;
		memcpy(aes_buffer+aes_pos, buf, to_write);
		aes_pos += to_write;
		buf += to_write;
		length -= to_write;
		if (aes_pos==16) {
			aes_encrypt(aes_iv, aes_buffer, aes_buffer, 16);
			fwrite(aes_buffer, 16, 1, out);
			aes_pos = 0;
		}
	}

	return count;
}

void write_finish(FILE *out)
{
	if (aes_pos!=0) {
		aes_encrypt(aes_iv, aes_buffer, aes_buffer, aes_pos);
		fwrite(aes_buffer, 1, aes_pos, out);
	}
	fclose(out);
}

s32 main(s32 argc, char* argv[])
{
	int i;
	s32 result = 0;

	u32 strip=0;

	fprintf(stdout, "stripios - IOS ELF stripper - by neimod\n");
	if (argc < 3 || argc==4)
	{
		fprintf(stderr,"Usage: %s <in.elf> <out.elf> [strip addr]\n", argv[0]);

		return -1;
	}
	else
	if(argc==5)
	{
	if(!strcmp(argv[3],"strip"))
		{
		 sscanf( argv[4], "%x",&strip );

		printf("strip: %x\n",strip);
		}
	else
		{
		fprintf(stderr,"Usage: %s <in.elf> <out.elf> [strip addr]\n", argv[0]);

		return -1;
		}
	}

	FILE* fin = fopen(argv[1], "rb");
	FILE* fout = fopen(argv[2], "wb");

	memset(aes_iv, 0, 16);
	srand((u32)time(NULL));
	for(i=4; i<8; i++)
		((int*)aes_iv)[i] = rand()*rand()+rand(); // to ensure a 32-bit int
	fwrite(aes_iv+16, 16, 1, fout);
	bn_mul(aes_key, aes_iv, aes_iv, 32);
	printf("Key: ");
	for (i=0; i < 32; i++)
		printf("%02X", aes_key[i]);
	printf("\n");
	gentables();
	gkey(4, 8, aes_key);
	memcpy(aes_iv, aes_key+16, 16);

	if (fin == 0 || fout == 0)
	{
		if (fin == 0)
			fprintf(stderr,"ERROR opening file %s\n", argv[1]);
		if (fout == 0)
			fprintf(stderr,"ERROR opening file %s\n", argv[2]);
		return 1;
	}

	elfheader header;

	if (fread(&header, sizeof(elfheader), 1, fin) != 1)
	{
		fprintf(stderr,"ERROR reading ELF header\n");
		return 1;
	}

	u32 elfmagicword = getbe32(&header.ident0);

	if (elfmagicword != 0x7F454C46)
	{
		fprintf(stderr,"ERROR not a valid ELF\n");
		return 1;
	}

	u32 phoff = getbe32(&header.phoff);
	u16 phnum = getbe16(&header.phnum);
	u32 memsz = 0, filesz = 0;
	u32 vaddr = 0, paddr = 0;

	putbe32(&header.ident1, 0x01020161);
	putbe32(&header.ident2, 0x01000000);
	putbe32(&header.ident3, 0);
	putbe32(&header.machinetype, 0x20028);
	putbe32(&header.version, 1);
	putbe32(&header.flags, 0x606);
	putbe16(&header.ehsize, 0x34);
	putbe16(&header.phentsize, 0x20);
	putbe16(&header.shentsize, 0);
	putbe16(&header.shnum, 0);
	putbe16(&header.shtrndx, 0);
	putbe32(&header.phoff, 0x34);
	putbe32(&header.shoff, 0);

	putbe16(&header.phnum, phnum + 2);

	elfphentry* origentries = new elfphentry[phnum];

	fseek(fin, phoff, SEEK_SET);
	if (fread(origentries, sizeof(elfphentry), phnum, fin) != phnum)
	{
		fprintf(stderr,"ERROR reading program header\n");
		return 1;
	}


	elfphentry* iosphentry = 0;


	// Find zero-address phentry
	for(s32 i=0; i<phnum; i++)
	{
		elfphentry* phentry = &origentries[i];

		if (getbe32(&phentry->paddr) == 0)
		{
			iosphentry = phentry;
		}
	}

	if (0 == iosphentry)
	{
		fprintf(stderr,"ERROR IOS table not found in program header\n");
		return 1;
	}


	elfphentry* entries = new elfphentry[phnum+2];
	offset_size_pair* offsetsizes = new offset_size_pair[phnum];

	elfphentry* q = entries;
	phoff = 0x34;

	for(s32 i=0; i<phnum; i++)
	{
		elfphentry phentry;
		elfphentry* p = &origentries[i];

		offsetsizes[i].offset = getbe32(&p->offset);
		offsetsizes[i].size = getbe32(&p->filesz);

		if (p == iosphentry)
		{
			u32 startoffset = phoff;
			u32 startvaddr = vaddr;
			u32 startpaddr = paddr;
			u32 totalsize = 0;

			filesz = memsz = (phnum+2) * 0x20;

			// PHDR
			putbe32(&phentry.type, 6);
			putbe32(&phentry.offset, phoff);
			putbe32(&phentry.vaddr, paddr);
			putbe32(&phentry.paddr, vaddr);
			putbe32(&phentry.filesz, filesz);
			putbe32(&phentry.memsz, memsz);
			putbe32(&phentry.flags, 0x00F00000);
			putbe32(&phentry.align, 0x4);

			*q++ = phentry;

			phoff += memsz;
			paddr += memsz;
			vaddr += memsz;
			totalsize += memsz;

			filesz = memsz = getbe32(&p->memsz);



			// NOTE
			putbe32(&phentry.type, 4);
			putbe32(&phentry.offset, phoff);
			putbe32(&phentry.vaddr, vaddr);
			putbe32(&phentry.paddr, paddr);
			putbe32(&phentry.filesz, filesz);
			putbe32(&phentry.memsz, memsz);
			putbe32(&phentry.flags, 0x00F00000);
			putbe32(&phentry.align, 0x4);



			*q++ = phentry;

			phoff += memsz;
			paddr += memsz;
			vaddr += memsz;
			totalsize += memsz;

			filesz = memsz = totalsize;

			// LOAD
			putbe32(&phentry.type, 1);
			putbe32(&phentry.offset, startoffset);
			putbe32(&phentry.vaddr, startvaddr);
			putbe32(&phentry.paddr, startpaddr);
			putbe32(&phentry.filesz, totalsize);
			putbe32(&phentry.memsz, totalsize);
			putbe32(&phentry.flags, 0x00F00000);
			putbe32(&phentry.align, 0x4000);

			*q++ = phentry;
		}
		else
		{

			filesz = getbe32(&p->filesz);
			memsz = getbe32(&p->memsz);
			//printf("flags %x\n",getbe32(&p->flags));
			if(strip==getbe32(&p->vaddr) && strip!=0) // strip zeroes
			{
			filesz=1;

			putbe32(&p->filesz,filesz);

			}

			putbe32(&phentry.type, getbe32(&p->type));
			putbe32(&phentry.offset, phoff);
			putbe32(&phentry.vaddr, getbe32(&p->vaddr));
			putbe32(&phentry.paddr, getbe32(&p->paddr));
			putbe32(&phentry.filesz, filesz);
			putbe32(&phentry.memsz, memsz);
			putbe32(&phentry.flags, getbe32(&p->flags)|0x00F00000);
			//putbe32(&phentry.align, getbe32(&p->align));
			putbe32(&phentry.align, 0x4);

			*q++ = phentry;

			phoff += filesz;
		}
	}

	if (write_out(((u8*)&header)+16, sizeof(elfheader)-16, 1, fout) != 1)
	{
		fprintf(stderr,"ERROR writing ELF header\n");
		return 1;
	}

	if (write_out((u8*)entries, sizeof(elfphentry), phnum+2, fout) != (phnum+2))
	{
		fprintf(stderr,"ERROR writing ELF program header\n");
		return 1;
	}

	for(s32 i=0; i<phnum; i++)
	{
		elfphentry *p = &origentries[i];

		u32 offset = getbe32(&p->offset);
		u32 filesz = getbe32(&p->filesz);

		if (filesz)
		{
			fseek(fin, offset, SEEK_SET);

			fprintf(stdout,"Writing segment 0x%08X to 0x%08X (%d bytes) - %x %s\n", getbe32(&p->vaddr), (u32)ftell(fout), filesz,getbe32(&p->memsz),
				(strip==getbe32(&p->vaddr)  && strip!=0 )? "- Stripped" : "");

			unsigned char* data = new unsigned char[filesz];


			if (fread(data, filesz, 1, fin) != 1 || write_out(data, filesz, 1, fout) != 1)
			{
				fprintf(stderr,"ERROR writing segment\n");
				delete[] data;
				return 1;
			}

			delete[] data;
		}
		else
		{
			fprintf(stdout,"Skipping segment 0x%08X\n", getbe32(&p->vaddr));
		}
	}

cleanup:
	if (offsetsizes)
		delete[] offsetsizes;
	if (entries)
		delete[] entries;
	if (origentries)
		delete[] origentries;
	if (fout)
		write_finish(fout);

	if (fin)
		fclose(fin);

	return result;
}

