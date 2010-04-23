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
	public enum SignatureType : int
	{
		RSA4096			= 0x00010000,
		RSA2048			= 0x00010001,
		EllipticCurve	= 0x00010002
	}

	public class Signature
	{
		public const int		IssuerSize	= 0x40;
		public const int		PaddingSize	= 0x3C;

		public SignatureType	Type;
		public u8[]				Data;
		public u8[]				Padding;
		public u8[]				Issuer;

		protected Signature()
		{
			Issuer = new u8[IssuerSize];
			Padding = new u8[PaddingSize];
		}

		public Signature(SignatureType type) : this()
		{
			Type = type;

			Data = new u8[GetDataSize(Type)];
		}

		public static Signature Create(Stream stream)
		{
			EndianReader reader = new EndianReader(stream, Endianness.BigEndian);

			SignatureType type = (SignatureType)reader.ReadInt32();

			Signature sig = new Signature(type);

			reader.Read(sig.Data, 0, sig.Data.Length);

			reader.Read(sig.Padding, 0, sig.Padding.Length);

			reader.Read(sig.Issuer, 0, sig.Issuer.Length);

			return sig;
		}

		public void Save(Stream stream)
		{
			EndianReader writer = new EndianReader(stream, Endianness.BigEndian);

			writer.Write((s32)Type);
			writer.Write(Data);
			writer.Write(Padding);
			writer.Write(Issuer);
		}

		public static int GetDataSize(SignatureType type)
		{
			switch (type) {
				case SignatureType.RSA2048:
					return 0x100;
				case SignatureType.RSA4096:
					return 0x200;
				case SignatureType.EllipticCurve:
					return 0x40;
				default:
					throw new ArgumentException();
			}
		}

		public int Size
		{
			get {
				return IssuerSize + PaddingSize + Data.Length + sizeof(SignatureType);
			}
		}
	}

	public class Certificate
	{
		public Signature	Signature;
		public u32			ID;
		public u8[]			Name;
		public u8[]			Modulus;
		public u32			Exponent;
		public u8[]			Padding;

		public Certificate(SignatureType type) : this(new Signature(type)) { }

		public Certificate(Signature sig)
		{
			Signature = sig;
			Name = new u8[0x40];
			Padding = new u8[0x34];

			switch (Signature.Type) {
				case SignatureType.RSA4096:
				case SignatureType.RSA2048:
					Modulus = new u8[Signature.Data.Length];
					break;
				default:
					throw new ArgumentException();
			}
		}

		public static Certificate Create(Stream stream)
		{
			Signature sig = Signature.Create(stream);
			Certificate cert = new Certificate(sig);

			EndianReader reader = new EndianReader(stream, Endianness.BigEndian);

			cert.ID = reader.ReadUInt32();
			reader.Read(cert.Name, 0, cert.Name.Length);
			reader.Read(cert.Modulus, 0, cert.Modulus.Length);
			cert.Exponent = reader.ReadUInt32();
			reader.Read(cert.Padding, 0, cert.Padding.Length);

			return cert;
		}

		public void Save(Stream stream)
		{
			Signature.Save(stream);

			EndianReader writer = new EndianReader(stream, Endianness.BigEndian);

			writer.Write(ID);
			writer.Write(Name);
			writer.Write(Modulus);
			writer.Write(Exponent);
			writer.Write(Padding);
		}
	}
}
