// Copyright 2007,2008  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// basic data types
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
#ifdef _MSC_VER
typedef unsigned __int64 u64;
#else
typedef unsigned long long u64;
#endif

typedef struct {
    unsigned long state[5];
    unsigned long count[2];
    unsigned char buffer[64];
} SHA1_CTX;

#define rol(value, bits) (((value) << (bits)) | ((value) >> (32 - (bits))))

/* blk0() and blk() perform the initial expand. */
/* I got the idea of expanding during the round function from SSLeay */
#define blk0(i) (block->l[i] = (rol(block->l[i],24)&0xFF00FF00) \
    |(rol(block->l[i],8)&0x00FF00FF))
#define blk(i) (block->l[i&15] = rol(block->l[(i+13)&15]^block->l[(i+8)&15] \
    ^block->l[(i+2)&15]^block->l[i&15],1))

/* (R0+R1), R2, R3, R4 are the different operations used in SHA1 */
#define R0(v,w,x,y,z,i) z+=((w&(x^y))^y)+blk0(i)+0x5A827999+rol(v,5);w=rol(w,30);
#define R1(v,w,x,y,z,i) z+=((w&(x^y))^y)+blk(i)+0x5A827999+rol(v,5);w=rol(w,30);
#define R2(v,w,x,y,z,i) z+=(w^x^y)+blk(i)+0x6ED9EBA1+rol(v,5);w=rol(w,30);
#define R3(v,w,x,y,z,i) z+=(((w|x)&y)|(w&x))+blk(i)+0x8F1BBCDC+rol(v,5);w=rol(w,30);
#define R4(v,w,x,y,z,i) z+=(w^x^y)+blk(i)+0xCA62C1D6+rol(v,5);w=rol(w,30);


/* Hash a single 512-bit block. This is the core of the algorithm. */

void SHA1Transform(unsigned long state[5], const unsigned char buffer[64])
{
	unsigned long a, b, c, d, e;
	typedef union {
	    unsigned char c[64];
	    unsigned long l[16];
	} CHAR64LONG16;
	CHAR64LONG16* block;

	static unsigned char workspace[64];
    block = (CHAR64LONG16*)workspace;
    memcpy(block, buffer, 64);

    /* Copy context->state[] to working vars */
    a = state[0];
    b = state[1];
    c = state[2];
    d = state[3];
    e = state[4];
    /* 4 rounds of 20 operations each. Loop unrolled. */
    R0(a,b,c,d,e, 0); R0(e,a,b,c,d, 1); R0(d,e,a,b,c, 2); R0(c,d,e,a,b, 3);
    R0(b,c,d,e,a, 4); R0(a,b,c,d,e, 5); R0(e,a,b,c,d, 6); R0(d,e,a,b,c, 7);
    R0(c,d,e,a,b, 8); R0(b,c,d,e,a, 9); R0(a,b,c,d,e,10); R0(e,a,b,c,d,11);
    R0(d,e,a,b,c,12); R0(c,d,e,a,b,13); R0(b,c,d,e,a,14); R0(a,b,c,d,e,15);
    R1(e,a,b,c,d,16); R1(d,e,a,b,c,17); R1(c,d,e,a,b,18); R1(b,c,d,e,a,19);
    R2(a,b,c,d,e,20); R2(e,a,b,c,d,21); R2(d,e,a,b,c,22); R2(c,d,e,a,b,23);
    R2(b,c,d,e,a,24); R2(a,b,c,d,e,25); R2(e,a,b,c,d,26); R2(d,e,a,b,c,27);
    R2(c,d,e,a,b,28); R2(b,c,d,e,a,29); R2(a,b,c,d,e,30); R2(e,a,b,c,d,31);
    R2(d,e,a,b,c,32); R2(c,d,e,a,b,33); R2(b,c,d,e,a,34); R2(a,b,c,d,e,35);
    R2(e,a,b,c,d,36); R2(d,e,a,b,c,37); R2(c,d,e,a,b,38); R2(b,c,d,e,a,39);
    R3(a,b,c,d,e,40); R3(e,a,b,c,d,41); R3(d,e,a,b,c,42); R3(c,d,e,a,b,43);
    R3(b,c,d,e,a,44); R3(a,b,c,d,e,45); R3(e,a,b,c,d,46); R3(d,e,a,b,c,47);
    R3(c,d,e,a,b,48); R3(b,c,d,e,a,49); R3(a,b,c,d,e,50); R3(e,a,b,c,d,51);
    R3(d,e,a,b,c,52); R3(c,d,e,a,b,53); R3(b,c,d,e,a,54); R3(a,b,c,d,e,55);
    R3(e,a,b,c,d,56); R3(d,e,a,b,c,57); R3(c,d,e,a,b,58); R3(b,c,d,e,a,59);
    R4(a,b,c,d,e,60); R4(e,a,b,c,d,61); R4(d,e,a,b,c,62); R4(c,d,e,a,b,63);
    R4(b,c,d,e,a,64); R4(a,b,c,d,e,65); R4(e,a,b,c,d,66); R4(d,e,a,b,c,67);
    R4(c,d,e,a,b,68); R4(b,c,d,e,a,69); R4(a,b,c,d,e,70); R4(e,a,b,c,d,71);
    R4(d,e,a,b,c,72); R4(c,d,e,a,b,73); R4(b,c,d,e,a,74); R4(a,b,c,d,e,75);
    R4(e,a,b,c,d,76); R4(d,e,a,b,c,77); R4(c,d,e,a,b,78); R4(b,c,d,e,a,79);
    /* Add the working vars back into context.state[] */
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
    /* Wipe variables */
    a = b = c = d = e = 0;
}


