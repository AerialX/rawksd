using System;
using System.Collections.Generic;
using System.IO;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.Harmonix
{
	public class ChunkedDtbStream : Stream
	{
		public const int SaveSize = 7340032;
		public const int ChunkSize = 0x40000;
		public const int Chunks = SaveSize / ChunkSize;
		public const int SeedLength = 65536;
		public const int SeedIndexCount = SaveSize / SeedLength;

		Stream Stream;
		EndianReader Reader;

		uint Offset;
		int Key;
		uint length;
		int[] ChunkMap;
		int[] Seeds;

		public override long Length
		{
			get { return length; }
		}

		public enum ChunkedMode
		{
			Chunk,
			Game
		}

		public ChunkedMode Mode;

		public ChunkedDtbStream(EndianReader reader)
		{
			Mode = ChunkedMode.Chunk;
			Stream = reader.Base;
			Reader = reader;

			uint i, j;

			Stream.Position = 4;
			Key = Reader.ReadInt32();

			ChunkMap = new int[Chunks + 1];
			Seeds = new int[SeedIndexCount];

			ChunkMap[0] = -8;
			for (i = 1; i <= Chunks; i++) {
				int pos;
				Stream.Position = (i - 1) * ChunkSize;
				byte[] buffer = Reader.ReadBytes(ChunkSize);
				for (pos = ChunkSize; pos > 0; pos--) {
					if (buffer[pos - 1] != 0)
						break;
				}
				ChunkMap[i] = ChunkMap[i - 1] + pos;
			}

			length = (uint)ChunkMap[ChunkMap.Length - 1];
			//Length = ChunkMap[(sizeof(ChunkMap)/sizeof(ChunkMap[0]))-1];

			// calculate seeds for faster seeking
			for (i = 0; i < length; i += SeedLength) {
				Seeds[i / SeedLength] = Key;
				for (j = 0; j < SeedLength; j++) {
					Key = CryptedDtbStream.XorKey(Key);
				}
			}

			Offset = 0;
			Key = Seeds[0];
			Stream.Position = 8;
		}

		public uint GetChunk(uint pos)
		{
			uint chunk = 0;
			while (ChunkMap[chunk + 1] < pos)
				chunk++;

			return chunk;
		}

		byte[] _Read(uint count)
		{
			byte[] data = Reader.ReadBytes((int)count);
			for (int i = 0; i < data.Length; i++) {
				Key = CryptedDtbStream.XorKey(Key);
				data[i] ^= (byte)Key;
			}
			Offset += (uint)data.Length;

			return data;
		}

		void _Write(byte[] buffer, uint offset, uint count)
		{
			byte[] buf = new byte[count];
			for (int i = 0; i < count; i++) {
				Key = CryptedDtbStream.XorKey(Key);
				buf[i] = (byte)(buffer[i + offset] ^ Key);
			}
			Stream.Write(buf, 0, (int)count);

			Offset += count;
		}

		public override bool CanRead
		{
			get { throw new NotImplementedException(); }
		}

		public override bool CanSeek
		{
			get { throw new NotImplementedException(); }
		}

		public override bool CanWrite
		{
			get { throw new NotImplementedException(); }
		}

		public override void Flush()
		{
			throw new NotImplementedException();
		}

		public override long Position
		{
			get
			{
				throw new NotImplementedException();
			}
			set
			{
				uint chunk;
				if (value == Offset)
					return;
				if (value > length)
					return;

				chunk = GetChunk((uint)value);

				// set file pointer position
				Stream.Position = (chunk * ChunkSize) + value - ChunkMap[chunk];

				// set key
				Key = Seeds[value / SeedLength];
				for (int i = 0; i < (value % SeedLength); i++)
					Key = CryptedDtbStream.XorKey(Key);

				Offset = (uint)value;
			}
		}

		public override int Read(byte[] buffer, int offset, int count)
		{
			switch (Mode) {
				case ChunkedMode.Chunk: {
						int i;
						uint start_chunk;
						uint end_chunk;
						uint end_pos = (uint)(Offset + count);
						if (count == 0)
							return 0;
						if (end_pos > length) {
							count = (int)(length - Offset);
							end_pos = length;
						}

						start_chunk = GetChunk(Offset);
						end_chunk = GetChunk(end_pos);
						i = 0;
						byte[] read;
						while (start_chunk != end_chunk) {
							start_chunk++;
							uint front = (uint)(ChunkMap[start_chunk] - Offset);
							read = _Read(front);
							Array.Copy(read, 0, buffer, offset + i, read.Length);
							i += read.Length;
							Stream.Position = start_chunk * ChunkSize;
						}
						read = _Read((uint)(count - i));
						Array.Copy(read, 0, buffer, offset + i, count - i);
						return count;
					}
				case ChunkedMode.Game: {
						if (ChunkSize - (Stream.Position % ChunkSize) < count)
							Stream.Position = (Stream.Position / ChunkSize + 1) * ChunkSize;

						byte[] read = _Read((uint)count);
						Array.Copy(read, 0, buffer, offset, read.Length);
						return read.Length;
					}
				default:
					return -1;
			}
		}

		public override long Seek(long offset, SeekOrigin origin)
		{
			throw new NotImplementedException();
		}

		public override void SetLength(long value)
		{
			throw new NotImplementedException();
		}

		public override void Write(byte[] buffer, int offset, int count)
		{
			switch (Mode) {
				case ChunkedMode.Chunk: {
					uint start_chunk;
					uint end_pos = (uint)(Offset + count);
					if (count == 0)
						return;
					
					start_chunk = GetChunk(Offset);
					if (end_pos > (start_chunk + 1) * ChunkSize) {
						uint front = (uint)((start_chunk + 1) * ChunkSize - Stream.Position);
						for (uint j = 0; j < front; j++)
							Reader.Write((byte)0);
						// fseek(f_in, (start_chunk + 1)*CHUNK_SIZE, SEEK_SET); // Should already be there; if not we're fucked
					}
					_Write(buffer, (uint)offset, (uint)count);
					break;
				}
				case ChunkedMode.Game: {
					//uint chunk = GetChunk(Offset);
					uint diff = (uint)(ChunkSize - (Stream.Position % ChunkSize));
					if (diff < count) {
						for (uint j = 0; j < diff; j++)
							Reader.Write((byte)0);
						//ChunkMap[chunk + 1] = (int)Offset;
						// TODO: Change the value of all ChunkMap entries after chunk + 1
					}
					
					_Write(buffer, (uint)offset, (uint)count);
					break;
				}
			}
		}
	}
}
