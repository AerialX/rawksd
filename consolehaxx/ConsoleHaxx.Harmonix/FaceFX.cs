using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.Harmonix
{
	public class FaceFX
	{
		List<string> Strings1;
		List<int> Ints1;
		List<string> Strings2;
		List<int> Ints2;
		byte[] Bytes1;
		List<int> Ints3;
		List<int> Ints4;
		List<int> Ints5;
		List<string> Strings3;
		List<int> Ints6;
		Stream Data;

		public FaceFX()
		{
			Strings1 = new List<string>();
			Strings2 = new List<string>();
			Strings3 = new List<string>();
			Ints1 = new List<int>();
			Ints2 = new List<int>();
			Ints3 = new List<int>();
			Ints4 = new List<int>();
			Ints5 = new List<int>();
			Ints6 = new List<int>();
		}

		public FaceFX(EndianReader reader) : this()
		{
			if (reader.ReadInt32() != 0x19)
				throw new NotSupportedException();

			for (int i = 0; i < 2; i++)
				Strings1.Add(ReadString(reader));

			for (int i = 0; i < 3; i++)
				Ints1.Add(reader.ReadInt32());

			for (int i = 0; i < 2; i++)
				Strings2.Add(ReadString(reader));

			for (int i = 0; i < 86; i++)
				Ints2.Add(reader.ReadInt32());

			Bytes1 = reader.ReadBytes(5);

			for (int i = 0; i < 7; i++)
				Ints3.Add(reader.ReadInt32());

			if (reader.ReadByte() != 0x00)
				throw new FormatException();
			byte[] magic = reader.ReadBytes(0x04);
			if (magic[0] != 0xAD || magic[1] != 0xDE || magic[2] != 0xAD || magic[3] != 0xDE)
				throw new FormatException();

			for (int i = 0; i < 2; i++)
				Ints4.Add(reader.ReadInt32());

			if (reader.ReadByte() != 0x00)
				throw new FormatException();

			for (int i = 0; i < 2; i++)
				Ints5.Add(reader.ReadInt32());

			int strings = reader.ReadInt32();
			for (int i = 0; i < strings; i++) // ?
				Strings3.Add(ReadString(reader));

			for (int i = 0; i < 2; i++)
				Ints6.Add(reader.ReadInt32());

			Data = new Substream(reader.Base, reader.Position, reader.Base.Length - reader.Position);
		}

		public void Save(EndianReader writer)
		{
			writer.Write((int)0x19);

			foreach (string str in Strings1)
				WriteString(str, writer);

			foreach (int i in Ints1)
				writer.Write(i);

			foreach (string str in Strings2)
				WriteString(str, writer);

			foreach (int i in Ints2)
				writer.Write(i);

			writer.Write(Bytes1);

			foreach (int i in Ints3)
				writer.Write(i);

			writer.Write((byte)0x00);
			writer.Write((byte)0xAD); writer.Write((byte)0xDE);
			writer.Write((byte)0xAD); writer.Write((byte)0xDE);

			foreach (int i in Ints4)
				writer.Write(i);

			writer.Write((byte)0x00);

			foreach (int i in Ints5)
				writer.Write(i);

			writer.Write((int)Strings3.Count);
			foreach (string str in Strings3)
				WriteString(str, writer);

			foreach (int i in Ints6)
				writer.Write(i);

			Data.Position = 0;
			Util.StreamCopy(writer.Base, Data);
		}

		private void WriteString(string str, EndianReader writer)
		{
			writer.Write((int)str.Length);
			writer.Write(Util.Encoding.GetBytes(str));
		}

		private string ReadString(EndianReader reader)
		{
			int len = reader.ReadInt32();
			string ret = string.Empty;
			for (int i = 0; i < len; i++)
				ret += (char)reader.ReadByte();
			return ret;
		}
	}
}
