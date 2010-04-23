using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace ConsoleHaxx.RawkSD
{
	public class AudioFormatFFmpeg : IAudioFormat
	{
		public static readonly AudioFormatFFmpeg Instance;
		static AudioFormatFFmpeg()
		{
			Instance = new AudioFormatFFmpeg();
		}

		public override int ID {
			get { return 0x0f; }
		}

		public override string Name {
			get { return "FFmpeg Audio"; }
		}

		public override bool Writable {
			get { return false; }
		}

		public override bool Readable {
			get { return true; }
		}

		public override AudioFormat DecodeAudio(FormatData data)
		{
			throw new NotImplementedException();
		}

		public override void EncodeAudio(AudioFormat data, FormatData destination, ProgressIndicator progress)
		{
			throw new NotImplementedException();
		}
	}
}
