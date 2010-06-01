using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Drawing;
using System.IO;
using ConsoleHaxx.Common;
using System.Drawing.Imaging;
using System.Security.Cryptography;

namespace ConsoleHaxx.Xbox360
{
	public class StfsArchive
	{
		public StfsFile Stfs;

		public DirectoryNode Root;

		public StfsArchive()
		{
			Stfs = new StfsFile();
			Root = new DirectoryNode();
		}

		public StfsArchive(Stream stream)
		{
			Stfs = new StfsFile(stream);

			Root = new DirectoryNode();
			Dictionary<int, DirectoryNode> Directories = new Dictionary<int, DirectoryNode>();
			Directories.Add(0xFFFF, Root);

			for (int i = 0; i < Stfs.Files.Count; i++) {
				StfsFile.FileDescriptor file = Stfs.Files[i];
				DirectoryNode parent = Directories[file.Parent];
				if ((file.Flags & 0x80) != 0) {
					DirectoryNode directory = new DirectoryNode(file.Name, parent);
					Directories.Add(i, directory);
				} else
					new FileNode(file.Name, parent, file.Size, new Substream(Stfs.Stream, Stfs.Stream.GetBlockOffset(file.Block), file.Size));
			}
		}

		public void Save(Stream stream)
		{
			Stfs.Files.Clear();
			uint block = 1;
			SaveFolder(Root, 0xFFFF, ref block);
			Stfs.Save(stream);
		}

		private void SaveFolder(DirectoryNode Root, ushort parent, ref uint block)
		{
			foreach (var node in Root.Children) {
				StfsFile.FileDescriptor desc = new StfsFile.FileDescriptor();
				if (node.Name.Length > 0x3F)
					throw new FormatException();
				desc.Name = node.Name;
				desc.Flags = (byte)desc.Name.Length;
				desc.Flags |= 0x40; // Unknown bit, seems to be set.
				desc.Parent = parent;
				Stfs.Files.Add(desc);
				if (node is DirectoryNode) {
					desc.Flags |= 0x80;
					SaveFolder(node as DirectoryNode, (ushort)Stfs.Files.IndexOf(desc), ref block);
				} else if (node is FileNode) {
					FileNode file = node as FileNode;
					desc.Size = (uint)file.Size;
					desc.Block = block;
					desc.BlockSize = (uint)(Util.RoundUp(file.Size, StfsFile.BlockSize) / StfsFile.BlockSize);
					desc.Data = file.Data;
					block += desc.BlockSize;
				}
			}
		}
	}

	public class StfsFile
	{
		public const int			BlockSize	= 0x1000;
		// internal ushort BaseBlock { get { return (ushort)(xBaseByte << 0xC); } }
		// xBaseByte = Type0 ? 0x0B : 0x0A
		// xBaseByte = (byte)(((xBlockInfo + 0xFFF) & 0xF000) >> 0xC); // 0x340

