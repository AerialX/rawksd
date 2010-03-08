/*  patchmii_core -- low-level functions to handle the downloading, patching
    and installation of updates on the Wii

    Copyright (C) 2008 bushing / hackmii.com

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 2.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ogcsys.h>
#include <gccore.h>
#include <stdarg.h>
#include <ctype.h>
#include <unistd.h>
#include <network.h>
#include <sys/errno.h>
#include <wiiuse/wpad.h>
#include <fat.h>
#include <sys/stat.h>

#include "certs_dat.h"
#include "patchmii_core.h"
#include "sha1.h"
#include "debug.h"
#include "http.h"
#include "mload_dat.h"

#include <map>
#include <vector>
using std::map;
using std::vector;

#define VERSION "0.1"

#define ALIGN(a,b) ((((a)+(b)-1)/(b))*(b))

map<u64, map<u32, u8*> > contents;
map<u64, map<u32, u8*> > tmds;
map<u64, u8*> tickets;

map<u64, map<u32, u32> > contentsize;

int http_status = 0;
int tmd_dirty = 0, tik_dirty = 0;

#define debug_printf printf
#define debug_printf(...)

s32 __ES_Close(void);
s32 __ES_Init(void);

// RawkEMU /dev/fs hook
unsigned char fs_orig1[51] = {0x10, 0x20, 0x05, 0x42, 0x40, 0x21, 0x00, 0x4C, 0x07, 0x00, 0xCB, 0x19,
0x1A, 0x68, 0x13, 0x2B, 0x00, 0xD1, 0x03, 0x23, 0x01, 0x60, 0x13, 0x1C,
0x10, 0xE0, 0x02, 0x31, 0x01, 0x29, 0x01, 0xD9, 0xF3, 0xBC, 0x10, 0xBC,
0x02, 0x47, 0x08, 0x20, 0x00, 0x90, 0x04, 0xB5, 0x00, 0x1C, 0x02, 0x20,
0x04, 0x42, 0x40};
unsigned char fs_patch1[51] = {0x00, 0x48, 0x0A, 0x22, 0x00, 0x2B, 0x06, 0xD1, 0x05, 0x68, 0xE1, 0x29,
0x6F, 0xD0, 0x07, 0x22, 0x01, 0x29, 0x64, 0xD0, 0x04, 0x68, 0x00, 0x28,
0x01, 0xD1, 0x04, 0x48, 0x04, 0x47, 0x00, 0x60, 0x02, 0x21, 0x01, 0x20,
0x01, 0xBC, 0x04, 0x47, 0x10, 0x46, 0xC0, 0x20, 0x00, 0x90, 0x04, 0x13,
0x8C, 0x00, 0x29};
unsigned char fs_orig2[16] = {0xD1, 0x02, 0xF7, 0xFF, 0xFD, 0xF2, 0xE0, 0x1B, 0x46, 0x43, 0x2B, 0x01,0xD1, 0x07, 0x68, 0xA8};
unsigned char fs_patch2[1] = {0xE0};
unsigned char fs_orig3[38] = {0xD0, 0x09, 0x68, 0xA0, 0xF7, 0xFF, 0xF9, 0xB4, 0x28, 0x00, 0xD1, 0x04,
0x1C, 0x20, 0xF7, 0xFF, 0xFA, 0x69, 0x1C, 0x01, 0xE0, 0x38, 0x68, 0x23,
0x2B, 0x01, 0xD0, 0x09, 0x68, 0xA0, 0xF0, 0x00, 0xFB, 0x67, 0x28, 0x00,
0xD1, 0x04};
unsigned char fs_patch3[38] = {0xD1, 0x0C, 0x68, 0x23, 0xF7, 0xFF, 0xF9, 0x9E, 0x28, 0x00, 0xD0, 0x11,
0x46, 0xC0, 0x46, 0xC0, 0x46, 0xC0, 0x46, 0xC0, 0x46, 0xC0, 0x46, 0xC0,
0x46, 0xC0, 0xE0, 0x35, 0x68, 0xA0, 0xF0, 0x00, 0xFB, 0x67, 0x28, 0x00,
0xD1, 0xED};
u32 fs_patch1_pos=0xCA89;
u32 fs_patch2_pos=0xCE9E;
u32 fs_patch3_pos=0xD744;

unsigned char gpio_orig[8] = {0xD1, 0x0F, 0x28, 0xFC, 0xD0, 0x33, 0x28, 0xFC};
unsigned char gpio_patch[2] = {0x46, 0xC0};
// u32 gpio_patch_pos = 0x22DBE; // IOS36.1042
u32 gpio_patch_pos = 0x22DE6; // IOS36.3351


static bool netinited = false;
int initnet()
{
	if (netinited)
		return true;
	
	while (true) {
		int retval = net_init();
		if (retval < 0) {
			if (retval != -EAGAIN) {
				printf("Failed to initialize network!\n");
				exit(0);
			}
		}
		if (!retval) break;
		usleep(100000);
		printf(".");
	}
	
	netinited = true;
	
	return netinited;
}

static bool fatinited = false;
int initfat()
{
	if (fatinited)
		return true;
	
	fatinited = fatInitDefault();
	return fatinited;
}

int apply_patch(u8 *data, u32 offset, u8 *orig, u32 orig_size, u8 *patch, u32 patch_size)
{
	if (memcmp(&data[offset], orig, orig_size) == 0) {
		memcpy(&data[offset], patch, patch_size);
		return -1;
	} else {
		return 0;
	}
}

void itdied(int param)
{
	printf("\nPress HOME to exit...\n");
	while (true) {
		WPAD_ScanPads();
		u32 pressed = WPAD_ButtonsDown(0);

		if (pressed & WPAD_BUTTON_HOME) {
			VIDEO_SetBlack(true);
			VIDEO_Flush();
			VIDEO_WaitVSync(); VIDEO_WaitVSync();
			//SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);
			exit(0);
		}
	}
}

bool pressa()
{
	while (true) {
		WPAD_ScanPads();
		u32 pressed = WPAD_ButtonsDown(0);

		if (pressed & WPAD_BUTTON_A)
			return true;
		if (pressed & WPAD_BUTTON_B)
			return false;
		if (pressed & WPAD_BUTTON_HOME)
			exit(0);
	}
}

#define exit itdied

#define ROUND_UP(x, n) (-(-(x) & -(n)))

int findunusedcid(tmd* p_tmd)
{
	int cid = 0x100;
	bool found = false;
	while (!found) {
		cid++;
		found = true;
		for (int i = 0; i < p_tmd->num_contents; i++)
			if (p_tmd->contents[i].cid == cid)
				found = false;
	}
	
	return cid;
}

void tmd_add_module(tmd *p_tmd,const u8 *elf, u32 elf_size)
{
	int cid = findunusedcid(p_tmd);
	int content_size = ROUND_UP(elf_size, 0x40);
	u8 *buf = (u8*)memalign(32, content_size);
	int index = p_tmd->num_contents;
	memset(buf, 0, content_size);
	memcpy(buf, elf, elf_size);
	
	p_tmd->contents[index].cid = cid;
	p_tmd->contents[index].type = 0x8001; // shared
	p_tmd->contents[index].size = content_size;
	p_tmd->contents[index].index = index;
	SHA1(buf, content_size, p_tmd->contents[index].hash);
	p_tmd->num_contents++;
	
	contents[p_tmd->title_id][cid] = buf;
	contentsize[p_tmd->title_id][cid] = content_size;
	
	tmd_dirty = 1;
}

int add_custom_modules(tmd *p_tmd)
{
	tmd_add_module(p_tmd, mload_dat, mload_dat_size);
	
	// Inverse mload and oh0 place in tmd
	tmd_content tmp = p_tmd->contents[3];
	p_tmd->contents[3] = p_tmd->contents[p_tmd->num_contents - 1];
	p_tmd->contents[p_tmd->num_contents - 1] = tmp;
	p_tmd->contents[3].index = 3;
	p_tmd->contents[p_tmd->num_contents - 1].index = p_tmd->num_contents - 1;
	
	return 1;
}

u8 commonkey[16] = { 0xeb, 0xe4, 0x2a, 0x22, 0x5e, 0x85, 0x93, 0xe4, 0x48, 0xd9, 0xc5, 0x45, 0x73, 0x81, 0xaa, 0xf7 };

int patch_diVid(u8* buf, u32 size) { 
	u32 i, match_count = 0; 
	const u8 old_table[] = {0x46, 0x51, 0x29, 0x00, 0xD1, 0x40, 0x68, 0xB3, 0x2B, 0x00};
	for (i=0; i<size-sizeof old_table; i++) {
		if (!memcmp(buf + i, old_table, 4) && !memcmp(buf+i+6, old_table+6, sizeof(old_table)-6)) {
			debug_printf("Found di_dvd check, patching...\n");
			buf[i] = 0x21;
			buf[i+1] = 0;
			debug_printf("Done\n");
			i+= sizeof(old_table);
			match_count++;
			continue;
		}
	}
	return match_count;
}

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

void console_setup(void) {
  VIDEO_Init();
  PAD_Init();
  
  rmode = VIDEO_GetPreferredMode(NULL);

  xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
  VIDEO_ClearFrameBuffer(rmode,xfb,COLOR_BLACK);
  VIDEO_Configure(rmode);
  VIDEO_SetNextFramebuffer(xfb);
  VIDEO_SetBlack(FALSE);
  VIDEO_Flush();
  VIDEO_WaitVSync();
  if(rmode->viTVMode & VI_NON_INTERLACE)
	  VIDEO_WaitVSync();
  CON_InitEx(rmode, 20, 30, rmode->fbWidth - 40, rmode->xfbHeight - 60);
}

int get_nus_object(u32 titleid1, u32 titleid2, const char *content, u8 **outbuf, u32 *outlen) {
	static char buf[128];
	
	if (initfat()) {
		snprintf(buf, 128, "/patch/%08x/%08x/%s", titleid1, titleid2, content);
		struct stat st;
		if (stat(buf, &st) == 0) {
			FILE* fd = fopen(buf, "rb");
			if (fd != NULL) {
				*outbuf = (u8*)memalign(32, st.st_size);
				*outlen = fread(*outbuf, 1, st.st_size, fd);
				fclose(fd);
				
				return 0;
			}
		}
	}
	
	if (initnet()) {
		u32 http_status;

		snprintf(buf, 128, "http://nus.cdn.shop.wii.com/ccs/download/%08x%08x/%s", titleid1, titleid2, content);

		int retval = http_request(buf, 1 << 31);
		if (!retval) {
			return -1;
		}
		
		retval = http_get_result(&http_status, outbuf, outlen); 
		if (((int)*outbuf & 0xF0000000) == 0xF0000000) {
			return (int)*outbuf;
		}
		
		return 0;
	}
	
	printf("Could not find the necessary files for patching.\nPlease make sure your internet connection is working or you've placed the appropriate files on your SD card.\n");
	exit(0);
}

void decrypt_buffer(u16 index, u8 *source, u8 *dest, u32 len) {
	static u8 iv[16];
	if (!source)
		exit(1);
	if (!dest)
		exit(1);

	memset(iv, 0, 16);
	memcpy(iv, &index, 2);
	aes_decrypt(iv, source, dest, len);
}

static u8 encrypt_iv[16];
void set_encrypt_iv(u16 index) {
	memset(encrypt_iv, 0, 16);
	memcpy(encrypt_iv, &index, 2);
}
  
void encrypt_buffer(u8 *source, u8 *dest, u32 len) {
	aes_encrypt(encrypt_iv, source, dest, len);
}

u8* getcontent(int title_h, int title_l, int contentid, u32* size);
s32 install_nus_object(tmd *p_tmd, u16 index) {
	static u8 bounce_buf1[1024] ATTRIBUTE_ALIGN(0x20);
	static u8 bounce_buf2[1024] ATTRIBUTE_ALIGN(0x20);
	u8* buffer;
	int retval, ret, cfd;
	set_encrypt_iv(index);
	
	buffer = getcontent(p_tmd->title_id >> 32, (u32)p_tmd->title_id, p_tmd->contents[index].cid, NULL);

	cfd = ES_AddContentStart(p_tmd->title_id, p_tmd->contents[index].cid);
	if (cfd < 0) {
		debug_printf("ES_AddContentStart(%08x, %08x) = %d", p_tmd->title_id, p_tmd->contents[index].cid, ret);
		ES_AddTitleCancel();
		return -1;
	}
	for (u32 i = 0; i < p_tmd->contents[index].size;) {
		u32 numbytes = ((p_tmd->contents[index].size - i) < 1024) ? p_tmd->contents[index].size - i : 1024;
		numbytes = ALIGN(numbytes, 32);
		memcpy(bounce_buf1, buffer + i, numbytes);
		retval = numbytes;
		encrypt_buffer(bounce_buf1, bounce_buf2, sizeof(bounce_buf1));
		ret = ES_AddContentData(cfd, bounce_buf2, retval);
		if (ret < 0) {
			ES_AddContentFinish(cfd);
			ES_AddTitleCancel();
			return ret;
		}
		i += retval;
	}
	ret = ES_AddContentFinish(cfd);
	if(ret < 0) {
		debug_printf("ES_AddContentFinish(%08x) = %d", cfd, ret);
		ES_AddTitleCancel();
		return -1;
	}
	
	return 0;
}

void get_title_key(signed_blob *s_tik)
{
	static u8 iv[16] ATTRIBUTE_ALIGN(0x20);
	static u8 keyin[16] ATTRIBUTE_ALIGN(0x20);
	static u8 keyout[16] ATTRIBUTE_ALIGN(0x20);

	tik *p_tik;
	p_tik = (tik*)SIGNATURE_PAYLOAD(s_tik);
	u8 *enc_key = (u8 *)&p_tik->cipher_title_key;
	memcpy(keyin, enc_key, sizeof keyin);
	memset(keyout, 0, sizeof keyout);
	memset(iv, 0, sizeof iv);
	memcpy(iv, &p_tik->titleid, sizeof p_tik->titleid);
 
	aes_set_key(commonkey);
	aes_decrypt(iv, keyin, keyout, sizeof keyin);
	
	aes_set_key(keyout);
}

int change_ticket_title_id(signed_blob *s_tik, u32 titleid1, u32 titleid2) {
	static u8 iv[0x10] ATTRIBUTE_ALIGN(0x20);
	static u8 keyout[0x10] ATTRIBUTE_ALIGN(0x20);
	int retval;

	tik *p_tik = (tik*)SIGNATURE_PAYLOAD(s_tik);
	memset(keyout, 0, sizeof keyout);
	memset(iv, 0, sizeof iv);
	memcpy(iv, &p_tik->titleid, sizeof p_tik->titleid);

	aes_set_key(commonkey);
	aes_decrypt(iv, p_tik->cipher_title_key, keyout, 0x10);
	p_tik->titleid = (u64)titleid1 << 32 | (u64)titleid2;
	memset(iv, 0, sizeof iv);
	memcpy(iv, &p_tik->titleid, sizeof p_tik->titleid);
	
	aes_encrypt(iv, keyout, p_tik->cipher_title_key, 0x10);

	aes_set_key(keyout);
	
	tik_dirty = 1;

	return retval;
}

void change_tmd_title_id(signed_blob *s_tmd, u32 titleid1, u32 titleid2) {
	tmd *p_tmd;
	u64 title_id = (u64)titleid1 << 32;
	title_id |= titleid2;
	p_tmd = (tmd*)SIGNATURE_PAYLOAD(s_tmd);
	p_tmd->title_id = title_id;
	tmd_dirty = 1;
}

int patch_hash_check(u8 *buf, u32 size) {
	u32 i;
	u32 match_count = 0;
	u8 new_hash_check[] = {0x20,0x07,0x4B,0x0B};
	u8 old_hash_check[] = {0x20,0x07,0x23,0xA2};
  
	for (i=0; i<size-4; i++) {
		if (!memcmp(buf + i, new_hash_check, sizeof new_hash_check)) {
			debug_printf("Found new-school ES hash check @ 0x%x, patching.\n", i);
			buf[i+1] = 0;
			i += 4;
			match_count++;
			continue;
		}

		if (!memcmp(buf + i, old_hash_check, sizeof old_hash_check)) {
			debug_printf("Found old-school ES hash check @ 0x%x, patching.\n", i);
			buf[i+1] = 0;
			i += 4;
			match_count++;
			continue;
		}
	}
	return match_count;
}

int patch_new_dvdlowunencrypted(u8 *buf, u32 size)
{
	u32 i;
	u32 match_count = 0;
	u8 old_table[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x7E, 0xD4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08};
	u8 new_table[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x46, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x7E, 0xD4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08};
	
	for (i=0; i<size-sizeof old_table; i++) {
		if (!memcmp(buf + i, old_table, sizeof old_table)) {
			debug_printf("Found new-school DVD_LowUnencryptedRead whitelist @ 0x%x, patching.\n", i);
			memcpy(buf + i, new_table, sizeof new_table);
			i += sizeof new_table;
			match_count++;
			continue;
		}
	}
	return match_count;
}

int patch_identify_check(u8 *buf, u32 size)
{
	u32 match_count = 0;
	u8 identify_check[] = { 0x28, 0x03, 0xD1, 0x23 };
	u32 i;
	
	for(i = 0; i < size - 4; i++)
	{
		if(!memcmp(buf + i, identify_check, sizeof identify_check))
		{
			debug_printf("Found ES Identify check @ 0x%x, patching... ", i);
			buf[i+2] = 0;
			buf[i+3] = 0;
			debug_printf("Done\n");
			i += 4;
			match_count++;
			continue;
		}
	}
	
	return match_count;
}

int FindInBuffer(const u8* buffer, const int bufferlength, const char* pattern, const int patternlength)
{
	for (int i = 0; i < bufferlength - patternlength; i++) {
		if (!memcmp(buffer + i, pattern, patternlength))
			return i;
	}
	
	return -1;
}

int patch_di_do(u8 *Address, int Size)
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

int patch_fsperms(u8 *buf, u32 size) 
{
	u32 i;
	u32 match_count = 0;
	u8 old_table[] = {0x42, 0x8B, 0xD0, 0x01, 0x25, 0x66};
	u8 new_table[] = {0x42, 0x8B, 0xE0, 0x01, 0x25, 0x66};
  
	for (i=0; i<size-sizeof old_table; i++) 
	{
		if (!memcmp(buf + i, old_table, sizeof old_table)) 
		{
			debug_printf("Found filesystem permissions, patching...\n");
			memcpy(buf + i, new_table, sizeof new_table);
			i += sizeof new_table;
			match_count++;
			continue;
		}
	}
	return match_count;
}

void zero_sig(signed_blob *sig) {
	u8 *sig_ptr = (u8 *)sig;
	memset(sig_ptr + 4, 0, SIGNATURE_SIZE(sig)-4);
}

void brute_tmd(tmd *p_tmd) {
	u16 fill;
	for(fill=0; fill<65535; fill++) {
		p_tmd->fill3=fill;
		sha1 hash;
		//    debug_printf("SHA1(%p, %x, %p)\n", p_tmd, TMD_SIZE(p_tmd), hash);
		SHA1((u8 *)p_tmd, TMD_SIZE(p_tmd), hash);;
  
		if (hash[0]==0) {
		//      debug_printf("setting fill3 to %04hx\n", fill);
			return;
		}
	}
	debug_printf("Unable to fix tmd :(\n");
	printf("Failure.\n");
	exit(4);
}

void brute_tik(tik *p_tik) {
	u16 fill;
	for(fill=0; fill<65535; fill++) {
		p_tik->padding=fill;
		sha1 hash;
		//    debug_printf("SHA1(%p, %x, %p)\n", p_tmd, TMD_SIZE(p_tmd), hash);
		SHA1((u8 *)p_tik, sizeof(tik), hash);
  
		if (hash[0]==0) return;
	}
	debug_printf("Unable to fix tik :(\n");
	printf("Failure!\n");
	exit(5);
}
    
void forge_tmd(signed_blob *s_tmd) {
	debug_printf("forging tmd sig\n");
	zero_sig(s_tmd);
	brute_tmd((tmd*)SIGNATURE_PAYLOAD(s_tmd));
}

void forge_tik(signed_blob *s_tik) {
	debug_printf("forging tik sig\n");
	zero_sig(s_tik);
	brute_tik((tik*)SIGNATURE_PAYLOAD(s_tik));
}

#define BLOCK 0x1000

s32 install_ticket(const signed_blob *s_tik, const signed_blob *s_certs, u32 certs_len) {
	u32 ret;

	debug_printf("Installing ticket...\n");
	ret = ES_AddTicket(s_tik,STD_SIGNED_TIK_SIZE,s_certs,certs_len, NULL, 0);
	if (ret < 0) {
		debug_printf("ES_AddTicket failed: %d\n",ret);
		return ret;
	}
	return 0;
}

extern s32 __ES_Close();
extern s32 __ES_Init();
s32 install(const signed_blob *s_tmd, const signed_blob *s_certs, u32 certs_len) {
	u32 ret, i;
	tmd *p_tmd = (tmd*)SIGNATURE_PAYLOAD(s_tmd);
	debug_printf("Adding title...\n");

	ret = ES_AddTitleStart(s_tmd, SIGNED_TMD_SIZE(s_tmd), s_certs, certs_len, NULL, 0);

	if(ret < 0) {
		debug_printf("ES_AddTitleStart failed: %d\n",ret);
		ES_AddTitleCancel();
		return ret;
	}

	for(i = 0; i < p_tmd->num_contents; i++) {
		printf(".");
		debug_printf("Adding content ID %08x", i);
		ret = install_nus_object((tmd *)SIGNATURE_PAYLOAD(s_tmd), i);
		if (ret) return ret;
	}

	ret = ES_AddTitleFinish();
	if(ret < 0) {
		debug_printf("ES_AddTitleFinish failed: %d\n",ret);
		ES_AddTitleCancel();
		return ret;
	}

	debug_printf("Installation complete!\n");
	return 0;
}

u8* gettmd(int title_h, int title_l, int version)
{
	u64 titleid = ((u64)title_h << 32) | (u64)title_l;
	u8* tm = tmds[titleid][version];
	if (tm != NULL)
		return tm;
	
	tm = (u8*)memalign(32, MAX_SIGNED_TMD_SIZE);
	
	u32 tmdsize;
	char tmdstr[10];
	if (version > 0)
		sprintf(tmdstr, "tmd.%d", version);
	else
		strcpy(tmdstr, "tmd");
	
	u8* tm2 = NULL;
	
	int retval = get_nus_object(title_h, title_l, tmdstr, &tm2, &tmdsize);
	
	if (retval < 0) {
		debug_printf("get_nus_object(tmd) returned %d, tmdsize = %u\n", retval, tmdsize);
		printf("Network failure!\n");
		exit(1);
	}
	if (tm2 == NULL) {
		debug_printf("Failed to allocate temp buffer for encrypted content, size was %u\n", tmdsize);
		printf("Network failure!\n");
		exit(1);
	}
	
	memcpy(tm, tm2, tmdsize);
	
	tmds[titleid][version] = tm;
	
	return tm;
}

u8* getticket(int title_h, int title_l)
{
	u64 titleid = (u64)title_h << 32 | (u64)title_l;
	u8* tm = tickets[titleid];
	if (tm != NULL)
		return tm;
	
	u32 size;
	
	int retval = get_nus_object(title_h, title_l, "cetk", &tm, &size);
	
	if (retval < 0) {
		debug_printf("get_nus_object(tmd) returned %d, size = %u\n", retval, size);
		printf("Network failure!\n");
		exit(1);
	}
	if (tm == NULL) {
		debug_printf("Failed to allocate temp buffer for encrypted content, size was %u\n", size);
		printf("Network failure!\n");
		exit(1);
	}
	
	tickets[titleid] = tm;
	
	return tm;
}

u8* getcontent(int title_h, int title_l, int contentid, u32* size)
{
	u64 titleid = (u64)title_h << 32 | (u64)title_l;
	u8* tm = contents[titleid][contentid];
	if (tm != NULL) {
		if (size)
			*size = contentsize[titleid][contentid];
		return tm;
	}
	
	char str[10];
	sprintf(str, "%08x", contentid);
	
	u32 msize;
	
	int retval = get_nus_object(title_h, title_l, str, &tm, size ? size : &msize);
	
	if (size)
		msize = *size;
	
	if (retval < 0) {
		debug_printf("get_nus_object(tmd) returned %d, size = %u\n", retval, msize);
		printf("Network failure!\n");
		exit(1);
	}
	if (tm == NULL) {
		debug_printf("Failed to allocate temp buffer for encrypted content, size was %u\n", msize);
		printf("Network failure!\n");
		exit(1);
	}
	
	contents[titleid][contentid] = tm;
	contentsize[titleid][contentid] = msize;
	
	return tm;
}

void replace_sd_module(tmd* p_tmd, signed_blob* s_tik)
{
	u32 sdsize;
	u8* sdcontent = getcontent(1, SD_IOS, SD_CONTENT, &sdsize);
	tmd* sd = (tmd*)SIGNATURE_PAYLOAD((signed_blob*)gettmd(1, SD_IOS, SD_VERSION));
	
	p_tmd->contents[4].size = sd->contents[4].size;
	//p_tmd->contents[4].cid = findunusedcid(p_tmd);
	
	get_title_key((signed_blob*)getticket(1, SD_IOS));
	u8* decrypted_buf = (u8*)malloc(sdsize);
	decrypt_buffer(4, sdcontent, decrypted_buf, sdsize);
	
	SHA1(decrypted_buf, p_tmd->contents[4].size, p_tmd->contents[4].hash);
	if (memcmp(sd->contents[4].hash, p_tmd->contents[4].hash, 0x10)) {
		printf("Network fail!\n");
		exit(0);
	}
	
	contents[p_tmd->title_id][p_tmd->contents[4].cid] = decrypted_buf;
	contentsize[p_tmd->title_id][p_tmd->contents[4].cid] = sdsize;
	
	get_title_key(s_tik); // Restore the original AES key for the title
	
	tmd_dirty = 1;
}

int patchmii(int input_h, int input_l, int output_h, int output_l, int patchtype, int comexploit, int input_version, int iosversion) {
	int retval;
	signed_blob *s_tmd = NULL, *s_tik = NULL, *s_certs = NULL;
	u8* tmdbuf;
	u8* tmdbuf2;
	u8* tikbuf;
  
	u32 tmdsize;
	int update_tmd;
	tmdbuf = gettmd(input_h, input_l, input_version);
	if (comexploit == 2)
		tmdbuf2 = gettmd(input_h, input_l, 0);
	s_tmd = (signed_blob *)tmdbuf;
	signed_blob* s_tmdtemp = (signed_blob *)tmdbuf2;
	if (!IS_VALID_SIGNATURE(s_tmd)) {
		printf("Fail!\n");
		exit(1);
  	}

	int ret = 0;
	if (comexploit == 2) {
		ret = ES_AddTitleStart(s_tmdtemp, SIGNED_TMD_SIZE(s_tmdtemp), (signed_blob*)certs_dat, certs_dat_size, NULL, 0);
		if (ret < 0) {
			printf("This isn't where the music's at, I guess I'm going back down there.\n");
			ES_AddTitleCancel();
			exit(1);
		}
		printf(".");
		__ES_Close();
		__ES_Init();
	} else if (comexploit == 1) {
		ISFS_Initialize();
		ret = ES_AddTitleStart(s_tmd, SIGNED_TMD_SIZE(s_tmd), (signed_blob*)certs_dat, certs_dat_size, NULL, 0);

		printf(".");

		if(ret < 0) 
		{
			if (ret == -1035)
			{
				printf("Failed to do shit.\n");
			} else
			{
			}
			ES_AddTitleCancel();
			return ret;
		}

		s32 file;
		char *tmd_path = "/tmp/title.tmd";
		ISFS_Delete(tmd_path);
		ISFS_CreateFile(tmd_path, 0, 3, 3, 3);
		file = ISFS_Open(tmd_path, ISFS_OPEN_WRITE);
		if(!file) {
			debug_printf("Error: ISFS_Open returned %d\n", file);
			ES_AddTitleCancel();
			ISFS_Deinitialize();
			printf("Failed some shit.\n");
			exit(1);
		}
		
		tmdbuf[0x1dc] = 0;
		tmdbuf[0x1dd] = 0;

		ret = ISFS_Write(file, (u8*)tmdbuf, SIGNED_TMD_SIZE(s_tmd));
		if(!ret) {
			debug_printf("Error: ISFS_Write returned %d\n", ret);
			ISFS_Close(file);
			ES_AddTitleCancel();
			ISFS_Deinitialize();
			printf("Fail!\n");
			exit(1);
		}

		printf(".");

		ISFS_Close(file);
		ES_AddTitleFinish();
		ISFS_Deinitialize();
		return true;
	}

	tikbuf = getticket(input_h, input_l);
	
	s_tik = (signed_blob *)tikbuf;
	if(!IS_VALID_SIGNATURE(s_tik)) {
		printf("Fail!\n");
		exit(1);
	}

	s_certs = (signed_blob *)certs_dat;
	if(!IS_VALID_SIGNATURE(s_certs)) {
		printf("Fail!\n");
		exit(1);
  	}

	get_title_key(s_tik);
	tmd *p_tmd;
	p_tmd = (tmd*)SIGNATURE_PAYLOAD(s_tmd);
	u16 i;
	sha1 hash;
	for (i = 0; i < p_tmd->num_contents; i++) {
		printf(".");
		u8 *content_buf;
		u32 content_size;
		content_buf = getcontent(input_h, input_l, p_tmd->contents[i].cid, &content_size);
		if (content_size % 16) {
			printf("Network fail!\n");
			exit(1);
		}
		if (content_size < p_tmd->contents[i].size) {
			printf("Network fail!\n");
			exit(1);
		} 
		u8* decrypted_buf = (u8*)malloc(content_size);
		decrypt_buffer(i, content_buf, decrypted_buf, content_size);
		memcpy(content_buf, decrypted_buf, content_size);
		free(decrypted_buf);
		SHA1(content_buf, p_tmd->contents[i].size, hash);
		
		if (!memcmp(p_tmd->contents[i].hash, hash, sizeof hash)) {
			update_tmd = 0;
			if (patchtype) {
				if (patch_hash_check(content_buf, content_size))
					update_tmd = 1;
				if (patch_new_dvdlowunencrypted(content_buf, content_size))
					update_tmd = 1;
				if (patch_fsperms(content_buf, content_size))
					update_tmd = 1;
				if (patch_identify_check(content_buf, content_size))
					update_tmd = 1;
				if (patch_diVid(content_buf, content_size))
					update_tmd = 1;
				
				if (patchtype == 2) { // 1337 haxx
#ifdef RAWKHAXX
					/* tueidj: /dev/fs hooks */
					if (apply_patch(content_buf, fs_patch1_pos, fs_orig1, sizeof(fs_orig1), fs_patch1, sizeof(fs_patch1)))
						update_tmd = 1;
					if (apply_patch(content_buf, fs_patch2_pos, fs_orig2, sizeof(fs_orig2), fs_patch2, sizeof(fs_patch2)))
						update_tmd = 1;
					if (apply_patch(content_buf, fs_patch3_pos, fs_orig3, sizeof(fs_orig3), fs_patch3, sizeof(fs_patch3)))
						update_tmd = 1;
