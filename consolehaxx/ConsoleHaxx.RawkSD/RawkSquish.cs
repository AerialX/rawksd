using System;
using System.Runtime.InteropServices;

namespace ConsoleHaxx.RawkSD
{
	public class RawkSquish
	{
		[DllImport("RawkSquish", EntryPoint = "RawkSquish")]
		public static extern void Squish(byte[] inbuffer, int width, int height, byte[] outbuffer);
		[DllImport("RawkSquish", EntryPoint = "RawkSquish")]
		public static extern void Squish(IntPtr inbuffer, int width, int height, byte[] outbuffer);
	}
}
