using System;
using System.Collections.Generic;
using System.IO;
using System.Security.Cryptography;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.Harmonix
{
	public class CryptedMoggStream : Stream
	{
		public Stream Base;
		private EndianReader Reader;
		private Aes Crypt;
		ICryptoTransform Decryptor;
		private long Offset;
		private byte[] ReadBuffer;
		private byte[] Buffer;
		private byte[] KeyBuffer;
		private long Block;
		private long Size;
		private uint HeaderSize;
		private bool Encrypted;

		public CryptedMoggStream(EndianReader reader)
		{
			Base = reader.Base;
			Reader = reader;

			Crypt = Aes.Create();
			Crypt.Padding = PaddingMode.None;
			Crypt.Mode = CipherMode.ECB;
			// MOGG Key:
			Crypt.Key = new byte[] { 0x37, 0xB2, 0xE2, 0xB9, 0x1C, 0x74, 0xFA, 0x9E, 0x38, 0x81, 0x08, 0xEA, 0x36, 0x23, 0xDB, 0xE4 };
			Decryptor = Crypt.CreateEncryptor();
			Offset = 1;

			if (Base.Length >= 8) {
				int magic = Reader.ReadInt32();
				if (magic == 0x0B || magic == 0x0A) {
					HeaderSize = Reader.ReadUInt32(); // HeaderSize
					Size = Base.Length - HeaderSize;
					Encrypted = magic == 0x0B;
					Position = 0;
				} else
					throw new FormatException();
			} else {
				HeaderSize = 0;
				Size = Base.Length;
			}
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
			get { return true; }
		}

		public override void Flush()
		{
			throw new NotImplementedException();
		}

		public override long Length
		{
			get { return Size; }
		}

		public override long Position
		{
			get
			{
				return Offset;
			}
			set
			{
				if (Encrypted) {
					if (value < Offset) {
						Reader.Position = HeaderSize - 0x10;
						Block = 0;
						KeyBuffer = Reader.ReadBytes(Util.AesKeySize);
						Buffer = new byte[Util.AesKeySize];
						Decryptor.TransformBlock(KeyBuffer, 0, Util.AesKeySize, Buffer, 0);
						ReadBuffer = Reader.ReadBytes(Util.AesKeySize);
					}

					Offset = value;
				} else
					Base.Position = HeaderSize + value;
			}
		}

		public override int Read(byte[] buffer, int offset, int count)
		{
			if (!Encrypted)
				return Base.Read(buffer, offset, count);

			if (Offset + count > Size)
				count = (int)(Size - Offset);

			long off = Util.RoundDown(Offset, Util.AesKeySize);
			Reader.Position = HeaderSize + Block + Util.AesKeySize;
			if (off >= Block + Util.AesKeySize) {
				for (long i = Block; i < off; i += Util.AesKeySize) {
					ReadBuffer = Reader.ReadBytes(Util.AesKeySize);
					DoBlock();
				}
				Block = off;
			}

			int diff = (int)(Offset - off);

			int pos = 0;
			while (pos < count) {
				int written = (int)((Util.AesKeySize - diff) > (count - pos) ? (count - pos) : (Util.AesKeySize - diff));
				//Array.Copy(Buffer, diff, buffer, offset + pos, written);
				for (int i = 0; i < written; i++)
					buffer[offset + pos + i] = (byte)(Buffer[diff + i] ^ ReadBuffer[diff + i]);
				pos += written;
				Offset += written;
				diff = 0;
				if (Util.RoundDown(Offset, Util.AesKeySize) > Block) {
					ReadBuffer = Reader.ReadBytes(Util.AesKeySize);
					DoBlock();
					Block += Util.AesKeySize;
				}
			}

			return pos;
		}

		private void DoBlock()
		{
			for (int j = 0; j < Util.AesKeySize; j++) {
				KeyBuffer[j]++;
				if (KeyBuffer[j] != 0)
					break;
			}

			Decryptor.TransformBlock(KeyBuffer, 0, Util.AesKeySize, Buffer, 0);
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
			Size = value;
		}

		public override void Write(byte[] buffer, int offset, int count)
		{
			long off = Util.RoundDown(Offset, Util.AesKeySize);
			if (off > Block + Util.AesKeySize) {
				for (long i = Block; i < off; i += Util.AesKeySize) {
					DoBlock();
				}
				Block = off;
			}

			int diff = (int)(Offset - off);

			int pos = 0;
			while (pos < count) {
				int written = (int)((Util.AesKeySize - diff) > (count - pos) ? (count - pos) : (Util.AesKeySize - diff));

				for (int i = 0; i < written; i++)
					//buffer[offset + pos + i] = (byte)(Buffer[diff + i] ^ ReadBuffer[diff + i]);
					Base.WriteByte((byte)(buffer[offset + pos + i] ^ Buffer[diff + i]));
				pos += written;
				Offset += written;
				diff = 0;
				if (Util.RoundDown(Offset, Util.AesKeySize) > Block) {
					DoBlock();
					Block += Util.AesKeySize;
				}
			}
		}

		public void WriteHeader()
		{
			Reader.Write(0x0B); // Encrypted
			Reader.Write(44); // Offset to OGG

			Reader.Write(0x0F); // Unknown; channels?
			Reader.Write(0x4E20); // Unknown
			Reader.Write(0x01); // Number of entries
			Reader.Write(0x00);
			Reader.Write(0x00);
			//Reader.Write(Util.Encoding.GetBytes("microwaveyourcat")); // MOGG Key
			//Reader.Write(Util.Encoding.GetBytes("lulzmediawikicms")); // MOGG Key
			Reader.Write(Util.Encoding.GetBytes("imrightbehindyou")); // MOGG Key

			Encrypted = true;

			HeaderSize = 44;
			Offset = 1;
			Position = 0;
		}

		public static void WriteHeader(EndianReader writer)
		{
			writer.Write(0x0A); // Unencrypted
			//Reader.Write(44); // Offset to OGG
			writer.Write(44 - 16); // Offset to OGG
			writer.Write(0x0F); // Unknown; channels?
			writer.Write(0x4E20); // Unknown
			writer.Write(0x01); // Number of entries
			writer.Write(0x00);
			writer.Write(0x00);
			//Reader.Write(Util.Encoding.GetBytes("lulzmediawikicms")); // MOGG Key
		}

		public override void Close()
		{
			Base.Close();
			base.Close();
		}
	}
}
