using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace ConsoleHaxx.RawkSD
{
	public class AudioFormatBeatlesBink : IAudioFormat
	{
		public static readonly AudioFormatBeatlesBink Instance;
		static AudioFormatBeatlesBink()
		{
			Instance = new AudioFormatBeatlesBink();
			Platform.AddFormat(Instance);
		}

		public override int ID {
			get { return 0x0e; }
		}

		public override string Name {
			get { return "The Beatles: Rock Band Wii Bink Audio"; }
		}

		public override bool Writable {
			get { return false; }
		}

		public override bool Readable {
			get { return false; }
		}

		public override AudioFormat DecodeAudio(FormatData data, ProgressIndicator progress)
		{
			throw new NotImplementedException();
		}

		public override void EncodeAudio(AudioFormat data, FormatData destination, ProgressIndicator progress)
		{
			throw new NotImplementedException();
		}
	}
}