		public PackageHeader		Header;				// 0x0000 - 0x032C - 0x032C
		public byte[]				ContentID;			// 0x032C - 0x0014 - 0x0340 - SHA1 hash of header
		public uint					EntryID;			// 0x0340 - 0x0004 - 0x0344
		public ContentTypes			ContentType;		// 0x0344 - 0x0004 - 0x0348
		public uint					MetadataVersion;	// 0x0348 - 0x0004 - 0x034C
		public ulong				ContentSize;		// 0x034C - 0x0008 - 0x0354
		public uint					MediaID;			// 0x0354 - 0x0004 - 0x0358
		public uint					Version;			// 0x0358 - 0x0004 - 0x035C
		public uint					BaseVersion;		// 0x035C - 0x0004 - 0x0360
		public uint					TitleID;			// 0x0360 - 0x0004 - 0x0364
		public byte					Platform;			// 0x0364 - 0x0001 - 0x0365
		public byte					ExecutableType;		// 0x0365 - 0x0001 - 0x0366
		public byte					DiscNumber;			// 0x0366 - 0x0001 - 0x0367
		public byte					DiscInSet;			// 0x0367 - 0x0001 - 0x0368
		public uint					SaveGameID;			// 0x0368 - 0x0004 - 0x036C
		public byte[]				ConsoleID;			// 0x036C - 0x0005 - 0x0371
		public ulong				ProfileID;			// 0x0371 - 0x0008 - 0x0379
		public PackageDescriptor	Descriptor;			// 0x0379 - 0x0024 - 0x039D
		public uint					DataFiles;			// 0x039D - 0x0004 - 0x03A1
		public ulong				DataFileSize;		// 0x03A1 - 0x0008 - 0x03A9
		public ulong				Reserved;			// 0x03A9 - 0x0008 - 0x03B1
		public byte[]				Padding;			// 0x03B1 - 0x004C - 0x03FD
		public byte[]				DeviceID;			// 0x03FD - 0x0014 - 0x0411
		public string[]				DisplayName;		// 0x0411 - 0x0900 - 0x0D11
		public string[]				DisplayDescription;	// 0x0D11 - 0x0900 - 0x1611
		public string				Publisher;			// 0x1611 - 0x0080 - 0x1691
		public string				TitleName;			// 0x1691 - 0x0080 - 0x1711
		public byte					TransferFlags;		// 0x1711 - 0x0001 - 0x1712
		public uint					ThumbnailSize;		// 0x1712 - 0x0004 - 0x1716
		public uint					TitleThumbnailSize;	// 0x1716 - 0x0004 - 0x171A
		public Bitmap				Thumbnail;			// 0x171A - 0x4000 - 0x571A
		public Bitmap				TitleThumbnail;		// 0x571A - 0x4000 - 0x971A

		public uint BaseBlock { get; set; }

		public List<FileDescriptor> Files { get; set; }

		public BlockStream Stream;

		public StfsFile()
		{
			ContentID = new byte[0x14];
			ConsoleID = new byte[0x05];
			Padding = new byte[0x4C];
			DeviceID = new byte[0x14];
			DisplayName = new string[0x12];
			DisplayDescription = new string[0x12];
			BaseBlock = 0x0C;
			EntryID = 0xAD0E;

			Files = new List<FileDescriptor>();
		}

		public StfsFile(Stream stream) : this()
		{
			EndianReader reader = new EndianReader(stream, Endianness.BigEndian);

			Header = PackageHeader.Create(reader);

			if (Header == null)
				throw new FormatException();

			reader.ReadBytes(ContentID);
			EntryID = reader.ReadUInt32();
			ContentType = (ContentTypes)reader.ReadUInt32();
			MetadataVersion = reader.ReadUInt32();
			ContentSize = reader.ReadUInt64();
			MediaID = reader.ReadUInt32();
			Version = reader.ReadUInt32();
			BaseVersion = reader.ReadUInt32();
			TitleID = reader.ReadUInt32();
			Platform = reader.ReadByte();
			ExecutableType = reader.ReadByte();
			DiscNumber = reader.ReadByte();
			DiscInSet = reader.ReadByte();
			SaveGameID = reader.ReadUInt32();
			reader.ReadBytes(ConsoleID);
			ProfileID = reader.ReadUInt64();
			Descriptor = PackageDescriptor.Create(reader);
			DataFiles = reader.ReadUInt32();
			DataFileSize = reader.ReadUInt64();
			Reserved = reader.ReadUInt64();
			reader.ReadBytes(Padding);
			reader.ReadBytes(DeviceID);
			for (int i = 0; i < DisplayName.Length; i++)
				DisplayName[i] = ReadString(reader);
			for (int i = 0; i < DisplayDescription.Length; i++)
				DisplayDescription[i] = ReadString(reader);
			Publisher = ReadString(reader);
			TitleName = ReadString(reader);
			TransferFlags = reader.ReadByte();
			ThumbnailSize = reader.ReadUInt32();
			TitleThumbnailSize = reader.ReadUInt32();

			Thumbnail = new Bitmap(new Substream(stream, 0x171A, ThumbnailSize));
			TitleThumbnail = new Bitmap(new Substream(stream, 0x571A, TitleThumbnailSize));

			// None of this seems right, I'm hardcoding 0x0C
			// BaseBlock = ((EntryID + 0xFFF) & 0xF000) / BlockSize;
			// BaseBlock = (uint)(Util.RoundUp(EntryID, BlockSize) / BlockSize) + 1;

			Stream = new BlockStream(this, stream);
			Stream.SeekBlock(Descriptor.FileTableBlock);
			EndianReader blockreader = new EndianReader(Stream, Endianness.BigEndian);
			while (true) {
				FileDescriptor file = FileDescriptor.Create(blockreader, Stream);
				if (file.IsNull())
					break;
				Files.Add(file);
			}
		}