#endif
					if (apply_patch(content_buf, gpio_patch_pos, gpio_orig, sizeof(gpio_orig), gpio_patch, sizeof(gpio_patch)))
						update_tmd = 1;
					if (patch_di_do(content_buf, content_size))
						update_tmd = 1;
				}
			}
			
			if (update_tmd == 1) {
				SHA1(content_buf, p_tmd->contents[i].size, p_tmd->contents[i].hash);
				//memcpy(p_tmd->contents[i].hash, hash, sizeof hash);
				tmd_dirty=1;
			}
		} else {
			printf("Network fail!\n");
			exit(1);
		}
	}
	
	if ((input_h != output_h) || (input_l != output_l)) {
		u64 oldid = p_tmd->title_id;
		change_ticket_title_id(s_tik, output_h, output_l);
		change_tmd_title_id(s_tmd, output_h, output_l);
		contents[p_tmd->title_id] = contents[oldid];
		contentsize[p_tmd->title_id] = contentsize[oldid];
	}
	
	if (patchtype == 2) {
		add_custom_modules((tmd*)p_tmd);
		replace_sd_module((tmd*)p_tmd, s_tik);
		p_tmd->version = HAXXED_NEW_VERSION;
		p_tmd->title_version = HAXXED_NEW_VERSION;
	}
	
	if (tmd_dirty) {
		forge_tmd(s_tmd);
		tmd_dirty = 0;
	}
	
	if (tik_dirty) {
		forge_tik(s_tik);
		tik_dirty = 0;
	}

	if (iosversion > 0) {
		WPAD_Shutdown();
		IOS_ReloadIOS(iosversion);
		WPAD_Init();
	}

	retval = install_ticket(s_tik, s_certs, certs_dat_size);
	if (retval) {
		printf("Installer fail!\n");
		exit(1);
	}

	retval = install(s_tmd, s_certs, certs_dat_size);
	
	if (retval) {
		printf("Installer fail!\n");
		exit(1);
	}

	return 0;
}

