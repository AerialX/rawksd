using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using ConsoleHaxx.Common;
using System.IO;

namespace ConsoleHaxx.RawkSD
{
	public class AudioFormatOgg : IAudioFormat
	{
		public const string FormatName = "map";
		public const string AudioName = "audio";

		public static AudioFormatOgg Instance;
		public static void Initialise()
		{
			Instance = new AudioFormatOgg();
			Platform.AddFormat(Instance);
		}

		public override int ID {
			get { return 0x07; }
		}

		public override string Name {
			get { return "OGG Vorbis Audio"; }
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

		public IList<Stream> GetAudioStreams(FormatData data)
		{
			List<Stream> streams = new List<Stream>();

			int i = 0;
			string name = AudioName;
			do {
				streams.Add(data.GetStream(this, name));
				i++;
				name = AudioName + "." + i.ToString();
			} while (data.HasStream(this, name));

			return streams;
		}

		public override AudioFormat DecodeAudio(FormatData data, ProgressIndicator progress)
		{
			IList<Stream> streams = GetAudioStreams(data);
			
			AudioFormat format = DecodeAudioFormat(data);
			format.Decoder = DecodeOggAudio(streams.ToArray());

			return format;
		}

		internal IDecoder DecodeOggAudio(params Stream[] streamparams)
		{
			IList<Stream> streams = streamparams.Where(s => s != null).ToList();
			IDecoder decoder = streams.Count == 1 ? null : new MultiDecoder(RawkAudio.Decoder.BufferSize);
			foreach (Stream stream in streams) {
				IDecoder sdecoder = new RawkAudio.Decoder(stream, RawkAudio.Decoder.AudioFormat.VorbisOgg);
				if (decoder == null)
					decoder = sdecoder;
				else
					(decoder as MultiDecoder).AddDecoder(sdecoder);
			}

			return decoder;
		}

		public override void EncodeAudio(AudioFormat data, FormatData destination, ProgressIndicator progress)
		{
			EncodeOggAudio(data, destination, this, destination.AddStream(this, AudioName), progress);
		}

		internal void EncodeOggAudio(AudioFormat data, FormatData destination, IFormat format, Stream stream, ProgressIndicator progress)
		{
			IDecoder decoder = data.Decoder;
			RawkAudio.Encoder encoder = new RawkAudio.Encoder(stream, decoder.Channels, decoder.SampleRate);

			AudioFormat.ProcessOffset(decoder, encoder, data.InitialOffset);

			progress.NewTask(1);

			AudioFormat.Transcode(encoder, decoder, progress);
			progress.Progress();

			encoder.Dispose();
			destination.CloseStream(format, AudioName);

			decoder.Dispose();

			data.Save(destination.AddStream(format, FormatName));
			destination.CloseStream(format, FormatName);

			progress.EndTask();
		}

		public override bool CanRemux(IFormat format)
		{
			return format is AudioFormatMogg;
		}

		public override void Remux(IFormat format, FormatData data, FormatData destination, ProgressIndicator progress)
		{
			if (!(format is AudioFormatMogg))
				throw new FormatException();

			Stream stream = destination.AddStream(this, AudioName);
			Util.StreamCopy(stream, AudioFormatMogg.Instance.GetDecryptedAudioStream(data));
			destination.CloseStream(stream);

			stream = destination.AddStream(this, FormatName);
			Util.StreamCopy(stream, AudioFormatMogg.Instance.GetFormatStream(data));
			destination.CloseStream(stream);
		}

		public void Create(FormatData data, Stream[] streams, AudioFormat format)
		{
			Stream formatstream = data.AddStream(this, FormatName);
			format.Save(formatstream);
			data.CloseStream(formatstream);

			for (int i = 0; i < streams.Length; i++)
				data.SetStream(this, AudioName + (i == 0 ? string.Empty : ("." + i.ToString())), streams[i]);
		}

		public Stream GetAudioStream(FormatData data)
		{
			return data.GetStream(this, AudioName);
		}

		public Stream GetFormatStream(FormatData data)
		{
			return data.GetStream(this, FormatName);
		}

		public Stream GetPreviewStream(FormatData data)
		{
			return null;
		}
	}
}
