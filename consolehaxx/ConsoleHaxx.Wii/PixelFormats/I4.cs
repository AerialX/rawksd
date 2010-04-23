using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Drawing;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.Wii
{
	public class I4 : PixelEncoding
	{
		public override uint ID
		{
			get { return 0x00; }
		}

		public override uint BitsPerPixel
		{
			get { return 0x04; }
		}

		public override Size Tilesize
		{
			get { return new Size(8, 8); }
		}

		public override Bitmap DecodeImage(Stream stream, uint width, uint height)
		{
			Bitmap image = new Bitmap((int)width, (int)height);

			EndianReader reader = new EndianReader(stream, Endianness.BigEndian);

			for (int y = 0; y < height; y += 4) {
				for (int x = 0; x < width; x += 8) {
					for (int y2 = 0; y2 < 4; y2++) {
						for (int x2 = 0; x2 < 8; x2 += 2) {
							byte pixeldata = reader.ReadByte();
							int pixely = y + y2;
							for (int x3 = 0; x3 < 2; x3++) {
								int pixelx = x + x2 + x3;
								byte pixel = (byte)((((int)pixeldata >> (x3 * 4)) & 0x0F) * 255 / 15);
								if (pixelx < width && pixely < height)
									image.SetPixel(pixelx, pixely, Color.FromArgb(255, pixel, pixel, pixel));
							}
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
						for (int x2 = 0; x2 < 8; x2 += 2) {
							byte pixeldata = 0;
							int pixely = y + y2;
							for (int x3 = 0; x3 < 2; x3++) {
								int pixelx = x + x2 + x3;
								if (pixelx < image.Width && pixely < image.Height)
									pixeldata |=(byte)((I8.GetIntensity(image.GetPixel(pixelx, pixely)) * 15 / 255) << (x3 * 4));
							}
							writer.Write(pixeldata);
						}
					}
				}
			}
		}
	}
}
