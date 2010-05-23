#pragma once

struct BinStream
{
	enum SeekType
	{
		Start = SEEK_SET,
		Current = SEEK_CUR,
		End = SEEK_END
	};

	int Construct(bool);
	int Destruct();
	
	int DisableEncryption();
	int EnableReadEncryption();
	int EnableWriteEncryption();
	int Read(void*, int);
	int ReadEndian(void*, int);
	int ReadString(char*, int);
	int Seek(int, SeekType);
	int Write(const void*, int);
	int WriteEndian(const void*, int);
};

struct MemStream : BinStream
{
	u8 data[0x20];

	MemStream(bool);
	~MemStream();
	
	static MemStream* Alloc()
	{
		return (MemStream*)malloc(0x20);
	}

	void Free()
	{
		this->~MemStream();
		free(this);
	}
};

