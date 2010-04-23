using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using ConsoleHaxx.Common;
using System.Drawing;
using ConsoleHaxx.Wii;

namespace ConsoleHaxx.Graces
{
	public class Txm
	{
		const uint Magic = 0x00020000;

		public List<Image> Images;

		public uint Unknown;

		public class Image
		{
			public uint Width;
			public uint Height;

			public uint Unknown1;
			public byte Unknown2;
			public byte Unknown3;

			public PixelEncoding PrimaryEncoding;

			public string Name;

			public Stream PrimaryData;
			public Stream SecondaryData;

			public uint PrimaryLength { get { return PrimaryEncoding.GetDataSize(Width, Height); } }
			public Bitmap PrimaryBitmap { get { return PrimaryData == null || PrimaryEncoding == null ? null : PrimaryEncoding.DecodeImage(PrimaryData, Width, Height, SecondaryData, Unknown2 >> 4); } }
			public uint SecondaryLength {
				get {
					switch (Unknown3) {
						case 0x01: // CI palette
							switch (PrimaryEncoding.BitsPerPixel) {
								case 0x04:
									return 0x20;
								case 0x08:
									return 0x200;
								case 0x10:
									return 0x8000;
							}
							break;
						case 0x0A:
							return Width * Height / 4;
					}

					return 0;
				}
			}
		}

		public Txm()
		{
			Images = new List<Image>();
		}

		public Txm(Stream stream, Stream data) : this()
		{
			EndianReader reader = new EndianReader(stream, Endianness.BigEndian);
			if (reader.ReadUInt32() != Magic)
				throw new FormatException();

			uint filesize = reader.ReadUInt32();
			Unknown = reader.ReadUInt32();
			uint files = reader.ReadUInt32();
			ulong zero = reader.ReadUInt64();
			if (zero != 0)
				throw new FormatException();

			Dictionary<Image, long> nameoffsets = new Dictionary<Image, long>();
			for (uint i = 0; i < files; i++) {
				Image image = new Image();
				image.Width = reader.ReadUInt32();
				image.Height = reader.ReadUInt32();
				image.Unknown1 = reader.ReadUInt32();
				image.Unknown2 = reader.ReadByte();
				image.Unknown3 = reader.ReadByte();
				
				image.PrimaryEncoding = PixelEncoding.GetEncoding(reader.ReadUInt16());
				nameoffsets[image] = reader.Position + reader.ReadUInt32();
				uint offset1 = reader.ReadUInt32();
				uint offset2 = reader.ReadUInt32();

				image.PrimaryData = new Substream(data, offset1, image.PrimaryLength);
				if (offset2 > 0) // Best hack I have for determining whether there's a secondary image or not
					image.SecondaryData = new Substream(data, offset2, image.SecondaryLength);

				Images.Add(image);
			}

			foreach (var offset in nameoffsets) {
				stream.Position = offset.Value;
				offset.Key.Name = stream.ReadCString();
			}
		}

		public void Save(Stream txm, Stream data)
		{
			EndianReader writer = new EndianReader(txm, Endianness.BigEndian);

			long basetxm = txm.Position;
			long basedata = data.Position;

			long nameoffset = basetxm + 0x18 + 0x1C * Images.Count;

			int namelen = 0;
			foreach (var image in Images)
				namelen += image.Name.Length + 1;

			writer.Write(Magic);
			writer.Write((uint)Util.RoundUp(0x18 + 0x1C * Images.Count + namelen, 0x20));
			writer.Write(Unknown);
			writer.Write((uint)Images.Count);
			writer.Write((ulong)0);

			foreach (var image in Images) {
				writer.Write(image.Width);
				writer.Write(image.Height);
				writer.Write(image.Unknown1);
				writer.Write(image.Unknown2);
				writer.Write(image.Unknown3);
				writer.Write((ushort)image.PrimaryEncoding.ID);
				writer.Write((uint)(nameoffset - writer.Position));

				nameoffset += image.Name.Length + 1;

				writer.Write((uint)(data.Position - basedata));

				image.PrimaryData.Position = 0;
				Util.StreamCopy(data, image.PrimaryData, image.PrimaryLength);

				if (image.SecondaryData != null) {
					writer.Write((uint)(data.Position - basedata));

					image.SecondaryData.Position = 0;
					Util.StreamCopy(data, image.SecondaryData);
				} else
					writer.Write((uint)0);
			}

			foreach (var image in Images) {
				writer.Write(image.Name);
				writer.Write((byte)0);
			}

			writer.PadToMultiple(0x10);
		}
	}
}
