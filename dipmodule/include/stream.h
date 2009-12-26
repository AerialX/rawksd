#pragma once

#include "cache.h"

#define DEVICE_TYPE_WII_DVD (('D'<<24)|('V'<<16)|('S'<<8)|'D')

extern const DISC_INTERFACE __io_dvd;

class Stream
{
public:
	virtual void Write(const u8* data, int size) { }
	virtual int Read(u8* data, int size) { return 0; }
	virtual void Seek(u64 pos) { }
	virtual void Close() { }
	virtual u64 GetPosition() { return 0; }

	virtual ~Stream() { }
	Stream() { }
};

class FileStream : public Stream
{
private:
	bool close;
	int position;
public:
	s32 Fid;
	FileStream(s32 fid);
	FileStream(char* path, u8 mode);

	void Write(const u8* data, int size);
	int Read(u8* data, int size);
	void Seek(u64 pos);
	void Close();

	u64 GetPosition();
};

class MemoryStream : public Stream
{
private:
	void* Memory;
	u64 position;
public:
	MemoryStream(u8* mem);

	void Write(const u8* data, int size);
	int Read(u8* data, int size);
	void Seek(u64 pos);
	void Close() { }

	u64 GetPosition();
};

class IsfsStream : public Stream
{
private:
	int Fid;
	bool close;
	int position;
public:
	IsfsStream(int fid);
	IsfsStream(char* path);

	void Write(const u8* data, int size);
	int Read(u8* data, int size);
	void Seek(u64 pos);
	void Close();

	u64 GetPosition();
};

class MultifileStream : public Stream
{
private:
	Stream** Streams;
	u32* Lengths;
	u32 Number;
	u64 Position;
public:
	MultifileStream();
	~MultifileStream();

	void AddStream(Stream* stream, u32 length);

	void Write(const u8* data, int size);
	int Read(u8* data, int size);
	void Seek(u64 pos);
	void Close();

	u64 GetPosition();
};

class PartitionStream : public Stream
{
private:
	Stream* Base;
	u64 PartitionOffset;
	u64 PartitionDataOffset;
	u64 Offset;
	u8 CryptedBuffer[0x8000];
	u8 Cache[0x7C00];
	u32 CachedBlock;
public:
	PartitionStream(Stream* base, u64 offset);

	void Write(const u8* data, int size) { }
	int Read(u8* data, int size);
	void Seek(u64 pos) { Offset = pos; }
	void Close() { }

	u64 GetPosition() { return Offset; }
};

class DvdStream : public Stream
{
private:
	u64 Offset;
	Cache Data;
	
public:
	DvdStream(int fd);
	void Write(const u8* data, int size) { }
	int Read(u8* data, int size);
	void Seek(u64 pos) { Offset = pos; }
	void Close() { }
	
	u64 GetPosition() { return Offset; }
};
