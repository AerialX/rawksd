using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Drawing;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.Wii
{
	public class NullEncoding : PixelEncoding
	{
		public override uint ID
		{
			get { return 0xAAE4; }
		}

		public override uint BitsPerPixel
		{
			get { return 0x04; }
		}

		public override Size Tilesize
		{
			get { return new Size(1, 1); }
		}

		public override Bitmap DecodeImage(Stream stream, uint width, uint height)
		{
			return new Bitmap((int)width, (int)height);
		}

		public override void EncodeImage(Stream stream, Bitmap image)
		{
			throw new NotImplementedException();
		}

		public override uint GetDataSize(uint width, uint height)
		{
			return 0;
		}
	}
}
