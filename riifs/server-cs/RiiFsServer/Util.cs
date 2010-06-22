using System;
using System.Collections.Generic;
using System.Text;
using System.IO;

#if NET_2_0 // For pre-.NET 3.x targets that want to use C# 3.0 extension methods
namespace System.Runtime.CompilerServices
{
	[AttributeUsage(AttributeTargets.Method)]
	public sealed class ExtensionAttribute : Attribute
	{
		public ExtensionAttribute() { }
	}
}
#endif

namespace ConsoleHaxx.Common
{
	public static class Util
	{
		public static readonly Encoding Encoding = Encoding.GetEncoding(28591);

		public static long RoundUp(long p, long round)
		{
			return RoundDown(p + round - 1, round);
		}

		public static ulong RoundUp(ulong p, ulong round)
		{
			return RoundDown(p + round - 1, round);
		}

		public static long RoundDown(long p, long round)
		{
			return p / round * round;
		}

		public static ulong RoundDown(ulong p, ulong round)
		{
			return p / round * round;
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

		public static string ReadCString(byte[] data, int startoffset)
		{
			return ReadCString(new MemoryStream(data, startoffset, data.Length - startoffset, false, true));
		}

		public static string ReadCString(this Stream stream)
		{
			return ReadCString(stream, 0);
		}

		public static string ReadCString(this Stream stream, int max)
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