		public void Save(Stream stream)
		{
			EndianReader writer = new EndianReader(stream, Endianness.BigEndian);

			Header.Save(writer);

			writer.Write(ContentID);
			writer.Write(EntryID);
			writer.Write((uint)ContentType);
			writer.Write(MetadataVersion);
			writer.Write(ContentSize);
			writer.Write(MediaID);
			writer.Write(Version);
			writer.Write(BaseVersion);
			writer.Write(TitleID);
			writer.Write(Platform);
			writer.Write(ExecutableType);
			writer.Write(DiscNumber);
			writer.Write(DiscInSet);
			writer.Write(SaveGameID);
			writer.Write(ConsoleID);
			writer.Write(ProfileID);
			Descriptor.Save(writer);
			writer.Write(DataFiles);
			writer.Write(DataFileSize);
			writer.Write(Reserved);
			writer.Write(Padding);
			writer.Write(DeviceID);
			for (int i = 0; i < DisplayName.Length; i++)
				WriteString(writer, DisplayName[i]);
			for (int i = 0; i < DisplayDescription.Length; i++)
				WriteString(writer, DisplayDescription[i]);
			WriteString(writer, Publisher);
			WriteString(writer, TitleName);
			writer.Write(TransferFlags);

			MemoryStream thumbnail = new MemoryStream();
			new Bitmap(Thumbnail, 64, 64).Save(thumbnail, ImageFormat.Png);
			MemoryStream titlethumbnail = new MemoryStream();
			new Bitmap(TitleThumbnail, 64, 64).Save(titlethumbnail, ImageFormat.Png);

			if (thumbnail.Length > 0x4000 || titlethumbnail.Length > 0x4000)
				throw new FormatException();

			writer.Write((uint)thumbnail.Length);
			writer.Write((uint)titlethumbnail.Length);

			writer.Position = 0x171A;
			thumbnail.Position = 0;
			Util.StreamCopy(writer, thumbnail);
			thumbnail.Close();

			writer.Position = 0x571A;
			titlethumbnail.Position = 0;
			Util.StreamCopy(writer, titlethumbnail);
			titlethumbnail.Close();

			BlockStream block = new BlockStream(this, stream);
			block.SeekBlock(Descriptor.FileTableBlock);
			EndianReader blockwriter = new EndianReader(block, Endianness.BigEndian);
			foreach (FileDescriptor file in Files) {
				file.Save(blockwriter);
			}
			FileDescriptor.Null.Save(blockwriter);

			foreach (FileDescriptor file in Files) {
				if ((file.Flags & 0x80) == 0) {
					block.SeekBlock(file.Block);
					file.Data.Position = 0;
					Util.StreamCopy(block, file.Data, file.Size);
				}
			}

			block.Position = block.Length;
			blockwriter.PadToMultiple(BlockSize);

			uint current = 0;
			uint blocks = (uint)(block.Length / BlockSize);
			uint subhashblock = 0;
			HashTable master = new HashTable();
			while (current < blocks) {
				HashTable hashes = new HashTable();
				block.SeekBlock(current);
				hashes.HashBlocks(blockwriter, current, blocks - current);
				for (uint fileblock = 0; fileblock < 0xAA; fileblock++) {
					HashTable.Hash hash = hashes.Hashes[fileblock];
					if (hash == null)
						continue;
					foreach (FileDescriptor file in Files) {
						if ((file.Flags & 0x80) == 0) {
							if (fileblock + current == file.Block + file.BlockSize - 1) {
								hash.ID = 0x80FFFFFF;
							}
						}
					}
					if (fileblock + current == 0)
						hash.ID = 0x80FFFFFF; // File table counts as a file
				}
				current += 0xAA;
				long position = BlockSize * 0xB;
				if (subhashblock > 0 && blocks > 0xAA)
					position = BlockSize * (0x0C + subhashblock * (0xB7 - 0x0C));
				stream.Position = position;
				hashes.Save(writer);
				stream.Position = position;
				master.HashBlock(stream, (int)(subhashblock % 0xAA), 0);
				subhashblock++;
			}
			stream.Position = BlockSize * 0xB6;
			master.Save(writer);
			if (blocks > 0xAA) {
				stream.Position = 0xB6FF0;
				writer.Write(blocks);
			}
			stream.Position = 0x395;
			writer.Write(blocks);

			writer.Position = 0x34C;
			writer.Write((ulong)(stream.Length - 0xB * BlockSize));

			stream.Position = blocks > 0xAA ? (BlockSize * 0xB6) : (BlockSize * 0x0B);
			byte[] sha1 = Util.SHA1Hash(stream, BlockSize);
			writer.Position = 0x381;
			writer.Write(sha1);

			stream.Position = 0x344;
			sha1 = Util.SHA1Hash(stream, 0xACBC);
			writer.Position = 0x32C;
			writer.Write(sha1);

			stream.Position = 0x22C;
			sha1 = Util.SHA1Hash(stream, 0x118);
			writer.Position = 4;
			RSACryptoServiceProvider rsa = new RSACryptoServiceProvider();
			RSAPKCS1SignatureFormatter rsaformat = new RSAPKCS1SignatureFormatter();
			RSAParameters rsaparams = new RSAParameters();
			EndianReader reader = new EndianReader(new MemoryStream(Properties.Resources.XK4), Endianness.LittleEndian);
			rsaparams.Exponent = new byte[] { 0, 0, 0, 3 };
			rsaparams.D = Properties.Resources.XK3;
			reader.Position = 0;
			rsaparams.Modulus = reader.ReadBytes(0x100);
			rsaparams.P = reader.ReadBytes(0x80);
			rsaparams.Q = reader.ReadBytes(0x80);
			rsaparams.DP = reader.ReadBytes(0x80);
			rsaparams.DQ = reader.ReadBytes(0x80);
			rsaparams.InverseQ = reader.ReadBytes(0x80);
			rsa.ImportParameters(rsaparams);
			rsaformat.SetHashAlgorithm("SHA1");
			rsaformat.SetKey(rsa);
			byte[] signature = rsaformat.CreateSignature(sha1);
			reader.Base.Close();
			Array.Reverse(signature);
			writer.Write(signature);
			writer.Pad(0x128);
		}

