using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.Graces
{
	public class FPS4
	{
		public DirectoryNode Root;

		public string Type;

		public FPS4Base Base;

		public FPS4(Stream stream) : this(new FPS4Base(stream)) { }

		public FPS4(FPS4Base fps4)
		{
			Base = fps4;

			Root = new DirectoryNode();

			for (uint i = 0; i < Base.Files - 1; i++) {
				FPS4Base.Node node = Base.Nodes[(int)i];
				FileNode file = new FileNode(node.Filename, Root, (ulong)node.Data.Length, node.Data);
			}

			Type = Util.ReadCString(Base.Type, 0); // There's more to the type I'm sure
		}
	}

	public class FPS4Base
	{
		const uint Magic = 0x46505334;
		const uint HeaderSize = 0x1C;

		public byte[] Type;

		public uint DataOffset;
		public ushort BlockSize;
		public uint Files;
		public uint TypeOffset;

		public List<Node> Nodes;

		public NodeFlags Flags;

		public FPS4Base()
		{
			Nodes = new List<Node>();
		}

		public FPS4Base(Stream stream) : this()
		{
			EndianReader reader = new EndianReader(stream, Endianness.BigEndian);

			if (reader.ReadUInt32() != Magic)
				throw new FormatException();

			Files = reader.ReadUInt32();

			if (reader.ReadUInt32() != HeaderSize)
				throw new FormatException();

			DataOffset = reader.ReadUInt32();
			BlockSize = reader.ReadUInt16();
			Flags = (NodeFlags)reader.ReadUInt16();

			reader.ReadInt32(); // padding

			TypeOffset = reader.ReadUInt32();

			for (uint i = 0; i < Files; i++) {
				uint offset = (Flags & NodeFlags.Offset) != 0 ? reader.ReadUInt32() : 0;
				uint sectionsize = (Flags & NodeFlags.SectionSize) != 0 ? reader.ReadUInt32() : 0;
				uint filesize = (Flags & NodeFlags.Filesize) != 0 ? reader.ReadUInt32() : 0;
				string filename = (Flags & NodeFlags.Filename) != 0 ? reader.ReadString(0x20) : i.ToString();
				uint unknown1 = (Flags & NodeFlags.Unknown1) != 0 ? reader.ReadUInt32() : 0;
				uint unknown2 = (Flags & NodeFlags.Unknown2) != 0 ? reader.ReadUInt32() : 0;

				Node node = new Node(offset, sectionsize, filesize, filename, unknown1, unknown2, new Substream(stream, offset, filesize > 0 ? filesize : sectionsize));
				Nodes.Add(node);
			}

			stream.Position = TypeOffset;
			Type = reader.ReadBytes((int)(DataOffset - TypeOffset)); // There's more to the type I'm sure
		}

		public void Reorder()
		{
			uint offset = DataOffset;
			Node node;
			for (uint i = 0; i < Files - 1; i++) {
				node = Nodes[(int)i];
				node.Offset = offset;
				node.Filesize = (uint)node.Data.Length;
				node.SectionSize = node.Filesize;

				offset = (uint)Util.RoundUp(offset + node.Filesize, 0x20);
			}

			node = Nodes[Nodes.Count - 1];
			node.Offset = offset;
			node.SectionSize = 0;
			node.Filesize = 0;
		}

		public void Save(Stream stream)
		{
			EndianReader writer = new EndianReader(stream, Endianness.BigEndian);

			writer.Write(Magic);
			writer.Write(Files);
			writer.Write(HeaderSize);

			writer.Write(DataOffset);
			writer.Write(BlockSize);
			writer.Write((ushort)Flags);

			writer.Write((int)0);

			writer.Write(TypeOffset);

			for (uint i = 0; i < Files; i++) {
				Node node = Nodes[(int)i];

				if ((Flags & NodeFlags.Offset) != 0)
					writer.Write(node.Offset);
				if ((Flags & NodeFlags.SectionSize) != 0)
					writer.Write(node.SectionSize);
				if ((Flags & NodeFlags.Filesize) != 0)
					writer.Write(node.Filesize);
				if ((Flags & NodeFlags.Filename) != 0)
					writer.Write(node.Filename, 0x20);
				if ((Flags & NodeFlags.Unknown1) != 0)
					writer.Write(node.Unknown1);
				if ((Flags & NodeFlags.Unknown2) != 0)
					writer.Write(node.Unknown2);
			}

			writer.PadTo(TypeOffset);

			writer.Write(Type);

			for (uint i = 0; i < Files; i++) {
				Node node = Nodes[(int)i];
				writer.PadTo(node.Offset);
				if (node.Data != null) {
					node.Data.Position = 0;
					Util.StreamCopy(stream, node.Data);
				}
			}
		}

		public class Node
		{
			public uint Offset;
			public uint SectionSize;
			public uint Filesize;
			public string Filename;
			public uint Unknown1;
			public uint Unknown2;
			public Stream Data;

			public Node(uint offset, uint sectionsize, uint filesize, string filename, uint unknown1, uint unknown2, Stream data)
			{
				Offset = offset;
				SectionSize = sectionsize;
				Filesize = filesize;
				Filename = filename;
				Unknown1 = unknown1;
				Unknown2 = unknown2;

				Data = data;
			}
		}

		[Flags]
		public enum NodeFlags : ushort
		{
			Offset		= 0x001,
			SectionSize	= 0x002,
			Filesize	= 0x004,
			Filename	= 0x008,
			Unknown1	= 0x040,
			Unknown2	= 0x100
		}
	}
}
