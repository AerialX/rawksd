using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Net.Sockets;
using ConsoleHaxx.Common;
using System.IO;
using System.Threading;
using System.Globalization;

namespace ConsoleHaxx.RiiFS
{
	enum Action : int
	{
		Send = 0x01,
		Receive = 0x02
	}

	enum Option : int
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

	enum Command : int
	{
		Handshake = 0x00,
		Goodbye = 0x01,
		Log = 0x02,

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

	[Flags]
	enum FileModes : int
	{
		O_RDONLY = 0,
		O_WRONLY = 1,
		O_RDWR = 2,
		O_APPEND = 8,
		O_CREAT = 0x200,
		O_TRUNC = 0x400,
		O_WDECR = 0x4000
	}

	[Flags]
	enum StatModes : int
	{
		S_IFDIR = 0x4000, /* directory */
		S_IFREG = 0x8000, /* regular */
	}

	struct Stat
	{
		public static List<string> IDs = new List<string>();

		public string Name;
		public int Device;
		public ulong Size;
		public int Mode;
		public ulong Identifier;

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

		public void Write(EndianReader writer)
		{
			writer.Write(Identifier);
			writer.Write(Size);
			writer.Write(Device);
			writer.Write(Mode);
		}
	}

	public class Connection
	{
		const string FileIdPath = "/mnt/identifier/";
		const int MAXPATHLEN = 0x400;
		const int DIRNEXT_CACHE_SIZE = 0x1000;

		public DateTime LastPing { get; protected set; }
		public Server Server { get; protected set; }
		public TcpClient Client { get; protected set; }

		public string Root { get { return Server.Root; } }
		public bool ReadOnly { get { return Server.ReadOnly; } }

		private Thread ServerThread;

		private Dictionary<Option, byte[]> Options;
		private Dictionary<int, Stream> OpenFiles;
		private Dictionary<int, Pair<List<Stat>, int>> OpenDirs;
		private int OpenFileFD;

		private NetworkStream Stream;
		private EndianReader Writer;

		private string Name;

		public Connection(Server server, TcpClient client)
		{
			Server = server;
			Client = client;

			Client = client;
			Stream = Client.GetStream();
			Writer = new EndianReader(Stream, Endianness.BigEndian);

			LastPing = DateTime.Now;

			Name = client.Client.RemoteEndPoint.ToString();

			Options = new Dictionary<Option, byte[]>();
			OpenFiles = new Dictionary<int, Stream>();
			OpenDirs = new Dictionary<int, Pair<List<Stat>, int>>();
			OpenFileFD = 1;
		}

		~Connection()
		{
			Close();
		}

		public void DebugPrint(string text)
		{
			Console.Write("[" + DateTime.Now.ToString() + "]");
			Console.Write(" - " + Name + " - ");
			Console.WriteLine(text);
		}

		public void StartAsync()
		{
			ServerThread = new Thread(Start);
			ServerThread.Start();
		}

		public void Start()
		{
			Server.AddConnection(this);
			while (Client.Connected) {
				try {
					Poll();
				} catch (Exception ex) {
					string indent = string.Empty;
					for (; ex != null; ex = ex.InnerException, indent += '\t') {
						DebugPrint(indent + ex.GetType().FullName + ": " + ex.Message);
						foreach (string line in ex.StackTrace.Split('\n'))
							DebugPrint(indent + line);
					}
				}
			}
			Close();
		}

		public void Close()
		{
			Server.RemoveConnection(this);

			if (Client.Connected) {
				try { Client.Client.Close(); } catch { }
				try { Client.Close(); } catch { }
			}
			foreach (var file in OpenFiles) {
				try { file.Value.Close(); } catch { }
			}
			OpenFiles.Clear();

			DebugPrint("Disconnected.");
		}

		protected void Return(int value)
		{
			Writer.Write(value);

			DebugPrint("\tReturn " + value);
		}

		protected byte[] GetData(int size)
		{
			byte[] data = new byte[size];

			int read = 0;
			while (read < size)
				read += Stream.Read(data, read, size - read);

			return data;
		}

		protected string GetPath()
		{
			return GetPath(Options[Option.Path]);
		}

