using System;
using System.Collections.Generic;
using System.IO;
using ConsoleHaxx.Common;
using System.Collections;

namespace ConsoleHaxx.Harmonix
{
	public class CryptedDtbStream : Stream
	{
		private Stream Stream;
		private int Key;
		private long Offset;
		private int key;

		public CryptedDtbStream(EndianReader reader)
		{
			Stream = reader.Base;
			Offset = 0;
			Stream.Position = 0;
			Key = reader.ReadInt32();
			key = Key;
		}

		public void WriteKey()
		{
			WriteKey(0x13370BAD); // Project Codename 0x13370BAD
		}

		public void WriteKey(int key)
		{
			Stream.Position = 0;
			Key = key;
			this.key = key;

			Stream.Write(LittleEndianConverter.GetBytes(key), 0, 4);
			Offset = 0;
		}

		internal static int XorKey(int key)
		{
			int val1 = (key / 0x1F31D) * 0xB14;
			int val2 = (key - ((key / 0x1F31D) * 0x1F31D)) * 0x41A7;
			val2 = val2 - val1;
			if (val2 <= 0)
				val2 += 0x7FFFFFFF;
			return val2;
		}

		public override bool CanRead
		{
			get { return Stream.CanRead; }
		}

		public override bool CanSeek
		{
			get { return Stream.CanSeek; }
		}

		public override bool CanWrite
		{
			get { return Stream.CanWrite; }
		}

		public override void Flush()
		{
			Stream.Flush();
		}

		public override long Length
		{
			get { return Stream.Length - 4; }
		}

		public override long Position
		{
			get
			{
				return Offset;
			}
			set
			{
				if (value < Offset) {
					key = Key;
					for (long i = 0; i < value; i++)
						key = XorKey(key);
				} else {
					for (long i = Offset; i < value; i++)
						key = XorKey(key);
				}

				Offset = value;

				Stream.Position = Offset + 4;
			}
		}

		public override int Read(byte[] buffer, int offset, int count)
		{
			byte[] data = new byte[count];
			int read = Stream.Read(data, 0, count);

			for (int i = 0; i < read; i++) {
				key = XorKey(key);
				buffer[i + offset] = (byte)(data[i] ^ key);
			}

			Offset += read;

			return read;
		}

		public override long Seek(long offset, SeekOrigin origin)
		{
			switch (origin) {
				case SeekOrigin.Begin:
					Position = offset;
					break;
				case SeekOrigin.Current:
					Position += offset;
					break;
				case SeekOrigin.End:
					Position = Length + offset;
					break;
				default:
					break;
			}

			return Offset;
		}

		public override void SetLength(long value)
		{
			Stream.SetLength(value);
		}

		public override void Write(byte[] buffer, int offset, int count)
		{
			byte[] data = new byte[count];

			for (int i = 0; i < count; i++) {
				key = XorKey(key);
				data[i] = (byte)(buffer[i + offset] ^ key);
			}

			Stream.Write(data, 0, count);

			Offset += count;
		}

		private class OldCryptTable
		{
			public int idx1;
			public int idx2;
			public uint[] table;

			public OldCryptTable(uint key)
			{
				table = new uint[0x100];

				uint val1 = key;

				for (int i = 0; i < 0x100; i++) {
					uint val2 = (val1 * 0x41C64E6D) + 0x3039;
					val1 = (val2 * 0x41C64E6D) + 0x3039;
					table[i] = (val1 & 0x7FFF0000) | (val2 >> 16);
				}

				idx1 = 0x00;
				idx2 = 0x67;
			}
		}

		public static void DecryptOld(Stream destination, EndianReader reader, int size)
		{
			uint key = reader.ReadUInt32();
			OldCryptTable table = new OldCryptTable(key);

			for (int i = 4; i < size; i++) {
				table.table[table.idx1] ^= table.table[table.idx2];
				destination.WriteByte((byte)(reader.ReadByte() ^ table.table[table.idx1]));

				table.idx1 = ((table.idx1 + 1) >= 0xF9) ? 0x00 : (table.idx1 + 1);
				table.idx2 = ((table.idx2 + 1) >= 0xF9) ? 0x00 : (table.idx2 + 1);
			}
		}
	}
}
