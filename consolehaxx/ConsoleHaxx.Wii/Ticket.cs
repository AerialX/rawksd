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
using System.Security.Cryptography;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.Wii
{
	public struct TicketLimit
	{
		public uint Tag;
		public uint Value;
	}

	public class Ticket
	{
		public const int		Unknown1Size	= 0x3F;
		public const int		Unknown3Size	= 0x3C;
		public const int		ContentMaskSize	= 0x40;
		public const int		LimitSize		= 0x08;
		public const int		TicketSize		= 0x2A4;

		public Signature		Signature;
		public u8[]				Unknown1;
		public u8[]				EncryptedKey;
		public u8				Unknown2;
		public u64				TicketID;
		public u32				ConsoleID;
		public u64				TitleID;
		public u16				AccessMask;
		public u8[]				Unknown3;
		public u8[]				ContentMask;
		public u16				Padding;
		public TicketLimit[]	Limits;

		public Ticket() : this(new Signature(SignatureType.RSA2048)) { }

		public Ticket(Signature signature)
		{
			Signature = signature;
			Unknown1 = new u8[Unknown1Size];
			EncryptedKey = new u8[0x10];
			Unknown3 = new u8[Unknown3Size];
			ContentMask = new u8[ContentMaskSize];

			Limits = new TicketLimit[LimitSize];
			for (int i = 0; i < LimitSize; i++)
				Limits[i] = new TicketLimit();
		}

		public static Ticket Create(Stream stream)
		{
			EndianReader reader = new EndianReader(stream, Endianness.BigEndian);

			Ticket ticket = new Ticket(Signature.Create(stream));
			reader.ReadBytes(ticket.Unknown1);
			reader.ReadBytes(ticket.EncryptedKey);
			ticket.Unknown2 = reader.ReadByte();
			ticket.TicketID = reader.ReadUInt64();
			ticket.ConsoleID = reader.ReadUInt32();
			ticket.TitleID = reader.ReadUInt64();
			ticket.AccessMask = reader.ReadUInt16();
			reader.ReadBytes(ticket.Unknown3);
			reader.ReadBytes(ticket.ContentMask);
			ticket.Padding = reader.ReadUInt16();

			for (int i = 0; i < LimitSize; i++) {
				ticket.Limits[i].Tag = reader.ReadUInt32();
				ticket.Limits[i].Value = reader.ReadUInt32();
			}

			return ticket;
		}

		public void Save(Stream destination)
		{
			EndianReader writer = new EndianReader(destination, Endianness.BigEndian);

			Signature.Save(destination);
			writer.Write(Unknown1);
			writer.Write(EncryptedKey);
			writer.Write(Unknown2);
			writer.Write(TicketID);
			writer.Write(ConsoleID);
			writer.Write(TitleID);
			writer.Write(AccessMask);
			writer.Write(Unknown3);
			writer.Write(ContentMask);
			writer.Write(Padding);

			for (int i = 0; i < LimitSize; i++) {
				writer.Write(Limits[i].Tag);
				writer.Write(Limits[i].Value);
			}
		}

		public byte[] Hash()
		{
			u8[] memory = new u8[TicketSize];
			MemoryStream stream = new MemoryStream(memory, true);
			Save(stream);
			stream.Close();
			return Util.SHA1Hash(memory, Signature.Size);
		}

		public bool Fakesign()
		{
			Util.Memset(Signature.Data, (byte)0);
			for (uint i = 0; i <= u16.MaxValue; i++) {
				Padding = (u16)i;
				if (Hash()[0] == 0)
					return true;
			}

			return false;
		}

		public enum KeyType : byte
		{
			CommonKey = 0,
			KoreanKey = 1
		}

		public byte[] Key
		{
			get
			{
				byte[] commonkey;

				switch (Unknown3[0x0B]) {
					case (u8)KeyType.CommonKey:
						commonkey = Constants.CommonKey;
						break;
					case (u8)KeyType.KoreanKey:
						commonkey = Constants.KoreanCommonKey;
						break;
					default:
						throw new ArgumentException();
				}

				byte[] key = new byte[0x10];
				byte[] iv = new byte[0x10];
				BigEndianConverter.GetBytes(TitleID).CopyTo(iv, 0);
				Util.AesCBC.CreateDecryptor(commonkey, iv).TransformBlock(EncryptedKey, 0, 0x10, key, 0);

				return key;
			}
		}

		public CryptoStream CreateDecryptionStream(TmdContent content, Stream data)
		{
			byte[] iv = new byte[0x10];
			BigEndianConverter.GetBytes(content.Index).CopyTo(iv, 0);
			CryptoStream stream = new CryptoStream(data, Util.AesCBC.CreateDecryptor(Key, iv), CryptoStreamMode.Read);
			stream.SetLength(content.Size);
			return stream;
		}
	}
}