		protected string GetPath(byte[] data)
		{
			string path = Util.Encoding.GetString(data);
			if (path.Contains(".."))
				throw new UnauthorizedAccessException();

			if (path.StartsWith(FileIdPath)) {
				ulong id = ulong.Parse(path.Substring(FileIdPath.Length), NumberStyles.HexNumber);
				if (Stat.IDs.Count > (int)id)
					return Stat.IDs[(int)id];
			} else if (path.StartsWith("/"))
				path = path.Substring(1);
			if (path.EndsWith("/"))
				path = path.Substring(0, path.Length - 1);
			return Path.Combine(Root, path);
		}

		protected int GetFD()
		{
			return BigEndianConverter.ToInt32(Options[Option.File]);
		}

		protected void Ping()
		{
			LastPing = DateTime.Now;
		}

		private void ReceiveOption(Option option)
		{
			int length = BigEndianConverter.ToInt32(GetData(4));
			byte[] data;
			if (length > 0) {
				data = GetData(length);
			} else
				data = new byte[0];

			Options[option] = data;

			if (option == Option.Ping)
				DebugPrint("Ping()");
		}

		protected void ActionHandshake()
		{
			string clientversion = Util.Encoding.GetString(Options[Option.Handshake]);
			DebugPrint("Handshake: Client Version \"" + clientversion + "\"");

			switch (clientversion) {
				case "1.02":
					Return(3);
					break;
				case "1.03":
					Return(4);
					break;
				default:
					Return(-1);
					break;
			}
		}

		protected void ActionLog()
		{
			string message = Util.Encoding.GetString(Options[Option.Data]);
			DebugPrint("Log: " + message);
			Return(0);
		}

		protected void ActionGoodbye()
		{
			DebugPrint("Goodbye");
			Return(1);
			Close();
		}

		protected void ActionOpen()
		{
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
				if (ReadOnly && (fmode == FileMode.Append || fmode == FileMode.Create || fmode == FileMode.Truncate || faccess == FileAccess.Write || faccess == FileAccess.ReadWrite)) {
					Return(-1);
					return;
				}
				Stream fstream = new FileStream(path, fmode, faccess, FileShare.ReadWrite | FileShare.Delete);
				fd = OpenFileFD++;

				OpenFiles.Add(fd, fstream);
			} catch { }

			Return(fd);
		}

		protected void ActionClose()
		{
			int fd = GetFD();
			DebugPrint("File_Close(" + fd + ");");
			if (!OpenFiles.ContainsKey(fd))
				Return(0);
			else {
				OpenFiles[fd].Close();
				OpenFiles.Remove(fd);
				Return(1);
			}
		}

		protected void ActionRead()
		{
			int fd = GetFD();
			int length = BigEndianConverter.ToInt32(Options[Option.Length]);

			DebugPrint("File_Read(" + fd + ", " + length + ");");

			if (!OpenFiles.ContainsKey(fd))
				Return(0);
			else {
				int ret = (int)Util.StreamCopy(Stream, OpenFiles[fd], length);
				Writer.Pad(length - ret);
				Return(ret);
			}
		}

		protected void ActionWrite()
		{
			int fd = GetFD();
			DebugPrint("File_Write(" + fd + ", " + Options[Option.Data].Length + ");");
			if (ReadOnly) {
				Return(-1);
				return;
			}
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
		}

		protected void ActionSeek()
		{
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
		}

		protected void ActionTell()
		{
			int fd = GetFD();
			DebugPrint("File_Tell(" + fd + ");");
			if (!OpenFiles.ContainsKey(fd))
				Return(-1);
			else {
				Return((int)OpenFiles[fd].Position);
			}
		}

		protected void ActionSync()
		{
			int fd = GetFD();
			DebugPrint("File_Sync(" + fd + ");");
			if (!OpenFiles.ContainsKey(fd))
				Return(0);
			else {
				OpenFiles[fd].Flush();
				Return(1);
			}
		}

		protected void ActionStat()
		{
			string path = Path.Combine(Root, GetPath());

			DebugPrint("File_Stat(\"" + path + "\");");

			FileInfo file = new FileInfo(path);
			DirectoryInfo dir = new DirectoryInfo(path);
			if (file.Exists) {
				new Stat(file).Write(Writer);
				Return(0);
			} else if (dir.Exists) {
				new Stat(dir).Write(Writer);
				Return(0);
			} else {
				new Stat().Write(Writer);
				Return(-1);
			}
		}

