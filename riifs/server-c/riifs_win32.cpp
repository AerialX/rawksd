#include "riifs.h"

#include <process.h>
#include <winsock2.h>
#include <sys/types.h>
#include <sys/stat.h>

void NetworkInit()
{
	WSADATA wsadata;
	WSAStartup(MAKEWORD(2,2), &wsadata);
}

string n_ip_to_string(int ip, unsigned short port)
{
	ostringstream ep;
	unsigned int hostip = ntohl(ip);
	ep << (hostip>>24) << '.' << ((hostip>>16)&0xFF) << '.' << ((hostip>>8)&0xFF) << '.' << (hostip&0xFF) << ':' << port;
	return ep.str();
}

class Win32TcpClient : public TcpClient
{
private:
	SOCKET sock;
public:
	void Close()
	{
		closesocket(sock);
		sock = INVALID_SOCKET;
		Connected = false;
	}

	int Write(void *data, int len)
	{
		if (!len)
			return 0;
		int ret = send(sock, (const char*)data, len, 0);
		if (ret == SOCKET_ERROR || ret==0)
		{
			Close();
			return 0;
		}
		return ret;
	}

	int Read(void *data, int len)
	{
		if (!len)
			return 0;
		int ret = recv(sock, (char*)data, len, 0);
		if (ret == SOCKET_ERROR || ret==0)
		{
			Close();
			return 0;
		}
		return ret;
	}

	Win32TcpClient(SOCKET s, string ep) : TcpClient(ep), sock(s) {}
	~Win32TcpClient()
	{
		if (Connected)
		{
			Close();
		}
	}
};

class Win32TcpListener : public TcpListener
{
private:
	SOCKET listen_socket;
	SOCKET locate_socket;
public:
	unsigned short port;
	Win32TcpListener(int _port) : port(_port)
	{
		SOCKADDR_IN saddr;
		int host_len = sizeof(saddr);
		memset(&saddr, 0, sizeof(saddr));
		saddr.sin_port = htons(port);
		saddr.sin_family = AF_INET;
		saddr.sin_addr.s_addr = htonl(INADDR_ANY);

		listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		bind(listen_socket, (SOCKADDR*)&saddr, sizeof(saddr));
		if (getsockname(listen_socket, (SOCKADDR*)&saddr, &host_len)==0 && host_len>0)
			LocalEndPoint = n_ip_to_string(saddr.sin_addr.s_addr, port);
	}

	int Start()
	{
		int broadcast_opt = 1;
		SOCKADDR_IN saddr;
		memset(&saddr, 0, sizeof(saddr));
		// open udp port 1137 and listen for broadcast messages
		locate_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (locate_socket==INVALID_SOCKET)
			return -1;

		if (setsockopt(locate_socket, SOL_SOCKET, SO_BROADCAST, (char*)&broadcast_opt, sizeof(int))==SOCKET_ERROR)
			return -1;
		saddr.sin_family = AF_INET;
		saddr.sin_port = htons(1137);
		saddr.sin_addr.s_addr = htonl(INADDR_ANY);
		if (bind(locate_socket, (SOCKADDR*)&saddr, sizeof(saddr))==SOCKET_ERROR)
			return -1;

		return listen(listen_socket, SOMAXCONN); // listen returns SOCKET_ERROR(-1) on error
	}

	// wait for up to 15 seconds for a "search" datagram
	void CheckForBroadcast()
	{
		fd_set to_read;
		int sel_ret;
		struct timeval timeout;
		SOCKADDR_IN saddr;
		int host_len;
		char data[4];

		while(1)
		{
			int recv_ret;
			FD_ZERO(&to_read);
			FD_SET(locate_socket, &to_read);
			timeout.tv_sec = 15;
			timeout.tv_usec = 0;
			sel_ret = select(1, &to_read, NULL, NULL, &timeout);
			if (sel_ret<=0) // 0(timeout) or SOCKET_ERROR(-1)
				return;

			host_len = sizeof(saddr);
			recv_ret = recvfrom(locate_socket, data, sizeof(data), 0, (SOCKADDR*)&saddr, &host_len);
			if ((recv_ret==4 || (recv_ret==SOCKET_ERROR && WSAGetLastError()==WSAEMSGSIZE)) && be32(data)==Option::Ping)
			{
				// reply with the actual server port
				unsigned int nport = htonl(port);
				cout << "Broadcast ping from " << n_ip_to_string(saddr.sin_addr.s_addr, ntohs(saddr.sin_port)) << ", replying with port " << port << endl;
				memcpy(data, &nport, sizeof(int));
				host_len = sendto(locate_socket, data, sizeof(data), 0, (SOCKADDR*)&saddr, host_len);
			}
		}
	}

