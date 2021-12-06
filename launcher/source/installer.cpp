#include "installer.h"

#include <libxml/nanohttp.h>
#include <stdio.h>
#include <string.h>
#include <network.h>
#include <fat.h>
#include <errno.h>
#include <unistd.h>
#include <sha1.h>
#include <http.h>
#include <malloc.h>

#include "haxx.h"
#include <files.h>

extern "C" {
	void aes_set_key(const u8 *key);
	void aes_decrypt(u8 *iv, const u8 *inbuf, u8 *outbuf, unsigned long long len);
	void aes_encrypt(u8 *iv, const u8 *inbuf, u8 *outbuf, unsigned long long len);
}

//#define NANOHTTP
#define PATCHMII_HTTP

#define ERROR(desc, value) \
	printf("\n\tError %s. (%d)\n", desc, value)

#define NUS_URL_BASE "http://nus.cdn.shop.wii.com/ccs/download"

#include "banner_tmd_dat.h"
#include "banner_tik_dat.h"
#include "banner_0_dat.h"
#include "banner_vwii_dat.h"

static bool initfat = false;
static bool initnet = false;

static bool netinit()
{
	printf("\tInitializing network... ");

	int ret;
	while ((ret = net_init()) == -EAGAIN)
		usleep(10000);
	if (ret < 0) {
		printf("Failed. (%d)\n", ret);
		return false;
	}

#ifdef NANOHTTP
	xmlNanoHTTPInit();
#endif

	printf("Success!\n");
	return true;
}

void Installer_Initialize()
{
	printf("\tMounting SD/USB... ");
	initfat = fatInitDefault();
	if (!initfat) {
		printf("Failed.\n");

		initnet = netinit();
	} else
		printf("Success!\n");
}

void Installer_Deinitialize()
{
	if (initnet)  {
#ifdef NANOHTTP
		xmlNanoHTTPCleanup();
#endif
		net_deinit();
	}
	if (initfat) { // Fucking libfat
		fatUnmount("sd:");
		fatUnmount("usb:");
	}
}

static u32 GetContent(u64 titleid, const char* filename, void* buffer, u32 length)
{
	if (initfat) {
		char path[0x100];
		sprintf(path, "/%08x/%08x/%s", (u32)(titleid >> 32), (u32)titleid, filename);
		FILE* fd = fopen(path, "rb");
		if (fd) {
			length = fread(buffer, 1, length, fd);

			fclose(fd);

			return length;
		}
	}

	if (!initnet)
		initnet = netinit();

	if (initnet) {
		char url[0x100];
		sprintf(url, "%s/%08x%08x/%s", NUS_URL_BASE, (u32)(titleid >> 32), (u32)titleid, filename);
#ifdef NANOHTTP
		void* http = xmlNanoHTTPOpen(url, NULL);
		if (!http)
			return 0;
		length = xmlNanoHTTPRead(http, buffer, length);
		xmlNanoHTTPClose(http);
#endif

#ifdef PATCHMII_HTTP
		if (!http_request(url, 0x0FFFFFFF)) {
			ERROR("downloading contents", -1);
			return 0;
		}
		u8* content;
		u32 size;
		u32 status;
		http_get_result(&status, &content, &size);
		length = MIN(size, length);
		memcpy(buffer, content, length);
		free(content);
#endif
		return length;
	}

	ERROR("retrieving contents", -1);
	return 0;
}

static void* GetContent(u64 titleid, const char* filename, u32* length)
{
	void* data = memalign(32, *length);
	if (!data)
		return NULL;
	*length = GetContent(titleid, filename, data, *length);
	if (!*length) {
		free(data);
		return NULL;
	}

	return data;
}

static void* GetContent(u64 titleid, int contentid, u32* length)
{
	char filename[0x10];
	sprintf(filename, "%08x", contentid);
	return GetContent(titleid, filename, length);
}

static signed_blob* GetTMD(u64 titleid, int version=0)
{
	char filename[0x10];
	u32 length = MAX_SIGNED_TMD_SIZE;
	if (version)
		sprintf(filename, "tmd.%d", version);
	else
		strcpy(filename, "tmd");
	return (signed_blob*)GetContent(titleid, filename, &length);
}

