using System;
using System.Collections.Generic;
using System.Net.Sockets;
using ConsoleHaxx.Common;
using System.IO;
using System.Threading;

namespace ConsoleHaxx.RiiFS
{
	[Flags]
	public enum FileModes : int
	{
		O_RDONLY = 0,
		O_WRONLY = 1,
		O_RDWR = 2,
		O_APPEND = 8,
		O_CREAT = 0x100,
		O_TRUNC = 0x400,
		O_WDECR = 0x4000
	}

	[Flags]
	public enum StatModes : int
	{
		S_IFDIR = 0x4000, /* directory */
		S_IFREG = 0x8000, /* regular */
	}

	public struct Stat
	{
		public static List<string> IDs = new List<string>();

		public Stat(FileInfo file)
		{
			Device = 0;
			int index = IDs.IndexOf(file.FullName);
			if (index < 0) {
				Identifier = (ulong)IDs.Count;
				IDs.Add(file.FullName);
			} else
				Identifier = (ulong)index;
			Size = (ulong)file.Length;
			Mode = (int)StatModes.S_IFREG;
			Name = file.Name;
		}

		public Stat(DirectoryInfo dir)
		{
			Device = 0;
			Identifier = 0;
			Size = 0;
			Mode = (int)StatModes.S_IFREG | (int)StatModes.S_IFDIR;
			Name = dir.Name;
		}

		public string Name;
		public int Device;
		public ulong Size;
		public int Mode;
		public ulong Identifier;

		public void Write(EndianReader writer)
		{
			writer.Write(Identifier);
			writer.Write(Size);
			writer.Write(Device);
			writer.Write(Mode);
		}
	}

	public enum Action : int
	{
		Send = 0x01,
		Receive = 0x02
	}

	public enum Option : int
	{
		Handshake = 0x00,

		File = 0x01,
		Path = 0x02,
		Mode = 0x03,
		Length = 0x04,
		Data = 0x05,
		SeekWhere = 0x06,
		SeekWhence = 0x07,
		RenameSource = 0x08,
		RenameDestination = 0x09,
		Ping = 0x10
	}

	public enum Command : int
	{
		Handshake = 0x00,
		Goodbye = 0x01,

		FileOpen = 0x10,
		FileRead = 0x11,
		FileWrite = 0x12,
		FileSeek = 0x13,
		FileTell = 0x14,
		FileSync = 0x15,
		FileClose = 0x16,
		FileStat = 0x17,
		FileCreate = 0x18,
		FileDelete = 0x19,
		FileRename = 0x1A,

		FileCreateDir = 0x20,
		FileOpenDir = 0x21,
		FileCloseDir = 0x22,
		FileNextDirPath = 0x23,
		FileNextDirStat = 0x24,
		FileNextDirCache = 0x25
	}

	class Connection
	{
		const string FileIdPath = "/mnt/identifier/";
		const int ServerVersion = 0x03;
		const int MAXPATHLEN = 1024;
		const int DIRNEXT_CACHE_SIZE = 0x1000;

		public string Root;

		public Dictionary<Option, byte[]> Options;
		public Dictionary<int, Stream> OpenFiles;
		public Dictionary<int, Pair<List<Stat>, int>> OpenDirs;
		public int OpenFileFD;

		public TcpClient Client;
		public NetworkStream Stream;
		public EndianReader Writer;

		public DateTime LastPing;

		public Connection(string root, TcpClient client)
		{
			OpenFileFD = 1;
			OpenDirs = new Dictionary<int, Pair<List<Stat>, int>>();
			OpenFiles = new Dictionary<int, Stream>();
			Options = new Dictionary<Option, byte[]>();

			Root = root;

			Client = client;
			Stream = Client.GetStream();
			Writer = new EndianReader(Stream, Endianness.BigEndian);

			LastPing = DateTime.Now;
		}

		public void Run()
		{
			while (Client.Connected) {
				try {
					if (!WaitForAction(Client))
						break;
				} catch { }
			}
			Close();
		}

		string GetPath()
		{
			string path = GetPath(Options[Option.Path]);
			if (!Path.IsPathRooted(path) || !path.ToLower().StartsWith(Root.ToLower()))
				throw new NotImplementedException();
			return path;
		}

		string GetPath(byte[] data)
		{
			string path = Util.Encoding.GetString(data);
			if (path.StartsWith(FileIdPath)) {
				ulong id = ulong.Parse(path.Substring(FileIdPath.Length), System.Globalization.NumberStyles.HexNumber);
				if (Stat.IDs.Count > (int)id)
					return Stat.IDs[(int)id];
			}
			if (path.StartsWith("/"))
				path = path.Substring(1);
			if (path.EndsWith("/"))
				path = path.Substring(0, path.Length - 1);
			return Path.Combine(Root, path);
		}