		public class HashTable
		{
			public Hash[] Hashes;

			public HashTable()
			{
				Hashes = new Hash[0xAA];
			}

			public static HashTable Create(EndianReader reader)
			{
				HashTable table = new HashTable();
				for (int i = 0; i < table.Hashes.Length; i++)
					table.Hashes[i] = Hash.Create(reader);
				return table;
			}

			public void HashBlocks(Stream stream, uint current, uint number)
			{
				number = Math.Min(0xAA, number);
				for (int i = 0; i < number; i++, current++) {
					HashBlock(stream, i, 0x80000000 | (current + 1));
				}
			}

			public void HashBlock(Stream stream, int index, uint id)
			{
				Hashes[index] = new Hash(Util.SHA1Hash(stream, BlockSize), id);
			}

			public void Save(EndianReader writer)
			{
				foreach (Hash hash in Hashes) {
					if (hash == null)
						new Hash().Save(writer);
					else
						hash.Save(writer);
				}
			}

			public class Hash
			{
				public byte[] SHA1;
				public uint ID;

				public Hash()
				{
					SHA1 = new byte[0x14];
				}

				public Hash(byte[] hash, uint id)
				{
					SHA1 = hash;
					ID = id;
				}

				public static Hash Create(EndianReader reader)
				{
					Hash hash = new Hash();

					reader.ReadBytes(hash.SHA1);
					hash.ID = reader.ReadUInt32();

					return hash;
				}

				public void Save(EndianReader writer)
				{
					writer.Write(SHA1);
					writer.Write(ID);
				}
			}
		}

