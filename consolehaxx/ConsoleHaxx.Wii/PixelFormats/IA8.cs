using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Drawing;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.Wii
{
	public class IA8 : PixelEncoding
	{
		public override uint ID
		{
			get { return 0x03; }
		}

		public override uint BitsPerPixel
		{
			get { return 0x10; }
		}

		public override Size Tilesize
		{
			get { return new Size(4, 4); }
		}

		public override Bitmap DecodeImage(Stream stream, uint width, uint height)
		{
			Bitmap image = new Bitmap((int)width, (int)height);

			EndianReader reader = new EndianReader(stream, Endianness.BigEndian);

			for (int y = 0; y < height; y += 4) {
				for (int x = 0; x < width; x += 4) {
					for (int y2 = 0; y2 < 4; y2++) {
						for (int x2 = 0; x2 < 4; x2++) {
							ushort pixel = reader.ReadUInt16();
							int pixelx = x + x2;
							int pixely = y + y2;
							if (pixelx < width && pixely < height)
								image.SetPixel(pixelx, pixely, GetPixel(pixel));
						}
					}
				}
			}

			return image;
		}

		public static Color GetPixel(ushort pixel)
		{
			byte intensity = (byte)(pixel >> 8);
			return Color.FromArgb(pixel & 0xFF, intensity, intensity, intensity);
		}

		public static ushort GetPixel(Color pixel)
		{
			byte intensity = (byte)(((int)pixel.R + (int)pixel.G + (int)pixel.B) / 3);
			return (ushort)((ushort)pixel.A | ((ushort)intensity << 8));
		}

		public override void EncodeImage(Stream stream, Bitmap image)
		{
			EndianReader writer = new EndianReader(stream, Endianness.BigEndian);

			for (int y = 0; y < image.Height; y += 4) {
				for (int x = 0; x < image.Width; x += 4) {
					for (int y2 = 0; y2 < 4; y2++) {
						for (int x2 = 0; x2 < 4; x2++) {
							ushort pixel = 0;
							int pixelx = x + x2;
							int pixely = y + y2;
							if (pixelx < image.Width && pixely < image.Height) {
								Color colour = image.GetPixel(pixelx, pixely);
								pixel = (byte)(colour.A | (I8.GetIntensity(colour) << 8));
							}
							writer.Write(pixel);
						}
					}
				}
			}
		}
	}
}
