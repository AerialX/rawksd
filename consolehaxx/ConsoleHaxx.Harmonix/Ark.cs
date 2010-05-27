using System;
using System.Collections.Generic;
using System.IO;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.Harmonix
{
	public class Ark
	{
		protected EndianReader HdrStream;
		protected Stream[] ArkStreams;

		public DirectoryNode Root;

		public uint Version;

		public const uint ArkMagic = 0x41524B00; // "ARK\0"
		public const uint ArkMagicLE = 0x004B5241;

		public Ark()
		{
			Root = new DirectoryNode();
		}

		public Ark(EndianReader hdrstream, params Stream[] arkstreams) : this()
		{
			HdrStream = hdrstream;
			ArkStreams = arkstreams;

			HdrStream.Position = 0;
			Version = HdrStream.ReadUInt32();
			uint numArks;
			uint numArks2;
			long[] arkSizes;
			int stringSize;
			Stream stringTable;
			int numOffsets;
			int[] offsetTable;
			int numEntries;

			if (Version == ArkMagic || Version == ArkMagicLE) { // FreQuency
				Version = HdrStream.ReadUInt32(); // == 2
				int fileoffset = HdrStream.ReadInt32();
				numEntries = HdrStream.ReadInt32();
				int diroffset = HdrStream.ReadInt32();
				numOffsets = HdrStream.ReadInt32();
				int stringoffset = HdrStream.ReadInt32();
				int numStrings = HdrStream.ReadInt32();
				HdrStream.ReadInt32();
				int sectorSize = HdrStream.ReadInt32();

				HdrStream.Position = diroffset;
				offsetTable = new int[numOffsets];
				for (int i = 0; i < numOffsets; i++) {
					HdrStream.ReadInt32(); // Hash + 0x0000
					offsetTable[i] = HdrStream.ReadInt32();
				}

				HdrStream.Position = stringoffset;
				stringTable = new MemoryStream();
				EndianReader writer = new EndianReader(stringTable, Endianness.BigEndian);
				for (int i = 0; i < numStrings; i++) {
					writer.Write(Util.ReadCString(HdrStream));
					writer.Write((byte)0);
				}

				HdrStream.Position = fileoffset;
				for (int i = 0; i < numEntries; i++) {
					HdrStream.ReadInt16(); // Hash
					HdrStream.ReadInt16(); // Flags
					int nameoffset = HdrStream.ReadInt32(); // Filename
					int directory = HdrStream.ReadInt16();
					int sectoroffset = HdrStream.ReadInt16();
					int sector = HdrStream.ReadInt32();
					uint size = HdrStream.ReadUInt32();
					HdrStream.ReadInt32(); // Extracted length

					stringTable.Position = offsetTable[directory] - stringoffset;
					string pathname = Util.ReadCString(stringTable);
					stringTable.Position = nameoffset - stringoffset;
					string filename = Util.ReadCString(stringTable);

					DirectoryNode dir = Root.Navigate(pathname, true) as DirectoryNode;
					dir.Children.Add(new FileNode(filename, dir, size, new Substream(HdrStream, sector * sectorSize + sectoroffset, (long)size)));
				}
			} else {
				if (Version > 5) { // Invalid version number, encrypted
					Stream stream = new CryptedDtbStream(HdrStream);
					HdrStream = new EndianReader(stream, HdrStream.Endian);
					HdrStream.Position = 0;
					Version = HdrStream.ReadUInt32();
				}

				if (Version <= 2) {
					numArks = 1;
					numArks2 = 1;
					arkSizes = new long[1] { HdrStream.Base.Length };
					ArkStreams = new Stream[1] { HdrStream.Base };

					numEntries = HdrStream.ReadInt32();
					HdrStream.Position += numEntries * (4 * 4 + 4);
				} else if (Version <= 5) {
					numArks = HdrStream.ReadUInt32();
					numArks2 = HdrStream.ReadUInt32();

					arkSizes = new long[numArks];

					for (int i = 0; i < numArks; i++) {
						if (Version == 4)
							arkSizes[i] = HdrStream.ReadInt64();
						else
							arkSizes[i] = (long)HdrStream.ReadUInt32();
					}

					if (Version == 5) { // ark file string list
						numArks = HdrStream.ReadUInt32();
						for (int i = 0; i < numArks; i++) {
							int strsize = HdrStream.ReadInt32();
							HdrStream.PadRead(strsize); // I'm not using them >.>
						}
					}
				} else
					throw new FormatException();

				stringSize = HdrStream.ReadInt32();
				stringTable = new MemoryStream(HdrStream.ReadBytes(stringSize), false);
				numOffsets = HdrStream.ReadInt32();
				offsetTable = new int[numOffsets];
				for (int i = 0; i < numOffsets; i++)
					offsetTable[i] = HdrStream.ReadInt32();

				if (Version <= 2)
					HdrStream.Position = 0x04;

				numEntries = HdrStream.ReadInt32();

				for (int i = 0; i < numEntries; i++) {
					long offset;
					if (Version <= 3)
						offset = (long)HdrStream.ReadUInt32();
					else
						offset = HdrStream.ReadInt64();
					uint filenameStringIndex = HdrStream.ReadUInt32();
					uint dirnameStringIndex = HdrStream.ReadUInt32();
					ulong size = 0;

					if (Version <= 2) {
						size = HdrStream.ReadUInt32();
						HdrStream.ReadInt32(); // unknown, flags?
					} else
						size = HdrStream.ReadUInt64();

					string pathname;
					if (dirnameStringIndex == 0xFFFFFFFF)
						pathname = string.Empty;
					else {
						stringTable.Position = offsetTable[dirnameStringIndex];
						pathname = Util.ReadCString(stringTable);
					}

					stringTable.Position = offsetTable[filenameStringIndex];
					string filename = Util.ReadCString(stringTable);

					if (pathname.StartsWith("./"))
						pathname = pathname.Substring(2);
					if (pathname == ".")
						pathname = string.Empty;
					
					DirectoryNode dir = Root.Navigate(pathname, true) as DirectoryNode;
					int ark;
					for (ark = 0; offset >= arkSizes[ark]; offset -= arkSizes[ark++])
						;
					dir.Children.Add(new FileNode(filename, dir, size, new Substream(ArkStreams[ark], offset, (long)size)));
				}
			}
		}
	}
}
