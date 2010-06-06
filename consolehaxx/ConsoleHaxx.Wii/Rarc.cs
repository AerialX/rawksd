using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.Wii
{
	public class Rarc
	{
		public const uint Magic = 0x52415243;

		public DirectoryNode Root;

		public Rarc()
		{
			Root = new DirectoryNode();
		}

		public Rarc(Stream stream) : this()
		{
			EndianReader reader = new EndianReader(stream, Endianness.BigEndian);

			uint magic = reader.ReadUInt32();
			if (magic == Yaz0.Magic) {
				stream.Position = 0;
				stream = Yaz0.Create(stream);
				reader = new EndianReader(stream, Endianness.BigEndian);
				magic = reader.ReadUInt32();
			}

			if (magic != Magic)
				throw new FormatException();

			uint size = reader.ReadUInt32();
			reader.ReadUInt32(); // Unknown
			uint dataoffset = reader.ReadUInt32() + 0x20;
			for (int i = 0; i < 4; i++) // Unknown
				reader.ReadUInt32();
			uint nodecount = reader.ReadUInt32();
			for (int i = 0; i < 2; i++) // Unknown
				reader.ReadUInt32();
			uint entryoffset = reader.ReadUInt32() + 0x20;
			reader.ReadUInt32(); // Unknown
			uint stringoffset = reader.ReadUInt32() + 0x20;
			for (int i = 0; i < 2; i++) // Unknown
				reader.ReadUInt32();

			List<NodeEntry> nodes = new List<NodeEntry>();
			for (int i = 0; i < nodecount; i++)
				nodes.Add(NodeEntry.Create(reader));

			reader.Position = stringoffset;
			Stream strings = new MemoryStream();
			Util.StreamCopy(strings, reader, dataoffset - stringoffset);
			EndianReader stringreader = new EndianReader(strings, Endianness.BigEndian);

			Root.Name = GetString(stringreader, nodes[0].NameOffset);

			ParseNodes(reader, stringreader, entryoffset, dataoffset, nodes[0], Root, nodes);
		}

		private void ParseNodes(EndianReader reader, EndianReader strings, uint entryoffset, uint dataoffset, NodeEntry root, DirectoryNode dir, List<NodeEntry> nodes)
		{
			reader.Position = entryoffset + root.FirstFileEntry * 20;
			List<FileEntry> files = new List<FileEntry>();
			for (ushort i = 0; i < root.FileCount; i++) {
				files.Add(FileEntry.Create(reader));
			}
			foreach (FileEntry entry in files) {
				string name = GetString(strings, entry.NameOffset);
				if (entry.ID == 0xFFFF) {
					if (name == "." || name == "..")
						continue;
					DirectoryNode subdir = new DirectoryNode(name, dir);
					ParseNodes(reader, strings, entryoffset, dataoffset, nodes[(int)entry.DataOffset], subdir, nodes);
				} else {
					new FileNode(name, dir, entry.Size, new Substream(reader, dataoffset + entry.DataOffset, entry.Size));
				}
			}
		}

		private string GetString(EndianReader strings, uint offset)
		{
			strings.Position = offset;
			return strings.ReadString();
		}

		class NodeEntry
		{
			public uint ID;
			public uint NameOffset;
			public ushort Unknown;
			public ushort FileCount;
			public uint FirstFileEntry;

			public static NodeEntry Create(EndianReader reader)
			{
				NodeEntry node = new NodeEntry();
				node.ID = reader.ReadUInt32();
				node.NameOffset = reader.ReadUInt32();
				node.Unknown = reader.ReadUInt16();
				node.FileCount = reader.ReadUInt16();
				node.FirstFileEntry = reader.ReadUInt32();
				return node;
			}
		}

		class FileEntry
		{
			public ushort ID;
			public ushort Unknown1;
			public ushort Unknown2;
			public ushort NameOffset;
			public uint DataOffset;
			public uint Size;
			public uint Zero;

			public static FileEntry Create(EndianReader reader)
			{
				FileEntry entry = new FileEntry();
				entry.ID = reader.ReadUInt16();
				entry.Unknown1 = reader.ReadUInt16();
				entry.Unknown2 = reader.ReadUInt16();
				entry.NameOffset = reader.ReadUInt16();
				entry.DataOffset = reader.ReadUInt32();
				entry.Size = reader.ReadUInt32();
				entry.Zero = reader.ReadUInt32();
				return entry;
			}
		}
	}

	public class RarcFile
	{
		public const int Magic = 0x52415243;

		

		public class Node
		{
			public uint Type;
			public ushort Unknown;
		}
	}
}
