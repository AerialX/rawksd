using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using ConsoleHaxx.Harmonix;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.RawkSD
{
	public class AudioFormatRB2Bink : IAudioFormat
	{
		public const string AudioName = "audio";

		public static readonly AudioFormatRB2Bink Instance;
		static AudioFormatRB2Bink()
		{
			Instance = new AudioFormatRB2Bink();
		}

		public override int ID {
			get { return 0x0d; }
		}

		public override string Name {
			get { return "Rock Band 2 Wii Bink Audio"; }
		}

		public override bool Writable {
			get { return false; }
		}

		public override bool Readable {
			get { return false; }
		}

		public override AudioFormat DecodeAudio(FormatData data)
		{
			throw new NotImplementedException();
		}

		public override void EncodeAudio(AudioFormat data, FormatData destination, ProgressIndicator progress)
		{
			throw new NotImplementedException();
		}

		public void Create(FormatData data, Stream audio)
		{
			data.SetStream(this, AudioName, audio);
		}
	}
}
