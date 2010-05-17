#region Using Shortcuts
using s8 = System.SByte;
using u8 = System.Byte;
using s16 = System.Int16;
using u16 = System.UInt16;
using s32 = System.Int32;
using u32 = System.UInt32;
using s64 = System.Int64;
using u64 = System.UInt64;
#endregion

using System;
using System.IO;
using System.Collections.Generic;
using ConsoleHaxx.Common;
using System.Security.Cryptography;

namespace ConsoleHaxx.Wii
{
	public class SaveHeader
	{
		public const int	ChecksumSize = 0x10;

		public u64			SavegameID;
		public u32			BannerSize;
		public u8			Permissions;
		public u8			Unknown;
		public u8[]			Checksum;
		public u16			Unknown2;

		public SaveHeader()
		{
			Checksum = new u8[ChecksumSize];
		}

		public static SaveHeader Create(Stream stream)
		{
			EndianReader reader = new EndianReader(stream, Endianness.BigEndian);

			SaveHeader header = new SaveHeader();
			header.SavegameID = reader.ReadUInt64();
			header.BannerSize = reader.ReadUInt32();
			header.Permissions = reader.ReadByte();
			header.Unknown = reader.ReadByte();
			reader.ReadBytes(header.Checksum);
			header.Unknown2 = reader.ReadUInt16();

			return header;
		}
	}

	public class SaveBanner
	{
		public const int	Magic		= 0x5749424E;
		public const int	FlagSize	= 4;
		public const int	UnknownSize	= 20;
		public const int	BannerSize	= 0x6000;
		public const int	IconSize	= 0x1200;

		public u32			Unknown;
		public u8[]			Flags;
		public u8[]			Unknown2;
		public u64			TitleID;
		public u64			SubTitleID;
		public u8[]			Banner;
		public List<u8[]>	Icons;

		public SaveBanner(s32 icons)
		{
			Flags = new u8[FlagSize];
			Unknown2 = new u8[UnknownSize];
			Banner = new u8[BannerSize];
			Icons = new List<byte[]>();
			for (int i = 0; i < icons; i++)
				Icons[i] = new byte[IconSize];
		}

		public static SaveBanner Create(Stream stream, s32 icons)
		{
			EndianReader reader = new EndianReader(stream, Endianness.BigEndian);

			if (reader.ReadInt32() != Magic) // "WIBN" Magic
				throw new FormatException();

			SaveBanner banner = new SaveBanner(icons);
			banner.Unknown = reader.ReadUInt32();
			reader.ReadBytes(banner.Flags);
			reader.ReadBytes(banner.Unknown2);
			banner.TitleID = reader.ReadUInt64();
			banner.SubTitleID = reader.ReadUInt64();
			reader.ReadBytes(banner.Banner);

			for (int i = 0; i < icons; i++)
				reader.ReadBytes(banner.Icons[i]);

			return banner;
		}
	}

	public class BackupHeader
	{
		public const int	HeaderSize			= 0x70;
		public const int	Magic				= 0x426B;	// "Bk"
		public const int	Version				= 0x01;
		public const int	PaddingSize			= 0x10;
		public const int	ContentIndexSize	= 0x40;

		public u32			ConsoleID;
		public u32			FileSize;
		public u32			TmdSize;
		public u32			ContentSize;
		public u32			TotalSize;
		public u8[]			ContentIndex;
		public u64			TitleID;
		public u64			MacAddress;
		public u8[]			Padding;
		public List<File>	Files;

		public BackupHeader()
		{
			Padding = new byte[PaddingSize];
			ContentIndex = new byte[ContentIndexSize];
			Files = new List<File>();
		}

