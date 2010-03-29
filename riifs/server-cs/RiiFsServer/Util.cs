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
			byte[] buffer = new byte[0x400 * 0x400 * 2]; // 2MB buffer

			ulong read = 0;
			while (read < len) {
				int ret = 0;
				try {
					ret = source.Read(buffer, 0, (int)Math.Min((ulong)buffer.Length, len - read));
				} catch { }

				if (ret == 0)
					break;

				destination.Write(buffer, 0, ret);

				read += (uint)ret;
			}

			return (long)read;
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

		public static T[] Reverse<T>(this T[] array)
		{
			T[] newarray = new T[array.Length];
			Array.Copy(array, newarray, array.Length);
			Array.Reverse(newarray);
			return newarray;
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
