using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.Graces
{
	public class UTF
	{
		public const uint UtfMagic = 0x40555446;

		public List<Value> ConstantEntries;
		public List<Value> ZeroEntries;
		public List<Entry> Columns;

		public List<Row> Rows;

		public UTF(Stream data)
		{
			Columns = new List<Entry>();
			ConstantEntries = new List<Value>();
			ZeroEntries = new List<Value>();
			Rows = new List<Row>();

			EndianReader reader = new EndianReader(data, Endianness.BigEndian);

			long baseoffset = data.Position;

			if (reader.ReadUInt32() != UtfMagic)
				throw new FormatException();

			uint tablesize = reader.ReadUInt32();
			long rowsoffset = baseoffset + reader.ReadUInt32();
			long nameoffset = baseoffset + reader.ReadUInt32();
			long dataoffset = baseoffset + reader.ReadUInt32();
			uint tablename = reader.ReadUInt32();

			ushort columns = reader.ReadUInt16();
			ushort rowsize = reader.ReadUInt16();
			uint rows = reader.ReadUInt32();

			for (uint i = 0; i < columns; i++) {
				Entry column = new Entry(reader, nameoffset);
				if (column.StorageType == Storage.Constant)
					ConstantEntries.Add(CreateValue(reader, column, nameoffset, dataoffset));
				else if (column.StorageType == Storage.Zero) {
					byte[] zeroes = new byte[column.Size];
					MemoryStream memorystream = new MemoryStream(zeroes);
					EndianReader memoryreader = new EndianReader(memorystream, Endianness.BigEndian);
					ZeroEntries.Add(CreateValue(memoryreader, column, 0, 0));
				} else if (column.StorageType == Storage.Value)
					Columns.Add(column);
			}

			reader.Position = rowsoffset + 8;

			for (uint i = 0; i < rows; i++) {
				Row row = new Row(rowsize);
				foreach (Entry column in Columns) {
					Value value = CreateValue(reader, column, nameoffset, dataoffset);
					row.Values.Add(value);
				}

				Rows.Add(row);
			}
		}

		public class Row
		{
			public Row(uint size)
			{
				Values = new List<Value>();
				Size = size;
			}

			public uint Size;
			public List<Value> Values;

			public Value FindValue(string name)
			{
				return Values.FirstOrDefault(v => v.Type.Name == name);
			}
		}

		public enum Data : byte
		{
			Byte = 0x00,
			Byte2 = 0x01,
			Short = 0x02,
			Short2 = 0x03,
			Int = 0x04,
			Long = 0x06,
			Float = 0x08,
			String = 0x0A,
			Data = 0x0B
		}

		public enum Storage : byte
		{
			Zero = 0x01,
			Constant = 0x03,
			Value = 0x05
		}

		public class Entry
		{
			public Entry(EndianReader reader, long nameoffset)
			{
				byte type = reader.ReadByte();
				StorageType = (Storage)(type >> 4);
				DataType = (Data)(type & 0x0F);
				NametableOffset = nameoffset;
				Reader = reader;

				NameOffset = reader.ReadUInt32();
			}

			public int Size
			{
				get
				{
					switch (DataType) {
						case Data.Byte:
						case Data.Byte2:
							return 1;
						case Data.Short:
						case Data.Short2:
							return 2;
						case Data.Int:
						case Data.Float:
						case Data.String:
							return 4;
						case Data.Long:
						case Data.Data:
							return 8;
						default:
							return 0;
					}
				}
			}

			public Storage StorageType;
			public Data DataType;
			public uint NameOffset;

			private long NametableOffset;
			private EndianReader Reader;

			public string Name { get {
				long oldposition = Reader.Position;
				Reader.Position = NametableOffset + 8 + NameOffset;
				string ret = Reader.ReadString();
				Reader.Position = oldposition;
				return ret;
			} }

			public override string ToString()
			{
				return Name;
			}
		}

		public static Value CreateValue(EndianReader reader, Entry type, long nameoffset, long dataoffset)
		{
			switch (type.DataType) {
				case Data.Byte:
				case Data.Byte2:
					return new ByteValue(reader, type);
				case Data.Short:
				case Data.Short2:
					return new ShortValue(reader, type);
				case Data.Int:
					return new IntValue(reader, type);
				case Data.Long:
					return new LongValue(reader, type);
				case Data.Float:
					return new FloatValue(reader, type);
				case Data.String:
					return new StringValue(reader, type, nameoffset);
				case Data.Data:
					return new DataValue(reader, type, dataoffset);
			}

			return null;
		}

		public class Value
		{
			public Value(EndianReader reader, Entry type)
			{
				Type = type;
				Data = reader.ReadBytes(Type.Size);
			}

			public Entry Type;
			public byte[] Data;
		}

		public class ByteValue : Value
		{
			public ByteValue(EndianReader reader, Entry type) : base(reader, type) { }

			public byte Value { get { return Data[0]; } }

			public override string ToString()
			{
				return Util.ToString(Value);
			}
		}

		public class IntValue : Value
		{
			public IntValue(EndianReader reader, Entry type) : base(reader, type) { }

			public uint Value { get { return BigEndianConverter.ToUInt32(Data); } }

			public override string ToString()
			{
				return Util.ToString(Value);
			}
		}

		public class LongValue : Value
		{
			public LongValue(EndianReader reader, Entry type) : base(reader, type) { }

			public ulong Value { get { return BigEndianConverter.ToUInt64(Data); } }

			public override string ToString()
			{
				return Util.ToString(Value);
			}
		}

		public class ShortValue : Value
		{
			public ShortValue(EndianReader reader, Entry type) : base(reader, type) { }

			public ushort Value { get { return BigEndianConverter.ToUInt16(Data); } }

			public override string ToString()
			{
				return Util.ToString(Value);
			}
		}

		public class FloatValue : Value
		{
			public FloatValue(EndianReader reader, Entry type) : base(reader, type) { }

			public float Value { get { return BigEndianConverter.ToFloat32(Data); } }

			public override string ToString()
			{
				return Value.ToString();
			}
		}

		public class StringValue : Value
		{
			private long NameOffset;
			private EndianReader Reader;

			public StringValue(EndianReader reader, Entry type, long nameoffset) : base(reader, type) { NameOffset = nameoffset; Reader = reader; }

			public uint Offset { get { return BigEndianConverter.ToUInt32(Data); } }

			public string Value { get {
				long oldposition = Reader.Position; 
				Reader.Position = NameOffset + 8 + Offset;
				string ret = Reader.ReadString();
				Reader.Position = oldposition;
				return ret;
			} }

			public override string ToString()
			{
				return Value;
			}
		}

		public class DataValue : Value
		{
			private long DataOffset;
			private EndianReader Reader;

			public DataValue(EndianReader reader, Entry type, long dataoffset) : base(reader, type) { DataOffset = dataoffset; Reader = reader; }

			public uint Offset { get { return BigEndianConverter.ToUInt32(Data); } }

			public uint Size { get { return BigEndianConverter.ToUInt32(Data.Skip(4).ToArray()); } }

			public byte[] Value { get {
				long oldposition = Reader.Position; 
				Reader.Position = DataOffset + Offset;
				byte[] ret = Reader.ReadBytes((int)Size);
				Reader.Position = oldposition;
				return ret;
			} }

			public override string ToString()
			{
				return "[" + Util.ToString(Size) + "]";
			}
		}
	}
}
