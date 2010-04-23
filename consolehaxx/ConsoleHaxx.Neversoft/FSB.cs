using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.Neversoft
{
	public class FSB
	{
		public const int Magic = 0x33425346;

		public uint Size;
		public uint ChunkSize;
		public uint Frequency;
		public ushort Channels;
		public string Filename;
		public Stream Data;

		public FSB(EndianReader reader)
		{
			if (reader.ReadUInt32() != Magic)
				throw new NotSupportedException();

			reader.ReadUInt32(); //1 sample in file
			reader.ReadUInt32(); //sampleheader length
			Size = reader.ReadUInt32(); //sampleheader length
			reader.ReadUInt32(); //header version 3.1
			reader.ReadUInt32(); //global mode flags

			//SampleHeader (80 byte version)
			reader.ReadUInt16(); //sampleheader length

			Filename = Util.Encoding.GetString(reader.ReadBytes(30)).Trim('\0'); // Filename

			ChunkSize = reader.ReadUInt32(); //sampleheader length
			Size = reader.ReadUInt32(); //compressed bytes
			reader.ReadUInt32(); //loop start
			reader.ReadUInt32(); //loop end
			reader.ReadUInt32(); //sample mode
			Frequency = reader.ReadUInt32(); //frequency
			reader.ReadUInt16(); //default volume
			reader.ReadUInt16(); //default pan
			reader.ReadUInt16(); //default pri
			Channels = reader.ReadUInt16(); //channels
			reader.ReadUInt32(); //min distance (float)
			reader.ReadUInt32(); //max distance (float)
			reader.ReadUInt32(); //varfreq
			reader.ReadUInt16(); //varvol
			reader.ReadUInt16(); //varpan

			reader.ReadUInt16(); // SOMETHING

			Data = new Substream(reader.Base, reader.Position, reader.Base.Length - reader.Position);
		}
	}
}
