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
		public static bool ImportExpertPlus = false;

		public static readonly PakFormat PakFormat = new PakFormat(null, null, null, PakFormatType.Wii);

		public static PlatformGH5WiiDisc Instance;
		public static void Initialise()
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

		public override string Name { get { return "Guitar Hero 4 / 5 / 6 Wii Disc"; } }

		public override bool AddSong(PlatformData data, SongData song, ProgressIndicator progress)
		{
			FormatData formatdata = new TemporaryFormatData(song, data);

			NeversoftMetadata.SaveSongItem(formatdata);

			DirectoryNode dir = data.Session["rootdir"] as DirectoryNode;

			FileNode chartpak = dir.Navigate("songs/" + song.ID + ".pak.ngc", false, true) as FileNode;

			if (chartpak == null)
				return false;

			if (data.Game == Game.GuitarHero5 || data.Game == Game.BandHero)
				ChartFormatGH5.Instance.Create(formatdata, new Stream[] { chartpak.Data }, ImportExpertPlus);
			else
				ChartFormatGH4.Instance.Create(formatdata, new Stream[] { chartpak.Data }, ImportExpertPlus);
			
			FileNode audiofile = dir.Navigate("music/" + song.ID + ".bik", false, true) as FileNode;
			FileNode previewfile = dir.Navigate("music/" + song.ID + "_preview.bik", false, true) as FileNode;

			if (audiofile != null)
				AudioFormatBink.Instance.Create(formatdata, audiofile.Data, previewfile == null ? null : previewfile.Data, null);

			data.AddSong(formatdata);

			chartpak.Data.Close();

			return true;
		}

		public override PlatformData Create(string path, Game game, ProgressIndicator progress)
		{
			PlatformData data = new PlatformData(this, game);
			
			DirectoryNode dir = data.GetDirectoryStructure(path);

			data.Game = Platform.DetermineGame(data);

			try {
				FileNode qbpak = dir.Navigate("pak/qb.pak.ngc") as FileNode;
				if (qbpak == null)
					throw new FormatException("Couldn't find qb.pak on Guitar Hero Wii disc.");

				FileNode qspak = dir.Navigate("pak/qs.pak.ngc") as FileNode;
				if (qspak == null)
					throw new FormatException("Couldn't find qs.pak on Guitar Hero Wii disc.");

				Pak qs = new Pak(new EndianReader(qspak.Data, Endianness.BigEndian));

				StringList strings = new StringList();
				foreach (Pak.Node node in qs.Nodes)
					strings.ParseFromStream(node.Data);

				Pak qb = new Pak(new EndianReader(qbpak.Data, Endianness.BigEndian));
				FileNode songlistfile = qb.FindFile(@"scripts\guitar\songlist.qb.ngc");
				if (songlistfile == null)
					songlistfile = qb.FindFile(@"scripts\guitar\songlist.qb");

				if (songlistfile == null)
					throw new FormatException("Couldn't find the songlist on the Guitar Hero Wii disc pak.");
				QbFile songlist = new QbFile(songlistfile.Data, PakFormat);

				data.Session["rootdir"] = dir;

				List<QbKey> listkeys = new List<QbKey>();
				foreach (uint songlistkey in NeversoftMetadata.SonglistKeys) {
					QbKey key = QbKey.Create(songlistkey);
					QbItemStruct list = songlist.FindItem(key, true) as QbItemStruct;
					if (list != null && list.Items.Count > 0)
						listkeys.Add(key);
				}

				progress.NewTask(listkeys.Count);
				List<string> songsadded = new List<string>();
				foreach (QbKey songlistkey in listkeys) {
					QbItemStruct list = songlist.FindItem(songlistkey, true) as QbItemStruct;

					progress.NewTask(list.Items.Count);

					foreach (QbItemArray item in list.Items.OfType<QbItemArray>()) {
						item.Items[0].ItemQbKey = item.ItemQbKey;
						SongData song = NeversoftMetadata.GetSongData(data, item.Items[0] as QbItemStruct, strings);

						progress.Progress();

						if (songsadded.Contains(song.ID))
							continue;

						try {
							if (AddSong(data, song, progress))
								songsadded.Add(song.ID);
						} catch (Exception exception) {
							Exceptions.Warning(exception, "Unable to properly parse " + song.Name);
						}
					}

					progress.EndTask();
					progress.Progress();
				}
				progress.EndTask();

				qbpak.Data.Close();
				qspak.Data.Close();
			} catch (Exception exception) {
				Exceptions.Error(exception, "An error occurred while parsing the Guitar Hero Wii disc.");
			}

			return data;
		}
	}
}
