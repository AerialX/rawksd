using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

namespace ConsoleHaxx.Common
{
	public class Iso9660
	{
		public const int SectorSize = 0x800;
		public const int MaxPathLen = 0x80;

		private const int PathTableEntrySize = 1 + 1 + 4 + 2;

		private const int OffsetExtended = 1;
		private const int OffsetSector = 6;
		private const int OffsetSize = 14;
		private const int OffsetFlags = 25;
		private const int OffsetNameLen = 32;
		private const int OffsetName = 33;
		private const int FlagDir = 2;

		public DirectoryNode Root;

		public Iso9660(Stream stream)
		{
			VolumeDescriptor volume = VolumeDescriptor.Create(stream, 2);
			bool unicode = false;
			if (volume == null)
				volume = VolumeDescriptor.Create(stream, 1);
			else
				unicode = true;

			if (volume == null)
				throw new FormatException();

			MemoryStream rootmem = new MemoryStream(volume.root);
			EndianReader rootreader = new EndianReader(rootmem, Endianness.BigEndian);
			//List<PathTableEntry> entries = new List<PathTableEntry>();
			PathTableEntry root = new PathTableEntry();
			rootreader.Position = OffsetExtended;
			root.ExtendedSectors = rootreader.ReadByte();
			rootreader.Position = OffsetSector;
			root.Sector = rootreader.ReadUInt32();
			root.Name = "";

			/*
			entries.Add(root);

			uint pathtable = volume.path_table_be;
			uint pathtablelen = volume.path_table_len_be;
			SeekSector(stream, pathtable);
			stream.Position += PathTableEntrySize + 2;
			
			for (ushort i = 1; i < 0xFFFF && stream.Position < (pathtable * SectorSize + pathtablelen); i++) {
				PathTableEntry entry = PathTableEntry.Create(stream, unicode);
				PathTableEntry parent = entries[entry.Parent - 1];
				parent.Children.Add(entry);
				
				entries.Add(entry);
			}
			*/
			Root = Stat(new EndianReader(stream, Endianness.BigEndian), root, unicode);
		}

		private DirectoryNode Stat(EndianReader reader, PathTableEntry entry, bool unicode, DirectoryNode parent = null)
		{
			SeekSector(reader, entry.Sector);

			long baseposition = (long)entry.Sector * SectorSize;

			DirectoryEntry dir = DirectoryEntry.Create(reader, unicode);
			DirectoryNode root = new DirectoryNode(entry.Name);
			if (parent != null)
				parent.AddChild(root);
			
			while (reader.Position < baseposition + dir.Size) {
				DirectoryEntry file = DirectoryEntry.Create(reader, unicode);
				if (file == null)
					continue;
				PathTableEntry childentry = new PathTableEntry();
				childentry.Sector = file.Sector;
				childentry.Name = file.Name;
				if ((file.Flags & FlagDir) == FlagDir) {
					long position = reader.Position;
					Stat(reader, childentry, unicode, root);
					reader.Position = position;
				} else
					root.AddChild(new FileNode(file.Name, file.Size, new Substream(reader, file.Sector * SectorSize, file.Size)));
			}

			return root;
		}

		private class DirectoryEntry
		{
			public byte ExtendedSectors;
			public uint Sector;
			public uint Size;
			public byte Flags;
			public string Name;

			public static DirectoryEntry Create(EndianReader reader, bool unicode)
			{
				DirectoryEntry dir = new DirectoryEntry();

				byte entrysize = reader.ReadByte();
				if (entrysize == 0) { // Will only happen if the entry we're reading won't fit in the current sector
					reader.Position = Util.RoundUp(reader.Position, SectorSize);
					entrysize = reader.ReadByte();
				}

				byte[] data = reader.ReadBytes(entrysize - 1);
				EndianReader entryreader = new EndianReader(new MemoryStream(data), Endianness.BigEndian);
				entryreader.Position = OffsetExtended - 1;
				dir.ExtendedSectors = entryreader.ReadByte();
				entryreader.Position = OffsetSector - 1;
				dir.Sector = entryreader.ReadUInt32();
				entryreader.Position = OffsetSize - 1;
				dir.Size = entryreader.ReadUInt32();
				entryreader.Position = OffsetFlags - 1;
				dir.Flags = entryreader.ReadByte();
				entryreader.Position = OffsetNameLen - 1;
				byte namelen = entryreader.ReadByte();
				dir.Name = (unicode ? Encoding.Unicode : Util.Encoding).GetString(entryreader.ReadBytes(namelen)).Trim('\0');

				if (dir.Name.Contains(';'))
					dir.Name = dir.Name.Split(';')[0];

				if (dir.Name.Length > 0 && dir.Name[0] == '\x0001')
					return null;

				return dir;
			}
		}