signed_blob* GetTicket(u64 titleid)
{
	u32 length = STD_SIGNED_TIK_SIZE;
	return (signed_blob*)GetContent(titleid, "cetk", &length);
}

static int InstallTicket(const signed_blob* ticket)
{
	return ES_AddTicket(ticket, STD_SIGNED_TIK_SIZE, (signed_blob*)sys_certs, SYS_CERTS_SIZE, NULL, 0);
}

static int CancelInstall()
{
	return ES_AddTitleCancel();
}

static int StartInstall(const signed_blob* meta)
{
	int ret = ES_AddTitleStart(meta, SIGNED_TMD_SIZE(meta), (signed_blob*)sys_certs, SYS_CERTS_SIZE, NULL, 0);
	if (ret < 0)
		CancelInstall();
	return ret;
}

static int EndInstall()
{
	return ES_AddTitleFinish();
}

#define ROUND_UP(p, round) \
	((p + round - 1) & ~(round - 1))
static int InstallContent(const tmd* meta, u16 index, const u8* data, u32 size, u8* key = NULL)
{
	static u8 buffer[0x1000] ATTRIBUTE_ALIGN(32);
	u8 iv[0x10];
	int fd = ES_AddContentStart(meta->title_id, meta->contents[index].cid);
	if (fd < 0)
		return fd;

	if (key) {
		aes_set_key(key);
		memset(iv, 0, sizeof(iv));
		memcpy(iv, &index, 2);
	}

	u32 written = 0;
	while (written < size) {
		if (!key) // Hack, but whatever. This is only installed with encryption when the GUI is running
			printf(".");
		int towrite = MIN(0x1000, size - written);
		if (key)
			aes_encrypt(iv, (u8*)data + written, buffer, towrite);
		else
			memcpy(buffer, data + written, towrite);
		written += towrite;
		towrite = ROUND_UP(towrite, 0x20);
		int ret = ES_AddContentData(fd, buffer, towrite);
		if (ret < 0) {
			ES_AddContentFinish(fd);
			return ret;
		}
	}

	return ES_AddContentFinish(fd);
}

static int InstallContent(const tmd* meta, u16 index)
{
	u32 size = ROUND_UP(meta->contents[index].size, 32);
	u8* data = (u8*)GetContent(meta->title_id, meta->contents[index].cid, &size);
	if (!data) {
		ERROR("downloading content", 0);
		return -1;
	}

	int ret = InstallContent(meta, index, data, size);

	free(data);
	return ret;
}