	TcpClient *AcceptTcpClient()
	{
		SOCKADDR_IN host;
		int host_len = sizeof(host);
		SOCKET new_sock;

		new_sock = accept(listen_socket, (SOCKADDR*)&host, &host_len);
		if (new_sock == INVALID_SOCKET)
			return NULL;

		return new Win32TcpClient(new_sock, n_ip_to_string(host.sin_addr.s_addr, host.sin_port));
	}
};

TcpListener* NewTcpListener(int port)
{
	return new Win32TcpListener(port);
}

void *Thread_Create(void* start, void* arg)
{
	return (void*) _beginthreadex(NULL, 0, (unsigned int(__stdcall*)(void*))start, arg, CREATE_SUSPENDED, NULL);
}

void Thread_Start(void* thread_handle)
{
	ResumeThread((HANDLE)thread_handle);
}

void Thread_Sleep(int time_sec)
{
	SleepEx(1000*time_sec, FALSE);
}

OSLock CreateLock()
{
	return CreateMutex(NULL, FALSE, NULL);
}
void GetLock(OSLock lock)
{
	WaitForSingleObject(lock, INFINITE);
}

void ReleaseLock(OSLock lock)
{
	ReleaseMutex(lock);
}

FileInfo::FileInfo(string path) :
FullName(path),
Length(0),
Exists(false)
{
	struct _stat st;
	if (_stat(path.c_str(), &st)!=0)
		return;

	Length = st.st_size;
	Exists = true;
	Name = path.substr(path.find_last_of("/")+1);
}

class Win32DirectoryInfo : public DirectoryInfo
{
private:
	HANDLE hFind;
public:
	Win32DirectoryInfo(string path);
	Stat GetNext();
	~Win32DirectoryInfo();
};

Win32DirectoryInfo::Win32DirectoryInfo(string path)
{
	WIN32_FIND_DATA FindFileData;
	Exists = false;

	hFind = FindFirstFile(path.c_str(), &FindFileData);

	if (hFind == INVALID_HANDLE_VALUE)
		return;

	if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	{
		Exists = true;
		Parent = path.substr(0, path.find_last_of("/"));
		Name = path.substr(path.find_last_of("/")+1);
	}
	FindClose(hFind);
	hFind = INVALID_HANDLE_VALUE;
}

Win32DirectoryInfo::~Win32DirectoryInfo()
{
	if (hFind != INVALID_HANDLE_VALUE)
		FindClose(hFind);
}

Stat Win32DirectoryInfo::GetNext()
{
	Stat st;
	WIN32_FIND_DATA FindFileData;

	if (hFind == INVALID_HANDLE_VALUE)
	{
		string path = Parent + "/" + Name + "\\*";
		hFind = FindFirstFile(path.c_str(), &FindFileData);
	}
	else if (FindNextFile(hFind, &FindFileData)==0)
	{
		FindClose(hFind);
		hFind = INVALID_HANDLE_VALUE;
		return st;
	}

	st.Mode |= S_IFREG;
	st.Name = FindFileData.cFileName;
	if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		st.Mode |= S_IFDIR;
	else {
		string path = Parent + "/" + Name + "/" + st.Name;
		vector<string>::iterator iter = find(st.IDs.begin(), st.IDs.end(), path);
		st.Identifier = iter - st.IDs.begin();
		if (iter == st.IDs.end())
			st.IDs.push_back(path);
		st.Size = ((u64)FindFileData.nFileSizeHigh << 32) + FindFileData.nFileSizeLow;
	}

	return st;
}

DirectoryInfo* CreateDirectoryInfo(string path)
{
	Win32DirectoryInfo *di = new Win32DirectoryInfo(path);
	return di;
}