using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Drawing;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.Wii
{
	public class DXT5 : PixelEncoding
	{
		public override uint ID
		{
			get { return 0xAAE4; }
		}

		public override uint BitsPerPixel
		{
			get { return 0x08; }
		}

		public override Size Tilesize
		{
			get { return new Size(4, 4); }
		}

		public override Bitmap DecodeImage(Stream stream, uint width, uint height)
		{
			Bitmap image = new Bitmap((int)width, (int)height);
			BlockDecompressImageDXT5((int)width, (int)height, new EndianReader(stream, Endianness.LittleEndian), image);
			return image;
		}

		public override void EncodeImage(Stream stream, Bitmap image)
		{
			throw new NotImplementedException();
		}

		// --------------------------------------------------------------------------------
		// S3TC DXT1 / DXT5 Texture Decompression Routines
		// Author: Benjamin Dobell - http://www.glassechidna.com.au
		//
		// Feel free to use these methods in any open-source, freeware or commercial
		// projects. It's not necessary to credit me however I would be grateful if you
		// chose to do so. I'll also be very interested to hear what projects make use of
		// this code. Feel free to drop me a line via the contact form on the Glass Echidna
		// website.
		//
		// NOTE: The code was written for a little endian system where sizeof(long) == 4.
		// --------------------------------------------------------------------------------

		void DecompressBlockDXT5(int x, int y, int width, EndianReader reader, Bitmap image)
		{
			byte alpha0 = reader.ReadByte();
			byte alpha1 = reader.ReadByte();
 
			byte[] bits = reader.ReadBytes(6);
			int alphaCode1 = bits[2] | (bits[3] << 8) | (bits[4] << 16) | (bits[5] << 24);
			int alphaCode2 = bits[0] | (bits[1] << 8);
 
			ushort color0 = reader.ReadUInt16();
			ushort color1 = reader.ReadUInt16();
 
			int temp;
 
			temp = (color0 >> 11) * 255 + 16;
			byte r0 = (byte)((temp/32 + temp)/32);
			temp = ((color0 & 0x07E0) >> 5) * 255 + 32;
			byte g0 = (byte)((temp/64 + temp)/64);
			temp = (color0 & 0x001F) * 255 + 16;
			byte b0 = (byte)((temp/32 + temp)/32);
 
			temp = (color1 >> 11) * 255 + 16;
			byte r1 = (byte)((temp/32 + temp)/32);
			temp = ((color1 & 0x07E0) >> 5) * 255 + 32;
			byte g1 = (byte)((temp/64 + temp)/64);
			temp = (color1 & 0x001F) * 255 + 16;
			byte b1 = (byte)((temp/32 + temp)/32);
 
			int code = reader.ReadInt32(); // Possibly little endianify this
 
			for (int j=0; j < 4; j++)
			{
				for (int i=0; i < 4; i++)
				{
					int alphaCodeIndex = 3*(4*j+i);
					int alphaCode;
 
					if (alphaCodeIndex <= 12)
					{
						alphaCode = (alphaCode2 >> alphaCodeIndex) & 0x07;
					}
					else if (alphaCodeIndex == 15)
					{
						alphaCode = (alphaCode2 >> 15) | ((alphaCode1 << 1) & 0x06);
					}
					else // alphaCodeIndex >= 18 && alphaCodeIndex <= 45
					{
						alphaCode = (alphaCode1 >> (alphaCodeIndex - 16)) & 0x07;
					}
 
					byte finalAlpha;
					if (alphaCode == 0)
					{
						finalAlpha = alpha0;
					}
					else if (alphaCode == 1)
					{
						finalAlpha = alpha1;
					}
					else
					{
						if (alpha0 > alpha1)
						{
							finalAlpha = (byte)(((8-alphaCode)*alpha0 + (alphaCode-1)*alpha1)/7);
						}
						else
						{
							if (alphaCode == 6)
								finalAlpha = 0;
							else if (alphaCode == 7)
								finalAlpha = 255;
							else
								finalAlpha = (byte)(((6-alphaCode)*alpha0 + (alphaCode-1)*alpha1)/5);
						}
					}
 
					byte colorCode = (byte)((code >> 2*(4*j+i)) & 0x03);
 
					Color finalColor = Color.White;
					switch (colorCode)
					{
						case 0:
							finalColor = Color.FromArgb(finalAlpha, r0, g0, b0);
							break;
						case 1:
							finalColor = Color.FromArgb(finalAlpha, r1, g1, b1);
							break;
						case 2:
							finalColor = Color.FromArgb(finalAlpha, (2 * r0 + r1) / 3, (2 * g0 + g1) / 3, (2 * b0 + b1) / 3);
							break;
						case 3:
							finalColor = Color.FromArgb(finalAlpha, (r0 + 2 * r1) / 3, (g0 + 2 * g1) / 3, (b0 + 2 * b1) / 3);
							break;
					}

					if (x + i < width)
						image.SetPixel(x + i, y + j, finalColor);
				}
			}
		}

		void BlockDecompressImageDXT5(int width, int height, EndianReader reader, Bitmap image)
		{
			int blockCountX = (width + 3) / 4;
			int blockCountY = (height + 3) / 4;
			int blockWidth = (width < 4) ? width : 4;
			int blockHeight = (height < 4) ? height : 4;
 
			for (int j = 0; j < blockCountY; j++) {
				for (int i = 0; i < blockCountX; i++)
					DecompressBlockDXT5(i * 4, j * 4, width, reader, image);
			}
		}
	}
}
