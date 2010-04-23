using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Security.Cryptography;

namespace ConsoleHaxx.Common
{
	public class AesStream : Stream
	{
		private Stream Stream;
		private Rijndael Crypt;
		ICryptoTransform Decryptor;
		ICryptoTransform Encryptor;

		private int Offset;
		private long Block;
		private byte[] Buffer;

		private bool Writing;

		public AesStream(Stream stream, byte[] key, byte[] iv)
		{
			Stream = stream;
			Crypt = Rijndael.Create();
			Crypt.Padding = PaddingMode.None;
			Crypt.Mode = CipherMode.CBC;
			Crypt.IV = iv;
			Crypt.Key = key;

			Decryptor = Crypt.CreateDecryptor();
			Block = 0;
			Offset = 0;
			Buffer = new byte[Decryptor.OutputBlockSize];

			Encryptor = Crypt.CreateEncryptor();

			Writing = false;
		}

		public override bool CanRead
		{
			get { return !Writing; }
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
			throw new NotImplementedException();
		}

		public override long Length
		{
			get { return Math.Max(Stream.Length, Block * Decryptor.OutputBlockSize + Offset); }
		}

		public override long Position
		{
			get
			{
				return Block * Decryptor.OutputBlockSize + Offset;
			}
			set
			{
				Block = value / Decryptor.OutputBlockSize;
				if (Block > 0) {
					Stream.Position = (Block - 1) * Decryptor.OutputBlockSize;
					byte[] iv = new byte[Decryptor.InputBlockSize];
					Stream.Read(iv, 0, Decryptor.InputBlockSize);
					Decryptor = Crypt.CreateDecryptor(Crypt.Key, iv);
					Encryptor = Crypt.CreateEncryptor(Crypt.Key, iv);
				} else {
					Stream.Position = 0;
					Decryptor = Crypt.CreateDecryptor();
					Encryptor = Crypt.CreateEncryptor();
				}

				Offset = (int)(value % Decryptor.OutputBlockSize);
				if (Offset > 0) {
					Stream.Read(Buffer, 0, Decryptor.OutputBlockSize);
					Decryptor.TransformBlock(Buffer, 0, Decryptor.OutputBlockSize, Buffer, 0);
				}
			}
		}

		public override int Read(byte[] buffer, int offset, int count)
		{
			if (Writing)
				throw new NotSupportedException();

			if (Block * Decryptor.OutputBlockSize + Offset + count >= Length)
				count = (int)(Length - (Block * Decryptor.OutputBlockSize + Offset));

			int pos = 0;
			while (pos < count) {
				if (Offset == 0) {
					Stream.Read(Buffer, 0, Decryptor.OutputBlockSize);
					Decryptor.TransformBlock(Buffer, 0, Decryptor.OutputBlockSize, Buffer, 0);
				}

				int written = (int)((Decryptor.OutputBlockSize - Offset) > (count - pos) ? (count - pos) : (Decryptor.OutputBlockSize - Offset));
				Array.Copy(Buffer, Offset, buffer, offset + pos, written);
				pos += written;
				Offset += written;

				if (Offset >= Decryptor.OutputBlockSize)
					Block++;
				Offset %= Decryptor.OutputBlockSize;
			}

			return pos;
		}

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

			return Offset;
		}

		public override void SetLength(long value)
		{
			Stream.SetLength(value);
		}

		public override void Write(byte[] buffer, int offset, int count)
		{
			int position = 0;
			while (position < count) {
				int towrite = Math.Min(Encryptor.InputBlockSize - Offset, count);

				Array.Copy(buffer, offset + position, Buffer, Offset, towrite);
				Offset += towrite;
				position += towrite;

				if (Offset == Encryptor.InputBlockSize)
					WriteBlock();
			}

			Writing = true;
		}

		private void WriteBlock()
		{
			Encryptor.TransformBlock(Buffer, 0, Encryptor.InputBlockSize, Buffer, 0);
			Stream.Write(Buffer, 0, Offset);

			Offset = 0;
			Buffer = new byte[Encryptor.InputBlockSize];
		}

		public override void Close()
		{
			if (Offset > 0 && Writing)
				WriteBlock();

			base.Close();
		}
	}
}