/* SHA1Init - Initialize new context */

void SHA1Init(SHA1_CTX* context)
{
    /* SHA1 initialization constants */
    context->state[0] = 0x67452301;
    context->state[1] = 0xEFCDAB89;
    context->state[2] = 0x98BADCFE;
    context->state[3] = 0x10325476;
    context->state[4] = 0xC3D2E1F0;
    context->count[0] = context->count[1] = 0;
}


/* Run your data through this. */

void SHA1Update(SHA1_CTX* context, const unsigned char* data, unsigned int len)
{
unsigned int i, j;

    j = (context->count[0] >> 3) & 63;
    if ((context->count[0] += len << 3) < (len << 3)) context->count[1]++;
    context->count[1] += (len >> 29);
    if ((j + len) > 63) {
        memcpy(&context->buffer[j], data, (i = 64-j));
        SHA1Transform(context->state, context->buffer);
        for ( ; i + 63 < len; i += 64) {
            SHA1Transform(context->state, &data[i]);
        }
        j = 0;
    }
    else i = 0;
    memcpy(&context->buffer[j], &data[i], len - i);
}


/* Add padding and return the message digest. */

void SHA1Final(unsigned char digest[20], SHA1_CTX* context)
{
	unsigned long i, j;
	unsigned char finalcount[8];

    for (i = 0; i < 8; i++) {
        finalcount[i] = (unsigned char)((context->count[(i >= 4 ? 0 : 1)]
         >> ((3-(i & 3)) * 8) ) & 255);  /* Endian independent */
    }
    SHA1Update(context, (unsigned char *)"\200", 1);
    while ((context->count[0] & 504) != 448) {
        SHA1Update(context, (unsigned char *)"\0", 1);
    }
    SHA1Update(context, finalcount, 8);  /* Should cause a SHA1Transform() */
    for (i = 0; i < 20; i++) {
        digest[i] = (unsigned char)
         ((context->state[i>>2] >> ((3-(i & 3)) * 8) ) & 255);
    }
    /* Wipe variables */
    i = j = 0;
    memset(context->buffer, 0, 64);
    memset(context->state, 0, 20);
    memset(context->count, 0, 8);
    memset(&finalcount, 0, 8);
    SHA1Transform(context->state, context->buffer);
}

u32 be32(const u8 *p)
{
	return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}

void wbe16(u8 *p, u16 x)
{
	p[0] = x >> 8;
	p[1] = x;
}

void wbe32(u8 *p, u32 x)
{
	wbe16(p, x >> 16);
	wbe16(p + 2, x);
}

struct section {
	u32 addr;
	u32 size;
	u32 offset;
	u32 elf_offset;
	u32 str_offset;
};

