using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using ConsoleHaxx.Common;
using ConsoleHaxx.Neversoft;
using Nanook.QueenBee.Parser;
using System.IO;

namespace ConsoleHaxx.RawkSD
{
	public class PlatformGH5WiiDisc : Engine
	{
		public static readonly PakFormat PakFormat = new PakFormat(null, null, null, PakFormatType.Wii);

		public static readonly PlatformGH5WiiDisc Instance;
		static PlatformGH5WiiDisc()
		{
			Instance = new PlatformGH5WiiDisc();
		}

		public override int ID { get { throw new NotImplementedException(); } }

		public override string Name { get { return "Guitar Hero 5 / Band Hero Wii Disc"; } }

		public override bool IsValid(string path)
		{
			throw new NotImplementedException();
		}

		public override bool AddSong(PlatformData data, SongData song)
		{
			FormatData formatdata = new MemoryFormatData(song);

			DirectoryNode dir = data.Session["rootdir"] as DirectoryNode;

			FileNode chartpak = dir.Navigate("songs/" + song.ID + ".pak.ngc", false, true) as FileNode;

			if (chartpak == null)
				return false;

			if (data.Game.ID == Games.GuitarHero5 || data.Game.ID == Games.BandHero)
				ChartFormatGH5.Instance.Create(formatdata, chartpak.Data, PakFormat.PakFormatType);
			else
				ChartFormatGH4.Instance.Create(formatdata, chartpak.Data, PakFormat.PakFormatType);
			
			FileNode audiofile = dir.Navigate("music/" + song.ID + ".bik", false, true) as FileNode;
			FileNode previewfile = dir.Navigate("music/" + song.ID + "_preview.bik", false, true) as FileNode;

			if (audiofile != null)
				AudioFormatBink.Instance.Create(formatdata, audiofile.Data, previewfile == null ? null : previewfile.Data);

			data.AddSong(formatdata);

			return true;
		}

		public override PlatformData Create(string path, Game game)
		{
			PlatformData data = new PlatformData(this, game);
			
			DirectoryNode dir = data.GetWiiDirectoryStructure(path);

			data.Game = Platform.DetermineGame(data);

			FileNode qbpak = dir.Navigate("pak/qb.pak.ngc") as FileNode;
			if (qbpak == null)
				throw new FormatException();

			FileNode qspak = dir.Navigate("pak/qs.pak.ngc") as FileNode;
			if (qspak == null)
				throw new FormatException();

			Pak qs = new Pak(new EndianReader(qspak.Data, Endianness.BigEndian));

			StringList strings = new StringList();
			foreach (Pak.Node node in qs.Nodes)
				strings.ParseFromStream(node.Data);

			Pak qb = new Pak(new EndianReader(qbpak.Data, Endianness.BigEndian));
			FileNode songlistfile = qb.Root.Find("songlist.qb.ngc", SearchOption.AllDirectories) as FileNode;
			QbFile songlist = new QbFile(songlistfile.Data, PakFormat);

			data.Session["rootdir"] = dir;

			List<string> songsadded = new List<string>();
			foreach (uint songlistkey in MetadataFormatNeversoft.SonglistKeys) {
				QbItemBase list = songlist.FindItem(QbKey.Create(songlistkey), true);
				if (list == null)
					continue;

				foreach (QbItemArray item in (list as QbItemStruct).Items.OfType<QbItemArray>()) {
					item.Items[0].ItemQbKey = item.ItemQbKey;
					SongData song = MetadataFormatNeversoft.GetSongData(item.Items[0] as QbItemStruct, strings);

					if (songsadded.Contains(song.ID))
						continue;

					if (AddSong(data, song))
						songsadded.Add(song.ID);
				}
			}

			return data;
		}
	}
}