		public static void SeekSector(Stream stream, long sector)
		{
			stream.Position = sector * SectorSize;
		}

		public class PathTableEntry
		{
			public PathTableEntry()
			{
				Children = new List<PathTableEntry>();
			}

			public List<PathTableEntry> Children;
			public byte ExtendedSectors;
			public uint Sector;
			public ushort Parent;
			public string Name;

			public static PathTableEntry Create(Stream stream, bool unicode)
			{
				PathTableEntry entry = new PathTableEntry();
				EndianReader reader = new EndianReader(stream, Endianness.BigEndian);
				byte namelength = reader.ReadByte();
				entry.ExtendedSectors = reader.ReadByte();
				entry.Sector = reader.ReadUInt32();
				entry.Parent = reader.ReadUInt16();
				entry.Name = (unicode ? Encoding.Unicode : Util.Encoding).GetString(reader.ReadBytes(namelength)).Trim('\0');
				if (namelength % 2 != 0)
					reader.ReadByte();
				return entry;
			}
		}

		public class VolumeDescriptor
		{
			public ulong id;
			public byte[] system_id;
			public byte[] volume_id;
			public ulong zero;
			public uint total_sector_le, total_sect_be;
			public byte[] zero2;
			public uint volume_set_size, volume_seq_nr;
			public ushort sector_size_le, sector_size_be;
			public uint path_table_len_le, path_table_len_be;
			public uint path_table_le, path_table_2nd_le;
			public uint path_table_be, path_table_2nd_be;
			public byte[] root;
			public byte[] volume_set_id, publisher_id, data_preparer_id, application_id;
			public byte[] copyright_file_id, abstract_file_id, bibliographical_file_id;

			public VolumeDescriptor()
			{
				system_id = new byte[32];
				volume_id = new byte[32];
				zero2 = new byte[32];
				root = new byte[34];
				volume_set_id = new byte[128];
				publisher_id = new byte[128];
				data_preparer_id = new byte[128];
				application_id = new byte[128];
				copyright_file_id = new byte[37];
				abstract_file_id = new byte[37];
				bibliographical_file_id = new byte[37];
			}

			public static VolumeDescriptor Create(Stream stream)
			{
				VolumeDescriptor vd = new VolumeDescriptor();

				EndianReader reader = new EndianReader(stream, Endianness.BigEndian);

				vd.id = reader.ReadUInt64();
				reader.ReadBytes(vd.system_id);
				reader.ReadBytes(vd.volume_id);
				vd.zero = reader.ReadUInt64();
				vd.total_sector_le = reader.ReadUInt32(Endianness.LittleEndian);
				vd.total_sect_be = reader.ReadUInt32();
				reader.ReadBytes(vd.zero2);
				
				vd.volume_set_size = reader.ReadUInt32();
				vd.volume_seq_nr = reader.ReadUInt32();
				vd.sector_size_le = reader.ReadUInt16(Endianness.LittleEndian);
				vd.sector_size_be = reader.ReadUInt16();
				vd.path_table_len_le = reader.ReadUInt32(Endianness.LittleEndian);
				vd.path_table_len_be = reader.ReadUInt32();
				vd.path_table_le = reader.ReadUInt32(Endianness.LittleEndian);
				vd.path_table_2nd_le = reader.ReadUInt32(Endianness.LittleEndian);
				vd.path_table_be = reader.ReadUInt32();
				vd.path_table_2nd_be = reader.ReadUInt32();

				reader.ReadBytes(vd.root);
				reader.ReadBytes(vd.volume_set_id);
				reader.ReadBytes(vd.publisher_id);
				reader.ReadBytes(vd.data_preparer_id);
				reader.ReadBytes(vd.application_id);

				reader.ReadBytes(vd.copyright_file_id);
				reader.ReadBytes(vd.abstract_file_id);
				reader.ReadBytes(vd.bibliographical_file_id);

				return vd;
			}

			public static VolumeDescriptor Create(Stream stream, byte index)
			{
				EndianReader reader = new EndianReader(stream, Endianness.BigEndian);

				for (int sector = 16; sector < 32; sector++) {
					SeekSector(stream, sector);
					byte id = reader.ReadByte();
					if (Util.Encoding.GetString(reader.ReadBytes(6)) == "CD001\x01" && id == index) {
						SeekSector(stream, sector);
						return Create(stream);
					}
				}

				return null;
			}
		}
	}
}
