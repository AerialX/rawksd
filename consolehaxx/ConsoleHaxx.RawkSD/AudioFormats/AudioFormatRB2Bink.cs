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
		public const string PreviewName = "preview";
		public const string FormatName = "map";

		public static readonly AudioFormatRB2Bink Instance;
		static AudioFormatRB2Bink()
		{
			Instance = new AudioFormatRB2Bink();
			Platform.AddFormat(Instance);
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

		public AudioFormat DecodeFormat(FormatData data)
		{
			Stream stream = GetFormatStream(data);
			if (stream == null)
				return null;
			AudioFormat format = AudioFormat.Create(stream);
			data.CloseStream(stream);
			return format;
		}

		public override AudioFormat DecodeAudio(FormatData data, ProgressIndicator progress)
		{
			throw new NotImplementedException();
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

		public Stream GetAudioStream(FormatData data)
		{
			return data.GetStream(this, AudioName);
		}

		public Stream GetPreviewStream(FormatData data, ProgressIndicator progress)
		{
			if (!data.HasStream(this, PreviewName)) {
				AudioFormat format = new AudioFormat();
				format.Decoder = new RawkAudio.Decoder(GetAudioStream(data), RawkAudio.Decoder.AudioFormat.BinkAudio);
				Stream stream = AudioFormat.AddPreviewDecoder(data, format, progress);
				format.Decoder.Dispose();
				if (stream == null)
					return null;
				data.SetStream(this, PreviewName, stream);
			}

			return data.GetStream(this, PreviewName);
		}

		public Stream GetFormatStream(FormatData data)
		{
			return data.GetStream(this, FormatName);
		}
	}
}
