using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.Wii
{
	public class Yaz0
	{
		public const uint Magic = 0x59617A30;

		public static Stream Create(Stream stream)
		{
			EndianReader reader = new EndianReader(stream, Endianness.BigEndian);
			if (reader.ReadUInt32() != Magic)
				throw new FormatException();

			uint totalsize = reader.ReadUInt32();
			if (reader.ReadUInt32() != 0 || reader.ReadUInt32() != 0)
				throw new FormatException();

			Stream ostream = new TemporaryStream();
			EndianReader writer = new EndianReader(ostream, Endianness.BigEndian);

			uint validBitCount = 0; //number of valid bits left in "code" byte
			byte currCodeByte = 0;
			while (ostream.Length < totalsize) {
				//read new "code" byte if the current one is used up
				if (validBitCount == 0) {
					currCodeByte = reader.ReadByte();
					validBitCount = 8;
				}

				if ((currCodeByte & 0x80) != 0) {
					//straight copy
					writer.Write(reader.ReadByte());
				} else {
					//RLE part
					byte byte1 = reader.ReadByte();
					byte byte2 = reader.ReadByte();

					int dist = ((byte1 & 0xF) << 8) | byte2;
					long copySource = writer.Position - (dist + 1);

					int numBytes = byte1 >> 4;
					if (numBytes == 0) {
						numBytes = reader.ReadByte() + 0x12;
					} else
						numBytes += 2;

					//copy run
					for (int i = 0; i < numBytes; ++i) {
						long position = writer.Position;
						writer.Position = copySource;
						byte b = writer.ReadByte();
						writer.Position = position;
						writer.Write(b);
						copySource++;
					}
				}

				//use next bit from "code" byte
				currCodeByte <<= 1;
				validBitCount -= 1;
			}

			ostream.Position = 0;

			return ostream;
		}
	}
}
