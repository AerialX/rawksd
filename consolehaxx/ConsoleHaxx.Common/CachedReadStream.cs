using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

namespace ConsoleHaxx.Common
{
	public class CachedReadStream : Stream
	{
		public Stream Base;
		public byte[] Cache;
		public long RealOffset;
		public long Offset;
		public long length;

		public const int CacheSize =           0x800000; // 8MB
		public const int CacheBackwardOffset = 0x400000; // 4MB

		public CachedReadStream(Stream stream)
		{
			Base = stream;

			Cache = new byte[CacheSize];
			RealOffset = 0;
			Offset = 0;
			Base.Position = 0;
			Base.Read(Cache, 0, CacheSize);
			length = Base.Length;
		}

		public override bool CanRead { get { return true; } }

		public override bool CanSeek { get { return true; } }

		public override bool CanWrite { get { return false; } }

		public override void Flush()
		{
			throw new NotImplementedException();
		}

		public override long Length { get { return length; } }

		public override long Position
		{
			get
			{
				return Offset;
			}
			set
			{
				Offset = value;

				if (Offset < RealOffset || Offset >= RealOffset + CacheSize) {
					RealOffset = Math.Max(Offset - CacheBackwardOffset, 0);
					Base.Position = RealOffset;
					Base.Read(Cache, 0, CacheSize);
				}
			}
		}

		public override int Read(byte[] buffer, int offset, int count)
		{
			int read = 0;
			while (read < count) {
				int cacheoffset = (int)(Offset - RealOffset);
				int cachelength = (int)Math.Min(CacheSize, length - RealOffset);

				int chunk = (int)Math.Min(count - read, cachelength - cacheoffset);
				if (chunk == 0)
					break;
				
				Util.Memcpy(buffer, offset + read, Cache, cacheoffset, chunk);

				read += chunk;
				Position += chunk;
			}

			return read;
		}

		public override long Seek(long offset, SeekOrigin origin)
		{
			switch (origin) {
				case SeekOrigin.Current:
					offset = Offset + offset;
					break;
				case SeekOrigin.End:
					offset += Length;
					break;
			}

			Position = offset;

			return offset;
		}

		public override void SetLength(long value)
		{
			throw new NotImplementedException();
		}

		public override void Write(byte[] buffer, int offset, int count)
		{
			throw new NotImplementedException();
		}

		public override void Close()
		{
			base.Close();

			Base.Close();
		}
	}
}
