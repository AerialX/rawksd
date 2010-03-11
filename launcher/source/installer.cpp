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

//#define NANOHTTP
#define PATCHMII_HTTP

#define ERROR(desc, value) \
	printf("Error %s. (%d)\n", desc, value)

#define NUS_URL_BASE "http://nus.cdn.shop.wii.com/ccs/download"

extern "C" {
	extern u8 certs_dat[];
	extern u32 certs_dat_size;
}

static bool initfat = false;
static bool initnet = false;
static const signed_blob* _certs;
static u32 _certlen;
static void SetCerts(const signed_blob* certs, u32 certsLength)
{
	_certs = certs;
	_certlen = certsLength;
}

static bool netinit()
{
	printf("Initializing network... ");

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
	SetCerts((const signed_blob*)certs_dat, certs_dat_size);
	printf("Mounting SD/USB... ");
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

static signed_blob* GetTMD(u64 titleid, int version)
{
	char filename[0x10];
	u32 length = MAX_SIGNED_TMD_SIZE;
	if (version)
		sprintf(filename, "tmd.%d", version);
	else
		strcpy(filename, "tmd");
	return (signed_blob*)GetContent(titleid, filename, &length);
}
static signed_blob* GetTMD(u64 titleid)
{
	return GetTMD(titleid, 0);
}

signed_blob* GetTicket(u64 titleid)
{
	u32 length = STD_SIGNED_TIK_SIZE;
	return (signed_blob*)GetContent(titleid, "cetk", &length);
}

static int InstallTicket(const signed_blob* ticket)
{
	return ES_AddTicket(ticket, STD_SIGNED_TIK_SIZE, _certs, _certlen, NULL, 0);
}

static int CancelInstall()
{
	return ES_AddTitleCancel();
}

static int StartInstall(const signed_blob* meta)
{
	int ret = ES_AddTitleStart(meta, SIGNED_TMD_SIZE(meta), _certs, _certlen, NULL, 0);
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
static int InstallContent(const tmd* meta, u16 index)
{
	static u8 buffer[0x1000] ATTRIBUTE_ALIGN(32);
	u32 size = ROUND_UP(meta->contents[index].size, 32);
	u8* data = (u8*)GetContent(meta->title_id, meta->contents[index].cid, &size);
	if (!data) {
		ERROR("downloading content", 0);
		return -1;
	}
	int fd = ES_AddContentStart(meta->title_id, meta->contents[index].cid);
	if (fd < 0)
		return fd;

	u32 written = 0;
	while (written < size) {
		printf(".");
		int towrite = MIN(0x1000, size - written);
		memcpy(buffer, data + written, towrite);
		towrite = ROUND_UP(towrite, 32);
		int ret = ES_AddContentData(fd, buffer, towrite);
		if (ret < 0) {
			free(data);
			ES_AddContentFinish(fd);
			return ret;
		}

		written += towrite;
	}

	free(data);

	return ES_AddContentFinish(fd);
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

static void FakesignTMD(signed_blob* blob)
{
	memset((u8*)blob + 4, 0, SIGNATURE_SIZE(blob) - 4);
	tmd* meta = (tmd*)SIGNATURE_PAYLOAD(blob);
	for(u32 fill = 0; fill < 0xFFFF; fill++) {
		meta->fill3 = fill;
		sha1 hash; SHA1((u8*)meta, TMD_SIZE(meta), hash);

		if (hash[0] == 0)
			return;
	}

	// Unable to fakesign it, fuck cIOScrap
}

int Install(u64 titleid, int version, bool comexploit)
{
	int ret;
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

	printf("Installing...");

	ret = InstallTicket(ticket);
	if (ret < 0) {
		ERROR("installing ticket", ret);
		return ret;
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