		public static BackupHeader Create(Stream stream)
		{
			EndianReader reader = new EndianReader(stream, Endianness.BigEndian);

			if (reader.ReadUInt32() != HeaderSize)
				throw new FormatException();
			if (reader.ReadUInt16() != Magic)
				throw new FormatException();
			if (reader.ReadUInt16() != Version)
				throw new FormatException();

			BackupHeader bk = new BackupHeader();
			bk.ConsoleID = reader.ReadUInt32();
			u32 numfiles = reader.ReadUInt32();
			bk.FileSize = reader.ReadUInt32();
			bk.TmdSize = reader.ReadUInt32();
			bk.ContentSize = reader.ReadUInt32();
			bk.TotalSize = reader.ReadUInt32();
			reader.ReadBytes(bk.ContentIndex);
			bk.TitleID = reader.ReadUInt64();
			bk.MacAddress = reader.ReadUInt64();
			reader.ReadBytes(bk.Padding);

			for (u32 i = 0; i < numfiles; i++)
				bk.Files.Add(File.Create(stream));

			return bk;
		}

		public void Save(Stream stream)
		{
			EndianReader writer = new EndianReader(stream, Endianness.BigEndian);

			writer.Write((u32)0x00000070);
			writer.Write((u16)0x426B);
			writer.Write((u16)0x0001);

			writer.Write(ConsoleID);
			writer.Write((u32)Files.Count);
			writer.Write(FileSize); //writer.Write(CountFileSize()); TODO: k, abstraction
			writer.Write(TmdSize);
			writer.Write(ContentSize);
			writer.Write(TotalSize);
			writer.Write(ContentIndex);
			writer.Write(TitleID);
			writer.Write(MacAddress);
			writer.Write(Padding);

			foreach (File file in Files) {
				file.Save(stream);
			}
		}

		protected u32 CountFileSize()
		{
			u32 count = 0;
			foreach (File file in Files)
				count += file.Size;
			return count;
		}

		public void Extract(string path)
		{
			foreach (File file in Files) {
				FileStream save = new FileStream(Path.Combine(path, file.Name), FileMode.Create);
				Util.StreamCopy(save, file.Data, file.Size);
				save.Close();
				// TODO: Directory
			}
		}

		public class File
		{
			public const int	Magic			= 0x03ADF17E;

			public u32			Size;
			public u8			Permissions;
			public u8			Attributes;
			public string		Name;
			public u8[]			IV;
			public Stream		RawData;
			public Stream		Data;

			public File()
			{
				IV = new u8[Util.AesKeySize];
			}

			public static File Create(Stream stream)
			{
				EndianReader reader = new EndianReader(stream, Endianness.BigEndian);

				if (reader.ReadUInt32() != Magic)
					return null;

				File file = new File();
				file.Size = reader.ReadUInt32();
				file.Permissions = reader.ReadByte();
				file.Attributes = reader.ReadByte();

				bool isfile = false;
				if (stream.ReadByte() == 1)
					isfile = true;

				long pos = stream.Position;
				file.Name = Util.ReadCString(stream);
				// TODO: Abstraction layer to read 0x45 bytes
				stream.Position = pos + 0x45;
				reader.ReadBytes(file.IV);

				// TODO: Padding[] variable
				reader.PadToMultiple(0x40); // Padding

				pos = stream.Position;

				if (isfile) {
					file.RawData = new Substream(stream, stream.Position, Util.RoundUp(file.Size, 0x40));

					file.Data = new CryptoStream(file.RawData, Util.AesCBC.CreateDecryptor(Constants.SdKey, file.IV), CryptoStreamMode.Read);
					file.Data.SetLength(file.Size);

					stream.Position = pos + file.RawData.Length;
				} else {
					// What the fuck do I do with a directory?
				}

				return file;
			}

			public void Save(Stream stream)
			{
				throw new NotImplementedException();
			}
		}
	}

	public class Savegame
	{
		public SaveHeader Header;
		public SaveBanner Banner;
		public BackupHeader Bk;

		public Savegame(Stream stream)
		{
			long pos = stream.Position;
			CryptoStream decrypt = new CryptoStream(stream, Util.AesCBC.CreateDecryptor(Constants.SdKey, Constants.SdIV), CryptoStreamMode.Read);
			Header = SaveHeader.Create(decrypt);
			Banner = SaveBanner.Create(decrypt, (s32)((Header.BannerSize - 0x60A0) / 0x1200));
			stream.Position = pos + 0xF0C0;

			Bk = BackupHeader.Create(stream);
		}
	}
}
