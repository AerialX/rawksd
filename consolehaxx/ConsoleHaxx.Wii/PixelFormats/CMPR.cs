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
	public class CMPR : PixelEncoding
	{
		public override uint ID
		{
			get { return 0x0E; }
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
			
			for (int y = 0; y < height; y += 8) {
				for (int x = 0; x < width; x += 8) {
					for (int y2 = 0; y2 < 8; y2 += 4) {
						for (int x2 = 0; x2 < 8; x2 += 4) {
							u16 c0 = reader.ReadUInt16();
							u16 c1 = reader.ReadUInt16();
							u32 bits = reader.ReadUInt32();
							u8 b0 = (u8)((c0 & 0x1f) << 3);
							u8 g0 = (u8)(((c0 >> 5) & 0x3f) << 2);
							u8 r0 = (u8)(((c0 >> 11) & 0x1f) << 3);
							u8 b1 = (u8)((c1 & 0x1f) << 3);
							u8 g1 = (u8)(((c1 >> 5) & 0x3f) << 2);
							u8 r1 = (u8)(((c1 >> 11) & 0x1f) << 3);
							for (int y3 = 3; y3 >= 0; y3--) {
								for (int x3 = 3; x3 >= 0; x3--) {
									int newx = x + x2 + x3;
									int newy = y + y2 + y3;
									if (newx >= width || newy >= height)
										continue;

									uint control = bits & 3;
									bits >>= 2;
									Color colour;
									switch (control) {
										case 0:
											colour = Color.FromArgb(255, r0, g0, b0);
											break;
										case 1:
											colour = Color.FromArgb(255, r1, g1, b1);
											break;
										case 2:
											if (c0 > c1)
												colour = Color.FromArgb(255, (2 * r0 + r1) / 3, (2 * g0 + g1) / 3, (2 * b0 + b1) / 3);
											else
												colour = Color.FromArgb(255, (r0 + r1) / 2, (g0 + g1) / 2, (b0 + b1) / 2);
											break;
										case 3:
											if (c0 > c1)
												colour = Color.FromArgb(255, (r0 + 2 * r1) / 3, (g0 + 2 * g1) / 3, (b0 + 2 * b1) / 3);
											else
												colour = Color.FromArgb(255, 0, 0, 0);
											break;
										default:
											continue;
									}

									image.SetPixel(newx, newy, colour);
								}
							}
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