		int GetFD()
		{
			return BigEndianConverter.ToInt32(Options[Option.File]);
		}

		public void DebugPrint(string text)
		{
			Console.Write("[" + DateTime.Now.ToString() + "]");
			try {
				Console.Write(" - " + Client.Client.RemoteEndPoint.ToString() + " - ");
			} catch { }
			Console.WriteLine(text);
		}

		~Connection()
		{
			Close();
		}

		public void Close()
		{
			DebugPrint("Disconnected.");
			Program.Connections.Remove(this);

			try { Client.Client.Close(); } catch { }
			try { Client.Close(); } catch { }
			foreach (var file in OpenFiles) {
				try { file.Value.Close(); } catch { }
			}
		}

		bool WaitForAction(TcpClient client)
		{
			Action action = (Action)BigEndianConverter.ToInt32(GetData(4));

			LastPing = DateTime.Now;

			switch (action) {
				case Action.Send: {
					Option option = (Option)BigEndianConverter.ToInt32(GetData(4));
					int length = BigEndianConverter.ToInt32(GetData(4));
					byte[] data;
					if (length > 0) {
						data = GetData(length);
					} else
						data = null;

					Options[option] = data;

					if (option == Option.Ping)
						DebugPrint("Ping()");

					break; }
				case Action.Receive:
					Command command = (Command)BigEndianConverter.ToInt32(GetData(4));
					switch (command) {
						case Command.Handshake: {
							string clientversion = Util.Encoding.GetString(Options[Option.Handshake]);
							DebugPrint("Handshake: Client Version \"" + clientversion + "\"");

							if (clientversion != "1.02")
								Return(-1);
							else
								Return(ServerVersion);
							break; }
						case Command.Goodbye:
							DebugPrint("Goodbye");
							Return(1);

							return false;
						case Command.FileOpen: {
							string path = Path.Combine(Root, GetPath());
							int mode = BigEndianConverter.ToInt32(Options[Option.Mode]);

							DebugPrint("File_Open(\"" + path + "\", 0x" + mode.ToString("X") + ");");

							int fd = -1;
							FileMode fmode = FileMode.Open;
							FileAccess faccess = FileAccess.ReadWrite;
							if ((mode & (int)FileModes.O_WRONLY) == (int)FileModes.O_WRONLY)
								faccess = FileAccess.Write;
							else if ((mode & (int)FileModes.O_RDWR) == (int)FileModes.O_RDWR)
								faccess = FileAccess.ReadWrite;
							else
								faccess = FileAccess.Read;
							if ((mode & (int)FileModes.O_CREAT) == (int)FileModes.O_CREAT)
								fmode = FileMode.Create;
							else if ((mode & (int)FileModes.O_TRUNC) == (int)FileModes.O_TRUNC)
								fmode = FileMode.Truncate;
							else if ((mode & (int)FileModes.O_APPEND) == (int)FileModes.O_APPEND)
								fmode = FileMode.Append;

							try {
								Stream fstream = new FileStream(path, fmode, faccess, FileShare.ReadWrite | FileShare.Delete);
								fd = OpenFileFD++;

								OpenFiles.Add(fd, fstream);
							} catch { }

							Return(fd);
							break; }
						case Command.FileRead: {
							int fd = GetFD();
							int length = BigEndianConverter.ToInt32(Options[Option.Length]);

							DebugPrint("File_Read(" + fd + ", " + length + ");");

							if (!OpenFiles.ContainsKey(fd))
								Return(0);
							else {
								int ret = (int)Util.StreamCopy(Stream, OpenFiles[fd], length);
								Writer.Pad(length - ret);
								Return(length);
							}

							break; }
						case Command.FileWrite: {
							Return(-1);
							break;
							int fd = GetFD();
							DebugPrint("File_Write(" + fd + ", " + Options[Option.Data].Length + ");");
							if (!OpenFiles.ContainsKey(fd))
								Return(0);
							else {
								try {
									OpenFiles[fd].Write(Options[Option.Data], 0, Options[Option.Data].Length);
									Return(Options[Option.Data].Length);
								} catch {
									Return(-1);
								}
							}
							break; }
						case Command.FileSeek: {
							int fd = GetFD();
							int where = BigEndianConverter.ToInt32(Options[Option.SeekWhere]);
							int whence = BigEndianConverter.ToInt32(Options[Option.SeekWhence]);
							DebugPrint("File_Seek(" + fd + ", " + where + ", " + whence + ");");
							if (!OpenFiles.ContainsKey(fd))
								Return(-1);
							else {
								OpenFiles[fd].Seek(where, (SeekOrigin)whence);
								Return(0);
							}
							break; }
						case Command.FileTell: {
							int fd = GetFD();
							DebugPrint("File_Tell(" + fd + ");");
							if (!OpenFiles.ContainsKey(fd))
								Return(-1);
							else {
								Return((int)OpenFiles[fd].Position);
							}
							break; }
						case Command.FileSync: {
							int fd = GetFD();
							DebugPrint("File_Sync(" + fd + ");");
							if (!OpenFiles.ContainsKey(fd))
								Return(0);
							else {
								OpenFiles[fd].Flush();
								Return(1);
							}
							break; }
						case Command.FileClose: {
							int fd = GetFD();
							DebugPrint("File_Close(" + fd + ");");
							if (!OpenFiles.ContainsKey(fd))
								Return(0);
							else {
								OpenFiles[fd].Close();
								OpenFiles.Remove(fd);
								Return(1);
							}
							break; }
						case Command.FileStat: {
							string path = Path.Combine(Root, GetPath());

							DebugPrint("File_Stat(\"" + path + "\");");

							FileInfo file = new FileInfo(path);
							if (!file.Exists) {
								new Stat().Write(Writer);
								Return(-1);
							} else {
								new Stat(file).Write(Writer);
								Return(0);
							}
							break; }
						case Command.FileCreate: {
							try {
								string path = Path.Combine(Root, GetPath());
								DebugPrint("File_Create(\"" + path + "\");");
								if (!File.Exists(path))
									File.Create(path);
								Return(1);
							} catch { Return(0); }
							break; }
						case Command.FileDelete: {
							try {
								string path = Path.Combine(Root, GetPath());
								DebugPrint("File_Delete(\"" + path + "\");");
								if (File.Exists(path)) {
									File.Delete(path);
									Return(1);
								} else if (Directory.Exists(path)) {
									Directory.Delete(path, true);
									Return(1);
								} else
									Return(0);
							} catch { Return(0); }
							break; }
						case Command.FileRename: {
							string source = GetPath(Options[Option.RenameSource]);
							string dest = GetPath(Options[Option.RenameDestination]);
							DebugPrint("File_Rename(\"" + source + "\", \"" + dest + "\");");
							try {
								FileInfo file = new FileInfo(source);
								if (!file.Exists)
									Return(0);
								else {
									file.MoveTo(dest);
									Return(1);
								}
							} catch { Return(0); }
							break; }
						case Command.FileCreateDir: {
							string path = Path.Combine(Root, GetPath());
							DebugPrint("File_CreateDir(\"" + path + "\");");
							try {
								if (!Directory.Exists(path))
									Directory.CreateDirectory(path);
								Return(1);
							} catch { Return(0); }
							break; }
						case Command.FileOpenDir: {
							string path = Path.Combine(Root, GetPath());
							DebugPrint("File_OpenDir(\"" + path + "\");");
							DirectoryInfo dir = new DirectoryInfo(path);

							if (!dir.Exists)
								Return(-1);
							else {
								int fd = OpenFileFD++;
								List<Stat> stats = new List<Stat>();
								Stat stat = new Stat(dir);
								stat.Name = ".";
								stats.Add(stat);
								stat = new Stat(dir.Parent);
								stat.Name = "..";
								stats.Add(stat);
								foreach (FileInfo file in dir.GetFiles())
									stats.Add(new Stat(file));
								foreach (DirectoryInfo subdir in dir.GetDirectories())
									stats.Add(new Stat(subdir));
								OpenDirs.Add(fd, new Pair<List<Stat>, int>(stats, 0));
								Return(fd);
							}
							break; }
						case Command.FileCloseDir: {
							int fd = GetFD();
							DebugPrint("File_CloseDir(" + fd + ");");
							if (!OpenDirs.ContainsKey(fd))
								Return(-1);
							else {
								OpenDirs.Remove(fd);
								Return(1);
							}
							break; }
						case Command.FileNextDirPath: {
							int fd = GetFD();
							DebugPrint("File_NextDir(" + fd + ");");
							if (!OpenDirs.ContainsKey(fd) || OpenDirs[fd].Value >= OpenDirs[fd].Key.Count) {
								Writer.Pad(MAXPATHLEN);
								Return(-1);
							} else {
								string name = OpenDirs[fd].Key[OpenDirs[fd].Value].Name;
								byte[] pathbuf = new byte[1024];
								Util.Encoding.GetBytes(name).CopyTo(pathbuf, 0);
								Writer.Write(pathbuf); // I blame net_recv...
								//Writer.Write(name);
								//Writer.Pad(MAXPATHLEN - name.Length);
								Return(name.Length);
							}
							break; }
						case Command.FileNextDirStat: {
							int fd = GetFD();
							if (!OpenDirs.ContainsKey(fd)) {
								new Stat().Write(Writer);
								Return(1);
							} else {
								OpenDirs[fd].Key[OpenDirs[fd].Value++].Write(Writer);
								Return(0);
							}
							break; }
						case Command.FileNextDirCache: { // NOTE: Not enabled in the current client release (1.02)
							int fd = GetFD();
							DebugPrint("File_NextDirCache(" + fd + ");");
							if (!OpenDirs.ContainsKey(fd)) {
								Writer.Pad(DIRNEXT_CACHE_SIZE);
								Return(-1);
							} else {
								List<int> OffsetTable = new List<int>();
								List<string> NameTable = new List<string>();
								List<Stat> StatTable = new List<Stat>();
								int size = 0;
								int strlen = 0;
								while (size < DIRNEXT_CACHE_SIZE && OpenDirs[fd].Value < OpenDirs[fd].Key.Count) {
									string name = OpenDirs[fd].Key[OpenDirs[fd].Value].Name;
									Stat stat = OpenDirs[fd].Key[OpenDirs[fd].Value++];
									OffsetTable.Add(strlen);
									NameTable.Add(name);
									StatTable.Add(stat);
									strlen += name.Length + 1;

									size = 4 + OffsetTable.Count * 4 + StatTable.Count * 24 + strlen;
								}
								if (OpenDirs[fd].Value == OpenDirs[fd].Key.Count && DIRNEXT_CACHE_SIZE - size >= 28) {
									OffsetTable.Add(-1);
									StatTable.Add(new Stat());
								} else { // We always go one-over
									OffsetTable.RemoveAt(OffsetTable.Count - 1);
									StatTable.RemoveAt(StatTable.Count - 1);
									NameTable.RemoveAt(NameTable.Count - 1);
									OpenDirs[fd].Value--;
								}
								byte[] outbuffer = new byte[DIRNEXT_CACHE_SIZE];
								MemoryStream outstream = new MemoryStream(outbuffer);
								EndianReader outwriter = new EndianReader(outstream, Endianness.BigEndian);
								outwriter.Write(OffsetTable.Count);
								foreach (int offset in OffsetTable)
									outwriter.Write(offset);
								foreach (Stat stat in StatTable)
									stat.Write(outwriter);
								foreach (string str in NameTable) {
									outwriter.Write(str);
									outwriter.Write((byte)0);
								}
								Writer.Write(outbuffer);
								Return(0);
							}
							break; }
						default:
							break;
					}
					break;
				default:
					break;
			}

			return true;
		}

