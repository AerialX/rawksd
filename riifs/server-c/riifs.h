#pragma once

#include <stdio.h>
#include <sys/stat.h>
#include <time.h>
#include <ctype.h>

#include <algorithm>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <list>
#include <utility>
#include <map>

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <fcntl.h>
#include <direct.h>

#define THREAD __stdcall

typedef unsigned __int64 u64;
typedef HANDLE OSLock;

#endif

#if defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN
#define be32(a)			((unsigned int)((a)[3]<<24)|((a)[2]<<16)|((a)[1]<<8)|(a)[0])
#else
#define be32(a)			((unsigned int)((a)[3])|((a)[2]<<8)|((a)[1]<<16)|((a)[0]<<24))
#endif
#define be64(a)			(((u64)be32(a)<<32)|be32((a)+4))

using namespace std;

class FileInfo
{
public:
	string FullName;
	u64 Length;
	string Name;
	bool Exists;
	FileInfo(string);
};

class DirectoryInfo
{
public:
	string Name;
	bool Exists;
	string Parent;
	virtual class Stat GetNext()=0;
	virtual ~DirectoryInfo() {}
};

DirectoryInfo* CreateDirectoryInfo(string); 

class TcpClient
{
public:
	bool Connected;
	string RemoteEndPoint;

	virtual void Close()=0;
	virtual int Write(void *data, int len)=0;
	virtual int Read(void *data, int len)=0;
	template<class Type> int Write(Type *a)	{return Write((void*)a, sizeof(Type));}
	int Write(ifstream*, int);
	int Read(ofstream*, int);

	TcpClient(string ep) : RemoteEndPoint(ep) {}
	virtual ~TcpClient() {}
};

class TcpListener
{
public:
	string LocalEndPoint;
	virtual TcpClient *AcceptTcpClient()=0;
	virtual int Start()=0;
	virtual void CheckForBroadcast()=0;
};

TcpListener *NewTcpListener(int port);

void NetworkInit();
void Thread_Sleep(int);
void *Thread_Create(void*, void*);
void Thread_Start(void*);
OSLock CreateLock();
void GetLock(OSLock);
void ReleaseLock(OSLock);

class Action
{
public:
	enum Enum
	{
		Send	= 0x01,
		Receive = 0x02
	};
};

class Option
{
public:
	enum Enum
	{
		Handshake			= 0x00,
		File				= 0x01,
		Path				= 0x02,
		Mode				= 0x03,
		Length				= 0x04,
		Data				= 0x05,
		SeekWhere			= 0x06,
		SeekWhence			= 0x07,
		RenameSource		= 0x08,
		RenameDestination	= 0x09,
		Ping				= 0x10
	};
};

class Command
{
public:
	enum Enum
	{
		Handshake			= 0x00,
		Goodbye				= 0x01,
		Log					= 0x02,
		
		FileOpen			= 0x10,
		FileRead			= 0x11,
		FileWrite			= 0x12,
		FileSeek			= 0x13,
		FileTell			= 0x14,
		FileSync			= 0x15,
		FileClose			= 0x16,
		FileStat			= 0x17,
		FileCreate			= 0x18,
		FileDelete			= 0x19,
		FileRename			= 0x1A,

		FileCreateDir		= 0x20,
		FileOpenDir			= 0x21,
		FileCloseDir		= 0x22,
		FileNextDirPath		= 0x23,
		FileNextDirStat		= 0x24,
		FileNextDirCache	= 0x25
	};
};

class Stat
{
public:
	string Name;
	int Device;
	u64 Size;
	int Mode;
	u64 Identifier;
	static vector<string> IDs;

	Stat() : Device(0),Size(0),Mode(0),Identifier(0) {}
	Stat(FileInfo);
	Stat(DirectoryInfo*);
	void Write(TcpClient*);
};

class Connection
{
private:
	static const string FileIdPath;
	static const int ServerVersion = 0x04;
	static const int MAXPATHLEN = 1024;
	static const int DIRNEXT_CACHE_SIZE = 0x1000;
public:
	string Root;
	map<Option::Enum, vector<unsigned char>> Options;
	map<int, fstream*> OpenFiles;
	map<int, pair<vector<Stat>, int>> OpenDirs;
	int OpenFileFD;

	time_t LastPing;
	string Name;

	TcpClient *Client;

	Connection(string, TcpClient*);
	static void THREAD Run(void*);
	string GetPath();
	string GetPath(vector<unsigned char>);
	unsigned int GetFD();
	void DebugPrint(string);
	~Connection();
	void Close();
	bool WaitForAction();
	void Return(int);
	vector<unsigned char> GetData(int);
	unsigned int GetBE32();
};
