using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using ConsoleHaxx.Harmonix;
using ConsoleHaxx.Common;
using System.IO;

namespace ConsoleHaxx.RawkSD
{
	public class AudioFormatMogg : IAudioFormat
	{
		public const string AudioName = "audio";

		public static readonly AudioFormatMogg Instance;
		static AudioFormatMogg()
		{
			Instance = new AudioFormatMogg();
		}

		public override int ID {
			get { return 0x0b; }
		}

		public override string Name {
			get { return "Rock Band OGG Audio"; }
		}

		public override bool Writable {
			get { return true; }
		}

		public override bool Readable {
			get { return true; }
		}

		public override AudioFormat DecodeAudio(FormatData data)
		{
			if (!data.HasStream(this, AudioName))
				throw new FormatException();

			AudioFormat format = MetadataFormatHarmonix.GetAudioFormat(data);
			Stream audio = new CryptedMoggStream(new EndianReader(data.GetStream(this, AudioName), Endianness.LittleEndian));
			format.Decoder = new RawkAudio.Decoder(audio, RawkAudio.Decoder.AudioFormat.VorbisOgg);

			return format;
		}

		public override void EncodeAudio(AudioFormat data, FormatData destination, ProgressIndicator progress)
		{
			CryptedMoggStream audio = new CryptedMoggStream(new EndianReader(destination.AddStream(this, AudioName), Endianness.LittleEndian));
			audio.WriteHeader();

			RawkAudio.Encoder encoder = new RawkAudio.Encoder(audio, data.Decoder.Channels, data.Decoder.SampleRate);
			AudioFormat.Transcode(encoder, data.Decoder, progress);
			destination.CloseStream(this, AudioName);

			MetadataFormatHarmonix.SaveAudioFormat(data, destination, ID);
		}

		public override bool CanRemux(IFormat format)
		{
			return format is AudioFormatOgg;
		}

		public override void Remux(IFormat format, FormatData data, FormatData destination, ProgressIndicator progress)
		{
			if (!(format is AudioFormatOgg))
				throw new FormatException();

			CryptedMoggStream audio = new CryptedMoggStream(new EndianReader(destination.AddStream(this, AudioName), Endianness.LittleEndian));
			audio.WriteHeader();

			Util.StreamCopy(audio, AudioFormatOgg.Instance.GetAudioStream(data));

			MetadataFormatHarmonix.SaveAudioFormat(MetadataFormatHarmonix.GetAudioFormat(data), destination, ID);

			destination.CloseStream(this, AudioName);
		}

		public void Create(FormatData data, Stream audio)
		{
			data.SetStream(this, AudioName, audio);
		}

		public Stream GetDecryptedAudioStream(FormatData data)
		{
			return new CryptedMoggStream(new EndianReader(data.GetStream(this, AudioName), Endianness.LittleEndian));
		}
	}
}