		void Return(int value)
		{
			Writer.Write(value);

			DebugPrint("\tReturn " + value);
		}

		byte[] GetData(int size)
		{
			byte[] data = new byte[size];

			int read = 0;
			while (read < size)
				read += Stream.Read(data, read, size - read);

			return data;
		}
	}

	class Program
	{
		public static string Root;
		public static List<Connection> Connections;

		static void AcceptClient(TcpClient client)
		{
			Connection connection = new Connection(Root, client);
			Thread thread = new Thread(connection.Run);
			connection.DebugPrint("Connection Established");
			Connections.Add(connection);
			thread.Start();
		}

		static void TimeoutThread()
		{
			while (true) {
				Thread.Sleep(TimeSpan.FromSeconds(30));
				foreach (Connection connection in Connections) {
					TimeSpan diff = DateTime.Now - connection.LastPing;
					if (diff > TimeSpan.FromSeconds(120)) {
						connection.DebugPrint("Ping Timeout (" + diff.TotalSeconds.ToString() + " seconds)");
						connection.Close();
					}
				}
			}
		}

		static void Main(string[] args)
		{
			int port = 1137;

			if (args.Length > 0)
				Root = args[0];
			else
				Root = Environment.CurrentDirectory;

			if (args.Length > 1)
				port = int.Parse(args[1]);

			Connections = new List<Connection>();

			Thread timeout = new Thread(TimeoutThread);
			timeout.Start();

			TcpListener listener = new TcpListener(port);
			listener.Start();
			Console.WriteLine("RiiFS C# Server is now ready for connections on " + listener.LocalEndpoint.ToString());
			while (true)
				AcceptClient(listener.AcceptTcpClient());
		}
	}
}
