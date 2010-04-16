#include "riifs.h"

#include <process.h>
#include <sys/types.h>
#include <sys/stat.h>

void NetworkInit()
{
	WSADATA wsadata;
	WSAStartup(MAKEWORD(2,2), &wsadata);
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

void *Thread_Create(void* start, void* arg)
{
	return (void*) _beginthreadex(NULL, 0, (unsigned int(__stdcall*)(void*))start, arg, CREATE_SUSPENDED, NULL);
}

void Thread_Start(void* thread_handle)
{
	ResumeThread((HANDLE)thread_handle);
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
	return new Win32DirectoryInfo(path);
}
