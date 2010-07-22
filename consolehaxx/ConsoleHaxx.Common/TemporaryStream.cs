using System;
using System.IO;

namespace ConsoleHaxx.Common
{
	public class TemporaryStream : Stream
	{
		public static string BasePath;

		protected FileStream Base;
		protected string Filename;
		protected bool Closed;

		public string Name { get { return Filename; } }

		public override bool CanRead { get { return true; } }

		public override bool CanSeek { get { return true; } }

		public override bool CanWrite { get { return true; } }

		public override long Length { get { return Base.Length; } }

		public override long Position { get { return Base.Position; } set { Base.Position = value; } }

		public override void Close()
		{
			if (!Closed)
				Base.Close();
			Util.Delete(Filename);
			Closed = true;
		}

		public void ClosePersist()
		{
			Base.Close();
			Closed = true;
		}

		public void Open(FileMode mode = FileMode.Create)
		{
			if (Closed) {
				Base = new FileStream(Filename, mode, FileAccess.ReadWrite);
				Closed = false;
			}
		}

		public TemporaryStream()
		{
			Filename = GetTempFileName();
			Closed = true;
			Open();
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

		public static string GetTempFileName()
		{
			if (BasePath != null)
				return Path.Combine(BasePath, Path.GetRandomFileName());
			else
				return Path.GetTempFileName();
		}
	}
}
