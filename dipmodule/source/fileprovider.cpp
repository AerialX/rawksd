#include "fileprovider.h"
#include "dip.h"
#include "rijndael.h"

#include <files.h>

#include "es.h"

namespace ProxiIOS { namespace DIP {
	bool FileProvider::ReadSectors(void* userdata, sec_t sector, sec_t numSectors, void* buffer)
	{
		u8 iv[0x10];

		FileProvider* provider = (FileProvider*)userdata;

		u64 offset = provider->Module->CurrentPartition + ((u64)provider->Partition.DataOffset << 2) + sector * 0x8000;

		aes_set_key(provider->PartitionKey);
		for (sec_t i = 0; i < numSectors; i++, offset += 0x8000, buffer = (u8*)buffer + 0x8000) {
			if (provider->UnencryptedRead(iv, 0x10, offset + 0x3D0) < 0)
				return false;
			if (provider->UnencryptedRead((u8*)buffer, 0x7C00, offset + 0x400) < 0)
				return false;
			aes_decrypt(iv, (u8*)buffer, (u8*)buffer, 0x7C00);
		}

		return true;
	}

	FileProvider::FileProvider(DIP* module, int file) : DiProvider(module), Kash(2, 1, 0x7C00, ReadSectors, this)
	{
		File = file;
		Position = 0;
		File_Seek(File, SEEK_SET, 0);
	}

	int FileProvider::UnencryptedRead(void* buffer, u32 size, u64 offset)
	{
		if (offset > Partition.GetDataSize() * 0x7C / 0x80)
			return -1;

		s64 difference = (s64)offset - Position;
		bool negative = difference < 0;
		if (negative)
			difference = -difference;
		while (difference & ~0x7FFFFFFF) {
			File_Seek(File, SEEK_CUR, negative ? 0x80000000 : 0x7FFFFFFF);
			difference -= 0x7FFFFFFF;
		}
		if (difference)
			File_Seek(File, SEEK_CUR, negative ? -(s32)difference : (s32)difference);

		int read = File_Read(File, buffer, size);

		Position = offset + read;

		return 1;
	}

	int FileProvider::Read(void* buffer, u32 size, u32 pos)
	{
		u64 offset = (u64)pos >> 2;
		if (Kash.Read(buffer, offset, size)) {
			return 1;
			os_sync_after_write(buffer, size);
		} else
			return UnencryptedRead(buffer, size, Module->CurrentPartition + ((u64)Partition.DataOffset << 2) + offset);
	}

	int FileProvider::ClosePartition()
	{
		memset(&Partition, 0, sizeof(Partition));
		return 1;
	}

	int FileProvider::UnencryptedRead(void* buffer, u32 size, u32 offset)
	{
		return UnencryptedRead(buffer, size, (u64)offset << 2);
	}

	int FileProvider::ReadBCA(void* buffer, u32 length)
	{
		// Let's copy crediar's SMNE for now
		memset(buffer, 0, length);
		*(u32*)((u8*)buffer + 0x30) = 0x00000001;
		os_sync_after_write(buffer, length);

		return 1;
	}

	int FileProvider::OpenPartition(u32 offset, void* ticket, void* certificate, u32 certificateLength, void* tmd, void* errors)
	{
		// TODO: Check return values
		if (ticket)
			UnencryptedRead(ticket, STD_SIGNED_TIK_SIZE, Module->CurrentPartition);

		UnencryptedRead(&Partition, sizeof(Partition), Module->CurrentPartition + STD_SIGNED_TIK_SIZE);

		if (tmd)
			UnencryptedRead(tmd, Partition.TmdSize, Module->CurrentPartition + ((u64)Partition.TmdOffset << 2));

		if (certificate && certificateLength)
			UnencryptedRead(certificate, MIN(Partition.CertificateChainSize, certificateLength), Module->CurrentPartition + ((u64)Partition.CertificateChainOffset << 2));

		Kash.Clear();

		u8 commonkey[0x10];
		u8 iv[0x10];
		tik* _ticket = (tik*)SIGNATURE_PAYLOAD((signed_blob*)ticket);
		os_get_key(ES_KEY_COMMON, commonkey);
		aes_set_key(commonkey);
		memcpy(iv, &_ticket->titleid, 0x08);
		aes_decrypt(iv, _ticket->cipher_title_key, PartitionKey, 0x10);

		return 0;
	}
} }
