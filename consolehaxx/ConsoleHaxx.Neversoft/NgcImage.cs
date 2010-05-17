using System;
using System.Collections.Generic;
using System.IO;
using ConsoleHaxx.Common;
using ConsoleHaxx.Wii;
using System.Drawing;

namespace ConsoleHaxx.Neversoft
{
	public class NgcImage
	{
		public int Width;
		public int Height;
		public byte BitsPerPixel;
		public Bitmap Bitmap;

		public static NgcImage Create(EndianReader stream)
		{
			NgcImage image = new NgcImage();
			image.BitsPerPixel = stream.ReadByte(); // If this isn't 4... I think we're fucked.
			stream.Position = 0x0A;
			image.Width = (int)Math.Pow(2, stream.ReadByte());
			image.Height = (int)Math.Pow(2, stream.ReadByte());

			stream.PadReadTo(0x20);
			Bitmap bitmap = PixelEncoding.GetEncoding<CMPR>().DecodeImage(stream.Base, (uint)image.Width, (uint)image.Height);

			image.Bitmap = new Bitmap(image.Width, image.Height);
			for (int x = 0; x < image.Width; x++)
				for (int y = 0; y < image.Height; y++)
					image.Bitmap.SetPixel(x, y, bitmap.GetPixel(x, image.Height - y - 1));

			return image;
		}
	}
}