		public class BlockStream : Stream
		{
			public StfsFile Stfs;
			public Stream Base;
			
			protected long Offset;
			protected uint Block;
			protected int BlockOffset;
			protected long length;

			public BlockStream(StfsFile stfs, Stream stream)
			{
				Base = stream;
				Stfs = stfs;
				Position = 0;
			}

			public void SeekBlock(uint block)
			{
				Position = GetBlockOffset(block);
			}

			public long GetBlockOffset(uint block)
			{
				return block * StfsFile.BlockSize;
			}

			public override long Position
			{
				get
				{
					return Offset;
				}
				set
				{
					Block = (uint)(value / StfsFile.BlockSize);
					// One hash block per 0xAA data blocks, starts at the beginning of the data blocks (so RoundDown(block, 0xAA) / 0xAA + 1)
					// One master hash block per 0xAA * 0xAA blocks (0x70E4). Starts at the beginning of the sector, except in the case of sector zero, where it starts before the *second* subhash.
					Block = Stfs.BaseBlock + Block +
						((Block < 0xAA) ? 0 : (uint)(Util.RoundDown(Block, 0x70E4) / 0x70E4 + 1)) +
						(uint)Util.RoundDown(Block, 0xAA) / 0xAA;
					BlockOffset = (int)(value % StfsFile.BlockSize);
					long position = Block * StfsFile.BlockSize + BlockOffset;
					if (Base.Position != position)
						Base.Position = position;
					Offset = value;
					if (Offset > Length)
						SetLength(Offset);
				}
			}

			public override bool CanRead { get { return true; } }

			public override bool CanSeek { get { return true; } }

			public override bool CanWrite { get { return false; } }

			public override void Flush()
			{
				throw new NotImplementedException();
			}

			public override long Length { get { return length; } }

			public override int Read(byte[] buffer, int offset, int count)
			{
				int pos = 0;
				while (pos < count) {
					int read = Math.Min(StfsFile.BlockSize - BlockOffset, count - pos);
					read = Base.Read(buffer, offset + pos, read);
					if (read == 0)
						break;

					pos += read;
					Offset += read;
					BlockOffset += read;

					if (BlockOffset == StfsFile.BlockSize)
						Position = Offset;
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

				return Position;
			}

			public override void SetLength(long value)
			{
				length = value;
			}

			public override void Write(byte[] buffer, int offset, int count)
			{
				int pos = 0;
				while (pos < count) {
					int read = Math.Min(StfsFile.BlockSize - BlockOffset, count - pos);
					Base.Write(buffer, offset + pos, read);

					pos += read;
					Offset += read;
					BlockOffset += read;

					if (BlockOffset == StfsFile.BlockSize)
						Position = Offset;

					if (Offset > Length)
						SetLength(Offset);
				}
			}
		}

		internal static string ReadString(EndianReader reader)
		{
			return Encoding.BigEndianUnicode.GetString(reader.ReadBytes(0x80)).Trim('\0');
		}

		internal static void WriteString(EndianReader writer, string str)
		{
			writer.Write(Encoding.BigEndianUnicode.GetBytes(str.PadRight(0x40, '\0')));
		}

		internal static uint ReadUInt24LE(EndianReader reader)
		{
			byte[] data = new byte[0x04];
			reader.Read(data, 0, 3);
			return LittleEndianConverter.ToUInt32(data);
		}

		internal static void WriteUInt24LE(EndianReader writer, uint value)
		{
			byte[] data = LittleEndianConverter.GetBytes(value);
			writer.Write(data, 0, 3);
		}

		public class FileDescriptor
		{
			public string Name;
			public byte Flags;
			public uint BlockSize; // uint24le
			public uint Block; // uint24le
			public ushort Parent;
			public uint Size;
			public uint UpdateTime;
			public uint AccessTime;

			public Stream Data;

			public FileDescriptor()
			{
				Name = string.Empty;
			}

			public static FileDescriptor Null { get { return new FileDescriptor(); } }

