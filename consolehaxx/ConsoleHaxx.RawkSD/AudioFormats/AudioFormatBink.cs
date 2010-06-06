using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.RawkSD
{
	public class AudioFormatBink : IAudioFormat
	{
		public const string FormatName = "map";
		public const string AudioName = "audio";
		public const string PreviewName = "preview";

		public static AudioFormatBink Instance;
		public static void Initialise()
		{
			Instance = new AudioFormatBink();
			Platform.AddFormat(Instance);
		}

		public override int ID {
			get { return 0x02; }
		}

		public override string Name {
			get { return "Guitar Hero Bink Audio"; }
		}

		public override bool Writable {
			get { return false; }
		}

		public override bool Readable {
			get { return true; }
		}

		public AudioFormat DecodeFormat(FormatData data)
		{
			Stream stream = GetFormatStream(data);
			if (stream == null)
				return NeversoftMetadata.GetAudioFormat(data.Song);
			AudioFormat format = AudioFormat.Create(stream);
			data.CloseStream(stream);
			return format;
		}

		public override AudioFormat DecodeAudio(FormatData data, ProgressIndicator progress)
		{
			AudioFormat format = DecodeFormat(data);
			Stream audio = data.GetStream(this, AudioName);
			Stream preview = null;
			format.Decoder = new RawkAudio.Decoder(audio, RawkAudio.Decoder.AudioFormat.BinkAudio);
			if (data.HasStream(this, PreviewName)) {
				preview = data.GetStream(this, PreviewName);
				IDecoder decoder = new RawkAudio.Decoder(preview, RawkAudio.Decoder.AudioFormat.BinkAudio);
				MultiDecoder multi = new MultiDecoder(RawkAudio.Decoder.BufferSize);
				multi.AddDecoder(format.Decoder);
				multi.AddDecoder(decoder);
				format.Decoder = multi;
			}

			format.SetDisposeStreams(data, new Stream[] { audio, preview });

			Game game = data.Song.Game;
			if (NeversoftMetadata.IsGuitarHero4(game) || NeversoftMetadata.IsGuitarHero5(game))
				format.Decoder = new AmplifyDecoder(format.Decoder, 1.30f);

			return format;
		}

		public override void EncodeAudio(AudioFormat data, FormatData destination, ProgressIndicator progress)
		{
			throw new NotImplementedException();
		}

		public void Create(FormatData data, Stream audio, Stream preview, AudioFormat format)
		{
			if (format != null) {
				Stream formatstream = data.AddStream(this, FormatName);
				format.Save(formatstream);
				data.CloseStream(formatstream);
			}

			data.SetStream(this, AudioName, audio);
			if (preview != null)
				data.SetStream(this, PreviewName, preview);
		}

		public Stream GetFormatStream(FormatData data)
		{
			return data.GetStream(this, FormatName);	
		}
	}
}
