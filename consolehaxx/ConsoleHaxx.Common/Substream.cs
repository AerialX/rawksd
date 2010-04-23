using System;
using System.Collections.Generic;
using System.Text;
using System.IO;

namespace ConsoleHaxx.Common
{
	public class Substream : Stream
	{
		public Stream Stream;
		public long Offset;

		private long _length;
		private long _position;

		public Substream(Stream stream, long offset) : this(stream, offset, long.MaxValue) { }

		public Substream(Stream stream, long offset, long length)
		{
			if (!stream.CanSeek)
				throw new NotSupportedException();

			Stream = stream;
			Offset = offset;
			_length = length;
			_position = 0;
		}

		public override bool CanRead
		{
			get { return Stream.CanRead; }
		}

		public override bool CanSeek
		{
			get { return Stream.CanSeek; }
		}

		public override bool CanWrite
		{
			get { return Stream.CanWrite; }
		}

		public override void Flush()
		{
			Stream.Flush();
		}

		public override long Length
		{
			get { return _length; }
		}

		public override long Position
		{
			get
			{
				return _position;
			}
			set
			{
				_position = value;
			}
		}

		public override int Read(byte[] buffer, int offset, int count)
		{
			// Don't read past the length
			if (_position + count > Length)
				count = (int)(Length - _position);

			Stream.Position = Offset + _position;
			int ret = Stream.Read(buffer, offset, count);
			_position += ret;
			return ret;
		}

		public override long Seek(long offset, SeekOrigin origin)
		{
			switch (origin) {
				case SeekOrigin.Begin:
					_position = offset;
					break;
				case SeekOrigin.Current:
					_position += offset;
					break;
				case SeekOrigin.End:
					_position = Length + offset;
					break;
				default:
					break;
			}

			return _position;
		}

		public override void SetLength(long value)
		{
			_length = value;
		}

		public override void Write(byte[] buffer, int offset, int count)
		{
			Stream.Position = Offset + _position;
			Stream.Write(buffer, offset, count);
		}
	}
}
