#region Using Shortcuts
using s8 = System.SByte;
using u8 = System.Byte;
using s16 = System.Int16;
using u16 = System.UInt16;
using s32 = System.Int32;
using u32 = System.UInt32;
using s64 = System.Int64;
using u64 = System.UInt64;
#endregion

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Drawing;
using System.IO;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.Wii
{
	public class ARGB : PixelEncoding
	{
		public override uint ID
		{
			get { return 0xAAE4; }
		}

		public override uint BitsPerPixel
		{
			get { return 0x20; }
		}

		public override Size Tilesize
		{
			get { return new Size(1, 1); }
		}

		public static Color GetPixel(uint pixel)
		{
			return Color.FromArgb((int)(pixel >> 24), (int)(pixel >> 16) & 0xff, (int)(pixel >> 8) & 0xff, (int)pixel & 0xff);
		}

		public static uint GetPixel(Color colour)
		{
			uint pixel = 0;
			pixel |= (uint)colour.A << 24;
			pixel |= (uint)colour.R << 16;
			pixel |= (uint)colour.G << 8;
			pixel |= (uint)colour.B;
			return pixel;
		}

		public override Bitmap DecodeImage(Stream stream, uint width, uint height)
		{
			Bitmap image = new Bitmap((int)width, (int)height);

			EndianReader reader = new EndianReader(stream, Endianness.BigEndian);

			for (int y = 0; y < height; y++) {
				for (int x = 0; x < width; x++)
					image.SetPixel(x, y, GetPixel(reader.ReadUInt32()));
			}

			return image;
		}

		public override void EncodeImage(Stream stream, Bitmap image)
		{
			EndianReader writer = new EndianReader(stream, Endianness.BigEndian);

			for (int y = 0; y < image.Height; y++) {
				for (int x = 0; x < image.Width; x++)
					writer.Write(GetPixel(image.GetPixel(x, y)));
			}
		}
	}
}
