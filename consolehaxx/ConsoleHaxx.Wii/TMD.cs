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
using System.Collections.Generic;
using System.IO;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.Wii
{
	public class TMD
	{
		public const int			PaddingSize			= 0x3E;
		public const int			ContentSize			= 0x10 + Util.SHA1HashSize;

		public Signature			Signature;
		public u8					Version;
		public u8					CaCrlVersion;
		public u8					SignerCrlVersion;
		public u8					Padding;
		public u64					SystemVersion;
		public u64					TitleID;
		public u32					TitleType;
		public u16					GroupID;
		public u8[]					Padding2;
		public u32					AccessRights;
		public u16					TitleVersion;
		public u16					BootIndex;
		public u16					Padding3;
		public List<TmdContent>		Contents;

		public TMD() : this(new Signature(SignatureType.RSA2048)) { }

		public TMD(Signature signature)
		{
			Signature = signature;
			Padding2 = new u8[PaddingSize];
			Contents = new List<TmdContent>();
		}

		public static TMD Create(Stream stream)
		{
			u16 numcontents;

			TMD tmd = new TMD(Signature.Create(stream));

			EndianReader reader = new EndianReader(stream, Endianness.BigEndian);

			tmd.Version = reader.ReadByte();
			tmd.CaCrlVersion = reader.ReadByte();
			tmd.SignerCrlVersion = reader.ReadByte();
			tmd.Padding = reader.ReadByte();
			tmd.SystemVersion = reader.ReadUInt64();
			tmd.TitleID = reader.ReadUInt64();
			tmd.TitleType = reader.ReadUInt32();
			tmd.GroupID = reader.ReadUInt16();
			reader.ReadBytes(tmd.Padding2);
			tmd.AccessRights = reader.ReadUInt32();
			tmd.TitleVersion = reader.ReadUInt16();
			numcontents = reader.ReadUInt16();
			tmd.BootIndex = reader.ReadUInt16();
			tmd.Padding3 = reader.ReadUInt16();

			for (int i = 0; i < numcontents; i++)
				tmd.Contents.Add(TmdContent.Create(stream));

			return tmd;
		}

		public int Size
		{
			get {
				return Signature.Size + 38 + PaddingSize + Contents.Count * ContentSize;
			}
		}

		public void Save(Stream destination)
		{
			EndianReader writer = new EndianReader(destination, Endianness.BigEndian);

			Signature.Save(destination);

			writer.Write(Version);
			writer.Write(CaCrlVersion);
			writer.Write(SignerCrlVersion);
			writer.Write(Padding);
			writer.Write(SystemVersion);
			writer.Write(TitleID);
			writer.Write(TitleType);
			writer.Write(GroupID);
			writer.Write(Padding2);
			writer.Write(AccessRights);
			writer.Write(TitleVersion);
			writer.Write((u16)Contents.Count);
			writer.Write(BootIndex);
			writer.Write(Padding3);

			foreach (TmdContent content in Contents)
				content.Save(destination);
		}

		public bool Fakesign()
		{
			Util.Memset(Signature.Data, (byte)0);
			for (int i = 0; i < u16.MaxValue; i++) {
				Padding3 = (u16)i;
				if (Hash()[0] == 0)
					return true;
			}

			return false;
		}

		public byte[] Hash()
		{
			byte[] data = new u8[Size];
			MemoryStream stream = new MemoryStream(data, true);
			Save(stream);
			stream.Close();
			return Util.SHA1Hash(data, Signature.Size, Size - Signature.Size);
		}
	}

	public class TmdContent
	{
		public u32	ContentID;
		public u16	Index;
		public u16	Type;
		public s64	Size;
		public u8[]	Hash;

		public TmdContent()
		{
			Hash = new byte[Util.SHA1HashSize];
		}

		public static TmdContent Create(Stream stream)
		{
			EndianReader reader = new EndianReader(stream, Endianness.BigEndian);

			TmdContent content = new TmdContent();
			content.ContentID = reader.ReadUInt32();
			content.Index = reader.ReadUInt16();
			content.Type = reader.ReadUInt16();
			content.Size = reader.ReadInt64();
			reader.ReadBytes(content.Hash);
			
			return content;
		}

		public void Save(Stream destination)
		{
			EndianReader writer = new EndianReader(destination, Endianness.BigEndian);

			writer.Write(ContentID);
			writer.Write(Index);
			writer.Write(Type);
			writer.Write(Size);
			writer.Write(Hash);
		}
	}
}
