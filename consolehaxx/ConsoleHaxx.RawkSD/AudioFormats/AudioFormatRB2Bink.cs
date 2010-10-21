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

		public static AudioFormatRB2Bink Instance;
		public static void Initialise()
		{
			Instance = new AudioFormatRB2Bink();
			Platform.AddFormat(Instance);
		}

		public override int ID {
			get { return 0x08; }
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
			throw new NotImplementedException();
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

		public Stream GetAudioStream(FormatData data)
		{
			return data.GetStream(this, AudioName);
		}

		public Stream GetPreviewStream(FormatData data, ProgressIndicator progress)
		{
			if (!data.HasStream(this, PreviewName)) {
				AudioFormat format = new AudioFormat();
				/* Let it crash if it can't decode it. KIBE biks we don't know about don't belong here.
				try { */
					format.Decoder = new RawkAudio.Decoder(GetAudioStream(data), RawkAudio.Decoder.AudioFormat.BinkAudio);
				/*} catch {
					format.Decoder = new ZeroDecoder(1, 28000, 0x7FFFFFFFFFFFFFFF);
				}*/
				Stream previewstream = new TemporaryStream();
				CryptedMoggStream mogg = new CryptedMoggStream(previewstream);
				mogg.WriteHeader();
				PlatformRB2WiiCustomDLC.TranscodePreview(data.Song.PreviewTimes, null, format.Decoder, mogg, progress);
				previewstream.Position = 0;
				format.Decoder.Dispose();
				return previewstream;
			}

			return data.GetStream(this, PreviewName);
		}

		public Stream GetFormatStream(FormatData data)
		{
			return data.GetStream(this, FormatName);
		}
	}
}
