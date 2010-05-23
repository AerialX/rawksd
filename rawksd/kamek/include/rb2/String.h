#pragma once

struct TextStream
{

};

struct String : TextStream
{
	u8 data[0x08];
	const char* string;

	String(const char*);
	int Destruct();

	const char* c_str();

	static String* Alloc()
	{
		return (String*)malloc(0x0C);
	}

	void Free()
	{
		Destruct();
		free(this);
	}
};
