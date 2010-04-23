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
using System.Drawing;
using System.IO;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.Wii
{
	public class RGBA8 : PixelEncoding
	{
		public override uint ID
		{
			get { return 0x06; }
		}

		public override uint BitsPerPixel
		{
			get { return 0x20; }
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
					byte[] data = reader.ReadBytes(0x40);
					for (int y2 = 0; y2 < 4; y2++) {
						for (int x2 = 0; x2 < 4; x2++) {
							int a = data[y2 * 8 + x2 * 2];
							int b = data[y2 * 8 + x2 * 2 + 0x21];
							int g = data[y2 * 8 + x2 * 2 + 0x20];
							int r = data[y2 * 8 + x2 * 2 + 1];
							int pixelx = x + x2;
							int pixely = y + y2;
							if (pixelx < width && pixely < height) // Clip/ignore bytes that go beyond the size of the image
								image.SetPixel(pixelx, pixely, Color.FromArgb(a, r, g, b));
						}
					}
				}
			}

			return image;
		}

		public override void EncodeImage(Stream stream, Bitmap image)
		{
			EndianReader writer = new EndianReader(stream, Endianness.BigEndian);

			byte[] data = new byte[0x40];
			for (int y = 0; y < image.Height; y += 4) {
				for (int x = 0; x < image.Width; x += 4) {
					Util.Memset<byte>(data, 0);
					for (int y2 = 0; y2 < 4; y2++) {
						for (int x2 = 0; x2 < 4; x2++) {
							int pixelx = x + x2;
							int pixely = y + y2;
							if (pixelx < image.Width && pixely < image.Height) {
								Color pixel = image.GetPixel(pixelx, pixely);
								data[y2 * 8 + x2 * 2] = pixel.A;
								data[y2 * 8 + x2 * 2 + 0x21] = pixel.B;
								data[y2 * 8 + x2 * 2 + 0x20] = pixel.G;
								data[y2 * 8 + x2 * 2 + 1] = pixel.R;
							}
						}
					}

					writer.Write(data);
				}
			}
		}
	}
}
