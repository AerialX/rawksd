using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.Graces
{
	public class Scs
	{
		public List<string> Entries;

		private static Encoding SJIS;

		public Scs()
		{
			Entries = new List<string>();
		}

		static Scs()
		{
			SJIS = Encoding.GetEncoding("Shift_JIS");
		}

		public static Scs Create(Stream stream)
		{
			EndianReader reader = new EndianReader(stream, Endianness.BigEndian);

			Scs scs = new Scs();

			uint entries = reader.ReadUInt32();

			uint[] offsets = new uint[entries];
			for (uint i = 0; i < entries; i++) {
				offsets[i] = reader.ReadUInt32();
			}

			for (uint i = 0; i < entries; i++) {
				reader.Position = offsets[i];
				scs.Entries.Add(SJIS.GetString(Util.Encoding.GetBytes(reader.ReadString())));
			}

			return scs;
		}

		public void Save(Stream stream)
		{
			EndianReader writer = new EndianReader(stream, Endianness.BigEndian);

			writer.Write((uint)Entries.Count);

			uint offset = 4 * (1 + (uint)Entries.Count);
			for (int i = 0; i < Entries.Count; i++) {
				writer.Write(offset);
				offset += (uint)SJIS.GetByteCount(Entries[i]) + 1;
			}

			for (int i = 0; i < Entries.Count; i++) {
				writer.Write(SJIS.GetBytes(Entries[i]));
				writer.Write((byte)0);
			}
		}
	}
}
