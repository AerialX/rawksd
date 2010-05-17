using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.RawkSD
{
	#region Interfaces
	public interface IDecoder : IDisposable
	{
		JaggedShortArray AudioBuffer { get; }
		int Channels { get; }
		int SampleRate { get; }
		long Samples { get; }

		int Read(int count);
		int Read();
		void Seek(long sample);
	}

	public interface IEncoder : IDisposable
	{
		int Channels { get; }
		int SampleRate { get; }
		int BitRate { get; }
		long Samples { get; }

		void Write(JaggedShortArray samples, int count);
	}
	#endregion

	#region AudioFormat
	public class AudioFormat
	{
		protected enum BlockType : int
		{
			Mapping = 1,
			Param = 2
		}

		public class Mapping : DataArray.Struct
		{
			public static DataArray.StructDescriptor Instance;
			static Mapping()
			{
				Instance = new DataArray.StructDescriptor(new Type[] { typeof(float), typeof(float), typeof(int) });
				DataArray.RegisterStruct(typeof(Mapping), Instance);
			}

			public Mapping() : this(0.0f, 0.0f, Instrument.Ambient) { }

			public Mapping(float volume, float balance, Instrument instrument)
				: base(Instance)
			{
				Volume = volume;
				Balance = balance;
				Instrument = instrument;
			}


			public float Volume { get { return (float)Values[0]; } set { Values[0] = (float)value; } }
			public float Balance { get { return (float)Values[1]; } set { Values[1] = (float)value; } }
			public Instrument Instrument { get { return (Instrument)Values[2]; } set { Values[2] = (int)value; } }
		}

		public IDecoder Decoder;

		public List<Mapping> Mappings;

		public int Channels { get { return Mappings.Count; } }

		public int InitialOffset;

		public AudioFormat()
		{
			Mappings = new List<Mapping>();
		}

		public void AutoBalance()
		{
			foreach (var instrument in Mappings.GroupBy(m => m.Instrument).Select(i => i.ToList())) {
				if (instrument.Count == 2) {
					instrument[0].Balance = -1.0f;
					instrument[1].Balance = 1.0f;
				} else if (instrument.Count == 1)
					instrument[0].Balance = 0.0f;
			}
		}

		public static AudioFormat Create(Stream stream)
		{
			AudioFormat format = new AudioFormat();

			DataArray data = DataArray.Create(stream);

			format.Mappings.AddRange(data.GetArray<Mapping>("mappings"));
			format.InitialOffset = data.GetValue<int>("offset");

			return format;
		}

		public void Save(Stream stream)
		{
			DataArray data = new DataArray();

			data.SetArray<Mapping>("mappings", Mappings.ToArray());
			data.SetValue("offset", InitialOffset);

			data.Save(stream);

			stream.Flush();
		}

		public static void Transcode(IEncoder encoder, IDecoder decoder, ProgressIndicator progress)
		{
			Transcode(encoder, decoder, decoder.Samples, progress);
		}

		public static void Transcode(IEncoder encoder, IDecoder decoder, long samples, ProgressIndicator progress)
		{
			long downmix = decoder.Channels == encoder.Channels ? 0 : encoder.Channels;
			progress.NewTask("Transcoding Audio", samples);
			while (samples > 0) {
				int read = decoder.Read((int)Math.Min(samples, RawkAudio.Decoder.BufferSize));
				if (read <= 0)
					break;

				if (downmix == 1)
					decoder.AudioBuffer.DownmixTo(decoder.AudioBuffer, new ushort[] { 0xFFFF }, read);

				encoder.Write(decoder.AudioBuffer, read);
				samples -= read;
				progress.Progress(read);
			}
			progress.EndTask();
		}

		public static long ProcessOffset(IDecoder decoder, IEncoder encoder, long offset)
		{
			offset = offset * encoder.SampleRate / 1000;
			if (offset > 0)
				decoder.Seek(offset);
			else if (offset < 0) { // Silence
				offset = -offset;
				JaggedShortArray buffer = new JaggedShortArray(encoder.Channels, decoder.AudioBuffer.Rank2);
				while (offset > 0) {
					int samples = (int)Math.Min(offset, buffer.Rank2);
					encoder.Write(buffer, samples);
					offset -= samples;
				}
			}
			return offset;
		}

		public static Stream AddPreviewDecoder(FormatData data, AudioFormat format, ProgressIndicator progress)
		{
			if (data.Song.PreviewTimes != null) {
				MultiDecoder multi = new MultiDecoder(RawkAudio.Decoder.BufferSize);
				multi.AddDecoder(format.Decoder);

				Stream previewstream = new TemporaryStream();
				IEncoder encoder = new RawkAudio.Encoder(previewstream, 1, format.Decoder.SampleRate, format.Decoder.SampleRate);

				long start = Math.Min(format.Decoder.Samples, (long)data.Song.PreviewTimes[0] * format.Decoder.SampleRate / 1000);
				format.Decoder.Seek(start);
				long duration = Math.Min(format.Decoder.Samples - start, (long)(data.Song.PreviewTimes[1] - data.Song.PreviewTimes[0]) * format.Decoder.SampleRate / 1000);
				AudioFormat.Transcode(encoder, format.Decoder, duration, progress);
				encoder.Dispose();

				previewstream.Position = 0;
				IDecoder previewdecoder = new RawkAudio.Decoder(previewstream, RawkAudio.Decoder.AudioFormat.VorbisOgg);
				multi.AddDecoder(previewdecoder);
				format.Mappings.Add(new AudioFormat.Mapping(0, previewdecoder.Channels == 1 ? 0 : -1, Instrument.Preview));
				if (previewdecoder.Channels == 2)
					format.Mappings.Add(new AudioFormat.Mapping(0, 1, Instrument.Preview));
				format.Decoder = multi;
				multi.Seek(0);

				return previewstream;
			}

			return null;
		}
	}
	#endregion

	#region Decoders
	public class ZeroDecoder : IDecoder
	{
		public JaggedShortArray AudioBuffer { get; protected set; }
		public int Channels { get; protected set; }
		public int SampleRate { get; protected set; }
		public long Samples { get; protected set; }

		protected long Position;

		public ZeroDecoder(int channels, int samplerate, long samples)
		{
			Channels = channels;
			SampleRate = samplerate;
			Samples = samples;
			Position = 0;

			AudioBuffer = new JaggedShortArray(Channels, RawkAudio.Decoder.BufferSize);
		}

		public int Read(int count)
		{
			count = (int)Math.Min(count, Samples - Position);
			Position += count;
			return count;
		}

		public int Read()
		{
			return Read(AudioBuffer.Rank2);
		}

		public void Seek(long sample)
		{
			Position = sample;
		}

		public void Dispose() { }
	}

	public class MultiDecoder : IDecoder
	{
		public List<IDecoder> Decoders;
		public List<Pair<int, Pair<IDecoder, int>>> Mapping;

		private int BufferSize;

		public MultiDecoder(int buffersize)
		{
			Decoders = new List<IDecoder>();
			Mapping = new List<Pair<int, Pair<IDecoder, int>>>();
			SampleRate = 0;
			BufferSize = buffersize;
			AudioBuffer = null;
			TemporaryAudioBuffer = null;
		}

		public JaggedShortArray AudioBuffer { get; protected set; }

		protected JaggedShortArray TemporaryAudioBuffer { get; set; }
		protected ushort[] Masks;

		public int Channels
		{
			get {
				return Mapping.Count > 0 ? Mapping.Max(m => m.Key) + 1 : 0;
			}
		}

		public long Samples
		{
			get {
				return Decoders.Max(d => d.Samples);
			}
		}

		public int SampleRate { get; protected set; }

		public void AddDecoder(IDecoder decoder, int channel)
		{
			AddDecoder(Channels, decoder, channel);
		}

		public void AddDecoder(int targetchannel, IDecoder decoder, int channel)
		{
			if (SampleRate == 0)
				SampleRate = decoder.SampleRate;
			else if (SampleRate != decoder.SampleRate)
				throw new FormatException();

			if (AudioBuffer != null && decoder.AudioBuffer.Rank2 != AudioBuffer.Rank2)
				throw new FormatException();

			Mapping.Add(new Pair<int, Pair<IDecoder, int>>(targetchannel, new Pair<IDecoder, int>(decoder, channel)));

			AudioBuffer = new JaggedShortArray(Channels, BufferSize);

			if (!Decoders.Contains(decoder)) {
				Decoders.Add(decoder);
				TemporaryAudioBuffer = new JaggedShortArray(Decoders.Sum(d => d.Channels), AudioBuffer.Rank2);
			}

			UpdateMasks();
		}

		protected void UpdateMasks()
		{
			int channel = 0;
			int[] channels = new int[Decoders.Count];
			int i = 0;
			foreach (IDecoder decoder in Decoders) {
				channels[i++] = channel;
				channel += decoder.Channels;
			}

			Masks = new ushort[Channels];
			foreach (var map in Mapping)
				Masks[map.Key] |= (ushort)(1 << (channels[Decoders.IndexOf(map.Value.Key)] + map.Value.Value));
		}

		public void AddDecoder(IDecoder decoder)
		{
			for (int i = 0; i < decoder.Channels; i++)
				AddDecoder(Channels, decoder, i);
		}

		public int Read(int count)
		{
			int channel = 0;
			int samples = 0;
			TemporaryAudioBuffer.Zero();
			foreach (IDecoder decoder in Decoders) {
				try {
					decoder.AudioBuffer.Zero();
					int dsamples = decoder.Read(count);
					if (dsamples > 0)
						decoder.AudioBuffer.CopyTo(TemporaryAudioBuffer, channel);
					samples = Math.Max(samples, dsamples);
				} catch (ArgumentException) { } catch (IOException) { }
				channel += decoder.Channels;
			}

			TemporaryAudioBuffer.DownmixTo(AudioBuffer, Masks, samples);
			
			return samples;
		}

		public int Read()
		{
			return Read(AudioBuffer.Rank2);
		}

		public void Seek(long sample)
		{
			foreach (IDecoder decoder in Decoders)
				decoder.Seek(sample);
		}

		public void Dispose()
		{
			foreach (IDecoder decoder in Decoders)
				decoder.Dispose();
		}
	}
	#endregion
}
