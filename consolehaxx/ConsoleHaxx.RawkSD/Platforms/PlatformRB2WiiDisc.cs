using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using ConsoleHaxx.Wii;
using ConsoleHaxx.Harmonix;
using ConsoleHaxx.Common;
using System.Text.RegularExpressions;

namespace ConsoleHaxx.RawkSD
{
	public class PlatformRB2WiiDisc : Engine
	{
		public static readonly PlatformRB2WiiDisc Instance;
		static PlatformRB2WiiDisc()
		{
			Instance = new PlatformRB2WiiDisc();
			
			PlatformDetection.AddSelection(Instance);
			PlatformDetection.DetectHarmonixArk += new Action<string, Ark, List<Pair<Engine, Game>>>(PlatformDetection_DetectHarmonixArk);
		}

		static void PlatformDetection_DetectHarmonixArk(string path, Ark ark, List<Pair<Engine, Game>> platforms)
		{
			if (ark.Version > 3)
				platforms.Add(new Pair<Engine, Game>(Instance, Game.Unknown));
		}

		public override int ID { get { throw new NotImplementedException(); } }

		public override string Name { get { return "Rock Band 1 / 2 Wii Disc"; } }

		public override bool AddSong(PlatformData data, SongData song, ProgressIndicator progress)
		{
			FormatData formatdata = new TemporaryFormatData(song, data);

			DirectoryNode songdir = data.Session["songdir"] as DirectoryNode;
			
			// SongsDTA dta = MetadataFormatHarmonix.GetSongsDTA(song);

			if (HarmonixMetadata.GetSongsDTA(song) == null) { // LRB's <addsong />
				SongsDTA dta = HarmonixMetadata.GetSongData(song);
				dta.Song.Cores = new List<int>() { -1, -1, -1, -1, -1, 1, 1, -1, -1 };
				dta.Song.Vols = new List<float>() { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
				dta.Song.Pans = new List<float>() { 0, 0, -1, 1, 0, -1, 1, 0, 0 };
				dta.Song.Tracks.Find(t => t.Name == "drum").Tracks.AddRange(new int[] { 0, 1, 2, 3 });
				dta.Song.Tracks.Find(t => t.Name == "bass").Tracks.AddRange(new int[] { 4 });
				dta.Song.Tracks.Find(t => t.Name == "guitar").Tracks.AddRange(new int[] { 5, 6 });
				dta.Song.Tracks.Find(t => t.Name == "vocals").Tracks.AddRange(new int[] { 7 });
				HarmonixMetadata.SetSongsDTA(song, dta.ToDTB());
			}

			DirectoryNode songnode = songdir.Find(song.ID) as DirectoryNode;

			FileNode songaudiofile = songnode.Navigate(song.ID + ".bik", false, true) as FileNode;
			AudioFormat audio = HarmonixMetadata.GetAudioFormat(song);
			if (songaudiofile == null) {
				songaudiofile = songnode.Navigate(song.ID + ".mogg", false, true) as FileNode;
				if (songaudiofile == null)
					return false;
				if (HarmonixMetadata.IsRockBand1(data.Game))
					audio.InitialOffset = 3000;
				AudioFormatMogg.Instance.Create(formatdata, songaudiofile.Data, audio);
			} else {
				AudioFormatRB2Bink.Instance.Create(formatdata, songaudiofile.Data, audio);
			}

			// TODO:	SongInfo
			// TODO:	Preview
			
			FileNode chartfile = songnode.Find(song.ID + ".mid") as FileNode;
			FileNode panfile = songnode.Find(song.ID + ".pan") as FileNode;
			FileNode weightsfile = songnode.Navigate("gen/" + song.ID + "_weights.bin") as FileNode;
			FileNode milofile = songnode.Navigate("gen/" + song.ID + ".milo_wii") as FileNode;
			FileNode albumfile = songnode.Navigate("gen/" + song.ID + "_keep.png_wii") as FileNode;

			if (chartfile == null)
				return false;

			if (albumfile == null)
				albumfile = songnode.Navigate("gen/" + song.ID + "_nomip_keep.bmp_wii") as FileNode;

			if (albumfile != null) {
				song.AlbumArt = WiiImage.Create(new EndianReader(albumfile.Data, Endianness.LittleEndian)).Bitmap;
				albumfile.Data.Close();
			}

			ChartFormatRB.Instance.Create(formatdata, chartfile == null ? null : chartfile.Data, panfile == null ? null : panfile.Data, weightsfile == null ? null : weightsfile.Data, milofile == null ? null : milofile.Data, false);

			data.AddSong(formatdata);

			return true;
		}

		public override PlatformData Create(string path, Game game, ProgressIndicator progress)
		{
			PlatformData data = new PlatformData(this, game);

			DirectoryNode dir = data.GetDirectoryStructure(path);
			Ark ark = HarmonixMetadata.GetHarmonixArk(dir);

			data.Game = Platform.DetermineGame(data);

			string[] songdirs = new string[] { "songs", "songs_regional/na", "songs_regional/eu" };
			progress.NewTask(songdirs.Length);
			foreach (string songdirname in songdirs) {
				DirectoryNode songdir = ark.Root.Navigate(songdirname) as DirectoryNode;
				if (songdir == null)
					continue;

				FileNode songsdtbfile = songdir.Navigate("gen/songs.dtb") as FileNode;
				if (songsdtbfile == null)
					continue;

				data.Session["songdir"] = songdir;

				List<SongsDTA> dtas = new List<SongsDTA>();
				DTB.NodeTree dtb = DTB.Create(new EndianReader(new CryptedDtbStream(new EndianReader(songsdtbfile.Data, Endianness.LittleEndian)), Endianness.LittleEndian));
				progress.NewTask(dtb.Nodes.Count);
				foreach (DTB.Node node in dtb.Nodes) {
					progress.Progress();
					DTB.NodeTree tree = node as DTB.NodeTree;
					if (tree == null || tree.Nodes[0].Type != 0x00000005 || songdir.Find((tree.Nodes[0] as DTB.NodeString).Text) == null)
						continue;

					SongsDTA dta = SongsDTA.Create(tree);
					if (dtas.Find(d => d.BaseName == dta.BaseName) != null)
						continue; // Don't import songs twice

					dtas.Add(dta);

					SongData song = HarmonixMetadata.GetSongData(data, tree);

					AddSong(data, song, progress);
				}
				progress.EndTask();
				progress.Progress();
			}
			progress.EndTask();

			return data;
		}
	}
}
