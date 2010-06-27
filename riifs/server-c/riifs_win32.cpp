/*
 * RiiFS win32 functions (C) 2010 tueidj
 *
 * This file is part of RiiFS server-c.
 *
 * server-c is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * server-c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with server-c; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

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
	// start might not return an unsigned int, but no matter
	return (void*)_beginthreadex(NULL, 0, (unsigned int(__stdcall*)(void*))start, arg, CREATE_SUSPENDED, NULL);
}

void Thread_Start(void* _thread)
{
	HANDLE thread = (HANDLE)_thread;
	ResumeThread(thread);
	/* _beginthreadex doesn't store the thread handle in the CRT thread struct, so neither
	*  _endthread nor _endthreadex will close it. So we close it here.
	*/
	CloseHandle(thread);
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
