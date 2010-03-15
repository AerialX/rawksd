using System;
using System.Collections.Generic;
using System.Text;
using System.IO;

namespace ConsoleHaxx.Common
{
	public static class Util
	{
		public static readonly Encoding Encoding = Encoding.GetEncoding(28591);

		public static long RoundUp(long p, int round)
		{
			unchecked {
				return (p + round - 1) & ~(round - 1);
			}
		}

		public static ulong RoundUp(ulong p, int round)
		{
			unchecked {
				return ((ulong)p + (uint)round - 1) & (ulong)~(round - 1);
			}
		}

		public static long RoundDown(long p, int round)
		{
			long ret = RoundUp(p, round);
			if (p - ret == 0)
				return ret;
			return ret - round;
		}

		public static ulong RoundDown(ulong p, int round)
		{
			ulong ret = RoundUp(p, round);
			if (p - ret == 0)
				return ret;
			return ret - (ulong)round;
		}

		public static long StreamCopy(Stream destination, Stream source)
		{
			long length = long.MaxValue;
			try {
				length = source.Length;
			} catch { /* Unlengthable streams */ }

			return StreamCopy(destination, source, length);
		}

		public static long StreamCopy(Stream destination, Stream source, long len)
		{
			return StreamCopy(destination, source, (ulong)len);
		}

		public static long StreamCopy(Stream destination, Stream source, ulong len)
		{
			//byte[] buffer = new byte[0x400 * 512]; // 512KB buffer
			byte[] buffer = new byte[0x400 * 0x400 * 2]; // 2MB buffer

			ulong pos = 0;
			while (pos < len) {
				int read = source.Read(buffer, 0, buffer.Length);
				pos += (uint)read;
				if (pos > len)
					read -= (int)(pos - len);

				if (read == 0)
					break;

				destination.Write(buffer, 0, read);

				if (read < buffer.Length)
					break;
			}

			return (long)pos;
		}

		public static string Pad(string str, int num, char ch)
		{
			for (int i = str.Length; i < num; i++) {
				str = ch + str;
			}

			return str;
		}

		public static string Pad(string str, int num)
		{
			return Pad(str, num, '0');
		}

		public static string GetTitlePath(ulong titleID)
		{
			int part1 = (int)(titleID >> 32);
			int part2 = (int)titleID;

			return Path.Combine(Util.Pad(part1.ToString("x"), 8), Util.Pad(part2.ToString("x"), 8));
		}

		public static string ReadCString(byte[] data, int startoffset)
		{
			return ReadCString(new MemoryStream(data, startoffset, data.Length - startoffset, false, true));
		}

		public static string ReadCString(Stream stream)
		{
			return ReadCString(stream, 0);
		}

		public static string ReadCString(Stream stream, int max)
		{
			StringBuilder ret = new StringBuilder();
			while (true) {
				int read = stream.ReadByte();
				if (read == -1)
					break;
				char character = (char)read;
				if (character == '\0')
					break;
				ret.Append(character);

				if (max > 0 && ret.Length == max)
					break;
			}

			return ret.ToString();
		}

		public static bool EOF(this Stream stream)
		{
			try {
				return stream.Position == stream.Length;
			} catch { /* Unlegthable streams */ }

			return false;
		}

		public static T[] Reverse<T>(this T[] array)
		{
			T[] newarray = new T[array.Length];
			Array.Copy(array, newarray, array.Length);
			Array.Reverse(newarray);
			return newarray;
		}

		public static void Memset<T>(T[] data, T value)
		{
			Memset(data, 0, value);
		}
		public static void Memset<T>(T[] data, int offset, T value)
		{
			Memset(data, offset, value, data.Length - offset);
		}
		public static void Memset<T>(T[] data, int offset, T value, int length)
		{
			for (int i = 0; i < length; i++)
				data[offset + i] = value;
		}

		public static void Memcpy<T>(T[] data, T[] value)
		{
			Memcpy(data, 0, value);
		}
		public static void Memcpy<T>(T[] data, int offset, T[] value)
		{
			Memcpy(data, offset, value, Math.Min(value.Length - offset, data.Length));
		}
		public static void Memcpy<T>(T[] data, T[] value, int length)
		{
			Memcpy(data, 0, value, length);
		}
		public static void Memcpy<T>(T[] data, int offset, T[] value, int length)
		{
			Memcpy(data, offset, value, 0, length);
		}
		public static void Memcpy<T>(T[] data, T[] value, int voffset, int length)
		{
			Memcpy(data, 0, value, voffset, length);
		}
		public static void Memcpy<T>(T[] data, int offset, T[] value, int voffset, int length)
		{
			for (int i = 0; i < length; i++)
				data[offset + i] = value[voffset + i];
		}

		public static string ToString(short value)
		{
			return Util.Pad(value.ToString("x"), 4);
		}
		public static string ToString(int value)
		{
			return Util.Pad(value.ToString("x"), 8);
		}
		public static string ToString(long value)
		{
			return Util.Pad(value.ToString("x"), 16);
		}
		public static string ToString(ushort value)
		{
			return ToString((short)value);
		}
		public static string ToString(uint value)
		{
			return ToString((int)value);
		}
		public static string ToString(ulong value)
		{
			return ToString((long)value);
		}
		public static string ToString(float value)
		{
			return value.ToString();
		}
		public static string ToString(double value)
		{
			return value.ToString();
		}

		public static bool HasValue(this string value)
		{
			return value != null && value.Length > 0;
		}
	}
}

namespace ConsoleHaxx.Common
{
	public class Pair<TKey, TValue>
	{
		public TKey Key;
		public TValue Value;

		public Pair(TKey key, TValue value)
		{
			Key = key;
			Value = value;
		}
	}
}
