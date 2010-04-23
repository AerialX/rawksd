using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using ConsoleHaxx.Common;
using System.IO;

namespace ConsoleHaxx.Harmonix
{
	public class InstrumentBank
	{
		public const uint SampleMagic = 0x53414D50; // SAMP
		public const uint SampleNameMagic = 0x53414E4D; // SANM
		public const uint SamplePathMagic = 0x5341464E; // SAFN
		public const uint BankMagic = 0x42414E4B; // BANK
		public const uint BankNameMagic = 0x424B4E4D; // BKNM
		public const uint InstrumentMagic = 0x494E5354; // INST
		public const uint InstrumentNameMagic = 0x494E4E4D; // INNM
		public const uint SoundMagic = 0x53444553; // SDES
		public const uint SoundNameMagic = 0x53444E4D; // SDNM

		public class Sample
		{
			public Sample()
			{
				Zeroes = new byte[0x06];
			}

			public int Channels; // Or Tag; Always 0x01?
			public int SampleRate;
			public byte[] Zeroes; // byte[0x06]
			public int Offset;
			public string Name;
			public string Path;
		}

		public class Bank
		{
			public Bank()
			{
				Flags = new byte[0x05];
			}

			public string Name;
			public int Tag; // Always 0x01?
			public byte[] Flags; // byte[0x05]

			public byte ID { get { return Flags[0x00]; } set { Flags[0x00] = value; } }
			public byte Volume { get { return Flags[0x02]; } set { Flags[0x02] = value; } }
			public byte Instruments { get { return Flags[0x03]; } set { Flags[0x03] = value; } }
		}

		public class Instrument
		{
			public Instrument()
			{
				Flags = new byte[0x08];
			}

			public string Name;
			public int Tag; // Always 0x01?
			public byte[] Flags; // byte[0x08]

			public byte Balance { get { return Flags[0x02]; } set { Flags[0x02] = value; } }
			public byte Volume { get { return Flags[0x03]; } set { Flags[0x03] = value; } }
			public byte ID { get { return Flags[0x00]; } set { Flags[0x00] = value; } }
			public byte Sounds { get { return Flags[0x06]; } set { Flags[0x06] = value; } }
		}

		public class Sound
		{
			public Sound()
			{
				Flags = new byte[0x19];
			}

			public string Name;
			public int Tag; // Always 0x03?
			public byte[] Flags; // byte[0x19]

			public byte ID0 { get { return Flags[0x00]; } set { Flags[0x00] = value; } }
			public byte ID1 { get { return Flags[0x01]; } set { Flags[0x01] = value; } }
			public byte ID2 { get { return Flags[0x02]; } set { Flags[0x02] = value; } }

			public byte Volume { get { return Flags[0x10]; } set { Flags[0x10] = value; } }
			public byte Balance { get { return Flags[0x11]; } set { Flags[0x11] = value; } }
			public byte Sample { get { return Flags[0x12]; } set { Flags[0x12] = value; } }

			public byte Unknown { get { return Flags[0x04]; } set { Flags[0x04] = value; } } // Synthesized has this with duplicate Sound entries, only difference is this being 0x07 and -0x07
		}

		public List<Sample> Samples;
		public List<Bank> Banks;
		public List<Instrument> Instruments;
		public List<Sound> Sounds;

		public InstrumentBank()
		{
			Samples = new List<Sample>();
			Banks = new List<Bank>();
			Instruments = new List<Instrument>();
			Sounds = new List<Sound>();
		}