void predownloadcontents(const tmd* t)
{
	for (int i = 0; i < t->num_contents; i++) {
		printf(".");
		getcontent(t->title_id >> 32, (s32)t->title_id, t->contents[i].cid, NULL);
	}
}

void installer_init(bool skipdowngrade)
{
#ifdef RAWKHAXX
	getticket(1, PATCHED_IOS);
#endif
	getticket(1, HAXXED_IOS);
	getticket(1, SD_IOS); gettmd(1, SD_IOS, SD_VERSION); getcontent(1, SD_IOS, SD_CONTENT, NULL);
	
	if (!skipdowngrade) {
		getticket(1, DOWNGRADED_IOS);
		predownloadcontents((const tmd*)SIGNATURE_PAYLOAD((signed_blob*)gettmd(1, DOWNGRADED_IOS, DOWNGRADED_VERSION)));
		predownloadcontents((const tmd*)SIGNATURE_PAYLOAD((signed_blob*)gettmd(1, DOWNGRADED_IOS, 0)));
	}
#ifdef RAWKHAXX
	predownloadcontents((const tmd*)SIGNATURE_PAYLOAD((signed_blob*)gettmd(1, PATCHED_IOS, 0)));
#endif
	predownloadcontents((const tmd*)SIGNATURE_PAYLOAD((signed_blob*)gettmd(1, HAXXED_IOS, HAXXED_VERSION)));
}

