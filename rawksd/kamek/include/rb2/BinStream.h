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

	int Construct(bool);
	int Destruct();

	static MemStream* Alloc()
	{
		return (MemStream*)malloc(0x20);
	}

	static MemStream* Alloc(bool b)
	{
		MemStream* ret = Alloc();
		ret->Construct(b);
		return ret;
	}

	void Free()
	{
		Destruct();
		free(this);
	}
};

