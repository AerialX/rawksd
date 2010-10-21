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
	public class TiledARGB : PixelEncoding
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
			get { return new Size(2, 2); }
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

		private void DecodeImage(EndianReader reader, int x, int y, int width, int height, Bitmap image, bool reverse)
		{
			if (width == 2 || height == 2) {
				if (reverse) {
					for (int x2 = 0; x2 < width; x2++)
						for (int y2 = 0; y2 < height; y2++)
							image.SetPixel(x + x2, y + y2, GetPixel(reader.ReadUInt32()));
				} else {
					for (int y2 = 0; y2 < height; y2++)
						for (int x2 = 0; x2 < width; x2++)
							image.SetPixel(x + x2, y + y2, GetPixel(reader.ReadUInt32()));
				}
			} else {
				int hheight = Math.Max(2, height / 2);
				int hwidth = Math.Max(2, width / 2);
				int half = Math.Min(hheight, hwidth);
				if (reverse) {
					for (int x2 = 0; x2 < width; x2 += half)
						for (int y2 = 0; y2 < height; y2 += half)
							DecodeImage(reader, x + x2, y + y2, half, half, image, reverse);		
				} else {
					for (int y2 = 0; y2 < height; y2 += half)
						for (int x2 = 0; x2 < width; x2 += half)
							DecodeImage(reader, x + x2, y + y2, half, half, image, reverse);
				}
			}
		}

		public override Bitmap DecodeImage(Stream stream, uint width, uint height)
		{
			Bitmap image = new Bitmap((int)width, (int)height);

			EndianReader reader = new EndianReader(stream, Endianness.BigEndian);

			DecodeImage(reader, 0, 0, (int)width, (int)height, image, width > height);

			/*
			for (int y = 0; y < height; y += 0x40) {
				for (int x = 0; x < width; x += 0x40) {
					for (int y7 = 0; y7 < 0x40; y7 += 0x20) {
						for (int x7 = 0; x7 < 0x40; x7 += 0x20) {
					for (int y2 = 0; y2 < 0x20; y2 += 0x10) {
						for (int x2 = 0; x2 < 0x20; x2 += 0x10) {
							for (int y3 = 0; y3 < 0x10; y3 += 0x08) {
								for (int x3 = 0; x3 < 0x10; x3 += 0x08) {
									for (int y4 = 0; y4 < 0x08; y4 += 0x04) {
										for (int x4 = 0; x4 < 0x08; x4 += 0x04) {
											for (int y5 = 0; y5 < 0x04; y5 += 0x02) {
												for (int x5 = 0; x5 < 0x04; x5 += 0x02) {
													for (int y6 = 0; y6 < 0x02; y6++) {
														for (int x6 = 0; x6 < 0x02; x6++) {
															image.SetPixel(x + x2 + x3 + x4 + x5 + x6 + x7, y + y2 + y3 + y4 + y5 + y6 + y7, GetPixel(reader.ReadUInt32()));
														}
													}
												}
											}
										}
									}
								}
							}
						}
							}
						}
					}
				}
			}
			*/

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
