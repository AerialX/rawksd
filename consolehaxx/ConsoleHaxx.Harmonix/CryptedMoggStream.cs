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
		//private EndianReader Reader;
		private SymmetricAlgorithm Crypt;
		ICryptoTransform Decryptor;
		private long Offset;
		private byte[] ReadBuffer;
		private byte[] Buffer;
		private byte[] KeyBuffer;
		private long Block;
		private long Size;
		private uint HeaderSize;
		private bool Encrypted;
		private List<Pair<uint, uint>> Entries;
		private long CurrentSample;
		private int CurrentEntry;

		public CryptedMoggStream(Stream stream)
		{
			Base = stream;
			EndianReader reader = new EndianReader(Base, Endianness.LittleEndian);
			
			ReadBuffer = new byte[Util.AesKeySize];
			KeyBuffer = new byte[Util.AesKeySize];
			Buffer = new byte[Util.AesKeySize];
			
			Crypt = Rijndael.Create();
			Crypt.Padding = PaddingMode.None;
			Crypt.Mode = CipherMode.ECB;
			// MOGG Key:
			Crypt.Key = new byte[] { 0x37, 0xB2, 0xE2, 0xB9, 0x1C, 0x74, 0xFA, 0x9E, 0x38, 0x81, 0x08, 0xEA, 0x36, 0x23, 0xDB, 0xE4 };
			Decryptor = Crypt.CreateEncryptor();
			Offset = 1;

			if (Base.Length >= 8) {
				int magic = reader.ReadInt32();
				if (magic == 0x0B || magic == 0x0A) {
					HeaderSize = reader.ReadUInt32(); // HeaderSize
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
						Base.Position = HeaderSize - 0x10;
						Block = 0;
						Base.Read(KeyBuffer, 0, Util.AesKeySize);
						Buffer = new byte[Util.AesKeySize];
						Decryptor.TransformBlock(KeyBuffer, 0, Util.AesKeySize, Buffer, 0);
						if (Length > HeaderSize)
							Base.Read(ReadBuffer, 0, Util.AesKeySize);
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
			Base.Position = HeaderSize + Block + Util.AesKeySize;
			if (off >= Block + Util.AesKeySize) {
				for (long i = Block; i < off; i += Util.AesKeySize) {
					Base.Read(ReadBuffer, 0, Util.AesKeySize);
					DoBlock();
				}
				Block = off;
			}

			int diff = (int)(Offset - off);

			int pos = 0;
			while (pos < count) {
				int written = (int)((Util.AesKeySize - diff) > (count - pos) ? (count - pos) : (Util.AesKeySize - diff));
				for (int i = 0; i < written; i++)
					buffer[offset + pos + i] = (byte)(Buffer[diff + i] ^ ReadBuffer[diff + i]);
				pos += written;
				Offset += written;
				diff = 0;
				if (Util.RoundDown(Offset, Util.AesKeySize) > Block) {
					Base.Read(ReadBuffer, 0, Util.AesKeySize);
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

		public void WriteHeader(long samples = 0)
		{
			int entrysize = 0x4E20;
			Entries = new List<Pair<uint, uint>>();
			for (; samples > 0; samples -= entrysize) {
				Entries.Insert(0, new Pair<uint, uint>((uint)samples, 0));
			}
			Entries.Insert(0, new Pair<uint, uint>(0, 0));

			HeaderSize = 36 + (uint)Entries.Count * 8;

			EndianReader writer = new EndianReader(Base, Endianness.LittleEndian);
			writer.Write(0x0B); // Encrypted
			writer.Write(HeaderSize); // Offset to OGG

			writer.Write(0x0F); // Unknown; channels?
			writer.Write(entrysize); // Unknown

			writer.Write((uint)Entries.Count); // Number of entries
			WriteEntries(writer);
			writer.Write(Util.Encoding.GetBytes("imrightbehindyou")); // MOGG Key

			Encrypted = true;

			Offset = 1;
			Position = 0;
		}

		public void WriteEntries()
		{
			Base.Position = 20;
			WriteEntries(new EndianReader(Base, Endianness.LittleEndian));
		}

		private void WriteEntries(EndianReader writer)
		{
			foreach (var entry in Entries) {
				writer.Write(entry.Value);
				writer.Write(entry.Key);
			}
		}

		public void Update(long samples)
		{
			CurrentSample += samples;
			if (Entries.Count == CurrentEntry + 1)
				return;

			var entry = Entries[CurrentEntry];
			var nextentry = Entries[CurrentEntry + 1];
			if ((uint)CurrentSample < nextentry.Key) {
				uint position = (uint)Util.RoundDown(Offset, 0x8000);
				if (entry.Value < position) {
					entry.Key = (uint)CurrentSample;
					entry.Value = position;
				}
			} else {
				CurrentEntry++;
				nextentry.Value = entry.Value;
				nextentry.Key = entry.Key;
			}
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
