using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using ConsoleHaxx.Neversoft;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.RawkSD
{
	public class AudioFormatGH3WiiFSB : IAudioFormat
	{
		public const string WadName = "wad";
		public const string DatName = "dat";

		public static AudioFormatGH3WiiFSB Instance;
		public static void Initialise()
		{
			Instance = new AudioFormatGH3WiiFSB();
			Platform.AddFormat(Instance);
		}

		public override int ID {
			get { return 0x04; }
		}

		public override string Name {
			get { return "Guitar Hero 3 Wii ADPCM Audio"; }
		}

		public override bool Writable {
			get { return false; }
		}

		public override bool Readable {
			get { return true; }
		}

		public override AudioFormat DecodeAudio(FormatData data, ProgressIndicator progress)
		{
			if (!data.HasStream(this, WadName) && !data.HasStream(this, DatName))
				throw new FormatException();

			AudioFormat format = new AudioFormat();
			format.Decoder = new MultiDecoder(RawkAudio.Decoder.BufferSize);

			Stream wad = data.GetStream(this, WadName);
			Stream dat = data.GetStream(this, DatName);

			DatWad audio = new DatWad(new EndianReader(dat, Endianness.BigEndian), wad);

			data.CloseStream(dat);

			foreach (DatWad.Node node in audio.Nodes) {
				string name = node.Filename.Substring(node.Filename.LastIndexOf('_') + 1).ToLower().Trim();
				Instrument instrument = Instrument.Ambient;
				if ("preview.wav".StartsWith(name))
					instrument = Instrument.Preview;
				else if ("guitar.wav".StartsWith(name))
					instrument = Instrument.Guitar;
				else if ("rhythm.wav".StartsWith(name))
					instrument = Instrument.Bass;
				else if ("song.wav".StartsWith(name))
					instrument = Instrument.Ambient;
				else
					continue;

				IDecoder decoder = new RawkAudio.Decoder(node.Data, RawkAudio.Decoder.AudioFormat.FmodSoundBank);
				(format.Decoder as MultiDecoder).AddDecoder(decoder);

				for (int i = 0; i < decoder.Channels; i++)
					format.Mappings.Add(new AudioFormat.Mapping(0, 0, instrument));
			}

			format.SetDisposeStreams(data, new Stream[] { wad, dat });

			format.AutoBalance();

			return format;
		}

		public override void EncodeAudio(AudioFormat data, FormatData destination, ProgressIndicator progress)
		{
			throw new NotImplementedException();
		}

		public FormatData Create(Stream dat, Stream wad)
		{
			return Create(dat, wad, false); 
		}

		public FormatData Create(Stream dat, Stream wad, bool ownership)
		{
			FormatData data = new FormatData();

			data.StreamOwnership = ownership;
			Create(data, dat, wad);

			return data;
		}

		public void Create(FormatData data, Stream dat, Stream wad)
		{
			data.SetStream(this, DatName, dat);
			data.SetStream(this, WadName, wad);
		}
	}
}
