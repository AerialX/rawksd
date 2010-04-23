using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Drawing;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.Wii
{
	public class CI14X2 : PixelEncoding
	{
		public override uint ID
		{
			get { return 0x0A; }
		}

		public override uint BitsPerPixel
		{
			get { return 0x10; }
		}

		public override Size Tilesize
		{
			get { return new Size(4, 4); }
		}

		public override Bitmap DecodeImage(Stream stream, uint width, uint height, Stream data, int type)
		{
			Bitmap image = new Bitmap((int)width, (int)height);

			EndianReader reader = new EndianReader(stream, Endianness.BigEndian);

			EndianReader datareader = new EndianReader(data, Endianness.BigEndian);
			List<Color> palette = new List<Color>();
			try {
				for (int i = 0; i < 0x4000; i++)
					palette.Add(CI8.ReadPaletteColour(datareader.ReadUInt16(), type));
			} catch { } // We can't be sure of the palette size thanks to this fucking format.

			for (int y = 0; y < height; y += 4) {
				for (int x = 0; x < width; x += 4) {
					for (int y1 = 0; y1 < 4; y1++) {
						for (int x1 = 0; x1 < 4; x1++) {
							ushort pixel = reader.ReadUInt16();

							int pixelx = x + x1;
							int pixely = y + y1;
							if (pixelx < width && pixely < height)
								image.SetPixel(pixelx, pixely, palette[pixel & 0x3FFF]);
						}
					}
				}
			}

			return image;

		}

		public override void EncodeImage(Stream stream, Bitmap image)
		{
			throw new NotImplementedException();
		}
	}
}
