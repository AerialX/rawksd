using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using ConsoleHaxx.Wii;
using ConsoleHaxx.Common;
using ConsoleHaxx.Harmonix;
using ConsoleHaxx.Neversoft;
using Nanook.QueenBee.Parser;

namespace ConsoleHaxx.RawkSD
{
	public class PlatformGH3WiiDisc : Engine
	{
		public static readonly PakFormat PakFormat = new PakFormat(null, null, null, PakFormatType.Wii);

		public static readonly PlatformGH3WiiDisc Instance;
		static PlatformGH3WiiDisc()
		{
			Instance = new PlatformGH3WiiDisc();
		}

		public override int ID { get { throw new NotImplementedException(); } }

		public override string Name { get { return "Guitar Hero 3 Disc"; } }

		public override bool IsValid(string path)
		{
			if (File.Exists(path)) {
				Stream stream = new FileStream(path, FileMode.Open, FileAccess.Read);
				bool ret = IsValid(stream);
				stream.Close();
				return ret;
			}

			if (Directory.Exists(path)) {
				throw new NotImplementedException();
			}

			return false;
		}

		public bool IsValid(Stream iso)
		{
			try {
				Disc disc = new Disc(iso);
				U8 u8 = disc.Partitions.Find(p => p.Type == PartitionType.Data).Root;
				FileNode hdr = u8.Root.Find("main_wii.hdr", SearchOption.AllDirectories) as FileNode;
				// TODO: main.hdr
				if (hdr != null)
					return true;
			} catch { }

			return false;
		}

		public override bool AddSong(PlatformData data, SongData song)
		{
			FormatData formatdata = new MemoryFormatData(song);

			DirectoryNode dir = data.Session["rootdir"] as DirectoryNode;
			Pak qb = data.Session["rootqb"] as Pak;

			FileNode chartpak = dir.Navigate("songs/" + song.ID + ".pak.ngc", false, true) as FileNode;

			if (chartpak == null)
				return false;

			chartpak.Data.Position = 0;
			Pak chartqb = new Pak(new EndianReader(chartpak.Data, Endianness.BigEndian));
			FileNode sectionfile = chartqb.Root.Find(song.ID + ".mid_text.qb.ngc", SearchOption.AllDirectories, true) as FileNode;
			if (sectionfile == null) // GHA stores it elsewhere
				sectionfile = qb.Root.Find(song.ID + ".mid_text.qb.ngc", SearchOption.AllDirectories, true) as FileNode;
			if (sectionfile == null) // Last resort, check for it raw on the disc partition
				sectionfile = dir.Find(song.ID + ".mid_text.qb.ngc", SearchOption.AllDirectories, true) as FileNode;

			ChartFormatGH3.Instance.Create(formatdata, chartpak.Data, sectionfile.Data, PakFormat.PakFormatType);

			FileNode datfile = dir.Navigate("music/" + song.ID + ".dat.ngc", false, true) as FileNode;
			FileNode wadfile = dir.Navigate("music/" + song.ID + ".wad.ngc", false, true) as FileNode;

			if (datfile != null && wadfile != null)
				AudioFormatGH3WiiFSB.Instance.Create(formatdata, datfile.Data, wadfile.Data);

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

			Pak qb = new Pak(new EndianReader(qbpak.Data, Endianness.BigEndian));
			FileNode songlistfile = qb.Root.Find("songlist.qb.ngc", SearchOption.AllDirectories) as FileNode;
			QbFile songlist = new QbFile(songlistfile.Data, PakFormat);
			QbItemBase list = songlist.FindItem(QbKey.Create(MetadataFormatNeversoft.SonglistKeys[0]), true);

			data.Session["rootdir"] = dir;
			data.Session["rootqb"] = qb;

			foreach (QbItemStruct item in (list as QbItemStruct).Items) {
				// TODO: Coop
				
				SongData song = MetadataFormatNeversoft.GetSongData(item);

				AddSong(data, song);
			}

			return data;
		}
	}
}
