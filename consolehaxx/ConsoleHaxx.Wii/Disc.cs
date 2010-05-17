using System;
using System.Collections.Generic;
using System.Linq;
using System.IO;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.Wii
{
	public class Disc
	{
		public const int Magic = 0x5D1C9EA3;

		private Stream Stream;

		public char Console;
		public string Gamecode;
		public char Region;
		public string Publisher;
		public byte Number;
		public byte Version;
		public byte AudioStreaming;
		public byte AudioStreamingBuffer;
		public string Title;

		public uint RegionSet;

		public List<Partition> Partitions;

		public Disc(Stream stream)
		{
			Stream = stream;
			EndianReader reader = new EndianReader(Stream, Endianness.BigEndian);

			Stream.Position = 0;
			Console = (char)Stream.ReadByte();
			Gamecode = ((char)Stream.ReadByte()).ToString() + (char)Stream.ReadByte();
			Region = (char)Stream.ReadByte();
			Publisher = ((char)Stream.ReadByte()).ToString() + (char)Stream.ReadByte();
			Number = (byte)Stream.ReadByte();
			Version = (byte)Stream.ReadByte();
			AudioStreaming = (byte)Stream.ReadByte();
			AudioStreamingBuffer = (byte)Stream.ReadByte();

			reader.Position = 0x18;
			if (reader.ReadUInt32() != Magic)
				throw new FormatException();

			reader.ReadUInt32();

			Title = Util.ReadCString(Stream);

			reader.Position = 0x40000;

			uint[][] partitiontable = new uint[4][];

			for (int i = 0; i < 4; i++) {
				partitiontable[i] = new uint[2];
				for (int j = 0; j < 2; j++)
					partitiontable[i][j] = reader.ReadUInt32();
			}

			Partitions = new List<Partition>();
			for (byte i = 0; i < 4; i++) {
				if (partitiontable[i][0] > 0)
					reader.Position = (long)partitiontable[i][1] << 2;
				for (int j = 0; j < partitiontable[i][0]; j++) {
					Partition partition = new Partition();
					Partitions.Add(partition);
					partition.Table = i;
					partition.Offset = (long)reader.ReadUInt32() << 2;
					partition.Type = (PartitionType)reader.ReadUInt32(); // May or may not correspond to the enum
				}
			}

			reader.Position = 0x4E000;
			RegionSet = reader.ReadUInt32();

			foreach (Partition partition in Partitions) {
				reader.Position = partition.Offset;
				partition.Ticket = Ticket.Create(Stream);
				reader.ReadUInt32(); // TMD Size
				reader.ReadUInt32(); // TMD Offset
				partition.CertificateChain = new byte[reader.ReadUInt32()];
				long certoffset = (long)reader.ReadUInt32() << 2;
				long h3offset = (long)reader.ReadUInt32() << 2;
				partition.DataOffset = (long)reader.ReadUInt32() << 2;
				partition.DataSize = (long)reader.ReadUInt32() << 2; ;

				partition.TMD = TMD.Create(Stream);

				reader.Position = partition.Offset + certoffset;
				reader.Read(partition.CertificateChain, 0, partition.CertificateChain.Length);

				partition.Stream = new PartitionStream(partition, Stream);

				// At 0x420: uint dolOffset, uint fstOffset, uint fstSize, uint fstSize
				partition.Stream.Position = 0x424;
				long fstOffset = (long)new EndianReader(partition.Stream, Endianness.BigEndian).ReadUInt32() << 2;
				partition.Stream.Position = fstOffset;
				partition.Root = new U8(partition.Stream, true);
			}
		}

		public void Save(Stream stream)
		{
			EndianReader writer = new EndianReader(stream, Endianness.BigEndian);

			long core = writer.Position;

			writer.Write((byte)Console);
			writer.Write(Util.Encoding.GetBytes(Gamecode));
			writer.Write((byte)Region);
			writer.Write(Util.Encoding.GetBytes(Publisher));
			writer.Write(Number);
			writer.Write(Version);
			writer.Write(AudioStreaming);
			writer.Write(AudioStreamingBuffer);

			writer.PadTo(0x18);
			writer.Write(0x5D1C9EA3);

			writer.Write((int)0);

			writer.Write(Util.Encoding.GetBytes(Title));
			writer.Write((byte)0);

			writer.PadTo(0x40000);

			int partitionoffset = 0x40020;
			List<int[]> partitiontableentries = new List<int[]>();
			for (int i = 0; i < 4; i++) {
				List<Partition> entrypartitions = Partitions.FindAll(p => p.Table == i);
				int partitioncount = entrypartitions.Count;
				writer.Write(partitioncount);
				writer.Write(partitioncount == 0 ? 0 : partitionoffset >> 2);
				partitionoffset += partitioncount * sizeof(int) * 2;
				int[] partitiontableentry = new int[partitioncount * 2];
				for (int k = 0; k < partitioncount; k++) {
					partitiontableentry[k * 2] = (int)(entrypartitions[k].Offset >> 2);
					partitiontableentry[k * 2 + 1] = (int)entrypartitions[k].Type;
				}
				partitiontableentries.Add(partitiontableentry);
			}

			writer.PadTo(0x40020);
			foreach (int[] partitiontableentry in partitiontableentries)
				foreach (int entry in partitiontableentry)
					writer.Write(entry);

			writer.PadTo(0x4E000);
			writer.Write(RegionSet);
			for (int i = 0; i < 3; i++)
				writer.Write((int)0);
			for (int i = 0; i < 16; i++) { // ?
				if (i == RegionSet)
					writer.Write((byte)0x11);
				else
					writer.Write((byte)0x80);
			}

			writer.PadTo(0x4FFFC);
			writer.Write((uint)0xC3F81A8E); // Magic

			foreach (Partition partition in Partitions) {
				writer.Write(new byte[partition.Offset - writer.Position]);

				int headeroffset = 0x2C0;
				partition.Ticket.Save(stream);
				MemoryStream tmd = new MemoryStream();
				partition.TMD.Save(tmd);

				writer.Write((int)tmd.Length);
				writer.Write(headeroffset >> 2);
				headeroffset += (int)tmd.Length;
				headeroffset = (int)Util.RoundUp(headeroffset, 0x20);

				writer.Write(partition.CertificateChain.Length);
				writer.Write(headeroffset >> 2);
				int certificatechainoffset = headeroffset;
				//headeroffset += partition.CertificateChain.Length;

				//headeroffset = (int)Util.RoundUp(headeroffset, 0x20);

				headeroffset = 0x8000;
				writer.Write(headeroffset >> 2);
				int h3offset = headeroffset;
				//headeroffset += 0x18000;

				headeroffset = 0x20000;
				writer.Write(headeroffset >> 2); // Data should be aligned to 0x20000
				writer.Write((int)(Util.RoundUp(partition.Stream.Length, 0x7C00 * 64) >> 2));
				int dataoffset = headeroffset;

				tmd.Position = 0;
				Util.StreamCopy(stream, tmd);

				writer.Write(new byte[partition.Offset + certificatechainoffset - writer.Position]);
				writer.Write(partition.CertificateChain);

				writer.Write(new byte[partition.Offset + h3offset - writer.Position]);
				byte[] h3 = new byte[0x18000];
				writer.Write(h3);

				// Now to write the partition data...
				writer.Write(new byte[partition.Offset + dataoffset - writer.Position]);

				byte[] hash;

				partition.Stream.Position = 0;
				EndianReader reader = new EndianReader(partition.Stream, Endianness.BigEndian);

				bool eof = false;
				long sections = Util.RoundUp(partition.Stream.Length, 0x7C00 * 64) / 0x7C00 / 64;
				for (long section = 0; section < sections; section++) {
					byte[] h2 = new byte[0xA0];

					byte[][] queuedata = new byte[64][];
					byte[][] queueh0 = new byte[64][];
					byte[][] queueh1 = new byte[8][];

					for (int group = 0; group < 8; group++) {
						byte[] h1 = new byte[0xA0];

						for (int cluster = 0; cluster < 8; cluster++) {
							if (reader.Base.Length - reader.Position == 0) {
								//eof = true;
								//break;
							}

							byte[] data = reader.ReadBytes(0x7C00);
							byte[] h0 = new byte[0x26C];

							for (int i = 0; i < 31; i++) {
								hash = Util.SHA1Hash(data, i * 0x400, 0x400);
								hash.CopyTo(h0, i * 20);
							}
							hash = Util.SHA1Hash(h0);
							hash.CopyTo(h1, cluster * 20);

							queuedata[group * 8 + cluster] = data;
							queueh0[group * 8 + cluster] = h0;
						}

						queueh1[group] = h1;

						hash = Util.SHA1Hash(h1);
						hash.CopyTo(h2, group * 20);

						if ((group == 7 || eof) && group != 0) { // Last group, hash the header for h3
							hash = Util.SHA1Hash(h2);
							hash.CopyTo(h3, section * 20);
						}

						if (eof)
							break;
					}

					for (int group = 0; group < 8; group++) {
						for (int cluster = 0; cluster < 8; cluster++) {
							int index = group * 8 + cluster;
							if (queuedata[index] != null) {
								byte[] iv = new byte[0x10];
								byte[] header = new byte[0x400];
								byte[] encryptedheader = new byte[0x400];
								queueh0[index].CopyTo(header, 0);
								queueh1[group].CopyTo(header, 0x280);
								h2.CopyTo(header, 0x340);

								Util.AesCBC.CreateEncryptor(partition.Ticket.Key, iv).TransformBlock(header, 0, 0x400, encryptedheader, 0);
								writer.Write(encryptedheader);
								Array.Copy(encryptedheader, 0x3D0, iv, 0, 0x10);
								Util.AesCBC.CreateEncryptor(partition.Ticket.Key, iv).TransformBlock(queuedata[index], 0, 0x7C00, queuedata[index], 0);
								writer.Write(queuedata[index]);
							}
						}
					}

					if (eof)
						break;
				}

				long endofpartition = writer.Position;

				writer.Position = partition.Offset + h3offset;
				writer.Write(h3);

				writer.Position = endofpartition;

				break;
			}
		}

		public Partition DataPartition { get {
			return Partitions.FirstOrDefault(p => p.Type == PartitionType.Data);
		} }
	}

	public enum PartitionType : uint
	{
		Data = 0,
		Update = 1,
		Installer = 2
	}

	public class Partition
	{
		public PartitionType Type;
		public Ticket Ticket;
		public TMD TMD;
		public byte[] CertificateChain;
		public Stream Stream;
		public long DataOffset;
		public long DataSize;
		public long Offset;
		public byte Table; // Partition Table index

		public U8 Root;
	}

	public class PartitionStream : Stream
	{
		private Stream Stream;
		private Partition Partition;
		private long Offset;
		private byte[] CryptedBuffer;
		private byte[] Cache;
		private uint CachedBlock;

		public PartitionStream(Partition partition, Stream stream)
		{
			Partition = partition;
			Stream = stream;
			CryptedBuffer = new byte[0x8000];
			Cache = new byte[0x7C00];
			CachedBlock = 0xFFFFFFFF;
		}

		public override bool CanRead
		{
			get { return Stream.CanRead; }
		}

		public override bool CanSeek
		{
			get { return true; }
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
			get {
				//return Partition.TMD.Contents[0].Size;
				return Partition.DataSize;
			}
		}

		public override long Position
		{
			get
			{
				return Offset;
			}
			set
			{
				Offset = value;
			}
		}

		public override int Read(byte[] buffer, int offset, int count)
		{
			/*
			if (!Partition.Encrypted) {
				Stream.Position = Partition.Offset + Partition.DataOffset + Offset;
				int ret = Stream.Read(buffer, offset, count);
				Offset += ret;
				return ret;
			}
			*/

			uint block = (uint)(Offset / 0x7C00);
			int cacheOffset = (int)(Offset % 0x7C00);
			int cacheSize;
			int dst = 0;

			count = (int)Math.Min(count, Length - Offset);

			while (count > 0) {
				if (!DecryptBlock(block)) {
					Offset += dst;
					return dst;
				}

				cacheSize = count;
				if (cacheSize + cacheOffset > 0x7C00)
					cacheSize = 0x7C00 - cacheOffset;

				Array.Copy(Cache, cacheOffset, buffer, offset + dst, cacheSize);
				dst += cacheSize;
				count -= cacheSize;
				cacheOffset = 0;

				block++;
			}

			Offset += dst;
			return dst;
		}

		public override long Seek(long offset, SeekOrigin origin)
		{
			switch (origin) {
				case SeekOrigin.Begin:
					Offset = offset;
					break;
				case SeekOrigin.Current:
					Offset += offset;
					break;
				case SeekOrigin.End:
					Offset = Length + offset;
					break;
				default:
					break;
			}

			return Offset;
		}

		public override void SetLength(long value)
		{
			throw new NotSupportedException();
		}

		public override void Write(byte[] buffer, int offset, int count)
		{
			throw new NotImplementedException();
		}

		private bool DecryptBlock(uint block)
		{
			if (block == CachedBlock)
				return true;

			Stream.Position = Partition.Offset + Partition.DataOffset + (long)block * 0x8000;
			if (Stream.Read(CryptedBuffer, 0, 0x8000) != 0x8000)
				return false;

			byte[] iv = new byte[0x10];
			Array.Copy(CryptedBuffer, 0x3D0, iv, 0, 0x10);
			Util.AesCBC.CreateDecryptor(Partition.Ticket.Key, iv).TransformBlock(CryptedBuffer, 0x400, 0x7C00, Cache, 0);

			CachedBlock = block;

			return true;
		}
	}
}
