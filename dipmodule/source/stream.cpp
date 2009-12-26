#include "dip.h"
#include "rijndael.h"

#ifdef YARR

FileStream::FileStream(s32 fid)
{
	Fid = fid;
	close = false;
	position = File_Tell(Fid);
}

FileStream::FileStream(char* path, u8 mode)
{
	Fid = File_Open(path, mode);

	close = true;
	position = 0;
}

void FileStream::Write(const u8* data, int size)
{
	File_Write(Fid, (const u8*)data, size);

	position += size;
}

int FileStream::Read(u8* data, int size)
{
	int ret = File_Read(Fid, data, size);
	
	position += ret;

	return ret;
}

void FileStream::Seek(u64 pos)
{
	File_Seek(Fid, pos, SEEK_SET);

	position = pos;
}

u64 FileStream::GetPosition()
{
	return position;
}

void FileStream::Close()
{
	if (close)
		File_Close(Fid);
}

MemoryStream::MemoryStream(u8* mem)
{
	Memory = mem;
	position = 0;
}
void MemoryStream::Write(const u8* data, int size)
{
	memcpy((u8*)Memory + position, data, size);
	position += size;
}
int MemoryStream::Read(u8* data, int size)
{
	memcpy(data, (u8*)Memory + position, size);
	position += size;
	return size;
}
void MemoryStream::Seek(u64 pos)
{
	position = pos;
}
u64 MemoryStream::GetPosition()
{
	return position;
}

IsfsStream::IsfsStream(int fid)
{
	Fid = fid;
	close = false;
	position = 0;
}
IsfsStream::IsfsStream(char* path)
{
	Fid = os_open(path, O_RDWR);
	close = true;
	position = 0;
}

//extern u32 __RoundUp(u32 p, int round);

void IsfsStream::Write(const u8* data, int size)
{
	/*
	//if (__RoundUp(data, 4) != data) { // Align pointers
		static u8 buffer[0x200] __attribute__ ((aligned (32)));
		for (int i = 0; i < size; i += 0x200) {
			int written = MIN(0x200, size - i);
			memcpy(buffer, (u8*)data + i, written);
			os_write(Fid, buffer, written);
		}
	//} else
	*/
	os_write(Fid, data, size);

	position += size;
}
int IsfsStream::Read(u8* data, int size)
{
	int ret = 0;
	/*
	//if (__RoundUp(data, 4) != data) { // Align pointers
		static u8 buffer[0x200] __attribute__ ((aligned (32)));
		for (int i = 0; i < size; i += 0x200) {
			int read = MIN(0x200, size - i);
			read = os_read(Fid, buffer, read);
			if (read <= 0)
				break;
			ret += read;
			memcpy((u8*)data + i, buffer, read);
		}
	//} else
	*/
	ret = os_read(Fid, data, size);
	
	position += ret;

	return ret;
}
void IsfsStream::Seek(u64 pos)
{
	os_seek(Fid, (s32)pos, SEEK_SET);

	position = pos;
}
void IsfsStream::Close()
{
	if (close)
		os_close(Fid);
}
u64 IsfsStream::GetPosition()
{
	return position;
}

MultifileStream::MultifileStream()
{
	Streams = NULL;
	Lengths = NULL;
	Position = 0;
	Number = 0;
}

void MultifileStream::AddStream(Stream* stream, u32 length)
{
	Streams = (Stream**)Realloc(Streams, (Number + 1) * sizeof(Stream*), Number * sizeof(Stream*));
	Lengths = (u32*)Realloc(Lengths, (Number + 1) * sizeof(u32), Number * sizeof(u32));
	Streams[Number] = stream;
	Lengths[Number] = length;
	Number++;
}

void MultifileStream::Write(const u8* data, int size)
{
	
}

int MultifileStream::Read(u8* data, int s)
{
	int size = s;
	int i = 0;
	u64 totallen = 0;
	for (i = 0; (totallen += Lengths[i]) <= Position; i++)
		;
	totallen -= Lengths[i];

	while (size > 0) {
		u32 toread = MIN(size, Lengths[i] - (Position - totallen));
		Streams[i]->Seek(Position - totallen);
		int read = Streams[i]->Read(data, toread);
		if (read == 0)
			break;
		size -= read;
		data = (u8*)data + read;
		Position += read;

		totallen += Lengths[i];
		i++;
	}

	return s - size;
}

void MultifileStream::Seek(u64 pos)
{
	Position = pos;
}

void MultifileStream::Close()
{
	
}

u64 MultifileStream::GetPosition()
{
	return Position;
}

