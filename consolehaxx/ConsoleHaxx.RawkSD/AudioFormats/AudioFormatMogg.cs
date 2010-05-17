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
		public const string PreviewName = "preview";

		public static readonly AudioFormatMogg Instance;
		static AudioFormatMogg()
		{
			Instance = new AudioFormatMogg();
			Platform.AddFormat(Instance);
		}

		public override int ID {
			get { return 0x0b; }
		}

		public override string Name {
			get { return "Rock Band OGG Vorbis Audio"; }
		}

		public override bool Writable {
			get { return true; }
		}

		public override bool Readable {
			get { return true; }
		}
		
		public override AudioFormat DecodeAudioFormat(FormatData data)
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
			return DecodeAudioBackend(data, progress, false);
		}

		private AudioFormat DecodeAudioBackend(FormatData data, ProgressIndicator progress, bool skippreview)
		{
			if (!data.HasStream(this, AudioName))
				return null;

			AudioFormat format = DecodeAudioFormat(data);
			Stream audio = GetAudioStream(data);
			if (audio != null)
				audio = new CryptedMoggStream(audio);

			Stream preview = null;
			if (!skippreview) {
				preview = GetDecryptedPreviewStream(data, progress);
				format.Mappings.Add(new AudioFormat.Mapping(0, 0, Instrument.Preview));
			}
			format.Decoder = AudioFormatOgg.Instance.DecodeOggAudio(audio, preview);

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

			Stream previewstream = AudioFormatOgg.Instance.GetPreviewStream(data);
			if (previewstream != null) {
				CryptedMoggStream preview = new CryptedMoggStream(destination.AddStream(this, PreviewName));
				preview.WriteHeader();
				Util.StreamCopy(preview, previewstream);
				destination.CloseStream(this, PreviewName);
			}

			destination.CloseStream(this, AudioName);
			destination.CloseStream(this, FormatName);
		}

		public void Create(FormatData data, Stream audio, Stream preview, AudioFormat format)
		{
			if (preview != null)
				data.SetStream(this, PreviewName, preview);
			Create(data, audio, format);
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

		public Stream GetPreviewStream(FormatData data, ProgressIndicator progress)
		{
			return data.GetStream(this, PreviewName);
		}

		public Stream GetDecryptedAudioStream(FormatData data)
		{
			return new CryptedMoggStream(GetAudioStream(data));
		}

		public Stream GetDecryptedPreviewStream(FormatData data, ProgressIndicator progress)
		{
			return new CryptedMoggStream(GetPreviewStream(data, progress));
		}

		public Stream GetFormatStream(FormatData data)
		{
			return data.GetStream(this, FormatName);
		}
	}
}
