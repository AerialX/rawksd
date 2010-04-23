#region Using Shortcuts
using s8 = System.SByte;
using u8 = System.Byte;
using s16 = System.Int16;
using u16 = System.UInt16;
using s32 = System.Int32;
using u32 = System.UInt32;
using s64 = System.Int64;
using u64 = System.UInt64;
#endregion

using System;
using System.Collections.Generic;
using ConsoleHaxx.Common;
using System.IO;

namespace ConsoleHaxx.PS3
{
	public class SceufArchive
	{
		public u64 Version;
		public u64 ImageVersion;

		public List<Node> Nodes;

		public class Node
		{
			public Node()
			{
				Hash = new u8[0x14];
			}

			public u8[] Hash { get; protected set; }
			public u64 Attribute { get; set; }

			public Stream Data { get; set; }
		}

		public SceufArchive(SceufFile sceuf) : this()
		{
			Version = sceuf.Version;
			ImageVersion = sceuf.ImageVersion;

			foreach (SceufFile.Node n in sceuf.Nodes) {
				Node node = new Node();
				n.Hash.CopyTo(node.Hash, 0);
				node.Attribute = n.Attribute;

				node.Data = new Substream(sceuf.Data, (s64)n.Offset - (s64)sceuf.DataOffset, (s64)n.Size);

				Nodes.Add(node);
			}
		}

		public SceufArchive()
		{
			Nodes = new List<Node>();
		}
	}

	public class SceufFile
	{
		public const u64 Magic = 0x5343455546000000; // "SCEUF"

		public u64 Version { get; set; }
		public u64 ImageVersion { get; set; }
		public u64 DataOffset { get; set; }
		public u64 DataSize { get; set; }
		public u64 NodeCount { get; set; }

		public List<Node> Nodes;

		public u8[] Hash { get; protected set; }

		public Stream Data { get; set; }

		public class Node
		{
			public Node()
			{
				Hash = new byte[0x14];
			}

			public void ReadLocation(EndianReader reader)
			{
				Attribute = reader.ReadUInt64();
				Offset = reader.ReadUInt64();
				Size = reader.ReadUInt64();
				LocationZero = reader.ReadUInt64();
			}

			public void ReadHash(EndianReader reader)
			{
				Index = reader.ReadUInt64();
				reader.Read(Hash, 0, 0x14);
				HashZero = reader.ReadUInt32();
			}

			public u64 Attribute { get; set; }
			public u64 Offset { get; set; }
			public u64 Size { get; set; }
			public u64 LocationZero { get; set; }
			public u64 Index { get; set; }
			public u8[] Hash { get; protected set; }
			public u32 HashZero { get; set; }
		}

		protected SceufFile()
		{
			Nodes = new List<Node>();
			Hash = new u8[0x14];
		}

		public SceufFile(Stream stream) : this(new EndianReader(stream, Endianness.BigEndian)) { }

		public SceufFile(EndianReader reader) : this()
		{
			if (reader.ReadUInt64(Endianness.BigEndian) != Magic)
				throw new NotSupportedException();

			Version = reader.ReadUInt64();
			// if (Version != 1)
			//	throw new NotSupportedException();
			ImageVersion = reader.ReadUInt64();
			NodeCount = reader.ReadUInt64();
			DataOffset = reader.ReadUInt64();
			DataSize = reader.ReadUInt64();

			for (u64 i = 0; i < NodeCount; i++) {
				Node node = new Node();
				node.ReadLocation(reader);
				Nodes.Add(node);
			}
			foreach (Node node in Nodes) {
				node.ReadHash(reader);
			}

			reader.Read(Hash, 0, 0x14);

			Data = new Substream(reader.Base, (s64)DataOffset, (s64)DataSize);
		}
	}
}
