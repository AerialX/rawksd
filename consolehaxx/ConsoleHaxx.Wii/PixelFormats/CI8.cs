using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Drawing;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.Wii
{
	public class CI8 : PixelEncoding
	{
		public override uint ID
		{
			get { return 0x09; }
		}

		public override uint BitsPerPixel
		{
			get { return 0x08; }
		}

		public override Size Tilesize
		{
			get { return new Size(8, 4); }
		}

		public override Bitmap DecodeImage(Stream stream, uint width, uint height, Stream data, int type)
		{
			Bitmap image = new Bitmap((int)width, (int)height);

			EndianReader reader = new EndianReader(stream, Endianness.BigEndian);

			EndianReader datareader = new EndianReader(data, Endianness.BigEndian);
			List<Color> palette = new List<Color>();
			try {
				for (int i = 0; i < 0x100; i++)
					palette.Add(ReadPaletteColour(datareader.ReadUInt16(), type));
			} catch { } // We can't be sure of the palette size thanks to this fucking format.

			for (int y = 0; y < height; y += 4) {
				for (int x = 0; x < width; x += 8) {
					for (int y1 = 0; y1 < 4; y1++) {
						int ypixel = y + y1;
						for (int x1 = 0; x1 < 8; x1++) {
							byte pixel = reader.ReadByte();
							int xpixel = x + x1;
							if (ypixel < height && xpixel < width)
								image.SetPixel(xpixel, ypixel, palette[pixel]);
						}
					}
				}
			}

			return image;
		}

		public static Color ReadPaletteColour(ushort pixel, int type)
		{
			switch (type) {
				case 0:
					return IA8.GetPixel(pixel);
				case 1:
					return RGB565.GetPixel(pixel);
				case 2:
					return RGB5A3.GetPixel(pixel);
				default:
					throw new FormatException();
			}
		}

		public static ushort WritePaletteColour(Color pixel, int type)
		{
			switch (type) {
				case 0:
					return IA8.GetPixel(pixel);
				case 1:
					return RGB565.GetPixel(pixel);
				case 2:
					return RGB5A3.GetPixel(pixel);
				default:
					throw new FormatException();
			}
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

			if (palette.Count > 0x100)
				throw new FormatException("Image palette does not fit in encoding (more than 256 colours used).");

			EndianReader datareader = new EndianReader(data, Endianness.BigEndian);
			for (int i = 0; i < palette.Count; i++)
				datareader.Write(WritePaletteColour(palette[i], type));
			datareader.PadTo(0x220);

			for (int y = 0; y < image.Height; y += 4) {
				for (int x = 0; x < image.Width; x += 8) {
					for (int y1 = 0; y1 < 4; y1++) {
						int ypixel = y + y1;
						for (int x1 = 0; x1 < 8; x1++) {
							byte pixel = 0;
							int xpixel = x + x1;
							if (ypixel < image.Height && xpixel < image.Width)
								pixel = (byte)palette.IndexOf(image.GetPixel(xpixel, ypixel));
							writer.Write(pixel);
						}
					}
				}
			}
		}
	}
}
