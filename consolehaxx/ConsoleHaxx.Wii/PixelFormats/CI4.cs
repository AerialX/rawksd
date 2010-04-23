using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Drawing;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.Wii
{
	public class CI4 : PixelEncoding
	{
		public override uint ID
		{
			get { return 0x08; }
		}

		public override uint BitsPerPixel
		{
			get { return 0x04; }
		}

		public override Size Tilesize
		{
			get { return new Size(8, 8); }
		}

		public override Bitmap DecodeImage(Stream stream, uint width, uint height, Stream data, int type)
		{
			Bitmap image = new Bitmap((int)width, (int)height);

			EndianReader reader = new EndianReader(stream, Endianness.BigEndian);

			EndianReader datareader = new EndianReader(data, Endianness.BigEndian);
			List<Color> palette = new List<Color>();
			try {
				for (int i = 0; i < 0x10; i++)
					palette.Add(CI8.ReadPaletteColour(datareader.ReadUInt16(), type));
			} catch { } // We can't be sure of the palette size thanks to this fucking format.

			for (int y = 0; y < height; y += 8) {
				for (int x = 0; x < width; x += 8) {
					for (int y1 = 0; y1 < 8; y1++) {
						int ypixel = y + y1;
						for (int x1 = 0; x1 < 8; x1 += 2) {
							byte pixel = reader.ReadByte();
							for (int x2 = 0; x2 < 2; x2++) {
								int xpixel = x + x1 + x2;
								if (ypixel < height && xpixel < width)
									image.SetPixel(xpixel, ypixel, palette[(pixel >> (4 * (1 - x2))) & 0x0F]);
							}
						}
					}
				}
			}

			return image;
		}

		public override void EncodeImage(Stream stream, Bitmap image, Stream data, int type)
		{
			EndianReader writer = new EndianReader(stream, Endianness.BigEndian);

			List<Color> palette = new List<Color>();
			for (int y = 0; y < image.Height; y++) {
				for (int x = 0; x < image.Width; x++) {
					Color colour = image.GetPixel(x, y);
					if (!palette.Contains(colour))
						palette.Add(colour);
				}
			}

			if (palette.Count > 0x10)
				throw new FormatException("Image palette does not fit in encoding.");

			EndianReader datareader = new EndianReader(data, Endianness.BigEndian);
			List<Color> datapalette = new List<Color>();
			try {
				for (int i = 0; i < 0x10; i++)
					datapalette.Add(CI8.ReadPaletteColour(datareader.ReadUInt16(), type));
			} catch { } // We can't be sure of the palette size thanks to this fucking format.

			foreach (Color colour in palette) {
				if (!datapalette.Contains(colour))
					throw new NotImplementedException("Palette in secondary data doesn't match.");
			}

			for (int y = 0; y < image.Height; y += 8) {
				for (int x = 0; x < image.Width; x += 8) {
					for (int y1 = 0; y1 < 8; y1++) {
						int ypixel = y + y1;
						for (int x1 = 0; x1 < 8; x1 += 2) {
							byte pixel = 0;
							for (int x2 = 0; x2 < 2; x2++) {
								int xpixel = x + x1 + x2;
								if (ypixel < image.Height && xpixel < image.Width)
									pixel |= (byte)(datapalette.IndexOf(image.GetPixel(xpixel, ypixel)) << (4 * (1 - x2)));
							}
							writer.Write(pixel);
						}
					}
				}
			}
		}
	}
}
