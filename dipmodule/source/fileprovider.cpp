#include "fileprovider.h"
#include "dip.h"
#include "rijndael.h"

#include <files.h>

#include "es.h"

//#define LOGGING

#ifdef YARR


#ifdef LOGGING
extern "C" void LogPrintf(const char *fmt, ...);
static void Hexdump(const void* buffer, int len)
{
	os_sync_before_read(buffer, len);
	if (len > 0x40)
		return;
	return;
	LogPrintf("%d\n", len);
	const u8* data = (const u8*)buffer;
	while (len > 0) {
		for (int i = 0; i < 0x10 && i < len; i++) {
			if (i == 0x08)
				LogPrintf(" ");
			LogPrintf("%02x ", data[i]);
		}
		LogPrintf(" |");
		for (int i = 0; i < 0x10 && i < len; i++) {
			LogPrintf("%c", (data[i] < 0x20 || data[i] >= 0x80) ? '.' : data[i]);
		}
		LogPrintf("|\n");
		data += 0x10;
		len -= 0x10;
	}
}

#else
#define Hexdump(...)
#define LogPrintf(...)
#endif

namespace ProxiIOS { namespace DIP {
	bool FileProvider::ReadSectors(void* userdata, sec_t sector, sec_t numSectors, void* buffer)
	{
		STACK_ALIGN(u8,iv,16,32);
		LogPrintf("ReadSectors: 0x%08x (0x%08x)\n", (u32)sector, (u32)numSectors);

		FileProvider* provider = (FileProvider*)userdata;

		u64 offset = provider->Module->CurrentPartition + ((u64)provider->Partition.DataOffset << 2) + (u64)sector * 0x8000;
		LogPrintf("\tOffset: 0x%08x%08x\n", (u32)(offset >> 32), (u32)offset);
		for (sec_t i = 0; i < numSectors; i++, offset += 0x8000, buffer = (u8*)buffer + 0x7C00) {
			if (provider->UnencryptedRead(iv, 0x10, offset + 0x3D0) != 1)
				return false;
			if (provider->UnencryptedRead((u8*)buffer, 0x7C00, offset + 0x400) != 1)
				return false;
			aes_decrypt(iv, (u8*)buffer, (u8*)buffer, 0x7C00);
		}

		return true;
	}

	FileProvider::FileProvider(DIP* module, const char* path) : DiProvider(module), Kash(3, 1, 0x7C00, ReadSectors, this)
	{
		int i = strlen(path);
		char *ex_path = (char*)Alloc(i+3);
		File[0] = File_Open(path, O_RDONLY);
		strcpy(ex_path, path);
		strcat(ex_path, "1");
		File[1] = File_Open(ex_path, O_RDONLY);
		ex_path[i] = '2';
		File[2] = File_Open(ex_path, O_RDONLY);
		Dealloc(ex_path);
	}

	int FileProvider::UnencryptedRead(void* buffer, u32 size, u64 offset)
	{
		int file_index = offset >> 31;
		LogPrintf("Unencrypted Read: 0x%08x%08x (0x%08x) - 0x%08x\n", (u32)(offset >> 32), (u32)offset, size, buffer);

		// FIXME: Needs to check for dual layer
		if (offset >= 0x118280000llu) {
			LogPrintf("Trying to read outside disc, returning 2\n");
			local_error = 0x00052100;
			return 2;
		}

		// FIXME: Needs to check offset against the proper table of allowed addresses
		if (offset == 0xbb800000llu) {
			LogPrintf("Outside bounds, returning 32\n");
			return 32;
		}

		File_Seek(File[file_index], (int)offset&0x7FFFFFFF, SEEK_SET);

		static u32 tempdata[0x20] ATTRIBUTE_ALIGN(32);
		if ((int)buffer == 0 && size <= 0x20 && size >= 4)
			buffer = tempdata;

		int read = File_Read(File[file_index], buffer, size);

		if (buffer == tempdata) { // Fucking memcpy
			*(u32*)0 = *tempdata;
			memcpy((void*)4, tempdata + 1, size - 4);
			os_sync_after_write(0, size);
		}

		Hexdump(buffer, read);

		local_error = 0;

		return 1;
	}

