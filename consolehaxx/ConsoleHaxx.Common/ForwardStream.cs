using System;
using System.IO;

namespace ConsoleHaxx.Common
{
	public class ForwardStream : Stream
	{
		Stream Base;
		long Offset;
		long length;

		public ForwardStream(Stream stream) : this(stream, 0) { }

		public ForwardStream(Stream stream, long initialPosition) : this(stream, initialPosition, -1) { }

		public ForwardStream(Stream stream, long initialPosition, long len)
		{
			Base = stream;
			Offset = initialPosition;
			length = len;
		}

		public override bool CanRead
		{
			get { return true; }
		}

		public override bool CanSeek
		{
			get { return true; }
		}

		public override bool CanWrite
		{
			get { return Base.CanWrite; }
		}

		public override void Flush()
		{
			Base.Flush();
		}

		public override long Length
		{
			get { return length < 0 ? Base.Length : length; }
		}

		public override long Position
		{
			get
			{
				return Offset;
			}
			set
			{
				if (value < Offset)
					throw new NotImplementedException();
				Seek(value, SeekOrigin.Begin);
			}
		}

		public override int Read(byte[] buffer, int offset, int count)
		{
			int read = Base.Read(buffer, offset, count);
			Offset += read;
			return read;
		}

		public override long Seek(long offset, SeekOrigin origin)
		{
			if (origin == SeekOrigin.Begin)
				offset -= Offset;
			else if (origin == SeekOrigin.End)
				offset = Length + offset - Offset;

			origin = SeekOrigin.Current;

			if (offset < 0)
				throw new NotImplementedException();

			for (long i = 0; i < offset; i++) {
				if (Base.ReadByte() == -1)
					break;
			}

			Offset += offset;

			return Offset;
		}

		public override void SetLength(long value)
		{
			length = value;
		}

		public override void Write(byte[] buffer, int offset, int count)
		{
			Base.Write(buffer, offset, count);
		}

		public override void Close()
		{
			Base.Close();
			base.Close();
		}
	}
}