MultifileStream::~MultifileStream()
{
	Dealloc(Streams);
	Dealloc(Lengths);
}

PartitionStream::PartitionStream(Stream* base, u64 offset)
{/*
	PartitionOffset = offset;
	Base = base;

	u64 lastpos = Base->GetPosition();
	
	Base->Seek(offset + 0x2B8);
	Base->Read((u8*)&PartitionDataOffset, 4);

	CachedBlock = 0xFFFFFFFF;

	u8 key[0x10];
	u8 buffer[0x10];
	u8 iv[0x10];
	memset(iv, 0, 0x10);
	Base->Seek(offset + 0x01dc);
	Base->Read(iv, 0x8);
	Base->Seek(offset + 0x01bf);
	Base->Read(buffer, 0x10);
	u8 commonkey[0x10] = { 0xeb, 0xe4, 0x2a, 0x22, 0x5e, 0x85, 0x93, 0xe4, 0x48, 0xd9, 0xc5, 0x45, 0x73, 0x81, 0xaa, 0xf7 };
	aes_set_key(commonkey);
	aes_decrypt2(iv, buffer, key, 0x10);

	aes_set_key(key);

	Base->Seek(lastpos);*/
}

extern "C" {
	void abort(void)
	{
		while (true)
			;
	}
	void exit(int i)
	{
		while (true)
			;
	}

	int __exidx_end;
	int __exidx_start;

	//unsigned long long __udivdi3 (unsigned long long, unsigned long long) { }
}

int PartitionStream::Read(u8* data, int count)
{
	u32 block = (u32)(Offset / 0x7C00);
	int cacheOffset = (int)(Offset % 0x7C00);
	int cacheSize;
	int dst = 0;

	while (count > 0) {
		if (block != CachedBlock) {
			Base->Seek(PartitionOffset + PartitionDataOffset + (long)block * 0x8000);
			if (Base->Read(CryptedBuffer, 0x8000) != 0x8000) {
				Offset += dst;
				return dst;
			}

			aes_decrypt2(CryptedBuffer + 0x3D0, CryptedBuffer + 0x400, Cache, 0x7C00);

			CachedBlock = block;
		}

		cacheSize = count;
		if (cacheSize + cacheOffset > 0x7C00)
			cacheSize = 0x7C00 - cacheOffset;

		memcpy((u8*)data + dst, (u8*)Cache + cacheOffset, cacheSize);
		dst += cacheSize;
		count -= cacheSize;
		cacheOffset = 0;

		block++;
	}

	Offset += dst;
	return dst;
}

static int dvd_fd = -1;
static u32 dvd_inbuffer[0x08] ATTRIBUTE_ALIGN();
bool dvd_IsInserted() { return dvd_fd >= 0; }
bool dvd_Startup() { return dvd_IsInserted(); }
bool dvd_WriteSectors(sec_t sector, sec_t numSectors, void* buffer) { return false; }
bool dvd_ClearStatus() { return true; }
bool dvd_Shutdown() { return true; }
bool dvd_ReadSectors(sec_t sector, sec_t numSectors, void* buffer)
{
	if (!dvd_IsInserted())
		return false;
	
	dvd_inbuffer[0] = 0xD0 << 24; 
	dvd_inbuffer[1] = 0;
	dvd_inbuffer[2] = 0;
	dvd_inbuffer[3] = numSectors;
	dvd_inbuffer[4] = sector;
	return os_ioctl(dvd_fd, 0xD0, dvd_inbuffer, 0x20, buffer, numSectors * 2048) > 0;
}

const DISC_INTERFACE __io_dvd = {
	DEVICE_TYPE_WII_DVD,
	FEATURE_MEDIUM_CANREAD,
	(FN_MEDIUM_STARTUP)&dvd_Startup,
	(FN_MEDIUM_ISINSERTED)&dvd_IsInserted,
	(FN_MEDIUM_READSECTORS)&dvd_ReadSectors,
	(FN_MEDIUM_WRITESECTORS)&dvd_WriteSectors,
	(FN_MEDIUM_CLEARSTATUS)&dvd_ClearStatus,
	(FN_MEDIUM_SHUTDOWN)&dvd_Shutdown
};

//                                  pages, sectorsPerPage
DvdStream::DvdStream(int fd) : Data(4, 4, &__io_dvd, 0x800)
{
	dvd_fd = fd;
	Offset = 0;
}

int DvdStream::Read(u8* data, int size)
{
	Data.Read(data, Offset, size);
	return size;
}

#endif
