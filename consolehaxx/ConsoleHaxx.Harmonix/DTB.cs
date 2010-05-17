using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.Harmonix
{
	public static class DTB
	{
		public static NodeTree Create(EndianReader stream)
		{
			if (stream.ReadByte() != 1)
				throw new FormatException();

			NodeTree dtb = new NodeTree();
			dtb.Type = 0x10;
			
			dtb.Data = stream.ReadBytes((int)dtb.Size);
			dtb.Parse(stream);

			ushort treefile;
			switch (stream.Endian) {
				case Endianness.BigEndian:
					treefile = BigEndianConverter.ToUInt16(dtb.Data); break;
				case Endianness.LittleEndian:
					treefile = LittleEndianConverter.ToUInt16(dtb.Data); break;
				default:
					throw new NotImplementedException();
			}
			Stack<ushort> treefiles = new Stack<ushort>(); treefiles.Push(1); treefiles.Push(treefile);
			Stack<NodeTree> parents = new Stack<NodeTree>(); parents.Push(null); parents.Push(dtb);
			while (parents.Peek() != null) {
				uint tag = stream.ReadUInt32();
				Node node = NodeTypes[tag].Produce();
				node.Data = stream.ReadBytes((int)node.Size);
				node.Parent = parents.Peek();
				node.Type = tag;
				parents.Peek().Nodes.Add(node);
				node.Parse(stream);

				treefiles.Push((ushort)(treefiles.Pop() - 1));
				while (treefiles.Peek() == 0) {
					treefiles.Pop();
					parents.Pop();
				}

				if (node is NodeTree) {
					ushort children;
					switch (stream.Endian) {
						case Endianness.BigEndian:
							children = BigEndianConverter.ToUInt16(node.Data); break;
						case Endianness.LittleEndian:
							children = LittleEndianConverter.ToUInt16(node.Data); break;
						default:
							throw new NotImplementedException();
					}
					if (children > 0) {
						treefiles.Push(children);
						parents.Push(node as NodeTree);
					}
				}
			}

			return dtb;
		}

		public static void Save(this NodeTree tree, EndianReader stream)
		{
			stream.Write((byte)1); // version
			stream.Write((ushort)tree.Nodes.Count);
			stream.Write((uint)1); // Initial ID / Line Number
			foreach (Node node in tree.Nodes)
				node.Write(stream);
		}

		static DTB()
		{
			NodeTypes = new Dictionary<uint, NodeProducer>();
			NodeTypes.Add(0x00000000, new GenericNodeProducer<NodeInt32>());
			NodeTypes.Add(0x00000001, new GenericNodeProducer<NodeFloat32>());
			NodeTypes.Add(0x00000002, new GenericNodeProducer<NodeFunction>());
			NodeTypes.Add(0x00000005, new GenericNodeProducer<NodeKeyword>());
			NodeTypes.Add(0x00000006, new NodeProducer(4));
			NodeTypes.Add(0x00000007, new GenericNodeProducer<NodeData>());
			NodeTypes.Add(0x00000008, new NodeProducer(4));
			NodeTypes.Add(0x00000009, new NodeProducer(4));
			NodeTypes.Add(0x00000010, new GenericNodeProducer<NodeTree>());
			NodeTypes.Add(0x00000011, new GenericNodeProducer<NodeTree>());
			NodeTypes.Add(0x00000012, new GenericNodeProducer<NodeString>());
			NodeTypes.Add(0x00000013, new GenericNodeProducer<NodeTree>());
			NodeTypes.Add(0x00000020, new GenericNodeProducer<NodeString>());
			NodeTypes.Add(0x00000021, new GenericNodeProducer<NodeFile>());
			NodeTypes.Add(0x00000022, new GenericNodeProducer<NodeFile>());
			NodeTypes.Add(0x00000023, new GenericNodeProducer<NodeString>());

			NodeTypes.Add(0x000000F8, new GenericNodeProducer<NodeData>());
		}

		public static Dictionary<uint, NodeProducer> NodeTypes;

		public class NodeInt32 : Node
		{
			public int Number
			{
				get
				{
					switch (Endian) {
						case Endianness.BigEndian:
							return BigEndianConverter.ToInt32(Data);
						case Endianness.LittleEndian:
							return LittleEndianConverter.ToInt32(Data);
						default:
							throw new NotImplementedException();
					}
					
				}
				set
				{
					switch (Endian) {
						case Endianness.BigEndian:
							Data = BigEndianConverter.GetBytes(value); break;
						case Endianness.LittleEndian:
							Data = LittleEndianConverter.GetBytes(value); break;
						default:
							throw new NotImplementedException();
					}
				}
			}

			public NodeInt32() : base(4) { Type = 0; }
		}
		public class NodeFloat32 : Node
		{
			public float Number
			{
				get
				{
					switch (Endian) {
						case Endianness.BigEndian:
							return BigEndianConverter.ToFloat32(Data);
						case Endianness.LittleEndian:
							return LittleEndianConverter.ToFloat32(Data);
						default:
							throw new NotImplementedException();
					}
				}
				set
				{
					switch (Endian) {
						case Endianness.BigEndian:
							Data = BigEndianConverter.GetBytes(value); break;
						case Endianness.LittleEndian:
							Data = LittleEndianConverter.GetBytes(value); break;
						default:
							throw new NotImplementedException();
					}
				}
			}

			public NodeFloat32() : base(4) { Type = 0x01; }
		}
		public class NodeFunction : NodeString
		{

		}
		public class NodeKeyword : NodeString
		{

		}
		public class NodeData : NodeInt32
		{
			private byte[] _Contents;
			public byte[] Contents
			{
				get
				{
					return _Contents;
				}
				set
				{
					_Contents = value;
					Number = _Contents.Length;
				}
			}

			public NodeData() : base() { }

			public override void Parse(EndianReader stream)
			{
				base.Parse(stream);

				Contents = new byte[Number];
				stream.Read(Contents, 0, Number);
			}

			public override void Write(EndianReader stream)
			{
				base.Write(stream);

				stream.Write(Contents, 0, Contents.Length);
			}
		}
		public class NodeTree : Node
		{
			public List<Node> Nodes;

			public uint ID;

			public static uint NodeID = 0;

			public NodeTree() : base(2)
			{
				Type = 0x10;
				Nodes = new List<Node>();
				Data = new byte[2];
				ID = NodeID++;
			}

			public NodeTree(uint id) : this()
			{
				ID = id;
			}

			public override void Parse(EndianReader stream)
			{
				byte[] data = new byte[4];
				stream.Read(data, 0, 2);
				stream.Read(data, 2, 2);
				switch (stream.Endian) {
					case Endianness.BigEndian:
						ID = BigEndianConverter.ToUInt32(data);
						break;
					case Endianness.LittleEndian:
						ID = LittleEndianConverter.ToUInt32(data);
						break;
					default:
						throw new NotImplementedException();
				}
			}

			public override void Write(EndianReader stream)
			{
				stream.Write(Type);
				stream.Write((ushort)Nodes.Count);
				byte[] data;
				switch (stream.Endian) {
					case Endianness.BigEndian:
						data = BigEndianConverter.GetBytes(ID);
						break;
					case Endianness.LittleEndian:
						data = LittleEndianConverter.GetBytes(ID);
						break;
					default:
						throw new NotImplementedException();
				}
				stream.Write(data, 0, 2);
				stream.Write(data, 2, 2);

				foreach (Node node in Nodes)
					node.Write(stream);
			}

			public NodeTree FindByKeyword(string key)
			{
				foreach (Node node in Nodes) {
					if (!(node is NodeTree))
						continue;

					NodeTree tree = node as NodeTree;
					if (tree.GetKeyword() == key)
						return tree;
				}

				return null;
			}

			public string GetKeyword()
			{
				if (Nodes.Count == 0)
					return null;

				Node first = Nodes[0];
				if (first is NodeKeyword)
					return (first as NodeKeyword).Text;

				return null;
			}

			public T GetValue<T>() where T : class
			{
				return GetValue<T>(1);
			}

			public T GetValue<T>(int index) where T : class
			{
				if (Nodes.Count < index + 1)
					return null;
				return Nodes[index] as T;
			}
		}
		public class NodeString : NodeData
		{
			public string Text
			{
				get
				{
					return Util.Encoding.GetString(Contents);
				}
				set
				{
					if (value == null)
						value = string.Empty;
					Contents = Util.Encoding.GetBytes(value);
				}
			}

			public NodeString() : base() { }
		}
		public class NodeFile : NodeString
		{

		}

		public class NodeProducer
		{
			public uint Size;

			public NodeProducer(uint size)
			{
				Size = size;
			}

			public virtual Node Produce()
			{
				return new Node(Size);
			}
		}

		public class GenericNodeProducer<N> : NodeProducer where N : Node
		{
			public GenericNodeProducer() : base(0) { }

			public override Node Produce()
			{
				// Default constructor
				return typeof(N).GetConstructor(new Type[] { }).Invoke(null) as Node;
			}
		}

		public class Node
		{
			public uint Size;
			public uint Type;
			public byte[] Data;
			public NodeTree Parent;

			protected Endianness Endian;

			public Node(uint size)
			{
				Size = size;
				Endian = Endianness.LittleEndian;
			}

			public virtual void Parse(EndianReader stream) { Endian = stream.Endian; }
			public virtual void Write(EndianReader stream)
			{
				stream.Write(Type);
				stream.Write(Data, 0, (int)Size);
			}
		}
	}
}
