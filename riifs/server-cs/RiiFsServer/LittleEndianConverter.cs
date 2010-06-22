using System;

namespace ConsoleHaxx.Common
{
	public class LittleEndianConverter
	{
		public static byte[] GetBytes(int value)
		{
			byte[] bytes = BitConverter.GetBytes(value);
			if (!BitConverter.IsLittleEndian)
				Array.Reverse(bytes);
			return bytes;
		}
		public static byte[] GetBytes(long value)
		{
			byte[] bytes = BitConverter.GetBytes(value);
			if (!BitConverter.IsLittleEndian)
				Array.Reverse(bytes);
			return bytes;
		}
		public static byte[] GetBytes(short value)
		{
			byte[] bytes = BitConverter.GetBytes(value);
			if (!BitConverter.IsLittleEndian)
				Array.Reverse(bytes);
			return bytes;
		}
		public static byte[] GetBytes(uint value)
		{
			return GetBytes((int)value);
		}
		public static byte[] GetBytes(ulong value)
		{
			return GetBytes((long)value);
		}
		public static byte[] GetBytes(ushort value)
		{
			return GetBytes((short)value);
		}
		public static byte[] GetBytes(float value)
		{
			byte[] bytes = BitConverter.GetBytes(value);
			if (!BitConverter.IsLittleEndian)
				Array.Reverse(bytes);
			return bytes;
		}
		public static byte[] GetBytes(double value)
		{
			byte[] bytes = BitConverter.GetBytes(value);
			if (!BitConverter.IsLittleEndian)
				Array.Reverse(bytes);
			return bytes;
		}

		public static short ToInt16(byte[] data)
		{
			if (!BitConverter.IsLittleEndian)
				data = data.Reverse();
			return BitConverter.ToInt16(data, 0);
		}

		public static int ToInt32(byte[] data)
		{
			if (!BitConverter.IsLittleEndian)
				data = data.Reverse();
			return BitConverter.ToInt32(data, 0);
		}

		public static long ToInt64(byte[] data)
		{
			if (!BitConverter.IsLittleEndian)
				data = data.Reverse();
			return BitConverter.ToInt64(data, 0);
		}

		public static ushort ToUInt16(byte[] data)
		{
			return (ushort)ToInt16(data);
		}

		public static uint ToUInt32(byte[] data)
		{
			return (uint)ToInt32(data);
		}

		public static ulong ToUInt64(byte[] data)
		{
			return (ulong)ToInt64(data);
		}

		public static float ToFloat32(byte[] data)
		{
			if (!BitConverter.IsLittleEndian)
				data = data.Reverse();
			return BitConverter.ToSingle(data, 0);
		}

		public static double ToFloat64(byte[] data)
		{
			if (!BitConverter.IsLittleEndian)
				data = data.Reverse();
			return BitConverter.ToDouble(data, 0);
		}
	}
}
