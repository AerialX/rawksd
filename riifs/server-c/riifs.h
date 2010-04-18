#pragma once

#include <stdio.h>
#include <sys/stat.h>
#include <time.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>

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
#include <direct.h>
#include <winsock2.h>

#define THREAD __stdcall
#define getcwd _getcwd
#define unlink _unlink
#define rmdir _rmdir
#define stat _stat
#define MIN(a, b) min(a, b)
#define MSG_TOO_BIG (WSAGetLastError()==WSAEMSGSIZE)

typedef unsigned __int64 u64;
typedef HANDLE OSLock;
typedef int socklen_t;

#else

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define THREAD
#define _mkdir(a) mkdir(a, 0777)
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define MSG_TOO_BIG 0
#define closesocket close

typedef unsigned long long u64;
typedef void* OSLock;
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;

#endif

#if defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN
#define be32(a)			((unsigned int)((a)[3]<<24)|((a)[2]<<16)|((a)[1]<<8)|(a)[0])
#else
#define be32(a)			((unsigned int)((a)[3])|((a)[2]<<8)|((a)[1]<<16)|((a)[0]<<24))
#endif
#define be64(a)			(((u64)be32(a)<<32)|be32((a)+4))

#define ARM_O_TRUNC	0x0400
#define ARM_O_APPEND	0x0008

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

// 2MB buffer
#define TCP_BUFFER_LEN 0x400*0x400*2

class TcpClient
{
private:
	char buffer[TCP_BUFFER_LEN];
	SOCKET sock;
public:
	bool Connected;
	string RemoteEndPoint;

	TcpClient(SOCKET s, string ep);
	~TcpClient();
	void Close();
	int Read(void *data, int len);
	int Write(void *data, int len);
	int Read(ofstream*, int);
	int Write(ifstream*, int);
	template<class Type> int Write(Type *a)	{return Write((void*)a, sizeof(Type));}

};

class TcpListener
{
private:
	SOCKET listen_socket;
	SOCKET locate_socket;
public:
	unsigned short port;
	string LocalEndPoint;

	TcpListener(int port);
	int Start();
	void CheckForBroadcast();
	TcpClient *AcceptTcpClient();
};

void NetworkInit();
void Thread_Sleep(int);
void *Thread_Create(void*, void*);
void Thread_Start(void*);
OSLock CreateLock();
void GetLock(OSLock);
void ReleaseLock(OSLock);
string ip_to_string(unsigned int ip, unsigned short port);

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
	map<Option::Enum, vector<unsigned char> > Options;
	map<int, fstream*> OpenFiles;
	map<int, pair<vector<Stat>, int> > OpenDirs;
	int OpenFileFD;

	time_t LastPing;
	string Name;

	TcpClient *Client;

	Connection(string, TcpClient*);
	~Connection();
	static void THREAD Run(void*);
	vector<unsigned char> GetData(int);
	string GetPath();
	string GetPath(vector<unsigned char>);
	unsigned int GetBE32();
	unsigned int GetFD();
	void DebugPrint(string);
	void Close();
	void Return(int);
	bool WaitForAction();
};