		protected void ActionDelete()
		{
			string path = Path.Combine(Root, GetPath());
			DebugPrint("File_Delete(\"" + path + "\");");
			if (ReadOnly) {
				Return(-1);
				return;
			}
			try {
				if (File.Exists(path)) {
					File.Delete(path);
					Return(1);
				} else if (Directory.Exists(path)) {
					Directory.Delete(path, true);
					Return(1);
				} else
					Return(0);
			} catch { Return(0); }
		}

		protected void ActionRename()
		{
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
		}

		protected void ActionCreateFile()
		{
			string path = Path.Combine(Root, GetPath());
			DebugPrint("File_Create(\"" + path + "\");");
			if (ReadOnly) {
				Return(-1);
				return;
			}
			try {
				if (!File.Exists(path))
					File.Create(path).Close();
				Return(1);
			} catch { Return(0); }
		}

		protected void ActionCreateDir()
		{
			string path = Path.Combine(Root, GetPath());
			DebugPrint("File_CreateDir(\"" + path + "\");");
			if (ReadOnly) {
				Return(-1);
				return;
			}
			try {
				if (!Directory.Exists(path))
					Directory.CreateDirectory(path);
				Return(1);
			} catch { Return(0); }
		}

		protected void ActionDirOpen()
		{
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
		}

		protected void ActionDirClose()
		{
			int fd = GetFD();
			DebugPrint("File_CloseDir(" + fd + ");");
			if (!OpenDirs.ContainsKey(fd))
				Return(-1);
			else {
				OpenDirs.Remove(fd);
				Return(1);
			}
		}

		protected void ActionDirNextPath()
		{
			int fd = GetFD();
			DebugPrint("File_NextDir(" + fd + ");");
			if (!OpenDirs.ContainsKey(fd) || OpenDirs[fd].Value >= OpenDirs[fd].Key.Count) {
				Writer.Pad(MAXPATHLEN);
				Return(-1);
			} else {
				string name = OpenDirs[fd].Key[OpenDirs[fd].Value].Name;
				byte[] pathbuf = new byte[MAXPATHLEN];
				Util.Encoding.GetBytes(name).CopyTo(pathbuf, 0);
				Writer.Write(pathbuf); // I blame net_recv...
				//Writer.Write(name);
				//Writer.Pad(MAXPATHLEN - name.Length);
				Return(name.Length);
			}
		}

		protected void ActionDirNextStat()
		{
			int fd = GetFD();
			if (!OpenDirs.ContainsKey(fd)) {
				new Stat().Write(Writer);
				Return(1);
			} else {
				OpenDirs[fd].Key[OpenDirs[fd].Value++].Write(Writer);
				Return(0);
			}
		}

		protected void ActionDirNextCache()
		{
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
		}

		protected void Poll()
		{
			Action action = (Action)BigEndianConverter.ToInt32(GetData(4));

			Ping();

			switch (action) {
				case Action.Send:
					ReceiveOption((Option)BigEndianConverter.ToInt32(GetData(4)));
					break;
				case Action.Receive:
					Command command = (Command)BigEndianConverter.ToInt32(GetData(4));
					switch (command) {
						case Command.Handshake:
							ActionHandshake();
							break;
						case Command.Goodbye:
							ActionGoodbye();
							break;
						case Command.Log:
							ActionLog();
							break;
						case Command.FileOpen:
							ActionOpen();
							break;
						case Command.FileRead:
							ActionRead();
							break;
						case Command.FileWrite:
							ActionWrite();
							break;
						case Command.FileSeek:
							ActionSeek();
							break;
						case Command.FileTell:
							ActionTell();
							break;
						case Command.FileSync:
							ActionSync();
							break;
						case Command.FileClose:
							ActionClose();
							break;
						case Command.FileStat:
							ActionStat();
							break;
						case Command.FileCreate:
							ActionCreateFile();
							break;
						case Command.FileDelete:
							ActionDelete();
							break;
						case Command.FileRename:
							ActionRename();
							break;
						case Command.FileCreateDir:
							ActionCreateDir();
							break;
						case Command.FileOpenDir:
							ActionDirOpen();
							break;
						case Command.FileCloseDir:
							ActionDirClose();
							break;
						case Command.FileNextDirPath:
							ActionDirNextPath();
							break;
						case Command.FileNextDirStat:
							ActionDirNextStat();
							break;
						case Command.FileNextDirCache:
							ActionDirNextCache();
							break;
						default:
							break;
					}
					break;
			}
		}
	}
}
