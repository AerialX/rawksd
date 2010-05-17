using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

namespace ConsoleHaxx.Common
{
	public class FSB
	{
		public List<Sample> Samples;

		public enum FsbType
		{
			FSB2 = 0x46534232,
			FSB3 = 0x46534233,
			FSB4 = 0x46534234
		}

		public FSB()
		{
			Samples = new List<Sample>();
		}

		public FSB(Stream stream) : this()
		{
			EndianReader reader = new EndianReader(stream, Endianness.LittleEndian);

			FsbType type = (FsbType)reader.ReadUInt32(Endianness.BigEndian);
			int headersize;
			switch (type) {
				case FsbType.FSB2:
					headersize = 0x10;
					break;
				case FsbType.FSB3:
					headersize = 0x18;
					break;
				case FsbType.FSB4:
					headersize = 0x30;
					break;
				default:
					throw new FormatException();
			}

			int samplecount = reader.ReadInt32();
			int sampleheadersize = reader.ReadInt32();
			int datasize = reader.ReadInt32();

			reader.Position = headersize; // Ignore the rest, it's not very interesting

			long position = headersize + sampleheadersize;
			for (int i = 0; i < samplecount; i++) {
				Sample sample = Sample.Create(reader, position);
				Samples.Add(sample);
				position += sample.Size;
			}
		}

		public class Sample
		{
			public string Name;
			public uint Samples;
			public uint Size;
			public uint LoopStart;
			public uint LoopEnd;

			public uint Mode;
			public int DefFrequency;
			public ushort DefVolume;
			public short DefBalance;
			public ushort DefPri;
			public ushort Channels;

			public float MinDistance;
			public float MaxDistance;
			public int VarFrequency;
			public ushort VarVolume;
			public short VarBalance;

			public Stream Data;

			public static Sample Create(EndianReader reader, long position)
			{
				Sample sample = new Sample();

				ushort size = reader.ReadUInt16();
				sample.Name = reader.ReadString(0x1E);
				size -= 0x20;

				sample.Samples = reader.ReadUInt32();
				sample.Size = reader.ReadUInt32();
				sample.LoopStart = reader.ReadUInt32();
				sample.LoopEnd = reader.ReadUInt32();
				size -= 0x10;

				sample.Data = new Substream(reader, position, sample.Size);

				if (size == 0)
					return sample;

				sample.Mode = reader.ReadUInt32();
				sample.DefFrequency = reader.ReadInt32();
				sample.DefVolume = reader.ReadUInt16();
				sample.DefBalance = reader.ReadInt16();
				sample.DefPri = reader.ReadUInt16();
				sample.Channels = reader.ReadUInt16();
				size -= 0x10;

				if (size == 0)
					return sample;

				sample.MinDistance = reader.ReadFloat32();
				sample.MaxDistance = reader.ReadFloat32();
				sample.VarFrequency = reader.ReadInt32();
				sample.VarVolume = reader.ReadUInt16();
				sample.VarBalance = reader.ReadInt16();

				if (size == 0)
					return sample;

				throw new FormatException();
			}
		}
	}
}
