// server-c.cpp : Defines the entry point for the console application.
//

#include "riifs.h"

static list<Connection*> Connections;
static OSLock ConnectionsLock;

vector<string> Stat::IDs;
const string Connection::FileIdPath = "/mnt/identifier";

static void AcceptClient(string Root, TcpClient *client)
{
	Connection *connection = new Connection(Root, client);
	void *new_thread = Thread_Create(connection->Run, connection);
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
		_getcwd(work_dir, sizeof(work_dir));
		Root = work_dir;
	}

	Root += "/";

	if (argc > 2)
		port = atoi(argv[2]);

	NetworkInit();
	ConnectionsLock = CreateLock();

	TcpListener *listener = NewTcpListener(port);
	if (listener->Start()<0) {
		cout << "Couldn't start listener, aborting..." << endl;
		delete listener;
		return -1;
	}

	void *timeout = Thread_Create(TimeoutThread, listener);
	Thread_Start(timeout);

	cout << "RiiFS C++ Server is now ready for connections on " << listener->LocalEndPoint << endl;
	
	while(true)
		AcceptClient(Root, listener->AcceptTcpClient());

	return 0;
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
	TcpClient *tmpClient = Client;
	if (tmpClient == NULL)
		return; // already closed
	DebugPrint("Disconnected.");
	for (map<int, fstream*>::iterator iter=OpenFiles.begin(); iter != OpenFiles.end(); iter++)
	{
		iter->second->close();
		delete iter->second;
	}
	OpenFiles.clear();

	Client = NULL;
	delete tmpClient;
}

Connection::Connection(string root, TcpClient *client) :
Root(root),
Client(client)
{
	OpenFileFD = 1;
	LastPing = time(NULL);
	Name = Client->RemoteEndPoint;
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

Connection::~Connection()
{
	Close();
	GetLock(ConnectionsLock);
	Connections.remove(this);
	ReleaseLock(ConnectionsLock);
}

bool Connection::WaitForAction()
{
	ostringstream dprint;
	Action::Enum action = (Action::Enum)GetBE32();
	LastPing = time(NULL);

	switch (action)
	{
		case Action::Send: {
			Option::Enum option = (Option::Enum)GetBE32();
			int length = GetBE32();
			vector<unsigned char> data;
			if (length > 0)
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

					if (clientversion == "1.02")
						Return(ServerVersion);
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
					int mode = be32(Options[Option::Mode]);

					dprint << "File_Open(\"" << path << "\", " << showbase << hex << mode << dec << noshowbase << ");";
					DebugPrint(dprint.str());
					
					int fd = -1;
					int fmode = ios_base::binary; // no more ios_base::nocreate, so O_CREAT is ignored here
					if (mode & O_WRONLY)
						fmode |= ios_base::out;
					else if (mode & O_RDWR)
						fmode |= ios_base::out|ios_base::in;
					else
						fmode |= ios_base::in;

					if (mode & O_TRUNC)
						fmode |= ios_base::trunc;
					else if (mode & O_APPEND)
						fmode |= ios_base::app;

					fstream *new_stream = new fstream(path.c_str(), fmode);
					if (new_stream->fail())
					{
						fd = -1;
						delete new_stream;
					}
					else
					{
						fd = OpenFileFD++;
						OpenFiles.insert(map<int, fstream*>::value_type(int(fd), new_stream));
					}

					Return(fd);
					break;
				}
				case Command::FileRead: {
					int fd = GetFD();
					int length = be32(Options[Option::Length]);
					dprint << "File_Read(" << fd << ", " << length << ");";
					DebugPrint(dprint.str());
					if (!OpenFiles.count(fd))
						Return(0);
					else
					{
						int ret = Client->Write((ifstream*)OpenFiles[fd], length);
						Return(ret);
					}
					break;
				}
				case Command::FileWrite: {
					int fd  = GetFD();
					int length = Options[Option::Data].size()-1;
					dprint << "File_Write(" << fd << ", " << length << ");";
					DebugPrint(dprint.str());
					if (!OpenFiles.count(fd))
						Return(0);
					else
					{
						OpenFiles[fd]->write((char*)&Options[Option::Data][0], length);
						if (OpenFiles[fd]->bad())
							Return(0);
						else
							Return(length);
					}
					break;
				}
				case Command::FileSeek: {
					int fd = GetFD();
					int where = be32(Options[Option::SeekWhere]);
					int whence = be32(Options[Option::SeekWhence]);
					dprint << "File_Seek(" << fd << ", " << where << ", " << whence << ");";
					DebugPrint(dprint.str());
					if (!OpenFiles.count(fd))
						Return(-1);
					else
					{
						// seekp for writing position
						OpenFiles[fd]->seekg(where, whence);
						Return(0);
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
						Stat empty;
						empty.Write(Client);
						Return(-1);
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
					FileInfo file(path);
					if (file.Exists) {
						_unlink(path.c_str());
						Return(1);
					} else {
						DirectoryInfo* dir = CreateDirectoryInfo(path);
						if (dir->Exists) {
							_rmdir(path.c_str());
							Return(1);
						} else
							Return(0);
						delete dir;
					}
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
						_mkdir(path.c_str());
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
						OpenDirs.insert(map<int, pair<vector<Stat>, int>>::value_type(fd, p));

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
			break;
	}
	return true;
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
	if (path[path.length()-1] == '/')
		path = path.substr(0, path.length()-1);
	return Root + path;
}

unsigned int Connection::GetFD()
{
	return be32(Options[Option::File]);
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

void Connection::Return(int value)
{
	ostringstream s;
	s << "\tReturn " << value;
	DebugPrint(s.str());
	value = be32(((unsigned char*)&value));
	Client->Write(&value);
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

unsigned int Connection::GetBE32()
{
	vector<unsigned char> data = GetData(4);
	return be32(data);
}

 // 2MB buffer
#define BUFFER_LEN 0x400*0x400*2
int TcpClient::Read(ofstream *dst, int len)
{
	char *buffer = new char[BUFFER_LEN];

	u64 read = 0;
	while (read < len) {
		int ret = 0;
		ret = Read(buffer, min(BUFFER_LEN, len-read));
		if (ret <= 0)
			break;

		dst->write(buffer, ret);

		read += (u64)read;
	}

	delete[] buffer;

	return (int)read;
}

int TcpClient::Write(ifstream *src, int len)
{
	char *buffer = new char[BUFFER_LEN];

	u64 read=0;
	while (read < len) {
		int ret = 0;
		src->read(buffer, min(BUFFER_LEN, len-read));
		ret = src->gcount();

		Write(buffer, ret);

		if (!(*src))
			break;

		read += ret;
	}

	delete[] buffer;

	return (int)read;
}