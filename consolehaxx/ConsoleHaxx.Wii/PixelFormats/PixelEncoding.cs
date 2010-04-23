using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Drawing;
using System.IO;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.Wii
{
	public abstract class PixelEncoding
	{
		private static List<PixelEncoding> Encodings;

		static PixelEncoding()
		{
			Encodings = new List<PixelEncoding>();
			Encodings.Add(new I4());
			Encodings.Add(new I8());
			Encodings.Add(new IA4());
			Encodings.Add(new IA8());
			Encodings.Add(new RGB565());
			Encodings.Add(new RGB5A3());
			Encodings.Add(new RGBA8());
			Encodings.Add(new CI4());
			Encodings.Add(new CI8());
			Encodings.Add(new CI14X2());
			Encodings.Add(new CMPR());
		}

		public static PixelEncoding GetEncoding(uint id)
		{
			return Encodings.SingleOrDefault(e => e.ID == id);
		}

		public static PixelEncoding GetEncoding<T>() where T : PixelEncoding
		{
			return Encodings.SingleOrDefault(e => e is T);
		}

		public abstract uint ID { get; }
		public abstract uint BitsPerPixel { get; }
		public abstract Size Tilesize { get; }

		public virtual uint GetDataSize(uint width, uint height)
		{
			return (uint)Util.RoundUp(width, Tilesize.Width) * (uint)Util.RoundUp(height, Tilesize.Height) * BitsPerPixel / 8;
		}

		public virtual Bitmap DecodeImage(Stream stream, uint width, uint height) { return DecodeImage(stream, width, height, null, 0); }
		public virtual Bitmap DecodeImage(Stream stream, uint width, uint height, Stream data, int type) { return DecodeImage(stream, width, height); }
		public virtual void EncodeImage(Stream stream, Bitmap image) { EncodeImage(stream, image, null, 0); }
		public virtual void EncodeImage(Stream stream, Bitmap image, Stream palette, int type) { EncodeImage(stream, image); }
	}
}
