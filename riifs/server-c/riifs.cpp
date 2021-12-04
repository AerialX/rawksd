/*
 * RiiFS main functions (C) 2010 tueidj
 *
 * based on code by Copyright (C) 2010 Aaron Lindsay
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

static list<Connection*> Connections;
static OSLock ConnectionsLock;

vector<string> Stat::IDs;
const string Connection::FileIdPath = "/mnt/identifier";

static void AcceptClient(string Root, TcpClient *client)
{
	void *new_thread = NULL;
	Connection *connection = new Connection(Root, client);
	if (connection)
		new_thread = Thread_Create((void*)connection->Run, connection);
	
	if (new_thread==NULL)
	{
		connection->DebugPrint("Couldn't create thread, closing connection");
		delete connection;
		return;
	}
	connection->DebugPrint("Connection Established");
	GetLock(ConnectionsLock);
	Connections.push_back(connection);
	ReleaseLock(ConnectionsLock);
	Thread_Start(new_thread);
}

static void THREAD TimeoutThread(void* _listener)
{
	TcpListener *listener = (TcpListener*)_listener;
	while (true)
	{
		listener->CheckForBroadcast();
		GetLock(ConnectionsLock);
		for (list<Connection*>::iterator iter=Connections.begin(); iter != Connections.end(); ++iter)
		{
			double diff = difftime(time(NULL), (*iter)->LastPing);
			// 120 second ping timeout
			if (diff > 120)
			{
				ostringstream dprint;
				dprint << "Ping Timeout (" << diff << " seconds)";
				(*iter)->DebugPrint(dprint.str());
				(*iter)->Close();
			}
		}
		ReleaseLock(ConnectionsLock);
	}
}

int main(int argc, char* argv[])
{
	string Root;
	int port = 1137;

	if (argc > 1)
		Root = argv[1];
	else
	{
		char work_dir[1024];
		if (getcwd(work_dir, sizeof(work_dir))!=NULL)
			Root = work_dir;
	}

	Root += "/";

	if (argc > 2)
		port = atoi(argv[2]);

	NetworkInit();
	ConnectionsLock = CreateLock();

	TcpListener *listener = new TcpListener(port);
	if (listener->Start()<0) {
		cout << "Couldn't start listener, aborting..." << endl;
		delete listener;
		return -1;
	}

	void *timeout = Thread_Create((void*)TimeoutThread, listener);
	Thread_Start(timeout);

	cout << "RiiFS C++ Server is now ready for connections on " << listener->LocalEndPoint << endl;

	while(true)
		AcceptClient(Root, listener->AcceptTcpClient());

	return 0;
}

FileInfo::FileInfo(string path) :
FullName(path),
Length(0),
Exists(false)
{
	struct stat st;
	if (stat(path.c_str(), &st)!=0 || st.st_mode&S_IFDIR)
		return;

	Length = st.st_size;
	Exists = true;
	Name = path.substr(path.find_last_of("/")+1);
}

TcpClient::TcpClient(SOCKET s, string ep)
{
	RemoteEndPoint= ep;
	sock = s;
	Connected = true;
}

TcpClient::~TcpClient()
{
	if (Connected)
	{
		Close();
	}
	closesocket(sock);
}

void TcpClient::Close()
{
	if (Connected) {
		Connected = false;
		shutdown(sock, 2);
	}
}

int TcpClient::Read(void *data, int len)
{
	if (!len || !Connected)
		return 0;
	int ret = recv(sock, (char*)data, len, 0);
	if (ret <= 0)
	{
		Connected = false;
		return 0;
	}
	return ret;
}

int TcpClient::Write(void *data, int len)
{
	if (!len || !Connected)
		return 0;
	int ret = send(sock, (const char*)data, len, 0);
	if (ret <= 0)
	{
		Connected = false;
		return 0;
	}
	return ret;
}

int TcpClient::Read(ofstream *dst, int len)
{
	int read = 0;
	while (read < len) {
		int ret = 0;
		ret = Read(buffer, MIN(TCP_BUFFER_LEN, len-read));
		if (ret <= 0)
			break;

		dst->write(buffer, ret);

		read += (u64)read;
	}

	return (int)read;
}

int TcpClient::Write(ifstream *src, int len)
{
	int read=0;
	while (read < len) {
		int ret;
		src->read(buffer, MIN(TCP_BUFFER_LEN, len-read));
		ret = src->gcount();

		Write(buffer, ret);

		read += ret;

		if (!(*src))
			break;
	}

	return (int)read;
}

void TcpClient::Pad(int len)
{
	memset(buffer, 0, TCP_BUFFER_LEN);
	while (len>0) {
		int to_send = MIN(TCP_BUFFER_LEN, len);
		Write(buffer, to_send);
		len -= to_send;
	}
}

TcpListener::TcpListener(int _port) : port(_port)
{
	SOCKADDR_IN saddr;
	socklen_t host_len = sizeof(saddr);
	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_port = htons(port);
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);

	listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	bind(listen_socket, (SOCKADDR*)&saddr, sizeof(saddr));
	if (getsockname(listen_socket, (SOCKADDR*)&saddr, &host_len)==0 && host_len>0) {
		port = ntohs(saddr.sin_port);
		LocalEndPoint = ip_to_string(ntohl(saddr.sin_addr.s_addr), port);
	}
}

int TcpListener::Start()
{
	int broadcast_opt = 1;
	SOCKADDR_IN saddr;
	memset(&saddr, 0, sizeof(saddr));
	// open udp port 1137 and listen for broadcast messages
	locate_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (locate_socket < 0)
		return -1;

	if (setsockopt(locate_socket, SOL_SOCKET, SO_BROADCAST, (char*)&broadcast_opt, sizeof(int)) < 0)
		return -1;
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(1137);
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(locate_socket, (SOCKADDR*)&saddr, sizeof(saddr)) < 0)
		return -1;

	return listen(listen_socket, SOMAXCONN); // listen returns SOCKET_ERROR(-1) on error
}

// wait for up to 15 seconds for a "search" datagram
void TcpListener::CheckForBroadcast()
{
	fd_set to_read;
	struct timeval timeout;
	SOCKADDR_IN saddr;
	socklen_t host_len;
	char data[4];

	while(1)
	{
		int recv_ret;
		FD_ZERO(&to_read);
		FD_SET(locate_socket, &to_read);
		timeout.tv_sec = 15;
		timeout.tv_usec = 0;
		recv_ret = select((int)locate_socket+1, &to_read, NULL, NULL, &timeout);
		if (recv_ret<=0) // 0(timeout) or SOCKET_ERROR(-1)
			return;

		host_len = sizeof(saddr);
		memset(data, 0xFF, sizeof(data));
		recv_ret = recvfrom(locate_socket, data, sizeof(data), 0, (SOCKADDR*)&saddr, &host_len);
		if ((recv_ret==4 || (recv_ret<0 && MSG_TOO_BIG)) && be32(data)==Option::Ping)
		{
			// reply with the actual server port
			unsigned int nport = htonl(port);
			cout << "Broadcast ping from " << ip_to_string(ntohl(saddr.sin_addr.s_addr), ntohs(saddr.sin_port)) << ", replying with port " << port << endl;
			memcpy(data, &nport, sizeof(int));
			host_len = sendto(locate_socket, data, sizeof(data), 0, (SOCKADDR*)&saddr, host_len);
		}
	}
}

TcpClient* TcpListener::AcceptTcpClient()
{
	SOCKADDR_IN host;
	socklen_t host_len = sizeof(host);
	SOCKET new_sock;

	new_sock = accept(listen_socket, (SOCKADDR*)&host, &host_len);
	if (new_sock < 0)
		return NULL;

	return new TcpClient(new_sock, ip_to_string(ntohl(host.sin_addr.s_addr), ntohs(host.sin_port)));
}

void Thread_Sleep(int secs)
{
	struct timeval timeout;
	timeout.tv_sec = secs;
	timeout.tv_usec = 0;
	select(0, NULL, NULL, NULL, &timeout);
}

string ip_to_string(unsigned int hostip, unsigned short port)
{
	ostringstream ep;
	ep << (hostip>>24) << '.' << ((hostip>>16)&0xFF) << '.' << ((hostip>>8)&0xFF) << '.' << (hostip&0xFF) << ':' << port;
	return ep.str();
}

Stat::Stat(FileInfo file)
{
	Device = 0;
	vector<string>::iterator iter = find(IDs.begin(), IDs.end(), file.FullName);
	Identifier = iter - IDs.begin();
	if (iter == IDs.end())
		IDs.push_back(file.FullName);
	Size = file.Length;
	Mode = S_IFREG;
	Name = file.Name;
}

Stat::Stat(DirectoryInfo* directory)
{
	Device = 0;
	Identifier = 0;
	Size = 0;
	Mode = S_IFREG | S_IFDIR;
	Name = directory->Name;
}

void Stat::Write(TcpClient *Client)
{
	u64 beIdentifier = be64((unsigned char*)&Identifier);
	u64 beSize = be64((unsigned char*)&Size);
	int beDevice = be32((unsigned char*)&Device);
	int beMode = be32((unsigned char*)&Mode);
	Client->Write(&beIdentifier);
	Client->Write(&beSize);
	Client->Write(&beDevice);
	Client->Write(&beMode);
}

Connection::Connection(string root, TcpClient *client) :
Root(root),
Client(client)
{
	OpenFileFD = 1;
	LastPing = time(NULL);
	Name = Client->RemoteEndPoint;
}

Connection::~Connection()
{
	GetLock(ConnectionsLock);
	Close();
	DebugPrint("Disconnected.");
	Connections.remove(this);
	ReleaseLock(ConnectionsLock);
	delete Client;
}

void Connection::Run(void *_p)
{
	Connection *p = (Connection*)_p;
	while (p->Client && p->Client->Connected)
	{
		if (!p->WaitForAction())
			break;
	}
	delete p;
}

vector<unsigned char> Connection::GetData(int size)
{
	vector<unsigned char> data(size);
	unsigned char *buffer = new unsigned char[size];
	int read = 0;
	while (Client && read < size)
	{
		int readed = Client->Read(buffer+read, size-read);
		if (readed<=0)
			break;
		read += readed;
	}
	if (read)
		data.assign(buffer, buffer+read);
	data.push_back(0);
	delete[] buffer;
	return data;
}

string Connection::GetPath()
{
	string path = GetPath(Options[Option::Path]);
	return path;
}

string Connection::GetPath(vector<unsigned char> data)
{
	if (data.size()==0)
		return "";
	string path((char*)&data[0]);
	if (!path.compare(0, FileIdPath.length(), FileIdPath)) {
		u64 id;
		istringstream filename(path.substr(FileIdPath.length()+1, 16));
		filename >> hex >> id;
		if (Stat::IDs.size() > (int)id)
			return Stat::IDs[(int)id];
	}
	if (path[0] == '/')
		path = path.substr(1);
	if (path.length()>0 && path[path.length()-1] == '/')
		path = path.substr(0, path.length()-1);
	return Root + path;
}

unsigned int Connection::GetBE32()
{
	vector<unsigned char> data = GetData(4);
	if (data.size()>=4)
		return be32(data);
	return 0;
}

int Connection::GetFD()
{
	if (Options[Option::File].size()>=4)
		return (int)be32(Options[Option::File]);
	return -1;
}

void Connection::DebugPrint(string text)
{
	char time_string[100];
	time_t now = time(NULL);
	struct tm *tm_now = localtime(&now);
	strftime(time_string, sizeof(time_string)-1, "%m/%d/%Y %I:%M:%S %p", tm_now);
	cout << '[' << time_string << ']' << " - " << Name << " - " << text << endl;
}

void Connection::Close()
{
	for (map<int, fstream*>::iterator iter=OpenFiles.begin(); iter != OpenFiles.end(); iter++)
	{
		iter->second->close();
		delete iter->second;
	}
	OpenFiles.clear();
	Client->Close();
}

void Connection::Return(int value)
{
	ostringstream s;
	s << "\tReturn " << value;
	DebugPrint(s.str());
	value = be32(((unsigned char*)&value));
	Client->Write(&value);
}

bool Connection::WaitForAction()
{
	ostringstream dprint;
	Action::Enum action = (Action::Enum)GetBE32();
	if (Client==NULL || !Client->Connected)
		return false;
	LastPing = time(NULL);

	switch (action)
	{
		case Action::Send: {
			Option::Enum option = (Option::Enum)GetBE32();
			int length = GetBE32();
			vector<unsigned char> data;
			if (length >= 0)
				data = GetData(length);

			Options[option] = data;

			if (option == Option::Ping)
				DebugPrint("Ping()");

			break;
		}
		case Action::Receive: {
			Command::Enum command = (Command::Enum)GetBE32();
			switch (command)
			{
				case Command::Handshake: {
					string clientversion((char*)&(Options[Option::Handshake][0]));
					dprint << "Handshake: Client Version \"" << clientversion << "\"";
					DebugPrint(dprint.str());

					if (clientversion == "1.03")
						Return(ServerVersion);
					else if (clientversion == "1.02")
						Return(3);
					else
						Return(-1);

					break;
				}
				case Command::Goodbye: {
					DebugPrint("Goodbye");
					Return(1);

					return false;
				}
				case Command::Log: {
					dprint << "Log: " << &(Options[Option::Data][0]);
					DebugPrint(dprint.str());
					Return(1);
					break;
				}
				case Command::FileOpen: {
					string path = GetPath();
					int mode = 0;
					if (Options[Option::Mode].size()>=4)
						mode = be32(Options[Option::Mode]);

					dprint << "File_Open(\"" << path << "\", " << showbase << hex << mode << dec << noshowbase << ");";
					DebugPrint(dprint.str());

					int fd = -1;
					ios_base::openmode fmode = ios_base::binary;
					if (!(mode & ARM_O_CREAT))
						fmode |= ios_base::in;
					if (mode&O_RDWR || mode&O_WRONLY)
						fmode |= ios_base::out;

					if (mode & ARM_O_TRUNC)
						fmode |= ios_base::trunc;
					else if (mode & ARM_O_APPEND)
						fmode |= ios_base::app;

					fstream *new_stream = new fstream(path.c_str(), fmode);
					if (new_stream->fail())
						delete new_stream;
					else
					{
						fd = OpenFileFD++;
						OpenFiles.insert(map<int, fstream*>::value_type(int(fd), new_stream));
					}

					Return(fd);
					break;
				}
				case Command::FileRead: {
					int ret = 0;
					int fd = GetFD();
					int length=0;
					if (Options[Option::Length].size()>=4)
						length = be32(Options[Option::Length]);
					dprint << "File_Read(" << fd << ", " << length << ");";
					DebugPrint(dprint.str());
					if (OpenFiles.count(fd)) {
						// pointless seek in case last op was a write
						OpenFiles[fd]->seekg(OpenFiles[fd]->tellg(), ios::beg);
						ret = Client->Write((ifstream*)OpenFiles[fd], length);
					}
					if (ret < length)
						Client->Pad(length - ret);
					Return(ret);
					break;
				}
				case Command::FileWrite: {
					int fd  = GetFD();
					int length = Options[Option::Data].size()-1;
					dprint << "File_Write(" << fd << ", " << length << ");";
					DebugPrint(dprint.str());
					if (length<=0 || !OpenFiles.count(fd))
						Return(0);
					else
					{
						// pointless seek in case last op was a read
						OpenFiles[fd]->seekg(OpenFiles[fd]->tellg(), ios::beg);
						OpenFiles[fd]->write((char*)&Options[Option::Data][0], length);
						if (OpenFiles[fd]->bad()) {
							Return(0);
							OpenFiles[fd]->clear();
						}
						else
							Return(length);
					}
					break;
				}
				case Command::FileSeek: {
					int fd = GetFD();
					fstream::off_type where=0;
					ios_base::seekdir whence = ios_base::cur;
					if (Options[Option::SeekWhere].size()<4 || Options[Option::SeekWhence].size()<4)
						fd = -1;
					else {
						where = (int)be32(Options[Option::SeekWhere]);
						switch(be32(Options[Option::SeekWhence])) {
							case 0:
								whence = ios_base::beg;
								break;
							case 2:
								whence = ios_base::end;
								break;
							//case 1:
							default:
								whence = ios_base::cur;
						}
					}
					dprint << "File_Seek(" << fd << ", " << where << ", " << whence << ");";
					DebugPrint(dprint.str());
					if (!OpenFiles.count(fd))
						Return(-1);
					else
					{
						// set the read pointer, it might fail if file is write only
						// in that case set the write pointer
						if (OpenFiles[fd]->seekg(where, whence).fail()) {
							OpenFiles[fd]->clear();
							OpenFiles[fd]->seekp(where, whence);
						}
						Return(OpenFiles[fd]->fail() ? -1 : 0);
						OpenFiles[fd]->clear();
					}
					break;
				}
				case Command::FileTell: {
					int fd = GetFD();
					dprint << "File_Tell(" << fd << ");";
					DebugPrint(dprint.str());
					if (!OpenFiles.count(fd))
						Return(-1);
					else
						// tellp for writing position
						Return((int)OpenFiles[fd]->tellg());
					break;
				}
				case Command::FileSync: {
					int fd = GetFD();
					dprint << "File_Sync(" << fd << ");";
					DebugPrint(dprint.str());
					if (!OpenFiles.count(fd))
						Return(-1);
					else
					{
						OpenFiles[fd]->flush();
						Return(1);
					}
					break;
				}
				case Command::FileClose: {
					int fd = GetFD();
					dprint << "File_Close(" << fd << ");";
					DebugPrint(dprint.str());
					if (!OpenFiles.count(fd))
						Return(0);
					else
					{
						OpenFiles[fd]->close();
						delete OpenFiles[fd];
						OpenFiles.erase(fd);
						Return(1);
					}
					break;
				}
				case Command::FileStat: {
					string path = GetPath();
					dprint << "File_Stat(\"" << path << "\");";
					DebugPrint(dprint.str());

					FileInfo file(path);
					if (!file.Exists) {
						DirectoryInfo *dir = CreateDirectoryInfo(path);
						if (!dir->Exists) {
							Stat empty;
							empty.Write(Client);
							Return(-1);
						} else {
							Stat st(dir);
							st.Write(Client);
							Return(0);
						}
						delete dir;
					} else {
						Stat st(file);
						st.Write(Client);
						Return(0);
					}
					break;
				}
				case Command::FileCreate: {
					string path = GetPath();
					dprint << "File_Create(\"" << path << "\");";
					DebugPrint(dprint.str());
					FileInfo file(path);
					if (!file.Exists) {
						ofstream f(path.c_str(), ios_base::out);
						if (!f) {
							Return(0);
							break;
						}
						else
							f.close();
					}
					Return(1);
					break;
				}
				case Command::FileDelete: {
					string path = GetPath();
					dprint << "File_Delete(\"" << path << "\");";
					DebugPrint(dprint.str());
					Return (!remove(path.c_str()));
					break;
				}
				case Command::FileRename: {
					string source = GetPath(Options[Option::RenameSource]);
					string dest = GetPath(Options[Option::RenameDestination]);
					dprint << "File_Rename(\"" << source << "\", \"" << dest << "\");";
					DebugPrint(dprint.str());
					FileInfo file(source);
					if (!file.Exists)
						Return(0);
					else {
						rename(source.c_str(), dest.c_str());
						Return(1);
					}
					break;
				}
				case Command::FileCreateDir: {
					string path = GetPath();
					dprint << "File_CreateDir(\"" + path + "\");";
					DebugPrint(dprint.str());
					DirectoryInfo* dir = CreateDirectoryInfo(path);
					if (!dir->Exists)
						Return(!mkdir(path.c_str()));
					else
						Return(1);
					delete dir;
					break;
				}
				case Command::FileOpenDir: {
					string path = GetPath();
					dprint << "File_OpenDir(\"" << path << "\");";
					DebugPrint(dprint.str());
					DirectoryInfo* dir = CreateDirectoryInfo(path);

					if (!dir->Exists)
						Return(-1);
					else {
						int fd = OpenFileFD++;
						vector<Stat> stats;
						Stat stat = dir->GetNext();
						while (stat.Mode & S_IFREG)
						{
							stats.push_back(stat);
							stat = dir->GetNext();
						}
						pair<vector<Stat>, int> p(stats, 0);
						OpenDirs.insert(map<int, pair<vector<Stat>, int> >::value_type(fd, p));

						Return (fd);
					}
					delete dir;
					break;
				}
				case Command::FileCloseDir: {
					int fd = GetFD();
					dprint << "File_CloseDir(" << fd << ");";
					DebugPrint(dprint.str());

					if (!OpenDirs.count(fd))
						Return(-1);
					else {
						OpenDirs[fd].first.clear();
						OpenDirs.erase(fd);
						Return(1);
					}
					break;
				}
				case Command::FileNextDirPath: {
					char pathbuf[MAXPATHLEN] = {0};
					int fd = GetFD();
					dprint << "File_NextDir(" << fd << ");";
					DebugPrint(dprint.str());
					if (!OpenDirs.count(fd) || OpenDirs[fd].second >= OpenDirs[fd].first.size()) {
						Client->Write(pathbuf, sizeof(pathbuf));
						Return(-1);
					} else {
						string name = OpenDirs[fd].first[OpenDirs[fd].second].Name;
						strcpy(pathbuf, name.c_str());
						Client->Write(pathbuf, sizeof(pathbuf));
						Return(name.length());
					}
					break;
				}
				case Command::FileNextDirStat: {
					int fd = GetFD();
					if (!OpenDirs.count(fd)) {
						Stat s;
						s.Write(Client);
						Return(1);
					} else {
						OpenDirs[fd].first[OpenDirs[fd].second++].Write(Client);
						Return(0);
					}
					break;
				}
				default:
					break;
			}
			break;
		}
		default:
			return true;
	}
	return true;
}
