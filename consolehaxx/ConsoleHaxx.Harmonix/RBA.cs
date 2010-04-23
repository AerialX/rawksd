using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Security.Cryptography;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.Harmonix
{
	public class RBA
	{
		public const int Magic = 0x46534252; // "RBSF"

		public Stream Data;
		public Stream Chart;
		public Stream Audio;
		public Stream Milo;
		public Stream AlbumArt;
		public Stream Weights;
		public Stream Metadata;
		public List<string> Strings;

		public RBA()
		{
			Strings = new List<string>();
		}

		public RBA(EndianReader reader) : this()
		{
			if (reader.ReadInt32() != Magic) // "RBSF"
				throw new FormatException();

			if (reader.ReadInt32() != 0x3) // version
				throw new FormatException();

			int files = 0x7;
			uint[] offsets = new uint[files];
			uint[] sizes = new uint[files];
			for (int i = 0; i < files; i++)
				offsets[i] = reader.ReadUInt32();
			for (int i = 0; i < files; i++)
				sizes[i] = reader.ReadUInt32();

			reader.ReadBytes(Util.SHA1HashSize * 8); // SHA1 Hashes (7 + header hash)
			// No, I'm not going to bother verifying them

			string current = string.Empty;
			while (reader.Position < offsets[0]) {
				byte b = reader.ReadByte();
				if (b == 0) {
					Strings.Add(current);
					current = string.Empty;
				} else
					current += (char)b;
			}

			Data = new Substream(reader, offsets[0], sizes[0]);
			Chart = new Substream(reader, offsets[1], sizes[1]);
			Audio = new Substream(reader, offsets[2], sizes[2]);
			Milo = new Substream(reader, offsets[3], sizes[3]);
			AlbumArt = new Substream(reader, offsets[4], sizes[4]);
			Weights = new Substream(reader, offsets[5], sizes[5]);
			Metadata = new Substream(reader, offsets[6], sizes[6]);
		}

		public void Save(EndianReader stream)
		{
			MemoryStream strings = new MemoryStream();
			foreach (string str in Strings) {
				strings.Write(Util.Encoding.GetBytes(str), 0, str.Length);
				strings.WriteByte(0);
			}

			MemoryStream mem = new MemoryStream();
			EndianReader writer = new EndianReader(mem, stream.Endian);

			writer.Write((int)0x46534252); // Magic
			writer.Write((int)0x3); // Version

			//            Header ver  files      hashes         strings
			int offset = 0x08 + 0x04 * 0x07 * 2 + 0x08 * 0x14 + (int)strings.Length;
			writer.Write(offset); offset += (int)Data.Length;
			writer.Write(offset); offset += (int)Chart.Length;
			writer.Write(offset); offset += (int)Audio.Length;
			writer.Write(offset); offset += (int)Milo.Length;
			writer.Write(offset); offset += (int)AlbumArt.Length;
			writer.Write(offset); offset += (int)Weights.Length;
			writer.Write(offset); offset += (int)Metadata.Length;

			writer.Write((uint)Data.Length);
			writer.Write((uint)Chart.Length);
			writer.Write((uint)Audio.Length);
			writer.Write((uint)Milo.Length);
			writer.Write((uint)AlbumArt.Length);
			writer.Write((uint)Weights.Length);
			writer.Write((uint)Metadata.Length);

			Sha1Hash(Data, writer);
			Sha1Hash(Chart, writer);
			Sha1Hash(Audio, writer);
			Sha1Hash(Milo, writer);
			Sha1Hash(AlbumArt, writer);
			Sha1Hash(Weights, writer);
			Sha1Hash(Metadata, writer);

			writer.Pad(Util.SHA1HashSize); // Zero'd hash for the hash

			strings.Position = 0;
			Util.StreamCopy(mem, strings);

			mem.Position = 0;
			byte[] hash = Util.SHA1Hash(mem);
			mem.Position = 0xCC; // Header hash position
			writer.Write(hash);
			mem.Position = 0;
			Util.StreamCopy(stream, mem);

			Util.StreamCopy(stream, Data);
			Util.StreamCopy(stream, Chart);
			Util.StreamCopy(stream, Audio);
			Util.StreamCopy(stream, Milo);
			Util.StreamCopy(stream, AlbumArt);
			Util.StreamCopy(stream, Weights);
			Util.StreamCopy(stream, Metadata);
		}

		private void Sha1Hash(Stream data, EndianReader writer)
		{
			data.Position = 0;
			writer.Write(Util.SHA1Hash(data));
			data.Position = 0; // Prepare it for the StreamCopy
		}
	}
}