			public static FileDescriptor Create(EndianReader reader, BlockStream block)
			{
				FileDescriptor file = new FileDescriptor();

				file.Name = reader.ReadString(0x28);
				byte namelen = reader.ReadByte();
				file.Flags = (byte)(namelen & 0xC0);
				file.BlockSize = ReadUInt24LE(reader);
				if (ReadUInt24LE(reader) != file.BlockSize)
					throw new FormatException();
				file.Block = ReadUInt24LE(reader);
				file.Parent = reader.ReadUInt16();
				file.Size = reader.ReadUInt32();
				file.UpdateTime = reader.ReadUInt32();
				file.AccessTime = reader.ReadUInt32();

				if ((file.Flags & 0x80) == 0)
					file.Data = new Substream(block, block.GetBlockOffset(file.Block), file.Size);

				return file;
			}

			public void Save(EndianReader writer)
			{
				writer.Write(Name, 0x28);
				writer.Write((byte)(Name.Length | Flags));
				WriteUInt24LE(writer, BlockSize);
				WriteUInt24LE(writer, BlockSize);
				WriteUInt24LE(writer, Block);
				writer.Write(Parent);
				writer.Write(Size);
				writer.Write(UpdateTime);
				writer.Write(AccessTime);
			}

			public bool IsNull()
			{
				return Name.Length == 0 && Flags == 0 && BlockSize == 0 && Block == 0 && Parent == 0 && Size == 0 && UpdateTime == 0 && AccessTime == 0;
			}
		}

		public class PackageDescriptor
		{
			public byte StructSize;
			public byte Reserved;
			public byte BlockSeparation;
			public ushort FileTableBlockCount;
			public uint FileTableBlock; // uint24le
			public byte[] HashTableHash;
			public uint TotalAllocatedBlocks;
			public uint TotalUnallocatedBlocks;

			public PackageDescriptor()
			{
				HashTableHash = new byte[0x14];
			}

			public static PackageDescriptor Create(EndianReader reader)
			{
				PackageDescriptor descriptor = new PackageDescriptor();

				descriptor.StructSize = reader.ReadByte();
				descriptor.Reserved = reader.ReadByte();
				descriptor.BlockSeparation = reader.ReadByte();
				descriptor.FileTableBlockCount = reader.ReadUInt16();
				descriptor.FileTableBlock = ReadUInt24LE(reader);
				reader.ReadBytes(descriptor.HashTableHash);
				descriptor.TotalAllocatedBlocks = reader.ReadUInt32();
				descriptor.TotalUnallocatedBlocks = reader.ReadUInt32();

				return descriptor;
			}

			public void Save(EndianReader writer)
			{
				writer.Write(StructSize);
				writer.Write(Reserved);
				writer.Write(BlockSeparation);
				writer.Write(FileTableBlockCount);
				WriteUInt24LE(writer, FileTableBlock);
				writer.Write(HashTableHash);
				writer.Write(TotalAllocatedBlocks);
				writer.Write(TotalUnallocatedBlocks);
			}
		}

		public static class Locales
		{
			public const int English	= 0;
			public const int Japanese	= 1;
			public const int German		= 2;
			public const int French		= 3;
			public const int Spanish	= 4;
			public const int Italian	= 5;
			public const int Korean		= 6;
			public const int Chinese	= 7;
			public const int Portuguese	= 8;
		}

		public class SignedHeader : PackageHeader
		{
			public byte[]		Signature;	// 0x0004 - 0x0100 - 0x0104
			public byte[]		Data;		// 0x0104 - 0x0128 - 0x022C
			public License[]	Licenses;	// 0x022C - 0x0100 - 0x032C

			public SignedHeader(FileTypes type) : base(type)
			{
				Signature = new byte[0x100];
				Data = new byte[0x128];
				Licenses = new License[0x10];
				for (int i = 0; i < Licenses.Length; i++)
					Licenses[i] = new License();
			}

			public override void Populate(EndianReader reader)
			{
				reader.ReadBytes(Signature);
				reader.ReadBytes(Data);
				for (int i = 0; i < Licenses.Length; i++)
					Licenses[i] = License.Create(reader);
			}

			public override void Save(EndianReader writer)
			{
				WriteMagic(writer);
				writer.Write(Signature);
				writer.Write(Data);
				foreach (License license in Licenses)
					license.Save(writer);
			}
		}

