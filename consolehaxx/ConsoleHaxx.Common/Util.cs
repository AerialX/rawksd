using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Security.Cryptography;

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

		public static readonly Rijndael AesCBC;

		public const int SHA1HashSize = 0x14;
		public const int AesKeySize = 0x10;

		static Util()
		{
			AesCBC = Rijndael.Create();
			AesCBC.Padding = PaddingMode.None;
			AesCBC.Mode = CipherMode.CBC;
		}

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

		public static string Pad(string str, int num, char ch = '0')
		{
			return str.PadLeft(num, ch);
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

		public static bool IsEmpty(this string str)
		{
			return str == null || str.Length == 0;
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

		public static void Write(this Stream stream, byte[] data)
		{
			stream.Write(data, 0, data.Length);
		}

		public static void AddOrReplace<T>(this List<T> list, int index, T item)
		{
			for (int i = list.Count; i < index; i++)
				list.Add(default(T));

			if (list.Count == index)
				list.Add(item);
			else
				list[index] = item;
		}

		public static void Extract(this DirectoryNode dir, string path)
		{
			Directory.CreateDirectory(path);
			foreach (Node node in dir) {
				string newpath = Path.Combine(path, node.Name);
				if (node is DirectoryNode)
					Extract(node as DirectoryNode, newpath);
				else if (node is FileNode) {
					FileStream stream = new FileStream(newpath, FileMode.Create, FileAccess.Write);
					FileNode file = node as FileNode;
					file.Data.Position = 0;
					StreamCopy(stream, file.Data, file.Size);
					stream.Close();
				}
			}
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

		public static bool Memcmp(byte[] data, byte[] data2)
		{
			return Memcmp(data, 0, data2);
		}
		public static bool Memcmp(byte[] data, int offset, byte[] data2)
		{
			return Memcmp(data, offset, data2, 0, data.Length - offset);
		}
		public static bool Memcmp(byte[] data, byte[] data2, int offset2)
		{
			return Memcmp(data, 0, data2, offset2, data2.Length - offset2);
		}
		public static bool Memcmp(byte[] data, int offset, byte[] data2, int length)
		{
			return Memcmp(data, offset, data2, 0, length);
		}
		public static bool Memcmp(byte[] data, int offset, byte[] data2, int offset2, int length)
		{
			for (int i = 0; i < length; i++) {
				if (data[offset + i] != data2[offset2 + i])
					return true;
			}

			return false;
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
			Array.Copy(value, voffset, data, offset, length);
		}

		public static byte[] SHA1Hash(Stream data)
		{
			return SHA1.Create().ComputeHash(data);
		}

		public static byte[] SHA1Hash(Stream data, int length)
		{
			return SHA1Hash(new Substream(data, data.Position, length));
		}

		public static byte[] SHA1Hash(byte[] data)
		{
			return SHA1Hash(data, 0, data.Length);
		}
		// DELETE THIS v
		public static byte[] SHA1Hash(byte[] data, int offset)
		{
			return SHA1Hash(data, offset, data.Length - offset);
		}

		public static byte[] SHA1Hash(byte[] data, int offset, int length)
		{
			return SHA1.Create().ComputeHash(data, offset, length);
		}

		public static string ToString(byte value)
		{
			return Util.Pad(value.ToString("X"), 2);
		}
		public static string ToString(short value)
		{
			return Util.Pad(value.ToString("X"), 4);
		}
		public static string ToString(int value)
		{
			return Util.Pad(value.ToString("X"), 8);
		}
		public static string ToString(long value)
		{
			return Util.Pad(value.ToString("X"), 16);
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

		public static string ToString(byte[] patch)
		{
			StringBuilder ret = new StringBuilder(patch.Length * 2);
			foreach (byte value in patch) {
				ret.Append(Util.ToString(value));
			}

			return ret.ToString();
		}

		public static string FileSizeToString(long bytesize)
		{
			double size = bytesize;
			string units = "B";
			while (size > 0x400 && units != "GB") {
				switch (units) {
					case "B":
						units = "KB";
						break;
					case "KB":
						units = "MB";
						break;
					case "MB":
						units = "GB";
						break;
				}

				size /= 0x400;
			}

			return size.ToString(units == "B" ? "0" : "0.00") + " " + units;
		}

		public static void Delete(string path)
		{
			try {
				File.Delete(path);
			} catch { }
		}
	}
}