		public static InstrumentBank Create(EndianReader globalreader)
		{
			InstrumentBank ret = new InstrumentBank();

			while (!globalreader.Base.EOF()) {
				uint magic = new EndianReader(globalreader, Endianness.BigEndian).ReadUInt32();
				uint size = globalreader.ReadUInt32();
				EndianReader reader = new EndianReader(new Substream(globalreader, globalreader.Position, size), globalreader.Endian);

				switch (magic) {
					case SampleMagic: {
						while (!reader.Base.EOF()) {
							Sample sample = new Sample();
							uint entrysize = reader.ReadUInt32();
							if (entrysize != 0x12)
								throw new FormatException();
							sample.Channels = reader.ReadInt32();
							sample.SampleRate = reader.ReadInt32();
							reader.Read(sample.Zeroes, 0, sample.Zeroes.Length);
							sample.Offset = reader.ReadInt32();

							ret.Samples.Add(sample);
						}
						break; }
					case SampleNameMagic: {
						int sample = 0;

						if (reader.ReadInt32() != 1)
							throw new FormatException();

						while (!reader.Base.EOF()) {
							int stringsize = reader.ReadInt32();
							ret.Samples[sample].Name = reader.ReadString(stringsize);

							sample++;
						}
						break; }
					case SamplePathMagic: {
						int sample = 0;

						if (reader.ReadInt32() != 1)
							throw new FormatException();

						while (!reader.Base.EOF()) {
							int stringsize = reader.ReadInt32();
							ret.Samples[sample].Path = reader.ReadString(stringsize);

							sample++;
						}
						break; }
					case BankMagic: {
						while (!reader.Base.EOF()) {
							Bank bank = new Bank();
							uint entrysize = reader.ReadUInt32();
							if (entrysize != 0x09)
								throw new FormatException();
							bank.Tag = reader.ReadInt32();
							reader.Read(bank.Flags, 0, bank.Flags.Length);

							ret.Banks.Add(bank);
						}
						break; }
					case BankNameMagic: {
						int bank = 0;

						if (reader.ReadInt32() != 1)
							throw new FormatException();

						while (!reader.Base.EOF()) {
							int stringsize = reader.ReadInt32();
							ret.Banks[bank].Name = reader.ReadString(stringsize);

							bank++;
						}
						break; }
					case InstrumentMagic: {
						while (!reader.Base.EOF()) {
							Instrument instrument = new Instrument();
							uint entrysize = reader.ReadUInt32();
							if (entrysize != 0x0C)
								throw new FormatException();

							instrument.Tag = reader.ReadInt32();
							
							reader.Read(instrument.Flags, 0, instrument.Flags.Length);

							ret.Instruments.Add(instrument);
						}
						break; }
					case InstrumentNameMagic: {
						int ins = 0;

						if (reader.ReadInt32() != 1)
							throw new FormatException();

						while (!reader.Base.EOF()) {
							int stringsize = reader.ReadInt32();
							ret.Instruments[ins].Name = reader.ReadString(stringsize);

							ins++;
						}
						break; }
					case SoundMagic: {
						while (!reader.Base.EOF()) {
							Sound sound = new Sound();
							uint entrysize = reader.ReadUInt32();
							if (entrysize == 0x1C)
								throw new NotImplementedException();
								//sound.Flags = new byte[0x18];
							else if (entrysize != 0x1D)
								throw new FormatException();
							sound.Tag = reader.ReadInt32();
							reader.Read(sound.Flags, 0, sound.Flags.Length);

							ret.Sounds.Add(sound);
						}
						break; }
					case SoundNameMagic: {
						int sound = 0;

						if (reader.ReadInt32() != 1)
							throw new FormatException();

						while (!reader.Base.EOF()) {
							int stringsize = reader.ReadInt32();
							ret.Sounds[sound].Name = reader.ReadString(stringsize);

							sound++;
						}
						break; }
					default:
						throw new FormatException();
				}
			}

			return ret;
		}

		private class SampleOffsetComparer : IComparer<Sample>
		{
			public int Compare(Sample x, Sample y)
			{
				return x.Offset.CompareTo(y.Offset);
			}
		}

		public short[] Decode(Stream nse, Sample sample, long samplecount)
		{
			byte[] data = new byte[samplecount];
			nse.Position = sample.Offset;
			nse.Read(data, 0, data.Length);

			int[] state = new int[2];
			short[] samples = new short[samplecount];

			long count = VgsADPCM.Decompress(state, data, samples);

			short[] ret = new short[count];
			Array.Copy(samples, ret, count);

			return ret;
		}

		public short[] Decode(Stream nse, Sample sample)
		{
			return Decode(nse, sample, VgsADPCM.BytesToSamples(nse.Length));
		}
	}
}