static void dol2elf(char *inname, char *outname)
{
	u8 dolheader[0x100];
	u8 elfheader[0x400] = {0};
	u8 segheader[0x400] = {0};
	u8 secheader[0x400] = {0};
	u8 strings[0x400] = "\0.strtab";
	u32 str_offset = 9;
	struct section section[19];
	FILE *in, *out;
	u32 n_text, n_data, n_total;
	u32 entry;
	u32 elf_offset;
	u32 i;
	u8 *p;

	in = fopen(inname, "rb");
	fread(dolheader, 1, sizeof dolheader, in);

	elf_offset = 0x1000;

	// 7 text, 11 data
	for (i = 0; i < 18; i++) {
		section[i].offset = be32(dolheader + 4*i);
		section[i].addr = be32(dolheader + 0x48 + 4*i);
		section[i].size = be32(dolheader + 0x90 + 4*i);
		section[i].elf_offset = elf_offset;
		elf_offset += -(-section[i].size & -0x100);
	}

	// bss
	section[18].offset = 0;
	section[18].addr = be32(dolheader + 0xd8);
	section[18].size = be32(dolheader + 0xdc);
	section[18].elf_offset = elf_offset;

	entry = be32(dolheader + 0xe0);

	n_text = 0;
	for (i = 0; i < 7; i++)
		if (section[i].size) {
			sprintf(strings + str_offset, ".text.%d", n_text);
			section[i].str_offset = str_offset;
			str_offset += 8;
			n_text++;
		}

	n_data = 0;
	for ( ; i < 18; i++)
		if (section[i].size) {
			sprintf(strings + str_offset, ".data.%d", n_data);
			section[i].str_offset = str_offset;
			str_offset += i < 16 ? 8 : 9;
			n_data++;
		}

	n_total = n_text + n_data;
	if (section[18].size) {
		sprintf(strings + str_offset, ".bss");
		section[i].str_offset = str_offset;
		str_offset += 5;
		n_total++;
	}

	printf("%d text sections, %d data sections, %d total (includes bss)\n",
	       n_text, n_data, n_total);
	printf("entry point = %08x\n", entry);

	memset(elfheader, 0, sizeof elfheader);
	elfheader[0] = 0x7f;
	elfheader[1] = 0x45;
	elfheader[2] = 0x4c;
	elfheader[3] = 0x46;
	elfheader[4] = 0x01;
	elfheader[5] = 0x02;
	elfheader[6] = 0x01;

	wbe16(elfheader + 0x10, 2);
	wbe16(elfheader + 0x12, 0x14);
	wbe32(elfheader + 0x14, 1);
	wbe32(elfheader + 0x18, entry);
	wbe32(elfheader + 0x1c, 0x400);
	wbe32(elfheader + 0x20, 0x800);
	wbe32(elfheader + 0x24, 0);
	wbe16(elfheader + 0x28, 0x34);
	wbe16(elfheader + 0x2a, 0x20);
	wbe16(elfheader + 0x2c, n_total);
	wbe16(elfheader + 0x2e, 0x28);
	wbe16(elfheader + 0x30, n_total + 2);
	wbe16(elfheader + 0x32, 1);

	p = segheader;
	for (i = 0; i < 19; i++)
		if (section[i].size) {
			wbe32(p + 0x00, 1);
			wbe32(p + 0x04, section[i].elf_offset);
			wbe32(p + 0x08, section[i].addr);
			wbe32(p + 0x0c, section[i].addr);
			wbe32(p + 0x10, i == 18 ? 0 : section[i].size);
			wbe32(p + 0x14, section[i].size);
			wbe32(p + 0x18, i < 7 ? 5 : 6);
			wbe32(p + 0x1c, 0x20);
			p += 0x20;
		}

	p = secheader + 0x28;
	wbe32(p + 0x00, 1);
	wbe32(p + 0x04, 3);
	wbe32(p + 0x08, 0);
	wbe32(p + 0x0c, 0);
	wbe32(p + 0x10, 0xc00);
	wbe32(p + 0x14, 0x400);
	wbe32(p + 0x18, 0);
	wbe32(p + 0x1c, 0);
	wbe32(p + 0x20, 1);
	wbe32(p + 0x24, 0);
	p += 0x28;

	for (i = 0; i < 19; i++)
		if (section[i].size) {
			wbe32(p + 0x00, section[i].str_offset);
			wbe32(p + 0x04, i == 18 ? 8 : 1);
			wbe32(p + 0x08, i < 7 ? 6 : 3);
			wbe32(p + 0x0c, section[i].addr);
			wbe32(p + 0x10, section[i].elf_offset);
			wbe32(p + 0x14, section[i].size);
			wbe32(p + 0x18, 0);
			wbe32(p + 0x1c, 0);
			wbe32(p + 0x20, 0x20);
			wbe32(p + 0x24, 0);
			p += 0x28;
		}

	out = fopen(outname, "wb");
	fwrite(elfheader, 1, sizeof elfheader, out);
	fwrite(segheader, 1, sizeof segheader, out);
	fwrite(secheader, 1, sizeof secheader, out);
	fwrite(strings, 1, sizeof strings, out);

	for (i = 0; i < 19; i++)
		if (section[i].size) {
			p = malloc(section[i].size);
			fseek(in, section[i].offset, SEEK_SET);
			fread(p, 1, section[i].size, in);
			fseek(out, section[i].elf_offset, SEEK_SET);
			fwrite(p, 1, section[i].size, out);
			free(p);
		}

	fclose(out);
	fclose(in);
}

static u8 elf_buf[0x8000];
static char salt[] = "Viva la Riivolution!";

int main(int argc, char **argv)
{
	int i;
	u8 hash[20];
	SHA1_CTX sha1_ctx;
	FILE *elf;

	if (argc != 3) {
		fprintf(stderr, "Usage: %s <dol> <elf>\n", argv[0]);
		exit(1);
	}

	dol2elf(argv[1], argv[2]);

	elf = fopen(argv[2], "rb+");
	if (elf) {
		size_t readed;
		SHA1Init(&sha1_ctx);

		while ((readed=fread(elf_buf, 1, sizeof(elf_buf), elf))>0)
			SHA1Update(&sha1_ctx, elf_buf, readed);

		SHA1Update(&sha1_ctx, salt, 20);
		SHA1Final(hash, &sha1_ctx);

		printf("Hash: ");
		for (i=0; i < sizeof(hash); i++)
			printf("%02X", hash[i]);
		printf("\n");

		fseek(elf, 0, SEEK_END);
		fwrite(hash, 1, sizeof(hash), elf);
		fclose(elf);
	}

	return 0;
}
