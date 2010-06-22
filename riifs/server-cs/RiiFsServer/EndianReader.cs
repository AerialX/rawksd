using System;
using System.IO;
using System.Text;

namespace ConsoleHaxx.Common
{
	public enum Endianness
	{
		BigEndian,
		LittleEndian,
		Mixed
	}

	public class EndianReader
	{
		public Stream Base { get; protected set; }
		public Endianness Endian { get; set; }

		public static implicit operator Stream(EndianReader reader)
		{
			return reader.Base;
		}

		public EndianReader(Stream input, Endianness endianness)
		{
			Endian = endianness;
			Base = input;
		}

		public short ReadInt16() { return ReadInt16(Endian); }
		public short ReadInt16(Endianness endian)
		{
			switch (endian) {
				case Endianness.BigEndian:
					return BigEndianConverter.ToInt16(ReadBytes(2));
				case Endianness.LittleEndian:
					return LittleEndianConverter.ToInt16(ReadBytes(2));
				default:
					throw new NotSupportedException();
			}
		}
		public int ReadInt32() { return ReadInt32(Endian); }
		public int ReadInt32(Endianness endian)
		{
			switch (endian) {
				case Endianness.BigEndian:
					return BigEndianConverter.ToInt32(ReadBytes(4));
				case Endianness.LittleEndian:
					return LittleEndianConverter.ToInt32(ReadBytes(4));
				default:
					throw new NotSupportedException();
			}
		}
		public long ReadInt64() { return ReadInt64(Endian); }
		public long ReadInt64(Endianness endian)
		{
			switch (endian) {
				case Endianness.BigEndian:
					return BigEndianConverter.ToInt64(ReadBytes(8));
				case Endianness.LittleEndian:
					return LittleEndianConverter.ToInt64(ReadBytes(8));
				default:
					throw new NotSupportedException();
			}
		}
		public ushort ReadUInt16() { return ReadUInt16(Endian); }
		public ushort ReadUInt16(Endianness endian)
		{
			return (ushort)ReadInt16(endian);
		}
		public uint ReadUInt32() { return ReadUInt32(Endian); }
		public uint ReadUInt32(Endianness endian)
		{
			return (uint)ReadInt32(endian);
		}
		public ulong ReadUInt64() { return ReadUInt64(Endian); }
		public ulong ReadUInt64(Endianness endian)
		{
			return (ulong)ReadInt64(endian);
		}
		public float ReadFloat32() { return ReadFloat32(Endian); }
		public float ReadFloat32(Endianness endian)
		{
			switch (endian) {
				case Endianness.BigEndian:
					return BigEndianConverter.ToFloat32(ReadBytes(4));
				case Endianness.LittleEndian:
					return LittleEndianConverter.ToFloat32(ReadBytes(4));
				default:
					throw new NotSupportedException();
			}
		}
		public double ReadFloat64() { return ReadFloat64(Endian); }
		public double ReadFloat64(Endianness endian)
		{
			switch (endian) {
				case Endianness.BigEndian:
					return BigEndianConverter.ToFloat64(ReadBytes(8));
				case Endianness.LittleEndian:
					return LittleEndianConverter.ToFloat64(ReadBytes(8));
				default:
					throw new NotSupportedException();
			}
		}

