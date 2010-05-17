using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

namespace ConsoleHaxx.RawkSD
{
	public class AudioFormatFFmpeg : IAudioFormat
	{
		public const string FormatName = "map";
		public const string AudioName = "audio";

		public static readonly AudioFormatFFmpeg Instance;
		static AudioFormatFFmpeg()
		{
			Instance = new AudioFormatFFmpeg();
			Platform.AddFormat(Instance);
		}

		public override int ID {
			get { return 0x0f; }
		}

		public override string Name {
			get { return "FFmpeg Audio"; }
		}

		public override bool Writable {
			get { return false; }
		}

		public override bool Readable {
			get { return true; }
		}

		public override AudioFormat DecodeAudio(FormatData data, ProgressIndicator progress)
		{
			IList<Stream> streams = GetAudioStreams(data);

			AudioFormat format = DecodeAudioFormat(data);

			format.Decoder = streams.Count == 1 ? null : new MultiDecoder(FFmpegDecoder.BufferSize);
			foreach (Stream stream in streams) {
				IDecoder sdecoder = new FFmpegDecoder(stream);
				if (format.Decoder == null)
					format.Decoder = sdecoder;
				else
					(format.Decoder as MultiDecoder).AddDecoder(sdecoder);
			}

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

		public override void EncodeAudio(AudioFormat data, FormatData destination, ProgressIndicator progress)
		{
			throw new NotImplementedException();
		}

		public void Create(FormatData data, Stream[] streams, AudioFormat format)
		{
			Stream formatstream = data.AddStream(this, FormatName);
			format.Save(formatstream);
			data.CloseStream(formatstream);

			for (int i = 0; i < streams.Length; i++)
				data.SetStream(this, AudioName + (i == 0 ? string.Empty : ("." + i.ToString())), streams[i]);
		}
	}
}
