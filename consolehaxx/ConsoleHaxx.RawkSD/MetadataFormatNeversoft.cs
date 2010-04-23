using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Nanook.QueenBee.Parser;
using ConsoleHaxx.Harmonix;
using ConsoleHaxx.Common;
using System.IO;
using ConsoleHaxx.Neversoft;

namespace ConsoleHaxx.RawkSD
{
	public static class MetadataFormatNeversoft
	{
		public static readonly uint[] QbKeysID = new uint[] { 0xA1DC81F9 };
		public static readonly uint[] QbKeysName = new uint[] { 0xD4C98794 };
		public static readonly uint[] QbKeysArtist = new uint[] { 0xFEA66978 };
		public static readonly uint[] QbKeysAlbum = new uint[] { 0xA271CDB2 };
		public static readonly uint[] QbKeysYear = new uint[] { 0x447D8CC8 };
		public static readonly uint[] QbKeysMaster = new uint[] { 0xADDA4AA8 };
		public static readonly uint[] QbKeysGenre = new uint[] { 0x7CAFCC07 };
		public static readonly uint[] QbKeysVocalist = new uint[] { 0x6749E668 };
		public static readonly uint[] QbKeysHopo = new uint[] { 0xDEB26ABF };
		public static readonly uint[] QbKeysBandVolume = new uint[] { 0xD8F335CF, 0x46507438 };
		public static readonly uint[] QbKeysGuitarVolume = new uint[] { 0xA449CAD3 };
		public static readonly Dictionary<Instrument, uint[]> QbKeysDifficulty = new Dictionary<Instrument, uint[]>() {
			{ Instrument.Ambient, new uint[] { 0xD163AD32 } },
			{ Instrument.Drums, new uint[] { 0x0AA62D30 } },
			{ Instrument.Bass, new uint[] { 0xDE88B0FB } },
			{ Instrument.Guitar, new uint[] { 0x1A166358 } },
			{ Instrument.Vocals, new uint[] { 0x3862937D } }
		};

		public static readonly Pair<uint[], string>[] QbKeysGenres = new Pair<uint[], string>[] {
			// GH5
			new Pair<uint[], string>(new uint[] { 0xC8B6445D }, "Rock"),
			new Pair<uint[], string>(new uint[] { 0x46FEED4A }, "Surf Rock"),
			new Pair<uint[], string>(new uint[] { 0x1474F917 }, "Blues Rock"),
			new Pair<uint[], string>(new uint[] { 0xF100A205 }, "Alternative"),
			new Pair<uint[], string>(new uint[] { 0x137EBAC2 }, "Classic Rock"),
			new Pair<uint[], string>(new uint[] { 0x979DCB71 }, "Pop Rock"),
			new Pair<uint[], string>(new uint[] { 0x605C9021 }, "Indie Rock"),
			new Pair<uint[], string>(new uint[] { 0x6B291F66 }, "Hip Hop"),
			new Pair<uint[], string>(new uint[] { 0x3CD1E8EC }, "Southern Rock"),
			new Pair<uint[], string>(new uint[] { 0xD1120C22 }, "Grunge"),
			new Pair<uint[], string>(new uint[] { 0xAA9491B5 }, "New Wave"),
			new Pair<uint[], string>(new uint[] { 0x990C6A70 }, "Death Metal"),
			new Pair<uint[], string>(new uint[] { 0x998B5B11 }, "Pop Punk"),
			new Pair<uint[], string>(new uint[] { 0x70E16585 }, "Industrial"),
			new Pair<uint[], string>(new uint[] { 0x3F9D0B36 }, "Metal"),
			new Pair<uint[], string>(new uint[] { 0xC6A0D43D }, "Punk"),
			new Pair<uint[], string>(new uint[] { 0xB3D2DC7E }, "Funk"),
			new Pair<uint[], string>(new uint[] { 0xAC8C3699 }, "Country"),
			new Pair<uint[], string>(new uint[] { 0xB8E3F63D }, "Progressive"),
			new Pair<uint[], string>(new uint[] { 0xE8D7A2A7 }, "Glam"),

			// GHWT
			new Pair<uint[], string>(new uint[] { 0xE62F48E9 }, "Pop"),
			new Pair<uint[], string>(new uint[] { 0x109A68B0 }, "Heavy Metal"),
			new Pair<uint[], string>(new uint[] { 0xCA3AA956 }, "Black Metal"),
			new Pair<uint[], string>(new uint[] { 0x33FB36DC }, "Goth"),
		};

		public static readonly Pair<uint[], string>[] QbKeysVocalists = new Pair<uint[], string>[] {
			new Pair<uint[], string>(new uint[] { 0xAA721F56 }, "male"),
		};