static int InstallContents(const signed_blob* stmd)
{
	const tmd* meta = (const tmd*)SIGNATURE_PAYLOAD(stmd);
	for (u16 i = 0; i < meta->num_contents; i++) {
		int ret = InstallContent(meta, i);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static bool FakesignTMD(signed_blob* blob)
{
	memset((u8*)blob + 4, 0, SIGNATURE_SIZE(blob) - 4);
	tmd* meta = (tmd*)SIGNATURE_PAYLOAD(blob);
	u32 size = TMD_SIZE(meta);
	for(u32 fill = 0; fill < 0xFFFF; fill++) {
		meta->fill2 = fill;
		sha1 hash; SHA1((u8*)meta, size, hash);

		if (hash[0] == 0)
			return true;
	}

	return false; // Unable to fakesign it, fuck cIOScrap
}

int Install(u64 titleid, int version, bool comexploit)
{
	int ret;
	if (!get_certs()) {
		ERROR("fetching certs", 0);
		return -1;
	}
	signed_blob* meta = GetTMD(titleid, version);
	signed_blob* newesttmd = GetTMD(titleid);
	signed_blob* ticket = GetTicket(titleid);

	if (!meta || !newesttmd) {
		ERROR("retrieving TMD", 0);
		return -1;
	}

	if (!ticket) {
		ERROR("retrieving ticket", 0);
		return -1;
	}

	printf("\tInstalling...");

	ret = InstallTicket(ticket);
	if (ret < 0) {
		ERROR("installing ticket", ret);
		return ret;
	}

	if (comexploit && version) {
		ret = StartInstall(newesttmd);
		if (ret == -1035) {
			// Either an unreleased IOS version is installed, or the version was fucked with (cIOScrap)
			// Let's try fucking with the TMD version ourselves and fakesigning.
			// Normally catering to cIOScrap users isn't something I'd do, but if it means we're removing part of it, go for it.
			((tmd*)SIGNATURE_PAYLOAD(newesttmd))->title_version = 0xFFFF;
			FakesignTMD(newesttmd);
			ret = StartInstall(newesttmd);
		}
		if (ret < 0) {
			ERROR("installing", ret);
			return ret;
		}
		__ES_Close();
		__ES_Init();
	}

	ret = StartInstall(meta);
	if (ret < 0) {
		ERROR("installing", ret);
		return ret;
	}

	ret = InstallContents(meta);
	if (ret < 0) {
		ERROR("installing contents", ret);
		CancelInstall();
		return ret;
	}

	ret = EndInstall();
	if (ret < 0) {
		ERROR("installing", ret);
		return ret;
	}

	printf("\n");

	return 0;
}

static u8* ReadChannelDol(const char* filename, u32* size)
{
	Stats st;
	if (File_Stat(filename, &st))
		return NULL;
	if (st.Mode & S_IFDIR || st.Size < 0x14)
		return NULL;

	int fd = File_Open(filename, O_RDONLY);
	if (fd < 0)
		return NULL;

	*size = (u32)st.Size;
	u8* ret = (u8*)memalign(0x20, ROUND_UP(*size, 0x20));
	if (!ret) {
		File_Close(fd);
		return NULL;
	}

	if (File_Read(fd, ret, *size) != (int)*size) {
		File_Close(fd);
		free(ret);
		return NULL;
	}
	File_Close(fd);

#ifndef NO_VERIFY_INSTALL
	u8 filehash[0x14];
	u8 hash[0x14];
	u8* hasharea = ret + *size - sizeof(hash);
	memcpy(filehash, hasharea, sizeof(hash));
	memcpy(hasharea, "Viva la Riivolution!", sizeof(hash));

	SHA1(ret, *size, hash);
	if (memcmp(hash, filehash, sizeof(hash))) { // We didn't get the expected hash
		free(ret);
		return NULL;
	}

	*size -= sizeof(hash);
#endif

	// patches to keep IOS happy when it boots the content
	if (!is_wiiu) {
		ret[0x07] = 0x61;
		ret[0x13] = 0x28;

		// translate all virtual address to physical
		for (fd=0; fd < ret[0x2D]; fd++) {
			u32 *addr = (u32*)(ret+0x3C+fd*0x20);
			*addr = *addr & 0x3FFFFFFF;
		}
	}

	return ret;
}

static u8* CopyChannelData(const u8* data, u32 isize, u32* size)
{
	*size = isize;
	u8* ret = (u8*)memalign(0x20, ROUND_UP(isize, 0x20));
	if (ret)
		memcpy(ret, data, *size);
	return ret;
}

static u8* ReadChannelData(int index, u32* size)
{
	u8 *ret;
	if (index < 1)
		return CopyChannelData(banner_0_dat, banner_0_dat_size, size);

	if (index < 2 && is_wiiu)
		return CopyChannelData(banner_vwii_dat, banner_vwii_dat_size, size);

	ret = ReadChannelDol("/apps/riivolution/boot.elf", size);
	if (!ret)
		ret = ReadChannelDol("/boot.elf", size);
	return ret;
}

const u8 commonkey[0x10] = { 0xeb, 0xe4, 0x2a, 0x22, 0x5e, 0x85, 0x93, 0xe4, 0x48, 0xd9, 0xc5, 0x45, 0x73, 0x81, 0xaa, 0xf7 };

static void GetTitleKey(const signed_blob *s_tik, u8 *key)
{
	u8 iv[0x10];
	u8 keyin[0x10];

	const tik* p_tik = (const tik*)SIGNATURE_PAYLOAD(s_tik);
	memcpy(keyin, p_tik->cipher_title_key, sizeof(keyin));
	memset(key, 0, sizeof(keyin));
	memset(iv, 0, sizeof(iv));
	memcpy(iv, &p_tik->titleid, sizeof(p_tik->titleid));

	aes_set_key(commonkey);
	aes_decrypt(iv, keyin, key, sizeof(keyin));
}

int InstallChannel()
{
	int i, ret;
	static u8 metadata[MAX_SIGNED_TMD_SIZE] ATTRIBUTE_ALIGN(32);
	static u8 tikdata[STD_SIGNED_TIK_SIZE] ATTRIBUTE_ALIGN(32);
	u8 key[0x10];

	signed_blob* meta = (signed_blob*)&metadata;
	signed_blob* ticket = (signed_blob*)&tikdata;
	memcpy(meta, banner_tmd_dat, banner_tmd_dat_size);
	memcpy(ticket, banner_tik_dat, banner_tik_dat_size);
	tmd* metatmd = (tmd*)SIGNATURE_PAYLOAD(meta);
	tik* metatik = (tik*)SIGNATURE_PAYLOAD(ticket);

	if (!get_certs()) {
		ERROR("fetching certs", 0);
		return -1;
	}

	int banner_count = is_wiiu ? 3 : 2;
	u32 banner_dat_size[banner_count];
	u8 *banner_dat[banner_count];

	ret = 0;
	metatmd->num_contents = banner_count;
	for (i=0; i < banner_count; i++)
		if ((banner_dat[i] = ReadChannelData(i, banner_dat_size+i))) {
			metatmd->contents[i].type = 1;
			metatmd->contents[i].index = i;
			metatmd->contents[i].cid = i;
			metatmd->contents[i].size = banner_dat_size[i];
			SHA1(banner_dat[i], banner_dat_size[i], metatmd->contents[i].hash);
		} else
			ret = -1;

	if (ret < 0) {
		for (i=0; i < banner_count; i++)
			free(banner_dat[i]);
		return -2;
	}


	if (metatik && metatik->devicetype) {
		// assume the ticket also contains a valid ECC public keypair
		u32 devicetype;
		if (ES_GetDeviceID(&devicetype) >= 0) {
			metatik->devicetype = devicetype;
		}
	}

	if (!forge_sig((u8*)ticket, banner_tik_dat_size) || !forge_sig((u8*)meta, 0x1e4 + 0x24 * banner_count))
		return -3;

	GetTitleKey(ticket, key);

	ret = InstallTicket(ticket); // intentionally fails if the console id is set

	// OR is used here because ret will be 0 if sig checking is disabled :(
	if (metatik && (ret==-2011 || metatik->devicetype)) {
		// the ECDH shared secret is left in RAM, so we can grab it and crypt the title key properly
		// this saves doing all the ECC shit manually and doesn't need any knowledge of the wii's keys,
		// thanks ninty! :D
		u8 ECDH_hash[0x20];
		u8 raw_key[16];
		u8 iv[16];
		// this address is hardcoded for IOS37 but it might be findable by searching:
		// it's on a 64-byte boundary and is followed "\0GCC: (GNU) 3.4.3\0\0\0"
		// ECDH certs/keys always starts with \0 or \1 and will be 30 bytes long, probably padded
		// by zeroes up to 64 bytes
		u8* const ECDH_Secret = (u8*)0x93A75180;
		DCInvalidateRange(ECDH_Secret, 32);

		SHA1(ECDH_Secret, 30, ECDH_hash);
		// got the hash, now encrypt the title key in the ticket
		memcpy(raw_key, metatik->cipher_title_key, 16);
		memset(iv+8, 0, sizeof(iv)-8);
		memcpy(iv, &metatik->ticketid, 8);
		aes_set_key(ECDH_hash);
		aes_encrypt(iv, raw_key, metatik->cipher_title_key, 16);
		// Install it again, this time it should work
		ret = InstallTicket(ticket);
	}

	if (ret>=0)
		ret = StartInstall(meta);

	if (ret < 0) {
		for (i = 0; i < banner_count; i++)
			free(banner_dat[i]);
		return ret;
	}

	for (i = 0; i < banner_count && ret >= 0; i++)
		ret = InstallContent(metatmd, i, banner_dat[i], banner_dat_size[i], key);

	for (i = 0; i < banner_count; i++)
		free(banner_dat[i]);

	if (ret < 0)
		CancelInstall();
	else
		ret = EndInstall();

	if (ret < 0)
		return ret;

	return 0;
}
