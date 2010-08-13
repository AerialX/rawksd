#pragma once

#include "diprovider.h"
#include "cache.h"

#ifdef YARR

namespace ProxiIOS { namespace DIP {
	class FileProvider : public DiProvider
	{
	protected:
		struct PartitionHeader {
			u32 TmdSize;
			u32 TmdOffset;
			u32 CertificateChainSize;
			u32 CertificateChainOffset;
			u32 H3Offset;
			u32 DataOffset;
			u32 DataSize;

			u64 GetDataSize() { return (u64)DataSize << 2; }
		} __attribute__((packed));

		int File[3];
		u64 Position;
		PartitionHeader Partition;
		u8 PartitionKey[0x10];
		Cache Kash; // Don't ask
		u32 local_error;
	public:
		static bool ReadSectors(void* userdata, sec_t sector, sec_t numSectors, void* buffer);

		FileProvider(DIP* module, const char* path);

		int UnencryptedRead(void* buffer, u32 size, u64 offset);

		virtual int Reset(int param);
		virtual int VerifyCover(void* output);
		virtual int ReadDiscID(void* discid) { return UnencryptedRead(discid, 0x20, (u64)0); }
		virtual int Read(void* buffer, u32 size, u32 offset);
		virtual int ClosePartition();
		virtual int UnencryptedRead(void* buffer, u32 size, u32 offset);
		virtual int ReadBCA(void* buffer, u32 length);
		virtual int OpenPartition(u32 offset, void* ticket, void* certificate, u32 certificateLength, void* tmd, void* errors);
		virtual int EnableDVD(bool enable) {return 1;}
		virtual int RequestError(void *errorcode);
		virtual int ReportKey();
	};
} }

#endif
