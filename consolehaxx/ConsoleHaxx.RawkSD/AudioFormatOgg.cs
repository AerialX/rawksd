using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using ConsoleHaxx.Common;
using System.IO;

namespace ConsoleHaxx.RawkSD
{
	public class AudioFormatOgg : IAudioFormat
	{
		public const string FormatName = "map";
		public const string AudioName = "audio";

		public static readonly AudioFormatOgg Instance;
		static AudioFormatOgg()
		{
			Instance = new AudioFormatOgg();
		}

		public override int ID {
			get { return 0x0a; }
		}

		public override string Name {
			get { return "OGG Vorbis Audio"; }
		}

		public override bool Writable {
			get { return true; }
		}

		public override bool Readable {
			get { return true; }
		}

		public override AudioFormat DecodeAudio(FormatData data)
		{
			if (!data.HasStream(this, FormatName) && !data.HasStream(this, AudioName))
				throw new FormatException();

			AudioFormat format = AudioFormat.Create(data.GetStream(this, FormatName));
			format.Decoder = new RawkAudio.Decoder(data.GetStream(this, AudioName), RawkAudio.Decoder.AudioFormat.VorbisOgg);

			return format;
		}

		public override void EncodeAudio(AudioFormat data, FormatData destination, ProgressIndicator progress)
		{
			RawkAudio.Encoder encoder = new RawkAudio.Encoder(destination.AddStream(this, AudioName), data.Decoder.Channels, data.Decoder.SampleRate);
			AudioFormat.Transcode(encoder, data.Decoder, progress);
			destination.CloseStream(this, AudioName);

			data.Save(destination.AddStream(this, FormatName));
			destination.CloseStream(this, FormatName);
		}

		public override bool CanRemux(IFormat format)
		{
			return format is AudioFormatMogg;
		}

		public override void Remux(IFormat format, FormatData data, FormatData destination, ProgressIndicator progress)
		{
			if (!(format is AudioFormatMogg))
				throw new FormatException();

			Util.StreamCopy(destination.AddStream(this, AudioName), AudioFormatMogg.Instance.GetDecryptedAudioStream(data));

			MetadataFormatHarmonix.SaveAudioFormat(MetadataFormatHarmonix.GetAudioFormat(data), destination, ID);

			destination.CloseStream(this, AudioName);
		}

		public Stream GetAudioStream(FormatData data)
		{
			return data.GetStream(this, AudioName);
		}
	}
}