		public void Write(short value) { Write(value, Endian); }
		public void Write(short value, Endianness endian)
		{
			switch (endian) {
				case Endianness.BigEndian:
					Write(BigEndianConverter.GetBytes(value));
					break;
				case Endianness.LittleEndian:
					Write(LittleEndianConverter.GetBytes(value));
					break;
				default:
					throw new NotSupportedException();
			}
		}
		public void Write(int value) { Write(value, Endian); }
		public void Write(int value, Endianness endian)
		{
			switch (endian) {
				case Endianness.BigEndian:
					Write(BigEndianConverter.GetBytes(value));
					break;
				case Endianness.LittleEndian:
					Write(LittleEndianConverter.GetBytes(value));
					break;
				default:
					throw new NotSupportedException();
			}
		}
		public void Write(long value) { Write(value, Endian); }
		public void Write(long value, Endianness endian)
		{
			switch (endian) {
				case Endianness.BigEndian:
					Write(BigEndianConverter.GetBytes(value));
					break;
				case Endianness.LittleEndian:
					Write(LittleEndianConverter.GetBytes(value));
					break;
				default:
					throw new NotSupportedException();
			}
		}
		public void Write(ushort value) { Write(value, Endian); }
		public void Write(ushort value, Endianness endian)
		{
			Write((short)value, endian);
		}
		public void Write(uint value) { Write(value, Endian); }
		public void Write(uint value, Endianness endian)
		{
			Write((int)value, endian);
		}
		public void Write(ulong value) { Write(value, Endian); }
		public void Write(ulong value, Endianness endian)
		{
			Write((long)value, endian);
		}
		public void Write(float value) { Write(value, Endian); }
		public void Write(float value, Endianness endian)
		{
			switch (endian) {
				case Endianness.BigEndian:
					Write(BigEndianConverter.GetBytes(value));
					break;
				case Endianness.LittleEndian:
					Write(LittleEndianConverter.GetBytes(value));
					break;
				default:
					throw new NotSupportedException();
			}
		}
		public void Write(double value) { Write(value, Endian); }
		public void Write(double value, Endianness endian)
		{
			switch (endian) {
				case Endianness.BigEndian:
					Write(BigEndianConverter.GetBytes(value));
					break;
				case Endianness.LittleEndian:
					Write(LittleEndianConverter.GetBytes(value));
					break;
				default:
					throw new NotSupportedException();
			}
		}

		public byte ReadByte()
		{
			return (byte)Base.ReadByte();
		}

		public void Write(byte data)
		{
			Base.WriteByte(data);
		}

		public void Write(byte[] data)
		{
			Base.Write(data, 0, data.Length);
		}

		public byte[] ReadBytes(int count)
		{
			byte[] buffer = new byte[count];

			ReadBytes(buffer);

			return buffer;
		}

		public void ReadBytes(byte[] buffer)
		{
			if (buffer.Length == 0)
				return;

			int index = 0;
			do {
				int num = Base.Read(buffer, index, buffer.Length - index);
				if (num == 0)
					throw new Exception();

				index += num;
			} while (index < buffer.Length);
		}

		public long Seek(long offset, SeekOrigin origin)
		{
			return Base.Seek(offset, origin);
		}

		public int Read(byte[] buffer, int offset, int count)
		{
			return Base.Read(buffer, offset, count);
		}

		public void Write(byte[] buffer, int offset, int count)
		{
			Base.Write(buffer, offset, count);
		}

		public void Write(string str)
		{
			Write(Util.Encoding.GetBytes(str));
		}

		public void Write(string str, int size)
		{
			size = Math.Max(str.Length, size);
			Write(str.Substring(0, Math.Min(str.Length, size)));
			if (size > str.Length)
				Pad(size - str.Length);
		}

		public string ReadString()
		{
			return Util.ReadCString(this);
		}

		public string ReadString(int length)
		{
			return Util.Encoding.GetString(ReadBytes(length)).TrimEnd('\0');
		}

		public long Position
		{
			get
			{
				return Base.Position;
			}
			set
			{
				Base.Position = value;
			}
		}

		public void Pad(long size, bool read)
		{
			byte[] padding = new byte[0x100];
			while (size > 0) {
				int sz = (int)Math.Min(0x100, size);
				if (read)
					Read(padding, 0, sz);
				else
					Write(padding, 0, sz);
				size -= sz;
			}
		}

		public void PadTo(long pos, bool read)
		{
			if (pos < Position)
				throw new ArgumentException();
			Pad(pos - Position, read);
		}

		public void PadToMultiple(int round, bool read)
		{
			PadTo(Util.RoundUp(Position, round), read);
		}

		public void PadToMultiple(int round)
		{
			PadToMultiple(round, false);
		}
		public void PadTo(long pos)
		{
			PadTo(pos, false);
		}
		public void PadReadTo(long pos)
		{
			PadTo(pos, true);
		}
		public void Pad(long size)
		{
			Pad(size, false);
		}
		public void PadRead(long size)
		{
			Pad(size, true);
		}
		public void PadReadToMultiple(int round)
		{
			PadToMultiple(round, true);
		}
	}
}