		public class License
		{
			public ulong	ID;		// 0x00 - 0x08 - 0x08
			public uint		Bits;	// 0x08 - 0x04 - 0x0C
			public uint		Flags;	// 0x0C - 0x04 - 0x10

			public static License Create(EndianReader reader)
			{
				License license = new License();
				license.ID = reader.ReadUInt64();
				license.Bits = reader.ReadUInt32();
				license.Flags = reader.ReadUInt32();
				return license;
			}

			public void Save(EndianReader writer)
			{
				writer.Write(ID);
				writer.Write(Bits);
				writer.Write(Flags);
			}
		}

		public class ConHeader : PackageHeader
		{
			public byte[]	ConsoleData;	// 0x0004 - 0x01A8 - 0x01AC
			public byte[]	FileData;		// 0x01AC - 0x0180 - 0x032C

			public ConHeader() : base(FileTypes.CON)
			{
				ConsoleData = new byte[0x1A8];
				FileData = new byte[0x180];
			}

			public override void Populate(EndianReader reader)
			{
				reader.ReadBytes(ConsoleData);
				reader.ReadBytes(FileData);
			}

			public override void Save(EndianReader writer)
			{
				WriteMagic(writer);
				writer.Write(ConsoleData);
				writer.Write(FileData);
			}
		}

		public abstract class PackageHeader
		{
			public FileTypes Type; // 0x0000 - 0x0004 - 0x0004

			protected PackageHeader(FileTypes type) { Type = type; }

			public static PackageHeader Create(EndianReader reader)
			{
				PackageHeader header;
				FileTypes type = (FileTypes)FromMagic(reader.ReadUInt32());
				switch (type) {
					case FileTypes.PIRS:
					case FileTypes.LIVE:
						header = new SignedHeader(type);
						break;
					case FileTypes.CON:
						header = new ConHeader();
						break;
					default:
						return null;
				}

				header.Populate(reader);

				return header;
			}

			private static FileTypes FromMagic(uint value)
			{
				FileTypes type = (FileTypes)value;
				if (type != FileTypes.CON && type != FileTypes.LIVE && type != FileTypes.PIRS)
					return FileTypes.Unknown;
				return type;
			}

			protected void WriteMagic(EndianReader writer)
			{
				writer.Write((uint)Type);
			}

			public abstract void Populate(EndianReader reader);

			public abstract void Save(EndianReader writer);
		}

		public enum ContentTypes : int
		{
			Unknown =			0x00000000,
			SaveGame =			0x00000001,
			Marketplace =		0x00000002,
			Publisher =			0x00000003,
			IptvDVR =			0x00000FFD,
			Xbox360Title =		0x00001000,
			IptvPauseBuffer =	0x00002000,
			XNACommunity =		0x00003000,
			InstalledGame =		0x00004000,
			XboxTitle =			0x00005000,
			SocialTitle =		0x00006000,
			GamesOnDemand =		0x00007000,
			SystemPacks =		0x00008000,
			AvatarItem =		0x00009000,
			Profile =			0x00010000,
			GamerPicture =		0x00020000,
			ThematicSkin =		0x00030000,
			Cache =				0x00040000,
			StorageDownload =	0x00050000,
			XboxSavedGame =		0x00060000,
			XboxDownload =		0x00070000,
			GameDemo =			0x00080000,
			Video =				0x00090000,
			GameTitle =			0x000A0000,
			Installer =			0x000B0000,
			GameTrailer =		0x000C0000,
			Arcade =			0x000D0000,
			XNA =				0x000E0000,
			LicenseStore =		0x000F0000,
			Movie =				0x00100000,
			TV =				0x00200000,
			MusicVideo =		0x00300000,
			GameVideo =			0x00400000,
			PodcastVideo =		0x00500000,
			ViralVideo =		0x00600000,
			CommunityGame =		0x02000000
		}

		public enum FileTypes
		{
			Unknown = 0,
			CON = 0x434f4e20,
			LIVE = 0x4C495645,
			PIRS = 0x50495253
		}
	}
}
