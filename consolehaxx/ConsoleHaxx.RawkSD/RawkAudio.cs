using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Runtime.InteropServices;
using System.IO;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.RawkSD
{
	public class RawkAudio
	{
		public class Encoder : IEncoder
		{
			public const int DefaultBitRate = 72000;

			public int Channels { get; protected set; }
			public int SampleRate { get; protected set; }
			public int BitRate { get; protected set; }
			public long Samples { get; protected set; }

			private IntPtr identifier;
			private Callbacks callbacks;
			private bool disposed;

			public Encoder(Stream stream, int channels, int samplerate) : this(stream, channels, samplerate, 0) { }
			public Encoder(Stream stream, int channels, int samplerate, int bitrate) : this(channels, samplerate, bitrate)
			{
				callbacks = new Callbacks(stream, false);

				RawkError error = CreateEncoder(ref callbacks.Struct, Channels, SampleRate, BitRate, 0, out identifier);

				ThrowRawkError(error);
			}

			public Encoder(string filename, int channels, int samplerate) : this(filename, channels, samplerate, 0) { }
			public Encoder(string filename, int channels, int samplerate, int bitrate) : this(channels, samplerate, bitrate)
			{
				RawkError error = CreateEncoder(filename, Channels, SampleRate, BitRate, 0, out identifier);

				ThrowRawkError(error);
			}

			~Encoder()
			{
				Dispose();
			}

			private Encoder(int channels, int samplerate, int bitrate)
			{
				disposed = false;

				if (bitrate == 0)
					bitrate = DefaultBitRate;

				Channels = channels;
				SampleRate = samplerate;
				BitRate = bitrate;
			}

			public void Write(JaggedShortArray samples, int count)
			{
				if (disposed)
					throw new ObjectDisposedException(string.Empty);

				RawkError error = Compress(identifier, samples.Pointer, count);

				ThrowRawkError(error);
			}

			public void Dispose()
			{
				if (disposed)
					return;

				DestroyEncoder(identifier);

				disposed = true;
			}
		}

		public class Decoder : IDecoder
		{
			public enum AudioFormat
			{
				VorbisOgg,
				FmodSoundBank,
				BinkAudio,
				Vgs,
				MoggEncrypted,
				MoggDecrypted,
				BinkEncryptedRB2
			}

			public const int BufferSize = 4096;

			public JaggedShortArray AudioBuffer { get; protected set; }
			public int Channels { get; protected set; }
			public int SampleRate { get; protected set; }
			public long Samples { get; protected set; }
			public AudioFormat Format { get; protected set; }

			private IntPtr identifier;
			private Callbacks callbacks;
			private bool disposed;

			public Decoder(Stream stream, AudioFormat format) : this(format)
			{
				callbacks = new Callbacks(stream, false);

				RawkError error = RawkError.Unknown;

				int channels; int samplerate; long samples;

				switch (format) {
					case AudioFormat.VorbisOgg:
						error = CreateVorbisDecoder(ref callbacks.Struct, out channels, out samplerate, out samples, out identifier);
						break;
					case AudioFormat.FmodSoundBank:
						error = CreateFsbDecoder(ref callbacks.Struct, out channels, out samplerate, out samples, out identifier);
						break;
					case AudioFormat.BinkAudio:
						error = CreateBinkDecoder(ref callbacks.Struct, out channels, out samplerate, out samples, out identifier);
						break;
					case AudioFormat.Vgs:
						error = CreateVgsDecoder(ref callbacks.Struct, out channels, out samplerate, out samples, out identifier);
						break;
					default:
						throw new ArgumentException();
				}

				Channels = channels; SampleRate = samplerate; Samples = samples;

				ThrowRawkError(error);

				AudioBuffer = new JaggedShortArray(Channels, BufferSize);
			}

			public Decoder(string filename, AudioFormat format) : this(format)
			{
				RawkError error = RawkError.Unknown;

				int channels; int samplerate; long samples;

				switch (Format) {
					case AudioFormat.VorbisOgg:
						error = CreateVorbisDecoder(filename, out channels, out samplerate, out samples, out identifier);
						break;
					case AudioFormat.FmodSoundBank:
						error = CreateFsbDecoder(filename, out channels, out samplerate, out samples, out identifier);
						break;
					case AudioFormat.BinkAudio:
						error = CreateBinkDecoder(filename, out channels, out samplerate, out samples, out identifier);
						break;
					case AudioFormat.Vgs:
						error = CreateVgsDecoder(filename, out channels, out samplerate, out samples, out identifier);
						break;
					default:
						throw new ArgumentException();
				}

				Channels = channels; SampleRate = samplerate; Samples = samples;

				ThrowRawkError(error);

				AudioBuffer = new JaggedShortArray(Channels, BufferSize);
			}

			private Decoder(AudioFormat format)
			{
				disposed = false;
				Format = format;
				AudioBuffer = new JaggedShortArray(Channels, BufferSize);
			}

			~Decoder()
			{
				Dispose();
			}

			public int Read()
			{
				return Read(AudioBuffer.Rank2);
			}

			public int Read(int count)
			{
				if (disposed)
					throw new ObjectDisposedException(string.Empty);

				RawkError error = RawkError.Unknown;

				count = Math.Min(count, BufferSize);

				switch (Format) {
					case AudioFormat.VorbisOgg:
						error = VorbisDecompress(identifier, AudioBuffer.Pointer, count);
						break;
					case AudioFormat.FmodSoundBank:
						error = FsbDecompress(identifier, AudioBuffer.Pointer, count);
						break;
					case AudioFormat.BinkAudio:
						error = BinkDecompress(identifier, AudioBuffer.Pointer, count);
						break;
					case AudioFormat.Vgs:
						error = VgsDecompress(identifier, AudioBuffer.Pointer, count);
						break;
					default:
						break;
				}

				if (error == RawkError.FileError)
					return 0; // EOF

				ThrowRawkError(error);

				count = (int)error; // Returns positive number of read samples

				return count;
			}

			public void Seek(long sample)
			{
				if (disposed)
					throw new ObjectDisposedException(string.Empty);

				RawkError error = RawkError.Unknown;

				switch (Format) {
					case AudioFormat.VorbisOgg:
						error = VorbisSeek(identifier, sample);
						break;
					case AudioFormat.FmodSoundBank:
						error = FsbSeek(identifier, sample);
						break;
					case AudioFormat.BinkAudio:
						error = BinkSeek(identifier, sample);
						break;
					case AudioFormat.Vgs:
						error = VgsSeek(identifier, sample);
						break;
				}

				ThrowRawkError(error);
			}

			public void Dispose()
			{
				if (disposed)
					return;

				switch (Format) {
					case AudioFormat.VorbisOgg:
						DestroyVorbisDecoder(identifier);
						break;
					case AudioFormat.FmodSoundBank:
						DestroyFsbDecoder(identifier);
						break;
					case AudioFormat.BinkAudio:
						DestroyBinkDecoder(identifier);
						break;
					case AudioFormat.Vgs:
						DestroyVgsDecoder(identifier);
						break;
				}

				disposed = true;
			}
		}

		private static void ThrowRawkError(RawkError error)
		{
			switch (error) {
				case RawkError.InvalidParameter:
					throw new ArgumentException();
				case RawkError.OutOfMemory:
					throw new InsufficientMemoryException();
				case RawkError.FileError:
					throw new IOException();
				case RawkError.NotVorbis:
				case RawkError.NotBink:
				case RawkError.NotFsb:
				case RawkError.NotVgs:
					throw new NotSupportedException("Input audio file could not be identified");
				case RawkError.MisalignedSamplingRates:
					throw new NotSupportedException("Streams with different sampling rates are not supported");
				case RawkError.Unknown:
					throw new Exception();
			}
		}

		private enum RawkError : int
		{
			OK = 0,
			InvalidParameter = -1,
			OutOfMemory = -2,
			FileError = -3,
			NotVorbis = -4,
			NotBink = -5,
			NotFsb = -7,
			NotVgs = -8,
			MisalignedSamplingRates = -6,
			Unknown = -127
		}

		private class Callbacks
		{
			public RawkCallbacks Struct;

			private bool owner;
			private Stream stream;

			public Callbacks(Stream stream, bool owner)
			{
				this.owner = owner;
				this.stream = stream;
				Struct = new RawkCallbacks() {
					CloseFunc = Close,
					ReadFunc = Read,
					SeekFunc = Seek,
					TellFunc = Tell,
					WriteFunc = Write,
					Descriptor = IntPtr.Zero
				};
			}

			public uint Read(IntPtr buffer, uint size, uint count, IntPtr fd)
			{
				int read = (int)(size * count);
				byte[] buf = new byte[read];
				read = stream.Read(buf, 0, read);
				Marshal.Copy(buf, 0, buffer, read);
				return (uint)(read / size);
			}

			public int Seek(IntPtr fd, long offset, SeekOrigin whence)
			{
				stream.Seek(offset, whence);
				return 0;
			}

			public int Close(IntPtr fd)
			{
				if (owner)
					stream.Close();
				return 0;
			}

			public int Tell(IntPtr fd)
			{
				return (int)stream.Position;
			}

			public uint Write(IntPtr buffer, uint size, uint count, IntPtr fd)
			{
				int write = (int)(size * count);
				byte[] buf = new byte[write];
				Marshal.Copy(buffer, buf, 0, write);
				stream.Write(buf, 0, write);
				return count;
			}
		}

		[StructLayout(LayoutKind.Sequential)]
		private struct RawkCallbacks
		{
			[UnmanagedFunctionPointerAttribute(CallingConvention.Cdecl)]
			public delegate uint Read(IntPtr buffer, uint size, uint count, IntPtr fd);
			[UnmanagedFunctionPointerAttribute(CallingConvention.Cdecl)]
			public delegate int Seek(IntPtr fd, long offset, System.IO.SeekOrigin whence);
			[UnmanagedFunctionPointerAttribute(CallingConvention.Cdecl)]
			public delegate int Close(IntPtr fd);
			[UnmanagedFunctionPointerAttribute(CallingConvention.Cdecl)]
			public delegate int Tell(IntPtr fd);
			[UnmanagedFunctionPointerAttribute(CallingConvention.Cdecl)]
			public delegate uint Write(IntPtr buffer, uint size, uint count, IntPtr fd);

			public Read ReadFunc;
			public Seek SeekFunc;
			public Close CloseFunc;
			public Tell TellFunc;
			public Write WriteFunc;
			public IntPtr Descriptor;
		}

		[DllImport("RawkAudio", EntryPoint = "rawk_vorbis_enc_create", CallingConvention = CallingConvention.Cdecl)]
		private static extern RawkError CreateEncoder([MarshalAs(UnmanagedType.LPStr)]string outpath, int channels, int rate, int bitrate, int m_header, out IntPtr stream);

		[DllImport("RawkAudio", EntryPoint = "rawk_vorbis_enc_create_cb")]
		private static extern RawkError CreateEncoder(ref RawkCallbacks cb, int channels, int rate, int bitrate, int m_header, out IntPtr stream);

		[DllImport("RawkAudio", EntryPoint = "rawk_vorbis_enc_destroy")]
		private static extern RawkError DestroyEncoder(IntPtr stream);

		[DllImport("RawkAudio", EntryPoint = "rawk_vorbis_enc_compress")]
		private static extern RawkError Compress(IntPtr stream, IntPtr samples, int sample_count);

		[DllImport("RawkAudio", EntryPoint = "rawk_vorbis_dec_create")]
		private static extern RawkError CreateVorbisDecoder([MarshalAs(UnmanagedType.LPStr)]string inpath, out int channels, out int rate, out long samples, out IntPtr stream);

		[DllImport("RawkAudio", EntryPoint = "rawk_vorbis_dec_create_cb")]
		private static extern RawkError CreateVorbisDecoder(ref RawkCallbacks cb, out int channels, out int rate, out long samples, out IntPtr stream);

		[DllImport("RawkAudio", EntryPoint = "rawk_vorbis_dec_destroy")]
		private static extern RawkError DestroyVorbisDecoder(IntPtr stream);

		[DllImport("RawkAudio", EntryPoint = "rawk_vorbis_dec_seek")]
		private static extern RawkError VorbisSeek(IntPtr stream, long sample_pos);

		[DllImport("RawkAudio", EntryPoint = "rawk_vorbis_dec_decompress")]
		private static extern RawkError VorbisDecompress(IntPtr stream, IntPtr samples, int length);

		[DllImport("RawkAudio", EntryPoint = "rawk_downmix")]
		private static extern RawkError Downmix(IntPtr input, int in_channels, IntPtr output, int out_channels, ushort[] masks, int samples);

		[DllImport("RawkAudio", EntryPoint = "rawk_fsb_dec_create")]
		private static extern RawkError CreateFsbDecoder([MarshalAs(UnmanagedType.LPStr)]string inpath, out int channels, out int rate, out long samples, out IntPtr stream);

		[DllImport("RawkAudio", EntryPoint = "rawk_fsb_dec_create_cb")]
		private static extern RawkError CreateFsbDecoder(ref RawkCallbacks cb, out int channels, out int rate, out long samples, out IntPtr stream);

		[DllImport("RawkAudio", EntryPoint = "rawk_fsb_dec_destroy")]
		private static extern RawkError DestroyFsbDecoder(IntPtr stream);

		[DllImport("RawkAudio", EntryPoint = "rawk_fsb_dec_seek")]
		private static extern RawkError FsbSeek(IntPtr stream, long sample_pos);

		[DllImport("RawkAudio", EntryPoint = "rawk_fsb_dec_decompress")]
		private static extern RawkError FsbDecompress(IntPtr stream, IntPtr samples, int length);

		[DllImport("RawkAudio", EntryPoint = "rawk_bink_dec_create")]
		private static extern RawkError CreateBinkDecoder([MarshalAs(UnmanagedType.LPStr)]string inpath, out int channels, out int rate, out long samples, out IntPtr stream);

		[DllImport("RawkAudio", EntryPoint = "rawk_bink_dec_create_cb")]
		private static extern RawkError CreateBinkDecoder(ref RawkCallbacks cb, out int channels, out int rate, out long samples, out IntPtr stream);

		[DllImport("RawkAudio", EntryPoint = "rawk_bink_dec_destroy")]
		private static extern RawkError DestroyBinkDecoder(IntPtr stream);

		[DllImport("RawkAudio", EntryPoint = "rawk_bink_dec_seek")]
		private static extern RawkError BinkSeek(IntPtr stream, long sample_pos);

		[DllImport("RawkAudio", EntryPoint = "rawk_bink_dec_decompress")]
		private static extern RawkError BinkDecompress(IntPtr stream, IntPtr samples, int length);

		[DllImport("RawkAudio", EntryPoint = "rawk_vgs_dec_create")]
		private static extern RawkError CreateVgsDecoder([MarshalAs(UnmanagedType.LPStr)]string inpath, out int channels, out int rate, out long samples, out IntPtr stream);

		[DllImport("RawkAudio", EntryPoint = "rawk_vgs_dec_create_cb")]
		private static extern RawkError CreateVgsDecoder(ref RawkCallbacks cb, out int channels, out int rate, out long samples, out IntPtr stream);

		[DllImport("RawkAudio", EntryPoint = "rawk_vgs_dec_destroy")]
		private static extern RawkError DestroyVgsDecoder(IntPtr stream);

		[DllImport("RawkAudio", EntryPoint = "rawk_vgs_dec_seek")]
		private static extern RawkError VgsSeek(IntPtr stream, long sample_pos);

		[DllImport("RawkAudio", EntryPoint = "rawk_vgs_dec_decompress")]
		private static extern RawkError VgsDecompress(IntPtr stream, IntPtr samples, int length);
	}

	public class JaggedShortArray
	{
		public short[][] Array;
		public IntPtr Pointer;
		public int Rank1;
		public int Rank2;

		private List<GCHandle> handles;
		private IntPtr[] pointerArray;

		public JaggedShortArray(int x, int y)
		{
			Rank1 = x;
			Rank2 = y;

			Array = new short[x][];
			handles = new List<GCHandle>();
			pointerArray = new IntPtr[Array.Length];

			GCHandle handle;
			for (int i = 0; i < x; i++) {
				Array[i] = new short[y];

				handle = GCHandle.Alloc(Array[i], GCHandleType.Pinned); handles.Add(handle);

				pointerArray[i] = handle.AddrOfPinnedObject();
			}
			handle = GCHandle.Alloc(pointerArray, GCHandleType.Pinned); handles.Add(handle);

			Pointer = handle.AddrOfPinnedObject();
		}

		public short[] this[int index]
		{
			get { return Array[index]; }
			set { Array[index] = value; }
		}

		~JaggedShortArray()
		{
			handles.ForEach(h => h.Free());
			handles.Clear();
		}

		public void CopyTo(JaggedShortArray destination, int sourceindex, int num, int index)
		{
			for (int i = sourceindex; i < sourceindex + num; i++) {
				this[i].CopyTo(destination[index + i - sourceindex], 0);
			}
		}

		public void CopyTo(JaggedShortArray destination, int index)
		{
			for (int i = 0; i < Rank1; i++) {
				this[i].CopyTo(destination[index + i], 0);
			}
		}

		public void Zero()
		{
			for (int i = 0; i < Rank1; i++)
				for (int l = 0; l < Rank2; l++)
					this[i][l] = 0;
		}
	}
}