		public static readonly uint[] SonglistKeys = new uint[] {
			0x5A93AE17, // GH3 + GH4
			0x3CC2A6C9, // GH5
			0x1254A0AA, // Band Hero
			0x0AA26E1F, // GH5.0
			0x5C00078F, // GH5 DLC
			0x92AA3758, // GH4
			0x39673CC9, // GH4 DLC
			0x4B98496F, // GH4.1
			0x6250FD9D, // GH4.2
			0xCC386C0C, // GH4.3
			0x8D024B7C, // GH5.2
			0x236ADAED, // GH5.3
			0xDE932298, // GH5.4
			0x5A93AE17 // GHVH
		};

		public static string GetSongDataString(QbItemStruct item, uint[] keys, StringList strings)
		{
			QbItemBase data = GetQbItem(item, keys);				

			if (data is QbItemString)
				return (data as QbItemString).Strings[0];

			if (data is QbItemQbKey)
				return strings.FindItem((data as QbItemQbKey).Values[0]) ?? string.Empty;

			return string.Empty;
		}

		public static SongData GetSongData(QbItemStruct item)
		{
			return GetSongData(item, new StringList());
		}

		public static SongData GetSongData(QbItemStruct item, StringList strings)
		{
			SongData data = new SongData();

			// TODO: QbKeysGenre

			// TOOD: QbKeysGenres

			// TOOD: QbKeysVocalists

			// TODO: QbKeysDifficulty

			data.ID = GetSongDataString(item, QbKeysID, strings);

			data.Name = GetSongDataString(item, QbKeysName, strings);

			data.Artist = GetSongDataString(item, QbKeysArtist, strings);

			data.Album = GetSongDataString(item, QbKeysAlbum, strings);

			QbItemInteger year = GetQbItem(item, QbKeysYear) as QbItemInteger;
			if (year != null)
				data.Year = (int)year.Values[0];
			else {
				int yearnum;
				if (int.TryParse(GetSongDataString(item, QbKeysYear, strings).TrimStart(',', ' '), out yearnum)) // ", 2009"
					data.Year = yearnum;
			}

			QbItemInteger master = GetQbItem(item, QbKeysMaster) as QbItemInteger;
			if (master != null)
				data.Master = master.Values[0] == 1;

			data.Vocalist = GetSongDataString(item, QbKeysVocalist, strings);

			QbItemFloat hopo = GetQbItem(item, QbKeysHopo) as QbItemFloat;
			if (hopo != null)
				data.HopoThreshold = (int)((float)NoteChart.DefaultTicksPerBeat / hopo.Values[0]);

			/* TODO: This
			QbItemFloat bandvolume = GetQbItem(item, QbKeysBandVolume) as QbItemFloat;
			data.BandVolume = bandvolume.Values[0];

			QbItemFloat guitarvolume = GetQbItem(item, QbKeysGuitarVolume) as QbItemFloat;
			data.GuitarVolume = guitarvolume.Values[0];
			*/

			/*
			MemoryStream templatefilestream = new MemoryStream(new byte[0x08]);
			EndianReader templatewriter = new EndianReader(templatefilestream, Endianness.BigEndian);
			templatewriter.Write((int)0x00000000); // Magic
			templatewriter.Write((int)0x00000008); // Filesize
			templatefilestream.Position = 0;
			
			QbFile file = new QbFile(templatefilestream, item.Root.PakFormat);

			file.AddItem(item);

			templatefilestream = new MemoryStream();
			templatefilestream.Position = 0;
			file.Write(templatefilestream);

			data.Data.SetValue("NeversoftSongItem", templatefilestream.GetBuffer());
			data.Data.SetValue("NeversoftSongType", (int)file.PakFormat.PakFormatType);
			*/

			MemoryStream itemstream = new MemoryStream();
			item.Root.Write(itemstream);

			data.Data.SetValue("NeversoftSongItem", itemstream.GetBuffer());
			data.Data.SetValue("NeversoftSongType", (int)item.Root.PakFormat.PakFormatType);
			data.Data.SetValue("NeversoftSongItemKey", item.ItemQbKey.Crc);

			return data;
		}

		public static PakFormat GetSongItemType(SongData song)
		{
			return new PakFormat("", "", "", (PakFormatType)song.Data.GetValue<int>("NeversoftSongType"));
		}

		public static QbItemStruct GetSongItem(SongData song)
		{
			QbItemBase item = new QbFile(new MemoryStream(song.Data.GetValue<byte[]>("NeversoftSongItem")), GetSongItemType(song)).FindItem(QbKey.Create(song.Data.GetValue<uint>("NeversoftSongItemKey")), true);
			if (item is QbItemArray) {
				item.Items[0].ItemQbKey = item.ItemQbKey;
				item = item.Items[0];
			}
			return item as QbItemStruct;
			//return new QbFile(new MemoryStream(song.Data.GetValue<byte[]>("NeversoftSongItem")), GetSongItemType(song)).Items[0] as QbItemStruct;
		}

		public static QbItemBase GetQbItem(QbItemStruct item, uint[] keys)
		{
			foreach (uint key in keys) {
				QbItemBase subitem = item.FindItem(QbKey.Create(key), false);
				if (subitem != null)
					return subitem;
			}

			return null;
		}
	}
}
