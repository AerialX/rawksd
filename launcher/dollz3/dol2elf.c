#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// basic data types
#ifdef _MSC_VER
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
#else
#include <stdint.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
#endif

typedef struct {
    u32 state[5];
    u32 count[2];
    u8 buffer[64];
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

void SHA1Transform(u32 state[5], const u8 buffer[64])
{
	u32 a, b, c, d, e;
	typedef union {
	    u8 c[64];
	    u32 l[16];
	} CHAR64LONG16;
	CHAR64LONG16* block;

	static u8 workspace[64];
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

void SHA1Update(SHA1_CTX* context, const u8* data, u32 len)
{
	u32 i, j;

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

void SHA1Final(u8 digest[20], SHA1_CTX* context)
{
	u32 i, j;
	u8 finalcount[8];

    for (i = 0; i < 8; i++) {
        finalcount[i] = (u8)((context->count[(i >= 4 ? 0 : 1)]
         >> ((3-(i & 3)) * 8) ) & 255);  /* Endian independent */
    }
    SHA1Update(context, (u8 *)"\200", 1);
    while ((context->count[0] & 504) != 448) {
        SHA1Update(context, (u8 *)"\0", 1);
    }
    SHA1Update(context, finalcount, 8);  /* Should cause a SHA1Transform() */
    for (i = 0; i < 20; i++) {
        digest[i] = (u8)
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

/* stub magic to init CPU for dollz loader */
static u8 stub [] = {
	0x3C, 0x60, 0x00, 0x11, // li r3, 0x0011
	0x60, 0x63, 0xCC, 0x64, // ori r3, r3, 0xCC64
	0x4C, 0x00, 0x01, 0x2C, // isync
	0x7C, 0x70, 0xFB, 0xA6, // mtspr HID0, r3
	0x4C, 0x00, 0x01, 0x2C, // isync
	0x38, 0x80, 0x20, 0x00, // li r4, 0x2000
	0x7C, 0x80, 0x01, 0x24, // mtmsr r4
	0x4C, 0x00, 0x01, 0x2C, // isync

	0x38, 0x00, 0x00, 0x00, // li r0, 0
	0x7C, 0x10, 0x83, 0xA6, // mtspr IBAT0U, r0
	0x7C, 0x12, 0x83, 0xA6, // mtspr IBAT1U, r0
	0x7C, 0x14, 0x83, 0xA6, // mtspr IBAT2U, r0
	0x7C, 0x16, 0x83, 0xA6, // mtspr IBAT3U, r0
	0x7C, 0x10, 0x8B, 0xA6, // mtspr IBAT4U, r0
	0x7C, 0x12, 0x8B, 0xA6, // mtspr IBAT5U, r0
	0x7C, 0x18, 0x83, 0xA6, // mtspr DBAT0U, r0
	0x7C, 0x1A, 0x83, 0xA6, // mtspr DBAT1U, r0
	0x7C, 0x1C, 0x83, 0xA6, // mtspr DBAT2U, r0
	0x7C, 0x1E, 0x83, 0xA6, // mtspr DBAT3U, r0
	0x7C, 0x18, 0x8B, 0xA6, // mtspr DBAT4U, r0
	0x7C, 0x1A, 0x8B, 0xA6, // mtspr DBAT5U, r0
	0x4C, 0x00, 0x01, 0x2C, // isync

	0x3C, 0x00, 0x80, 0x00, // lis r0, 0x8000
	0x7C, 0x00, 0x01, 0xA4, // mtsr sr0, r0
	0x7C, 0x01, 0x01, 0xA4, // mtsr sr1, r0
	0x7C, 0x02, 0x01, 0xA4, // mtsr sr2, r0
	0x7C, 0x03, 0x01, 0xA4, // mtsr sr3, r0
	0x7C, 0x04, 0x01, 0xA4, // mtsr sr4, r0
	0x7C, 0x05, 0x01, 0xA4, // mtsr sr5, r0
	0x7C, 0x06, 0x01, 0xA4, // mtsr sr6, r0
	0x7C, 0x07, 0x01, 0xA4, // mtsr sr7, r0
	0x7C, 0x08, 0x01, 0xA4, // mtsr sr8, r0
	0x7C, 0x09, 0x01, 0xA4, // mtsr sr9, r0
	0x7C, 0x0A, 0x01, 0xA4, // mtsr sr10, r0
	0x7C, 0x0B, 0x01, 0xA4, // mtsr sr11, r0
	0x7C, 0x0C, 0x01, 0xA4, // mtsr sr12, r0
	0x7C, 0x0D, 0x01, 0xA4, // mtsr sr13, r0
	0x7C, 0x0E, 0x01, 0xA4, // mtsr sr14, r0
	0x7C, 0x0F, 0x01, 0xA4, // mtsr sr15, r0
	0x4C, 0x00, 0x01, 0x2C, // isync

	0x38, 0x80, 0x00, 0x02, // li r4, 2
	0x3C, 0x60, 0x80, 0x00, // lis r3, 0x8000
	0x38, 0x63, 0x1F, 0xFF, // addi r3, r3, 0x1FFF
	0x4C, 0x00, 0x01, 0x2C, // isync
	0x7C, 0x99, 0x83, 0xA6, // mtspr DBAT0L, r4
	0x7C, 0x78, 0x83, 0xA6, // mtspr DBAT0U, r3
	0x7C, 0x91, 0x83, 0xA6, // mtspr IBAT0L, r4
	0x7C, 0x70, 0x83, 0xA6, // mtspr IBAT0U, r3
	0x4C, 0x00, 0x01, 0x2C, // isync

	0x3C, 0x84, 0x10, 0x00, // addis r4, r4, 0x1000
	0x3C, 0x63, 0x10, 0x00, // addis r3, r3, 0x1000
	0x4C, 0x00, 0x01, 0x2C, // isync
	0x7C, 0x99, 0x8B, 0xA6, // mtspr DBAT4L, r4
	0x7C, 0x78, 0x8B, 0xA6, // mtspr DBAT4U, r3
	0x7C, 0x91, 0x8B, 0xA6, // mtspr IBAT4L, r4
	0x7C, 0x70, 0x8B, 0xA6, // mtspr IBAT4U, r3
	0x4C, 0x00, 0x01, 0x2C, // isync

	0x38, 0x80, 0x00, 0x2A, // li r4, 0x2A
	0x3C, 0x63, 0x30, 0x00, // addis r3, r3, 0x3000
	0x4C, 0x00, 0x01, 0x2C, // isync
	0x7C, 0x9B, 0x83, 0xA6, // mtspr DBAT1L, r4
	0x7C, 0x7A, 0x83, 0xA6, // mtspr DBAT1U, r3
	0x7C, 0x93, 0x83, 0xA6, // mtspr IBAT1L, r4
	0x7C, 0x72, 0x83, 0xA6, // mtspr IBAT1U, r3
	0x4C, 0x00, 0x01, 0x2C, // isync

	0x3C, 0x84, 0x10, 0x00, // addis r4, r4, 0x1000
	0x3C, 0x63, 0x10, 0x00, // addis r3, r3, 0x1000
	0x4C, 0x00, 0x01, 0x2C, // isync
	0x7C, 0x9D, 0x83, 0xA6, // mtspr DBAT2L, r4
	0x7C, 0x7C, 0x83, 0xA6, // mtspr DBAT2U, r3
	0x7C, 0x95, 0x83, 0xA6, // mtspr IBAT2L, r4
	0x7C, 0x74, 0x83, 0xA6, // mtspr IBAT2U, r3
	0x4C, 0x00, 0x01, 0x2C, // isync

	0x7C, 0x60, 0x00, 0xA6, // mfmsr r3
	0x60, 0x63, 0x00, 0x30, // ori r3, r3, MSR_DR|MSR_IR
	0x7C, 0x7B, 0x03, 0xA6, // mtsrr1 r3
	0x3C, 0x60, 0x00, 0x00, // lis r3, _entry@h
	0x60, 0x63, 0x00, 0x00, // ori r3, r3, _entry@l
	0x7C, 0x7A, 0x03, 0xA6, // mtsrr0 r3
	0x4C, 0x00, 0x00, 0x64  // rfi
};

struct section {
	u32 addr;
	u32 size;
	u32 offset;
	u32 elf_offset;
};

static void dol2elf(char *inname, char *outname)
{
	u8 dolheader[0x100];
	u8 elfheader[0x34];
	u8 segheader[0x240] = {0};
	struct section section[19];
	FILE *in, *out;
	u32 n_text, n_data, n_total;
	u32 entry;
	u32 elf_offset;
	u32 i;
	u8 *p;

	in = fopen(inname, "rb");
	fread(dolheader, 1, sizeof dolheader, in);

	memset(section, 0, sizeof section);
	elf_offset = 0x300;

	// 7 text, 11 data
	// first text section is the channel stub
	section[0].addr = 0x80003400;
	section[0].size = sizeof(stub);
	section[0].elf_offset = elf_offset;

	for (i = 0; i < 17; i++) {
		elf_offset += -(-section[i].size & -0x100);
		section[i+1].offset = be32(dolheader + 4*i);
		section[i+1].addr = be32(dolheader + 0x48 + 4*i);
		section[i+1].size = be32(dolheader + 0x90 + 4*i);
		section[i+1].elf_offset = elf_offset;
	}

	// bss
	section[18].addr = be32(dolheader + 0xd8);
	section[18].size = be32(dolheader + 0xdc);

	entry = be32(dolheader + 0xe0);

	stub[sizeof(stub)-14] = (entry >> 24) | 0x80; // virtual address
	stub[sizeof(stub)-13] = entry >> 16;
	stub[sizeof(stub)-10] = entry >> 8;
	stub[sizeof(stub)-9] = entry;

	n_text = 0;
	for (i = 0; i < 7; i++)
		if (section[i].size)
			n_text++;

	n_data = 0;
	for ( ; i < 18; i++)
		if (section[i].size)
			n_data++;

	n_total = n_text + n_data;
	if (section[18].size)
		n_total++;

	printf("%d text sections, %d data sections, %d total (includes bss)\n",
	       n_text, n_data, n_total);
	printf("entry point = %08x\n", entry);

	memset(elfheader, 0, sizeof elfheader);
	elfheader[0] = 0x7f; // ELF magic
	elfheader[1] = 0x45;
	elfheader[2] = 0x4c;
	elfheader[3] = 0x46;
	elfheader[4] = 0x01; // ELFCLASS32
	elfheader[5] = 0x02; // ELFDATA2MSB
	elfheader[6] = 0x01; // EV_CURRENT
	elfheader[7] = 0x00; // change to ninty's EI_OSABI(0x61) when installing on NAND because IOS is stupid)

	wbe16(elfheader + 0x10, 2);                       // e_type = ET_EXEC
	wbe16(elfheader + 0x12, 0x14);                    // e_machine (change to EM_ARM(0x28) when.. see above)
	wbe32(elfheader + 0x14, 1);                       // e_version = EV_CURRENT
	wbe32(elfheader + 0x18, entry);                   // e_entry
	wbe32(elfheader + 0x1c, sizeof(elfheader));       // e_phoff (0x34)
	wbe32(elfheader + 0x20, 0);                       // e_shoff
	wbe32(elfheader + 0x24, 0x80000000);              // e_flags (powerpc)
	wbe16(elfheader + 0x28, sizeof(elfheader));       // e_ehsize (0x34)
	wbe16(elfheader + 0x2a, 0x20);                    // e_phentsize (0x20)
	wbe16(elfheader + 0x2c, n_total);                 // e_phnum
	wbe16(elfheader + 0x2e, 0x28);                    // e_shentsize
	wbe16(elfheader + 0x30, 0);                       // e_shnum
	wbe16(elfheader + 0x32, 0);                       // e_shstrndx = SHN_UNDEF

	p = segheader;
	for (i = 0; i < 19; i++)
		if (section[i].size) {
			wbe32(p + 0x00, 1);                             // p_type = PT_LOAD
			wbe32(p + 0x04, section[i].elf_offset);         // p_offset
			wbe32(p + 0x08, section[i].addr);               // p_vaddr, use physical address for IOS
			wbe32(p + 0x0c, i ? section[i].addr : 0x80004000); // p_paddr (hack for section 0 for HBC)
			wbe32(p + 0x10, i == 18 ? 0 : section[i].size); // p_filesz
			wbe32(p + 0x14, section[i].size);               // p_memsz
			wbe32(p + 0x18, 0x07F00000 | (i < 7 ? 5 : 6));  // p_flags, tweaked for IOS
			wbe32(p + 0x1c, 0x20);                          // p_align
			p += 0x20;
		}

	out = fopen(outname, "wb");
	fwrite(elfheader, 1, sizeof elfheader, out);
	fwrite(segheader, 1, sizeof segheader, out);

	fseek(out, section[0].elf_offset, SEEK_SET);
	fwrite(stub, 1, section[0].size, out);

	for (i = 1; i < 19; i++)
		if (section[i].size && section[i].offset) {
			p = (u8 *)malloc(section[i].size);
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
static u8 salt[] = "Viva la Riivolution!";

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
