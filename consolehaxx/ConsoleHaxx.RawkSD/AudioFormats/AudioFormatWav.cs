using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

namespace ConsoleHaxx.RawkSD
{
	public class AudioFormatWav : IAudioFormat
	{
		public const string FormatName = "map";
		public const string AudioName = "audio";
		public const string PreviewName = "preview";

		public static readonly AudioFormatWav Instance;
		static AudioFormatWav()
		{
			Instance = new AudioFormatWav();
		}

		public override int ID {
			get { throw new NotImplementedException(); }
		}

		public override string Name {
			get { return "WAV Audio"; }
		}

		public override bool Writable {
			get { return false; }
		}

		public override bool Readable {
			get { return true; }
		}

		public AudioFormat DecodeFormat(FormatData data)
		{
			Stream stream = data.GetStream(this, FormatName);
			if (stream == null)
				return null;
			AudioFormat format = AudioFormat.Create(stream);
			data.CloseStream(stream);
			return format;
		}

		public override AudioFormat DecodeAudio(FormatData data, ProgressIndicator progress)
		{
			if (!data.HasStream(this, FormatName) && !data.HasStream(this, AudioName))
				throw new FormatException();

			AudioFormat format = DecodeFormat(data);
			format.Decoder = new RawkAudio.Decoder(data.GetStream(this, AudioName), RawkAudio.Decoder.AudioFormat.Wav);

			return format;
		}

		public override void EncodeAudio(AudioFormat data, FormatData destination, ProgressIndicator progress)
		{
			throw new NotImplementedException();
		}

		public Stream GetFormatStream(FormatData formatdata)
		{
			throw new NotImplementedException();
		}

		public Stream GetAudioStream(FormatData data)
		{
			return data.GetStream(this, AudioName);
		}

		public Stream GetPreviewStream(FormatData data)
		{
			return data.GetStream(this, PreviewName);
		}
	}
}
