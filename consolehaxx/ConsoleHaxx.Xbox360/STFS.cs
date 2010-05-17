using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Drawing;
using System.IO;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.Xbox360
{
	public class StfsArchive
	{
		StfsFile Stfs;

		public DirectoryNode Root;

		public StfsArchive()
		{
			Stfs = new StfsFile();
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
				FileDescriptor file = FileDescriptor.Create(blockreader);
				if (file.IsNull())
					break;
				Files.Add(file);
			}
		}

		public class BlockStream : Stream
		{
			public StfsFile Stfs;
			public Stream Base;
			
			protected long Offset;
			protected uint Block;
			protected int BlockOffset;

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
				}
			}

			public override bool CanRead { get { return true; } }

			public override bool CanSeek { get { return true; } }

			public override bool CanWrite { get { return false; } }

			public override void Flush()
			{
				throw new NotImplementedException();
			}

			public override long Length { get { throw new NotImplementedException(); } }

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
				throw new NotImplementedException();
			}

			public override void Write(byte[] buffer, int offset, int count)
			{
				throw new NotImplementedException();
			}
		}

		internal static string ReadString(EndianReader reader)
		{
			return Encoding.BigEndianUnicode.GetString(reader.ReadBytes(0x80)).Trim('\0');
		}

		internal static uint ReadUInt24LE(EndianReader reader)
		{
			byte[] data = new byte[0x04];
			reader.Read(data, 0, 3);
			return LittleEndianConverter.ToUInt32(data);
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

			public static FileDescriptor Create(EndianReader reader)
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

				return file;
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
			}

			public override void Populate(EndianReader reader)
			{
				reader.ReadBytes(Signature);
				reader.ReadBytes(Data);
				for (int i = 0; i < Licenses.Length; i++)
					Licenses[i] = License.Create(reader);
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
				switch (value) {
					case 0x4C495645: // "LIVE"
						return FileTypes.LIVE;
					case 0x50495253: // "PIRS"
						return FileTypes.PIRS;
					case 0x434f4e20: // "CON "
						return FileTypes.CON;
				}
				return FileTypes.Unknown;
			}

			public abstract void Populate(EndianReader reader);
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
			CON,
			LIVE,
			PIRS
		}
	}
}
