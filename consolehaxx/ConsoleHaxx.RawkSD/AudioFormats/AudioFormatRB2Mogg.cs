using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using ConsoleHaxx.Harmonix;
using ConsoleHaxx.Common;
using System.IO;

namespace ConsoleHaxx.RawkSD
{
	public class AudioFormatRB2Mogg : IAudioFormat
	{
		public const string FormatName = "map";
		public const string AudioName = "audio";
		public const string PreviewName = "preview";

		public static AudioFormatRB2Mogg Instance;
		public static void Initialise()
		{
			Instance = new AudioFormatRB2Mogg();
			Platform.AddFormat(Instance);
		}

		public override int ID {
			get { return 0x09; }
		}

		public override string Name {
			get { return "Rock Band 2 OGG Vorbis Audio"; }
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
			Stream audio = GetDecryptedAudioStream(data);
			Stream preview = GetDecryptedPreviewStream(data);

			List<Stream> streams = new List<Stream>();
			if (audio is CryptedMoggStream)
				streams.Add((audio as CryptedMoggStream).Base);
			if (preview is CryptedMoggStream)
				streams.Add((preview as CryptedMoggStream).Base);

			format.Decoder = AudioFormatOgg.Instance.DecodeOggAudio(audio, preview);
			MultiDecoder multi = format.Decoder as MultiDecoder;
			if (multi != null) {
				IDecoder decoder = multi.Decoders[1];
				for (int i = 0; i < decoder.Channels; i++) {
					format.Mappings.Add(new AudioFormat.Mapping(0, decoder.Channels == 1 ? 0 : (i == 0 ? -1 : 1), Instrument.Preview));
				}
			}

			format.SetDisposeStreams(data, streams);

			return format;
		}

		public override void EncodeAudio(AudioFormat audioformat, FormatData formatdata, ProgressIndicator progress)
		{
			progress.NewTask(20);

			Stream audio = formatdata.AddStream(this, AudioName);
			progress.SetNextWeight(1);
			List<AudioFormat.Mapping> oldmaps = audioformat.Mappings;
			ushort[] masks = PlatformRB2WiiCustomDLC.RemixAudioTracks(formatdata.Song, audioformat);

			progress.SetNextWeight(14);
			long samples = audioformat.Decoder.Samples;
			CryptedMoggStream mogg = new CryptedMoggStream(audio);
			//mogg.WriteHeader(samples);
			mogg.WriteHeader();
			RawkAudio.Encoder encoder = new RawkAudio.Encoder(mogg, audioformat.Channels, audioformat.Decoder.SampleRate, 28000);
			progress.NewTask("Transcoding Audio", samples);
			JaggedShortArray buffer = new JaggedShortArray(encoder.Channels, audioformat.Decoder.AudioBuffer.Rank2);
			AudioFormat.ProcessOffset(audioformat.Decoder, encoder, audioformat.InitialOffset);
			while (samples > 0) {
				//int read = audioformat.Decoder.Read((int)Math.Min(samples, 0x4E20));
				//int read = audioformat.Decoder.Read((int)Math.Min(samples, 0x20));
				int read = audioformat.Decoder.Read((int)Math.Min(samples, buffer.Rank2));
				if (read <= 0)
					break;

				audioformat.Decoder.AudioBuffer.DownmixTo(buffer, masks, read);

				encoder.Write(buffer, read);
				samples -= read;
				progress.Progress(read);
				//mogg.Update(read);
			}
			progress.EndTask();
			progress.Progress(14);
			encoder.Dispose();
			mogg.WriteEntries();
			formatdata.CloseStream(audio);

			progress.SetNextWeight(6);
			Stream preview = formatdata.AddStream(this, PreviewName);
			mogg = new CryptedMoggStream(preview);
			mogg.WriteHeader();
			PlatformRB2WiiCustomDLC.TranscodePreview(formatdata.Song.PreviewTimes, oldmaps, audioformat.Decoder != null ? audioformat.Decoder : new ZeroDecoder(1, 28000, 0x7FFFFFFFFFFFFFFF), mogg, progress);
			formatdata.CloseStream(preview);

			progress.EndTask();
			progress.Progress(6);

			Stream formatstream = formatdata.AddStream(this, FormatName);
			audioformat.Save(formatstream);
			formatdata.CloseStream(formatstream);
		}

		public void Create(FormatData data, Stream audio, Stream preview, AudioFormat format)
		{
			data.SetStream(this, AudioName, audio);
			data.SetStream(this, PreviewName, preview);
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
			Stream stream = GetAudioStream(data);
			if (stream == null)
				return null;
			return new CryptedMoggStream(stream);
		}

		public Stream GetPreviewStream(FormatData data)
		{
			return data.GetStream(this, PreviewName);
		}

		public Stream GetDecryptedPreviewStream(FormatData data)
		{
			Stream stream = GetPreviewStream(data);
			if (stream == null)
				return null;
			return new CryptedMoggStream(stream);
		}

		public Stream GetFormatStream(FormatData data)
		{
			return data.GetStream(this, FormatName);
		}
	}
}
