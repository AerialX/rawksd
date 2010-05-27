using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using ConsoleHaxx.Harmonix;
using ConsoleHaxx.Common;
using System.IO;

namespace ConsoleHaxx.RawkSD
{
	public class AudioFormatMogg : IAudioFormat
	{
		public const string FormatName = "map";
		public const string AudioName = "audio";

		public static AudioFormatMogg Instance;
		public static void Initialise()
		{
			Instance = new AudioFormatMogg();
			Platform.AddFormat(Instance);
		}

		public override int ID {
			get { return 0x05; }
		}

		public override string Name {
			get { return "Rock Band OGG Vorbis Audio"; }
		}

		public override bool Writable {
			get { return false; }
		}

		public override bool Readable {
			get { return true; }
		}
		
		public override AudioFormat DecodeAudioFormat(FormatData data)
		{
			Stream stream = GetFormatStream(data);
			if (stream == null)
				return HarmonixMetadata.GetAudioFormat(data.Song);
			AudioFormat format = AudioFormat.Create(stream);
			data.CloseStream(stream);
			return format;
		}

		public override AudioFormat DecodeAudio(FormatData data, ProgressIndicator progress)
		{
			if (!data.HasStream(this, AudioName))
				return null;

			AudioFormat format = DecodeAudioFormat(data);
			Stream audio = GetAudioStream(data);
			if (audio != null)
				audio = new CryptedMoggStream(audio);

			format.Decoder = AudioFormatOgg.Instance.DecodeOggAudio(audio);

			return format;
		}

		public override void EncodeAudio(AudioFormat data, FormatData destination, ProgressIndicator progress)
		{
			throw new NotImplementedException();
		}

		public override bool CanRemux(IFormat format)
		{
			return format is AudioFormatOgg;
		}
		
		public override void Remux(IFormat format, FormatData data, FormatData destination, ProgressIndicator progress)
		{
			if (!(format is AudioFormatOgg))
				throw new FormatException();

			bool musttranscode = AudioFormatOgg.Instance.GetAudioStreams(data).Count > 1;

			if (musttranscode) { // Remuxing isn't possible with more than one audio stream
				Platform.TranscodeOnly(format, data, AudioFormatMogg.Instance, destination, progress);
				return;
			}

			Util.StreamCopy(destination.AddStream(this, FormatName), AudioFormatOgg.Instance.GetFormatStream(data));

			CryptedMoggStream audio = new CryptedMoggStream(destination.AddStream(this, AudioName));
			audio.WriteHeader();
			Util.StreamCopy(audio, AudioFormatOgg.Instance.GetAudioStream(data));

			destination.CloseStream(this, AudioName);
			destination.CloseStream(this, FormatName);
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

		public Stream GetAudioStream(FormatData data)
		{
			return data.GetStream(this, AudioName);
		}

		public Stream GetDecryptedAudioStream(FormatData data)
		{
			return new CryptedMoggStream(GetAudioStream(data));
		}

		public Stream GetFormatStream(FormatData data)
		{
			return data.GetStream(this, FormatName);
		}
	}
}
