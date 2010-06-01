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

		public static AudioFormatVGS Instance;
		public static void Initialise()
		{
			Instance = new AudioFormatVGS();
			Platform.AddFormat(Instance);
		}

		public override int ID {
			get { return 0x0a; }
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

		public override AudioFormat DecodeAudioFormat(FormatData data)
		{
			Stream stream = data.GetStream(this, FormatName);
			if (stream == null)
				return HarmonixMetadata.GetAudioFormat(data.Song);
			AudioFormat format = AudioFormat.Create(stream);
			data.CloseStream(stream);
			return format;
		}

		public override AudioFormat DecodeAudio(FormatData data, ProgressIndicator progress)
		{
			AudioFormat format = DecodeAudioFormat(data);
			Stream stream = data.GetStream(this, AudioName);
			format.Decoder = new RawkAudio.Decoder(stream, RawkAudio.Decoder.AudioFormat.Vgs);

			format.SetDisposeStreams(data, new Stream[] { stream });

			return format;
		}

		public override void EncodeAudio(AudioFormat data, FormatData destination, ProgressIndicator progress)
		{
			throw new NotImplementedException();
		}

		public void Create(FormatData data, Stream audio, AudioFormat format)
		{
			data.SetStream(this, AudioName, audio);
			if (format != null) {
				Stream formatstream = data.AddStream(this, FormatName);
				format.Save(formatstream);
				data.CloseStream(formatstream);
			}
		}
	}
}
