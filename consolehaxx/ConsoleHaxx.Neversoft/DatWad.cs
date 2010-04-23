using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using ConsoleHaxx.Common;
using Nanook.QueenBee.Parser;

namespace ConsoleHaxx.Neversoft
{
	public class DatWad
	{
		public List<Node> Nodes;
		public Stream Wad;

		public DatWad()
		{
			Nodes = new List<Node>();
		}

		public DatWad(EndianReader dat, Stream wad) : this()
		{
			Nodes.Clear();
			Wad = wad;

			uint num = dat.ReadUInt32(); // Number of nodes
			dat.ReadUInt32(); // Header size
			for (int i = 0; i < num; i++) {
				QbKey item = QbKey.Create(dat.ReadUInt32());
				Node node = new Node() {
					Key = item,
					Offset = dat.ReadUInt32(),
					Size = dat.ReadUInt32(),
					Reserved = dat.ReadBytes(8)
				};
				node.Data = new Substream(Wad, node.Offset, node.Size);
				Nodes.Add(node);
			}

			foreach (Node node in Nodes) {
				Wad.Seek((long)(node.Offset + 0x1A), SeekOrigin.Begin);
				node.Filename = Util.ReadCString(Wad, 30);
			}
		}

		public const uint Alignment = 0x4000;

		public class Node
		{
			public uint Offset;
			public uint Size;
			public QbKey Key;
			public byte[] Reserved;

			public Stream Data;

			public string Filename;
		}
	}
}
