using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

namespace ConsoleHaxx.Common
{
	public class Multistream : Stream
	{
		public struct Descriptor
		{
			public Stream Stream;
			public long Offset;

			public Descriptor(Stream stream, long offset)
			{
				Stream = stream;
				Offset = offset;
			}
		}

		List<Descriptor> Streams;
		bool Ownership;

		public Multistream() : this(false) { }

		public Multistream(bool ownership)
		{
			Streams = new List<Descriptor>();
			Ownership = ownership;
		}

		public override bool CanRead { get { return true; } }
		public override bool CanSeek { get { return true; } }
		public override bool CanWrite { get { return false; } }
		public override void Flush() { }

		public override long Length
		{
			get {
				throw new NotImplementedException();
			}
		}

		public override long Position { get; set; }

		public override long Seek(long offset, SeekOrigin origin)
		{
			switch (origin) {
				case SeekOrigin.Begin:
					Position = offset;
					break;
				case SeekOrigin.Current:
					Position += offset;
					break;
				case SeekOrigin.End:
					Position = Length + offset;
					break;
				default:
					break;
			}

			return Position;
		}

		public override void SetLength(long value)
		{
			throw new NotImplementedException();
		}

		public override void Write(byte[] buffer, int offset, int count)
		{
			throw new NotImplementedException();
		}

		public void AddStream(Stream stream, long offset)
		{
			AddStream(stream, offset, Streams.Count);
		}

		public void AddStream(Stream stream, long offset, int order)
		{
			Streams.Insert(order, new Descriptor(stream, offset));
		}

		public override int Read(byte[] buffer, int bufferoffset, int count)
		{
			foreach (Descriptor desc in Streams) {
				long length = desc.Stream.Length;
				long offset = desc.Offset - Position;
				long fileoffset = 0;
				if (offset < 0) {
					length += offset;
					fileoffset -= offset;
					offset = 0;
				}
				length = Math.Min(length, count) - offset;

				if (offset >= 0 && length > 0) {
					desc.Stream.Position = offset;
					desc.Stream.Read(buffer, (int)fileoffset, (int)length);
				}
			}

			return count;
		}

		public override void Close()
		{
			if (Ownership) {
				foreach (Descriptor desc in Streams)
					desc.Stream.Close();
			}

			Streams.Clear();

			base.Close();
		}
	}
}
