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
using System.Security.Cryptography;

namespace ConsoleHaxx.Wii
{
	public class DlcBin
	{
		public BackupHeader	Bk;
		public TMD			TMD;
		public TmdContent	Content;
		public Stream		Data;
		public Stream		RawData;
		public u8[]			Key;

		public DlcBin()
		{
			Bk = new BackupHeader();
			TMD = new TMD();
			Key = new u8[Util.AesKeySize];
		}

		public DlcBin(Stream stream)
		{
			Key = new u8[Util.AesKeySize];
			Bk = BackupHeader.Create(stream);
			stream.Position = Util.RoundUp(stream.Position, 0x40);
			TMD = TMD.Create(stream);
			stream.Position = Util.RoundUp(stream.Position, 0x40);

			u16 index = u16.MaxValue;
			for (int i = 0; i < Bk.ContentIndex.Length; i++) {
				if (Bk.ContentIndex[i] != 0) {
					index = (u16)(i * 8);
					for (int k = 1; k < 8; k++)
						if ((Bk.ContentIndex[i] & (1 << k)) != 0)
							index += (u16)k;
				}
			}

			if (index != ushort.MaxValue)
				Content = TMD.Contents[index];

			RawData = new Substream(stream, stream.Position, Content != null ? Util.RoundUp(Content.Size, 0x40) : (stream.Length - stream.Position));

			if (Content != null) {
				byte[] iv = new byte[0x10];
				BigEndianConverter.GetBytes(Content.Index).CopyTo(iv, 0);
				Data = new Substream(new AesStream(RawData, Key, iv), 0, Content.Size);
			}
		}

		// Prepares the backup header for you, assuming that a Content and data stream has been assigned
		public void Generate()
		{
			Bk.TitleID = TMD.TitleID;
			Util.Memset(Bk.ContentIndex, (byte)0);
			Bk.ContentIndex[Content.Index / 8] = (byte)(1 << (Content.Index % 8));
			Bk.TmdSize = (u32)TMD.Size;
			Bk.ContentSize = (u32)Util.RoundUp((RawData != null ? RawData : Data).Length, 0x40);
			Bk.TotalSize = (u32)(0x80 + Util.RoundUp(Bk.TmdSize, 0x40) + Bk.ContentSize);
		}

		public void Save(Stream stream)
		{
			EndianReader writer = new EndianReader(stream, Endianness.BigEndian);

			Bk.Save(stream);
			writer.PadToMultiple(0x40);

			TMD.Save(stream);
			writer.PadToMultiple(0x40);

			if (RawData != null) {
				RawData.Position = 0;
				Util.StreamCopy(stream, RawData);
			} else if (Content != null) {
				byte[] iv = new byte[0x10];
				BigEndianConverter.GetBytes(Content.Index).CopyTo(iv, 0);
				CryptoStream astream = new CryptoStream(stream, Util.AesCBC.CreateEncryptor(Key, iv), CryptoStreamMode.Write);
				Data.Position = 0;
				Util.StreamCopy(astream, Data);
				astream.Close();
			}

			writer.PadToMultiple(0x40);
		}
	}
}
