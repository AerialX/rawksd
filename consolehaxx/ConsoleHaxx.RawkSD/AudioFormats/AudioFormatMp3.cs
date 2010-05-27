using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.RawkSD
{
	public class AudioFormatMp3 : IAudioFormat
	{
		public const string FormatName = "map";
		public const string AudioName = "audio";

		public static AudioFormatMp3 Instance;
		public static void Initialise()
		{
			Instance = new AudioFormatMp3();
			Platform.AddFormat(Instance);
		}

		public override int ID {
			get { return 0x06; }
		}

		public override string Name {
			get { return "MP3 Audio"; }
		}

		public override bool Writable {
			get { return false; }
		}

		public override bool Readable {
			get { return false; }
		}

		public override AudioFormat DecodeAudioFormat(FormatData data)
		{
			throw new NotImplementedException();
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

			format.Decoder = streams.Count == 1 ? null : new MultiDecoder(RawkAudio.Decoder.BufferSize);
			foreach (Stream stream in streams) {
				IDecoder sdecoder = new RawkAudio.Decoder(stream, RawkAudio.Decoder.AudioFormat.VorbisOgg);
				if (format.Decoder == null)
					format.Decoder = sdecoder;
				else
					(format.Decoder as MultiDecoder).AddDecoder(sdecoder);
			}

			return format;
		}

		public override void EncodeAudio(AudioFormat data, FormatData destination, ProgressIndicator progress)
		{
			throw new NotImplementedException();
		}

		public override bool CanRemux(IFormat format)
		{
			return format is AudioFormatMogg;
		}

		public override void Remux(IFormat format, FormatData data, FormatData destination, ProgressIndicator progress)
		{
			throw new NotImplementedException();
		}

		public void CreateFromFSB(FormatData data, Stream[] fsbstreams, AudioFormat format)
		{
			List<Stream> streams = new List<Stream>();
			foreach (Stream stream in fsbstreams) {
				FSB fsb = new FSB(stream);
				foreach (FSB.Sample sample in fsb.Samples) {
					if (sample.Channels <= 2) {
						streams.Add(sample.Data);
						continue;
					}

					byte[] buffer = new byte[0x04];
					sample.Data.Position = 0;
					Stream[] samples = new Stream[sample.Channels];
					for (int i = 0; i < sample.Channels; i++) {
						samples[i] = new TemporaryStream();
						streams.Add(samples[i]);
					}
					int channel = -1;
					while (!sample.Data.EOF()) {
						sample.Data.Read(buffer, 0, 4);
						try {
							Mp3.Header header = new Mp3.Header(buffer);
							channel = (channel + 1) % sample.Channels;
						} catch (FormatException) { }

						samples[channel].Write(buffer);
					}
				}
			}

			Create(data, streams.ToArray(), format);
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

		internal bool CompatibleOptions(AudioFormat audioFormat)
		{
			return true;
		}
	}
}
