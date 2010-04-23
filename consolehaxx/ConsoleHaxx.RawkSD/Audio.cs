using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.RawkSD
{
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

	public class MultiDecoder : IDecoder
	{
		public List<IDecoder> Decoders;
		public Dictionary<int, Pair<IDecoder, int>> Mapping;

		private int BufferSize;

		public MultiDecoder(int buffersize)
		{
			Decoders = new List<IDecoder>();
			Mapping = new Dictionary<int, Pair<IDecoder, int>>();
			SampleRate = 0;
			BufferSize = buffersize;
			AudioBuffer = null;
		}

		public JaggedShortArray AudioBuffer { get; protected set; }

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

		public void AddDecoder(int targetchannel, IDecoder decoder, int channel)
		{
			if (SampleRate == 0)
				SampleRate = decoder.SampleRate;
			else if (SampleRate != decoder.SampleRate)
				throw new FormatException();

			if (AudioBuffer != null && decoder.AudioBuffer.Rank2 != AudioBuffer.Rank2)
				throw new FormatException();

			Mapping.Add(targetchannel, new Pair<IDecoder, int>(decoder, channel));

			AudioBuffer = new JaggedShortArray(Channels, BufferSize);

			if (!Decoders.Contains(decoder))
				Decoders.Add(decoder);
		}

		public void AddDecoder(IDecoder decoder)
		{
			for (int i = 0; i < decoder.Channels; i++)
				AddDecoder(Channels, decoder, i);
		}

		public int Read(int count)
		{
			Dictionary<IDecoder, int> Samples = new Dictionary<IDecoder,int>();
			foreach (IDecoder decoder in Decoders) {
				try {
					Samples.Add(decoder, decoder.Read(count));
				} catch (ArgumentException) { } catch (IOException) { }
			}

			for (int i = 0; i < Channels; i++) {
				int samples = 0;
				int channel = 0;
				IDecoder decoder = null;
				if (Mapping.ContainsKey(i)) {
					var m = Mapping[i];
					decoder = m.Key;
					channel = m.Value;
					if (Samples.ContainsKey(decoder))
						samples = Samples[decoder];
				}

				if (samples > 0)
					Util.Memcpy(AudioBuffer.Array[i], decoder.AudioBuffer[channel], samples);
				
				Util.Memset(AudioBuffer.Array[i], samples, (short)0, AudioBuffer.Rank2 - samples);
			}

			return Samples.Max(s => s.Value);
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

			public Mapping() : base(Instance)
			{
				Volume = 0.0f;
				Balance = 0.0f;
				Instrument = Instrument.Ambient;
			}

			public float Volume { get { return (float)Values[0]; } set { Values[0] = (float)value; } }
			public float Balance { get { return (float)Values[1]; } set { Values[1] = (float)value; } }
			public Instrument Instrument { get { return (Instrument)Values[2]; } set { Values[2] = (int)value; } }
		}

		public IDecoder Decoder;

		public List<Mapping> Mappings;

		public int Channels { get { return Mappings.Count; } }

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

			return format;
		}

		public void Save(Stream stream)
		{
			DataArray data = new DataArray();

			data.SetArray<Mapping>("mappings", Mappings.ToArray());

			data.Save(stream);

			stream.Flush();
		}

		public static void Transcode(IEncoder encoder, IDecoder decoder)
		{
			Transcode(encoder, decoder, null);
		}

		public static void Transcode(IEncoder encoder, IDecoder decoder, ProgressIndicator progress)
		{
			long samples = decoder.Samples;
			while (samples > 0) {
				int read = decoder.Read();
				if (read <= 0)
					break;
				encoder.Write(decoder.AudioBuffer, read);
				samples -= read;
			}
			decoder.Dispose();
			encoder.Dispose();
		}
	}
}
