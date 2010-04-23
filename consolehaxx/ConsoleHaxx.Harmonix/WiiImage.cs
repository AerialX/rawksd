using System;
using System.Collections.Generic;
using System.IO;
using ConsoleHaxx.Common;
using ConsoleHaxx.Wii;
using System.Drawing;

namespace ConsoleHaxx.Harmonix
{
	public class WiiImage
	{
		public ushort Width;
		public ushort Height;
		public byte BitsPerPixel;
		public Bitmap Bitmap;

		public static WiiImage Create(EndianReader stream)
		{
			byte id = stream.ReadByte();
			if (id != 1 && id != 2)
				throw new FormatException(); // ID byte

			WiiImage image = new WiiImage();
			image.BitsPerPixel = stream.ReadByte(); // If this isn't 4... I think we're fucked.
			stream.Position += 5; // 5 bytes of who knows what
			image.Width = stream.ReadUInt16();
			image.Height = stream.ReadUInt16();

			stream.ReadByte(); // Dunno...

			stream.PadReadTo(0x20);
			image.Bitmap = PixelEncoding.GetEncoding<CMPR>().DecodeImage(stream.Base, image.Width, image.Height);

			return image;
		}
	}
}
