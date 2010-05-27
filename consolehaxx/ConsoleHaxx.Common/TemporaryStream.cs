using System;
using System.IO;

namespace ConsoleHaxx.Common
{
	public class TemporaryStream : Stream
	{
		public static string BasePath;

		protected Stream Base;
		protected string Filename;
		protected bool Closed;

		public override bool CanRead
		{
			get
			{
				return true;
			}
		}

		public override bool CanSeek
		{
			get
			{
				return true;
			}
		}

		public override bool CanWrite
		{
			get
			{
				return true;
			}
		}

		public override long Length
		{
			get
			{
				return Base.Length;
			}
		}

		public override long Position
		{
			get
			{
				return Base.Position;
			}
			set
			{
				Base.Position = value;
			}
		}

		public override void Close()
		{
			Base.Close();
			File.Delete(Filename);
			Closed = true;
		}

		public TemporaryStream()
		{
			if (BasePath != null)
				Filename = Path.Combine(BasePath, Path.GetRandomFileName());
			else
				Filename = Path.GetTempFileName();
			Base = new FileStream(Filename, FileMode.Create, FileAccess.ReadWrite);
			Closed = false;
		}

		~TemporaryStream()
		{
			if (!Closed)
				Close();
		}

		public override void Flush()
		{
			Base.Flush();
		}

		public override int Read(byte[] buffer, int offset, int count)
		{
			return Base.Read(buffer, offset, count);
		}

		public override long Seek(long offset, SeekOrigin origin)
		{
			return Base.Seek(offset, origin);
		}

		public override void SetLength(long value)
		{
			Base.SetLength(value);
		}

		public override void Write(byte[] buffer, int offset, int count)
		{
			Base.Write(buffer, offset, count);
		}
	}
}
