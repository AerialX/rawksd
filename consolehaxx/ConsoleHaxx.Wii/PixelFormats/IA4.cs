using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Drawing;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.Wii
{
	public class IA4 : PixelEncoding
	{
		public override uint ID
		{
			get { return 0x02; }
		}

		public override uint BitsPerPixel
		{
			get { return 0x8; }
		}

		public override Size Tilesize
		{
			get { return new Size(8, 4); }
		}

		public override Bitmap DecodeImage(Stream stream, uint width, uint height)
		{
			Bitmap image = new Bitmap((int)width, (int)height);

			EndianReader reader = new EndianReader(stream, Endianness.BigEndian);

			for (int y = 0; y < height; y += 4) {
				for (int x = 0; x < width; x += 8) {
					for (int y2 = 0; y2 < 4; y2++) {
						for (int x2 = 0; x2 < 8; x2++) {
							byte pixel = reader.ReadByte();
							byte intensity = (byte)((int)(pixel & 0x0F) * 255 / 15);
							int pixelx = x + x2;
							int pixely = y + y2;
							if (pixelx < width && pixely < height)
								image.SetPixel(pixelx, pixely, Color.FromArgb((int)(pixel >> 4) * 255 / 15, intensity, intensity, intensity));
						}
					}
				}
			}

			return image;
		}

		public override void EncodeImage(Stream stream, Bitmap image)
		{
			EndianReader writer = new EndianReader(stream, Endianness.BigEndian);

			for (int y = 0; y < image.Height; y += 4) {
				for (int x = 0; x < image.Width; x += 8) {
					for (int y2 = 0; y2 < 4; y2++) {
						for (int x2 = 0; x2 < 8; x2++) {
							byte pixel = 0;
							int pixelx = x + x2;
							int pixely = y + y2;
							if (pixelx < image.Width && pixely < image.Height) {
								Color colour = image.GetPixel(pixelx, pixely);
								pixel = (byte)(((colour.A * 15 / 255) << 4) | (byte)((I8.GetIntensity(colour) * 15 / 255) & 0x0F));
							}
							writer.Write(pixel);
						}
					}
				}
			}
		}
	}
}
