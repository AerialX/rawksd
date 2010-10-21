using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using ConsoleHaxx.Common;
using Nanook.QueenBee.Parser;

namespace ConsoleHaxx.Neversoft
{
	public class Pak
	{
		public const int FilenameLength = 160;

		public List<Node> Nodes;

		public DirectoryNode Root;

		public Pak()
		{
			Nodes = new List<Node>();
			Root = new DirectoryNode();
		}

		public FileNode FindFile(string path)
		{
			string filename = Path.GetFileName(path);
			FileNode file = Root.Find(filename, SearchOption.AllDirectories, true) as FileNode;
			if (file != null)
				return file;

			uint fullkey = QbKey.Create(path).Crc;
			uint key = QbKey.Create(filename).Crc;
			foreach (Node node in Nodes) {
				string name = null;
				if (node.FilenameCRC == fullkey || node.FilenameKey == fullkey || node.FilenamePakKey == fullkey)
					name = path;
				if (node.FilenameCRC == key || node.FilenameKey == key || node.FilenamePakKey == key)
					name = filename;
				if (name != null)
					return new FileNode(name, node.Size, node.Data);
			}
			return null;
		}

		public FileNode FindFileType(uint type)
		{
			foreach (Node node in Nodes) {
				if (node.FileType == type)
					return new FileNode(node.Filename, node.Size, node.Data);
			}
			return null;
		}

		public Pak(EndianReader reader) : this()
		{
			QbKey end = QbKey.Create("last");
			QbKey end2 = QbKey.Create(".last");
			while (true) {
				Node node = new Node();
				uint headerstart = (uint)reader.Position;
				node.FileType = reader.ReadUInt32();
				if (node.FileType == end || node.FileType == end2)
					break;
				node.Offset = reader.ReadUInt32() + headerstart;
				node.Size = reader.ReadUInt32();
				node.FilenamePakKey = reader.ReadUInt32();
				node.FilenameKey = reader.ReadUInt32();
				node.FilenameCRC = reader.ReadUInt32();
				node.Unknown = reader.ReadUInt32();
				node.Flags = (Flags)reader.ReadUInt32();
				node.Data = new Substream(reader, node.Offset, node.Size);

				if ((node.Flags & Flags.Filename) == Flags.Filename) {
					node.Filename = reader.ReadString(FilenameLength);

					string filename = node.Filename.Replace('\\', '/');
					int slash = filename.LastIndexOf('/');
					DirectoryNode dir = Root;
					if (slash >= 0) {
						string path = filename.Substring(0, slash);
						dir = (DirectoryNode)Root.Navigate(path, true);
						filename = node.Filename.Substring(path.Length + 1);
					}
					if (dir != null)
						dir.AddChild(new FileNode(filename, node.Size, node.Data));
				} else
					node.Filename = string.Empty;

				Nodes.Add(node);
			}
		}

		public class Node
		{
			public string Filename;
			public uint FileType;
			public uint Offset;
			public uint Size;
			public uint FilenamePakKey;
			public uint FilenameKey;
			public uint FilenameCRC;
			public uint Unknown;
			public Flags Flags;

			public Stream Data;
		}

		[Flags()]
		public enum Flags
		{
			Filename = 0x20
		}
	}
}
