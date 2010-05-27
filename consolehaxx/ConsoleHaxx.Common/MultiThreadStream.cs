using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Threading;

namespace ConsoleHaxx.Common
{
	public class MultiThreadStream : Stream
	{
		public Stream Base;

		public Mutex Mutex;
		public Mutex PositionMutex;

		public Dictionary<Thread, long> Positions;

		public Thread OriginalThread;

		public MultiThreadStream(Stream stream)
		{
			Positions = new Dictionary<Thread, long>();
			Mutex = new Mutex();
			PositionMutex = new Mutex();

			Base = stream;

			OriginalThread = Thread.CurrentThread;
			Positions.Add(OriginalThread, Base.Position);
		}

		public override bool CanRead { get { return Base.CanRead; } }

		public override bool CanSeek { get { return Base.CanSeek; } }

		public override bool CanWrite { get { return Base.CanWrite; } }

		public override void Flush()
		{
			Mutex.WaitOne();

			Base.Flush();

			Mutex.ReleaseMutex();
		}

		public override long Length
		{
			get { return Base.Length; }
		}

		public override long Position
		{
			get
			{
				PositionMutex.WaitOne();

				Thread thread = Thread.CurrentThread;
				if (!Positions.ContainsKey(thread)) {
					long value = Positions[OriginalThread];
					Positions.Add(thread, value);
					return value;
				}
				long ret = Positions[thread];

				PositionMutex.ReleaseMutex();

				return ret;
			}
			set
			{
				PositionMutex.WaitOne();

				Positions[Thread.CurrentThread] = value;

				PositionMutex.ReleaseMutex();
			}
		}

		public override int Read(byte[] buffer, int offset, int count)
		{
			Mutex.WaitOne();

			long position = Position;
			if (position != Base.Position)
				Base.Position = position;

			int ret = Base.Read(buffer, offset, count);

			if (ret > 0)
				Position += ret;

			Mutex.ReleaseMutex();

			return ret;
		}

		public override long Seek(long offset, SeekOrigin origin)
		{
			switch (origin) {
				case SeekOrigin.Current:
					offset += Position;
					break;
				case SeekOrigin.End:
					offset += Length;
					break;
				default:
					break;
			}

			Position = offset;
			return offset;
		}

		public override void SetLength(long value)
		{
			Mutex.WaitOne();

			Base.SetLength(value);

			Mutex.ReleaseMutex();
		}

		public override void Write(byte[] buffer, int offset, int count)
		{
			Mutex.WaitOne();

			long position = Position;
			if (position != Base.Position)
				Base.Position = position;

			Base.Write(buffer, offset, count);

			Position += count;

			Mutex.ReleaseMutex();
		}

		public override void Close()
		{
			Mutex.WaitOne();

			base.Close();

			Mutex.ReleaseMutex();
		}
	}
}
