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

		public static readonly AudioFormatBink Instance;
		static AudioFormatBink()
		{
			Instance = new AudioFormatBink();
		}

		public override int ID {
			get { return 0x0c; }
		}

		public override string Name {
			get { return "Bink Audio"; }
		}

		public override bool Writable {
			get { return false; }
		}

		public override bool Readable {
			get { return true; }
		}

		public override AudioFormat DecodeAudio(FormatData data)
		{
			if (!data.HasStream(this, FormatName) && !data.HasStream(this, AudioName))
				throw new FormatException();

			AudioFormat format = AudioFormat.Create(data.GetStream(this, FormatName));
			format.Decoder = new RawkAudio.Decoder(data.GetStream(this, AudioName), RawkAudio.Decoder.AudioFormat.BinkAudio);
			if (data.HasStream(this, PreviewName)) {
				Stream preview = data.GetStream(this, PreviewName);
				IDecoder decoder = new RawkAudio.Decoder(preview, RawkAudio.Decoder.AudioFormat.BinkAudio);
				MultiDecoder multi = new MultiDecoder(RawkAudio.Decoder.BufferSize);
				multi.AddDecoder(format.Decoder);
				multi.AddDecoder(decoder);
				format.Decoder = multi;
			}

			return format;
		}

		public override void EncodeAudio(AudioFormat data, FormatData destination, ProgressIndicator progress)
		{
			throw new NotImplementedException();
		}

		public void Create(FormatData data, Stream audio, Stream preview)
		{
			AudioFormat format = new AudioFormat();

			format.Save(data.AddStream(this, FormatName));

			data.SetStream(this, AudioName, audio);
			if (preview != null)
				data.SetStream(this, PreviewName, preview);
		}
	}
}
