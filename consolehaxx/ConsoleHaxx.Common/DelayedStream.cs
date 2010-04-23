using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

namespace ConsoleHaxx.Common
{
	public class DelayedStream : Stream
	{
		public Stream Base;
		protected Func<Stream> Function;

		public DelayedStream(Func<Stream> func)
		{
			Function = func;
		}

		protected void Open()
		{
			if (Base == null)
				Base = Function();
		}

		public override bool CanRead
		{
			get { Open(); return Base.CanRead; }
		}

		public override bool CanSeek
		{
			get { Open(); return Base.CanSeek; }
		}

		public override bool CanWrite
		{
			get { Open(); return Base.CanWrite; }
		}

		public override void Flush()
		{
			Open();
			Base.Flush();
		}

		public override long Length
		{
			get { Open(); return Base.Length; }
		}

		public override long Position {
			get { Open(); return Base.Position; }
			set { Open(); Base.Position = value; }
		}

		public override int Read(byte[] buffer, int offset, int count)
		{
			Open();
			return Base.Read(buffer, offset, count);
		}

		public override long Seek(long offset, SeekOrigin origin)
		{
			Open();
			return Base.Seek(offset, origin);
		}

		public override void SetLength(long value)
		{
			Open();
			Base.SetLength(value);
		}

		public override void Write(byte[] buffer, int offset, int count)
		{
			Open();
			Base.Write(buffer, offset, count);
		}
	}

	public class DelayedStreamCache : IDisposable
	{
		protected List<Stream> Streams;

		public DelayedStreamCache()
		{
			Streams = new List<Stream>();
		}

		public Func<Stream> GenerateFileStream(string path, FileMode mode, FileAccess access)
		{
			return delegate() {
				Stream stream = new FileStream(path, mode, access);
				AddStream(stream);
				return stream;
			};
		}

		~DelayedStreamCache()
		{
			Dispose();
		}

		public void Dispose()
		{
			foreach (Stream stream in Streams)
				stream.Close();
			Streams.Clear();
		}

		public void AddStream(Stream stream)
		{
			Streams.Add(stream);
		}
	}
}
