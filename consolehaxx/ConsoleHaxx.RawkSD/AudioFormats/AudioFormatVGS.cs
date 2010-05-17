using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.RawkSD
{
	public class AudioFormatVGS : IAudioFormat
	{
		public const string FormatName = "map";
		public const string AudioName = "audio";

		public static readonly AudioFormatVGS Instance;
		static AudioFormatVGS()
		{
			Instance = new AudioFormatVGS();
			Platform.AddFormat(Instance);
		}

		public override int ID {
			get { return 0x08; }
		}

		public override string Name {
			get { return "VGS Audio"; }
		}

		public override bool Writable {
			get { return false; }
		}

		public override bool Readable {
			get { return true; }
		}

		public override AudioFormat DecodeAudio(FormatData data, ProgressIndicator progress)
		{
			if (!data.HasStream(this, FormatName) && !data.HasStream(this, AudioName))
				throw new FormatException();

			AudioFormat format = AudioFormat.Create(data.GetStream(this, FormatName));
			format.Decoder = new RawkAudio.Decoder(data.GetStream(this, AudioName), RawkAudio.Decoder.AudioFormat.Vgs);

			AudioFormat.AddPreviewDecoder(data, format, progress);

			return format;
		}

		public override void EncodeAudio(AudioFormat data, FormatData destination, ProgressIndicator progress)
		{
			throw new NotImplementedException();
		}

		public void Create(FormatData data, Stream audio, AudioFormat format)
		{
			data.SetStream(this, AudioName, audio);
			Stream formatstream = data.AddStream(this, FormatName);
			format.Save(formatstream);
			data.CloseStream(formatstream);
		}
	}
}
