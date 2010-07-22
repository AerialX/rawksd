using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.Wii
{
	public class U8
	{
		public const int Magic = 0x55AA382D;

		public DirectoryNode Root;

		public U8()
		{
			Root = new DirectoryNode();
		}

		public U8(Stream stream) : this(stream, false) { }

		public U8(Stream stream, bool wiidisc) : this()
		{
			EndianReader reader = new EndianReader(stream, Endianness.BigEndian);
			long beginning = 0;
			if (!wiidisc) {
				beginning = reader.Position;

				int tag = reader.ReadInt32();
				if (tag != Magic)
					throw new FormatException();
				if (reader.ReadUInt32() != 0x20)
					throw new FormatException();

				reader.ReadUInt32(); // header_size

				uint dataOffset = reader.ReadUInt32();
				reader.ReadBytes(0x10);
			}

			// Parse nodes
			uint nodebegin = reader.ReadUInt32(); // Type and name offset
			if ((nodebegin >> 24) != 0x01) // Root must be a DirectoryNode
				throw new FormatException();
			reader.ReadUInt32(); // Data Offset
			uint numfiles = reader.ReadUInt32() - 2 + 1; // Size (+1 because we don't want the stack to shit itself)

			DirectoryNode parent = Root;
			Stack<uint> dirfiles = new Stack<uint>();
			dirfiles.Push(numfiles);
			for (int i = 0; i < numfiles; i++) {
				Node node;
				uint read = reader.ReadUInt32();
				byte type = (byte)((read & 0xFF000000) >> 56);
				uint nameOffset = read & 0x00FFFFFF;
				uint fileOffset = reader.ReadUInt32();
				uint filesize = reader.ReadUInt32();

				if (wiidisc)
					fileOffset <<= 2;

				switch (type) {
					case 0x00:
						node = new FileNode(null, filesize, new Substream(reader.Base, beginning + fileOffset, filesize));
						parent.AddChild(node);
						break;
					case 0x01:
						node = new DirectoryNode(null);
						parent.AddChild(node);
						parent = node as DirectoryNode;
						dirfiles.Push(filesize - 2);
						break;
					default:
						node = new Node();
						break;
				}

				node.Name = nameOffset.ToString(); // <-- The most idiotic thing you'll ever see me do.

				while (i == dirfiles.Peek()) {
					dirfiles.Pop();
					parent = parent.Parent;
				}
			}

			NameNodes(stream, Root, reader.Position);
		}

		private static void NameNodes(Stream stream, DirectoryNode list, long strings)
		{
			foreach (Node node in list) {
				uint nameOffset = uint.Parse(node.Name);

				stream.Position = strings + nameOffset;
				node.Name = Util.ReadCString(stream);

				if (node is DirectoryNode)
					NameNodes(stream, (node as DirectoryNode), strings);
			}
		}

		public void Save(Stream stream)
		{
			MemoryStream nodes = new MemoryStream();
			MemoryStream strings = new MemoryStream();
			SaveNodes(nodes, strings);

			EndianReader writer = new EndianReader(stream, Endianness.BigEndian);

			long dataoffset = Util.RoundUp(nodes.Length + strings.Length + 0x20, 0x40);

			writer.Write((uint)Magic); // U8 tag/magic
			writer.Write((uint)0x20); // Offset to root node
			writer.Write((uint)(nodes.Length + strings.Length));
			writer.Write((uint)dataoffset);

			writer.Pad(0x10);

			// Fucking U8 format
			EndianReader reader = new EndianReader(nodes, Endianness.BigEndian);
			reader.Position = 0; // Root node
			while (reader.Position != nodes.Length) {
				if (reader.ReadUInt16() == 0) {
					reader.Position += 2;
					uint offset = reader.ReadUInt32();
					reader.Position -= 4;
					reader.Write((uint)(offset + dataoffset));
					reader.Position -= 6;
				}
				reader.Position -= 2;

				reader.Position += 0xC;
			}

			writer.Write(nodes.ToArray());
			writer.Write(strings.ToArray());
			writer.PadTo(dataoffset);

			SaveFiles(stream);
		}

		private void SaveFiles(Stream stream)
		{
			SaveFiles(Root, stream);
		}

		private void SaveFiles(DirectoryNode dirnode, Stream stream)
		{
			foreach (Node node in dirnode) {
				if (node is DirectoryNode)
					SaveFiles(node as DirectoryNode, stream);
				else {
					FileNode FileNode = node as FileNode;
					FileNode.Data.Position = 0;
					Util.StreamCopy(stream, FileNode.Data, FileNode.Size);

					byte[] padding = new byte[Util.RoundUp(stream.Position, 0x20) - stream.Position];
					stream.Write(padding, 0, padding.Length);
				}
			}
		}

		private void SaveNodes(Stream nodes, Stream strings)
		{
			uint position = 0;
			SaveNodes(Root, nodes, strings, 0, -1, ref position);
		}

		private uint SaveNodes(Node node, Stream nodes, Stream strings, uint dataoffset, int recursion, ref uint position)
		{
			EndianReader nodewriter = new EndianReader(nodes, Endianness.BigEndian);

			position++;
			uint nameoffset = (uint)strings.Position;
			strings.Write(Util.Encoding.GetBytes(node.Name), 0, node.Name.Length);
			strings.WriteByte(0);
			if (node is DirectoryNode) {
				nodewriter.Write((ushort)0x0100); // Type
				nodewriter.Write((ushort)nameoffset); // Name offset
				nodewriter.Write((uint)Math.Max(recursion, 0)); // Data Offset // TODO: Not really recursion but parent number index
				nodewriter.Write((uint)SaveCountNodes(node as DirectoryNode) + position);
				foreach (Node subnode in node as DirectoryNode)
					dataoffset = SaveNodes(subnode, nodes, strings, dataoffset, recursion + 1, ref position);
			} else if (node is FileNode) {
				nodewriter.Write((ushort)0); // Type
				nodewriter.Write((ushort)nameoffset); // Name offset
				nodewriter.Write((uint)dataoffset); // Data Offset
				nodewriter.Write((uint)(node as FileNode).Size); // Size

				dataoffset += (uint)(node as FileNode).Size;
				dataoffset = (uint)Util.RoundUp(dataoffset, 0x20);
			}


			return dataoffset;
		}

		private uint SaveCountNodes(DirectoryNode dirnode, uint dirs = 0)
		{
			foreach (Node node in dirnode) {
				dirs++;
				if (node is DirectoryNode)
					dirs = SaveCountNodes(node as DirectoryNode, dirs);
			}

			return dirs;
		}
	}
}