	int FileProvider::Read(void* buffer, u32 size, u32 pos)
	{
		u64 offset = (u64)pos << 2;
		LogPrintf("Read: 0x%08x%08x (0x%08x) - 0x%08x\n", (u32)(offset >> 32), (u32)offset, size, buffer);
		if (Kash.Read(buffer, offset, size)) {
			os_sync_after_write(buffer, size);
			Hexdump(buffer, size);
			return 1;
		} else {
			LogPrintf("Epic Fail\n");
			return UnencryptedRead(buffer, size, Module->CurrentPartition + ((u64)Partition.DataOffset << 2) + offset);
		}
	}

	int FileProvider::UnencryptedRead(void* buffer, u32 size, u32 offset)
	{
		return UnencryptedRead(buffer, size, (u64)offset << 2);
	}

	int FileProvider::ReadBCA(void* buffer, u32 length)
	{
		// FIXME: Use memcpy to safeguard against alignment problems
		LogPrintf("ReadBCA 0x%08X - 0x%08X\n", length, buffer);
		// Let's copy crediar's SMNE for now
		*(u32*)((u8*)buffer + 0x30) = 0x00000001;
		*(u32*)((u8*)buffer + 0x34) = 0xFFFFFFFF;
		os_sync_after_write(buffer, length);

		return 1;
	}

	int FileProvider::OpenPartition(u32 offset, void* ticket, void* certificate, u32 certificateLength, void* tmd, void* errors)
	{
		LogPrintf("Open Partition: 0x%08x\n", offset);
		u64 partition = (u64)offset << 2;

		// TODO: Check return values
		static u8 tikbuf[0x2A4] ATTRIBUTE_ALIGN(32);
		if (!ticket) {
			ticket = tikbuf;
		}
		UnencryptedRead(ticket, 0x2A4, partition);

		UnencryptedRead(&Partition, sizeof(Partition), partition + 0x2A4);

		if (tmd)
			UnencryptedRead(tmd, Partition.TmdSize, partition + ((u64)Partition.TmdOffset << 2));

		if (certificate && certificateLength)
			UnencryptedRead(certificate, MIN(Partition.CertificateChainSize, certificateLength), partition + ((u64)Partition.CertificateChainOffset << 2));

		Kash.Clear();

		u8 commonkey[0x10] = {0xeb, 0xe4, 0x2a, 0x22, 0x5e, 0x85, 0x93, 0xe4, 0x48, 0xd9, 0xc5, 0x45, 0x73, 0x81, 0xaa, 0xf7};
		aes_set_key(commonkey);
		u8 iv[0x10];
		memset(iv, 0, 0x10);
		memcpy(iv, (u8*)ticket + 0x1DC, 0x08);

		LogPrintf("Key\n");
		Hexdump((u8*)ticket + 0x1BF, 0x10);
		LogPrintf("IV\n");
		Hexdump(iv, 0x10);

		aes_decrypt(iv, (u8*)ticket + 0x1BF, PartitionKey, 0x10);
		aes_set_key(PartitionKey);

		LogPrintf("Partition Key\n");
		Hexdump(PartitionKey, 0x10);

		return 0;
	}

	int FileProvider::Reset(int param)
	{
		ClosePartition();
		return 1;
	}

	int FileProvider::VerifyCover(void* output)
	{
		*(u32*)output = 0; // Disc inserted
		os_sync_after_write(output, 4);
		return 1;
	}

	int FileProvider::ClosePartition()
	{
		LogPrintf("ClosePartition\n");
		memset(&Partition, 0, sizeof(Partition));
		Kash.Clear();
		return 1;
	}

	int FileProvider::RequestError(void *_error)
	{
		LogPrintf("RequestError 0x%08x, - 0x%08x\n", _error, local_error);
		u32 *error = (u32*)_error;
		*error = local_error;
		return 1;
	}

	int FileProvider::ReportKey()
	{
		LogPrintf("ReportKey\n");
		local_error =0x00052000;
		return 2;
	}
} }

#endif
