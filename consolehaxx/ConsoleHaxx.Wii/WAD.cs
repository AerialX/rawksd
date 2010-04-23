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
using ConsoleHaxx.Common;
using System.IO;
using System.Security.Cryptography;

namespace ConsoleHaxx.Wii
{
	public class WAD
	{
		public u32			HeaderSize;
		public u32			WadType;
		public u32			CertificateChainSize;
		public u32			Reserved;
		public u32			TicketSize;
		public u32			TmdSize;
		public u32			DataSize;
		public u32			FooterSize;

		public Ticket		Ticket;
		public TMD			TMD;
		public Stream		DataEncrypted;
		public List<Stream>	Data;
		public u8[]			Footer;

		public WAD()
		{
			Data = new List<Stream>();
		}

		public static WAD Create(Stream stream)
		{
			EndianReader reader = new EndianReader(stream, Endianness.BigEndian);

			WAD wad = new WAD();
			wad.HeaderSize = reader.ReadUInt32();
			wad.WadType = reader.ReadUInt32();
			wad.CertificateChainSize = reader.ReadUInt32();
			wad.Reserved = reader.ReadUInt32();
			wad.TicketSize = reader.ReadUInt32();
			wad.TmdSize = reader.ReadUInt32();
			wad.DataSize = reader.ReadUInt32();
			wad.FooterSize = reader.ReadUInt32();

			reader.PadToMultiple(0x40);
			reader.Seek(wad.CertificateChainSize, SeekOrigin.Current);

			reader.PadToMultiple(0x40);
			wad.Ticket = Ticket.Create(stream);

			reader.PadToMultiple(0x40);
			wad.TMD = TMD.Create(stream);

			reader.PadToMultiple(0x40);
			wad.DataEncrypted = new Substream(reader.Base, reader.Position, wad.DataSize);
			long offset = 0;
			foreach (TmdContent content in wad.TMD.Contents) {
				byte[] iv = new byte[0x10];
				BigEndianConverter.GetBytes(content.Index).CopyTo(iv, 0);
				wad.Data.Add(new CryptoStream(new Substream(wad.DataEncrypted, offset), Util.AesCBC.CreateDecryptor(wad.Ticket.Key, iv), CryptoStreamMode.Read));

				offset = (long)Util.RoundUp(offset + content.Size, 0x40);
			}
			stream.Position += wad.DataSize;

			reader.PadToMultiple(0x40);
			wad.Footer = reader.ReadBytes((int)wad.FooterSize);

			return wad;
		}
	}
}
