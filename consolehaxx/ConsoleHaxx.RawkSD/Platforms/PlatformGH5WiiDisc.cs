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

			PlatformDetection.AddSelection(Instance);
			PlatformDetection.DetectDirectoryNode += new Action<string, DirectoryNode, List<Pair<Engine, Game>>>(PlatformDetection_DetectDirectoryNode);
		}

		static void PlatformDetection_DetectDirectoryNode(string path, DirectoryNode root, List<Pair<Engine, Game>> platforms)
		{
			if (root.Navigate("pak/qb.pak.ngc") != null && root.Navigate("pak/qs.pak.ngc") != null)
				platforms.Add(new Pair<Engine, Game>(Instance, Game.Unknown));
		}

		public override int ID { get { throw new NotImplementedException(); } }

		public override string Name { get { return "Guitar Hero World Tour / Metallica / Smash Hits / Van Halen / 5 / Band Hero Wii Disc"; } }

		public override bool AddSong(PlatformData data, SongData song, ProgressIndicator progress)
		{
			FormatData formatdata = new TemporaryFormatData(song, data);

			DirectoryNode dir = data.Session["rootdir"] as DirectoryNode;

			FileNode chartpak = dir.Navigate("songs/" + song.ID + ".pak.ngc", false, true) as FileNode;

			if (chartpak == null)
				return false;

			if (data.Game == Game.GuitarHero5 || data.Game == Game.BandHero)
				ChartFormatGH5.Instance.Create(formatdata, chartpak.Data, PakFormat.PakFormatType);
			else
				ChartFormatGH4.Instance.Create(formatdata, chartpak.Data, PakFormat.PakFormatType);
			
			FileNode audiofile = dir.Navigate("music/" + song.ID + ".bik", false, true) as FileNode;
			FileNode previewfile = dir.Navigate("music/" + song.ID + "_preview.bik", false, true) as FileNode;

			AudioFormat audioformat = new AudioFormat();
			// GH4 engine games that came out after GHWT use a different drum audio scheme; the GH5 engine uses the same as GHWT
			bool gh4v2 = NeversoftMetadata.IsGuitarHero4(data.Game) && data.Game != Game.GuitarHeroWorldTour;
			if (gh4v2) {
				// Kick
				audioformat.Mappings.Add(new AudioFormat.Mapping(0, -1, Instrument.Drums));
				audioformat.Mappings.Add(new AudioFormat.Mapping(0, 1, Instrument.Drums));
				// Snare
				audioformat.Mappings.Add(new AudioFormat.Mapping(0, -1, Instrument.Drums));
				audioformat.Mappings.Add(new AudioFormat.Mapping(0, 1, Instrument.Drums));
			} else {
				// Kick
				audioformat.Mappings.Add(new AudioFormat.Mapping(0, 0, Instrument.Drums));
				// Snare
				audioformat.Mappings.Add(new AudioFormat.Mapping(0, 0, Instrument.Drums));
			}
			// Overhead
			audioformat.Mappings.Add(new AudioFormat.Mapping(0, -1, Instrument.Drums));
			audioformat.Mappings.Add(new AudioFormat.Mapping(0, 1, Instrument.Drums));
			// Guitar
			audioformat.Mappings.Add(new AudioFormat.Mapping(0, -1, Instrument.Guitar));
			audioformat.Mappings.Add(new AudioFormat.Mapping(0, 1, Instrument.Guitar));
			// Bass
			audioformat.Mappings.Add(new AudioFormat.Mapping(0, 0, Instrument.Bass));
			// Else / Vocals
			audioformat.Mappings.Add(new AudioFormat.Mapping(0, -1, Instrument.Ambient));
			audioformat.Mappings.Add(new AudioFormat.Mapping(0, 1, Instrument.Ambient));
			// Preview
			audioformat.Mappings.Add(new AudioFormat.Mapping(0, -1, Instrument.Preview));
			audioformat.Mappings.Add(new AudioFormat.Mapping(0, 1, Instrument.Preview));

			if (audiofile != null)
				AudioFormatBink.Instance.Create(formatdata, audiofile.Data, previewfile == null ? null : previewfile.Data, audioformat);

			data.AddSong(formatdata);

			chartpak.Data.Close();

			return true;
		}

		public override PlatformData Create(string path, Game game, ProgressIndicator progress)
		{
			PlatformData data = new PlatformData(this, game);
			
			DirectoryNode dir = data.GetDirectoryStructure(path);

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
			foreach (uint songlistkey in NeversoftMetadata.SonglistKeys) {
				QbItemBase list = songlist.FindItem(QbKey.Create(songlistkey), true);
				if (list == null || list.Items.Count == 0)
					continue;

				progress.NewTask(list.Items.Count);

				foreach (QbItemArray item in (list as QbItemStruct).Items.OfType<QbItemArray>()) {
					item.Items[0].ItemQbKey = item.ItemQbKey;
					SongData song = NeversoftMetadata.GetSongData(data, item.Items[0] as QbItemStruct, strings);

					progress.Progress();

					if (songsadded.Contains(song.ID))
						continue;

					if (AddSong(data, song, progress))
						songsadded.Add(song.ID);
				}

				progress.EndTask();
			}

			return data;
		}
	}
}