void installer_downgrade(bool skipdowngrade)
{
	if (skipdowngrade)
		return;
	//patchmii(1, DOWNGRADED_IOS, 1, DOWNGRADED_IOS, 0, 1, 0, 0);
	//patchmii(1, DOWNGRADED_IOS, 1, DOWNGRADED_IOS, 0, 0, DOWNGRADED_VERSION, 0);
	patchmii(1, DOWNGRADED_IOS, 1, DOWNGRADED_IOS, 0, 2, DOWNGRADED_VERSION, 0);
}

void installer_go(bool skipdowngrade)
{
#ifdef RAWKHAXX
	patchmii(1, PATCHED_IOS, 1, PATCHED_IOS, 1, 0, 0, skipdowngrade ? 0 : DOWNGRADED_IOS);
	patchmii(1, HAXXED_IOS, 1, HAXXED_NEW_IOS, 2, 0, HAXXED_VERSION, 0);
#else
	patchmii(1, HAXXED_IOS, 1, HAXXED_NEW_IOS, 2, 0, HAXXED_VERSION, skipdowngrade ? 0 : DOWNGRADED_IOS);
#endif
	
	patchmii(1, DOWNGRADED_IOS, 1, DOWNGRADED_IOS, 0, 0, 0, 0);
}

void installer_cleanup()
{
	vector<u8*> deleted;
	for (map<u64, map<u32, u8*> >::iterator iter = contents.begin(); iter != contents.end(); iter++) {
		for (map<u32, u8*>::iterator iter2 = iter->second.begin(); iter2 != iter->second.end(); iter2++) {
			bool found = false;
			for (vector<u8*>::iterator ptr = deleted.begin(); ptr != deleted.end(); ptr++) {
				if (*ptr == iter2->second)
					found = true;
			}
			if (!found) {
				free(iter2->second);
				deleted.push_back(iter2->second);
			}
		}
	}
}

#if 0
int main(int argc, char** argv)
{
	bool ohshiterror = false;
	int initios = IOS_GetPreferredVersion();
	if (initios < 0)
		initios = 36;
	if (IOS_ReloadIOS(initios) < 0)
		ohshiterror = true;
	console_setup();
	WPAD_Init();
	
	if (ohshiterror) {
		printf("There is something seriously wrong with IOS, unable to patch at this time.\nYou should try manually installing a fresh IOS36 and run the patcher again.\n");
		exit(0);
	}
	
	printf("Please be patient as the required files are located.\nThis may take a few minutes depending on your internet connection...");
	
	installer_init();
	
	printf("\nEverything has been found, now patching...");
	
	installer_go();
	
	printf("\nThe patching process has completed successfully!\n");
	itdied(0);
	
	return 0;
}
#endif
