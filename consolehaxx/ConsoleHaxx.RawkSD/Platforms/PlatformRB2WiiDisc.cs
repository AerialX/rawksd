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
		public static PlatformRB2WiiDisc Instance;
		public static void Initialise()
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

		public override string Name { get { return "Rock Band Wii Disc"; } }

		public override bool AddSong(PlatformData data, SongData song, ProgressIndicator progress)
		{
			FormatData formatdata = new TemporaryFormatData(song, data);

			DirectoryNode songdir = data.Session["songdir"] as DirectoryNode;
			
			SongsDTA dta = HarmonixMetadata.GetSongsDTA(song);

			if (dta == null) { // LRB's <addsong />
				dta = HarmonixMetadata.GetSongData(song);
				dta.Song.Cores = new List<int>() { -1, -1, -1, -1, -1, 1, 1, -1, -1 };
				dta.Song.Vols = new List<float>() { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
				dta.Song.Pans = new List<float>() { 0, 0, -1, 1, 0, -1, 1, 0, 0 };
				dta.Song.Tracks.Find(t => t.Name == "drum").Tracks.AddRange(new int[] { 0, 1, 2, 3 });
				dta.Song.Tracks.Find(t => t.Name == "bass").Tracks.AddRange(new int[] { 4 });
				dta.Song.Tracks.Find(t => t.Name == "guitar").Tracks.AddRange(new int[] { 5, 6 });
				dta.Song.Tracks.Find(t => t.Name == "vocals").Tracks.AddRange(new int[] { 7 });
				dta.Song.Name = "songs/" + song.ID + "/" + song.ID;
				HarmonixMetadata.SetSongsDTA(song, dta.ToDTB());
			}

			string dtaname = dta.Song.Name;
			if (dtaname.StartsWith("dlc"))
				dtaname = dtaname.Split(new char[] { '/' }, 4)[3];
			
			int lastslash = dtaname.LastIndexOf('/');
			string basename = dtaname.Substring(lastslash + 1);
			dtaname = dtaname.Substring(0, lastslash);
			DirectoryNode songnode = songdir.Navigate(dtaname) as DirectoryNode;
			if (songnode == null)
				return false;

			FileNode songaudiofile = songnode.Navigate(basename + ".bik", false, true) as FileNode;
			AudioFormat audio = null;
			if (songaudiofile == null) {
				songaudiofile = songnode.Navigate(basename + ".mogg", false, true) as FileNode;
				if (songaudiofile == null)
					return false;
				if (HarmonixMetadata.IsRockBand1(data.Game)) {
					audio = HarmonixMetadata.GetAudioFormat(song);
					audio.InitialOffset = 3000;
				}
				AudioFormatMogg.Instance.Create(formatdata, songaudiofile.Data, audio);
			} else {
				AudioFormatRB2Bink.Instance.Create(formatdata, songaudiofile.Data, null);
			}

			// TODO:	SongInfo
			// TODO:	Preview

			FileNode chartfile = songnode.Find(basename + ".mid") as FileNode;
			FileNode panfile = songnode.Find(basename + ".pan") as FileNode;
			FileNode weightsfile = songnode.Navigate("gen/" + basename + "_weights.bin") as FileNode;
			FileNode milofile = songnode.Navigate("gen/" + basename + ".milo_wii") as FileNode;
			FileNode albumfile = songnode.Navigate("gen/" + basename + "_keep.png_wii") as FileNode;

			if (chartfile == null)
				return false;

			if (albumfile == null)
				albumfile = songnode.Navigate("gen/" + basename + "_nomip_keep.bmp_wii") as FileNode;

			if (albumfile != null) {
				song.AlbumArt = WiiImage.Create(new EndianReader(albumfile.Data, Endianness.LittleEndian)).Bitmap;
				albumfile.Data.Close();
			}

			ChartFormatRB.Instance.Create(formatdata, chartfile == null ? null : chartfile.Data, panfile == null ? null : panfile.Data, weightsfile == null ? null : weightsfile.Data, milofile == null ? null : milofile.Data, false, false, data.Game);

			data.AddSong(formatdata);

			return true;
		}

		public override PlatformData Create(string path, Game game, ProgressIndicator progress)
		{
			PlatformData data = new PlatformData(this, game);

			Ark ark = data.GetHarmonixArk(path);

			data.Game = Platform.DetermineGame(data);

			if (data.Game == Game.RockBand2 || data.Game == Game.RockBandBeatles)
				Exceptions.Error("Unable to parse song list from Rock Band Wii disc.");

			data.Session["songdir"] = ark.Root;

			string[] songdirs = new string[] { "songs", "songs_regional/na", "songs_regional/eu" };
			progress.NewTask(songdirs.Length);
			foreach (string songdirname in songdirs) {
				DirectoryNode songdir = ark.Root.Navigate(songdirname) as DirectoryNode;
				if (songdir == null)
					continue;

				FileNode songsdtbfile = songdir.Navigate("gen/songs.dtb") as FileNode;
				if (songsdtbfile == null)
					continue;

				try {
					List<SongsDTA> dtas = new List<SongsDTA>();
					DTB.NodeTree dtb = DTB.Create(new EndianReader(new CryptedDtbStream(new EndianReader(songsdtbfile.Data, Endianness.LittleEndian)), Endianness.LittleEndian));
					progress.NewTask(dtb.Nodes.Count);
					foreach (DTB.Node node in dtb.Nodes) {
						DTB.NodeTree tree = node as DTB.NodeTree;
						if (tree == null || tree.Nodes[0].Type != 0x00000005 || songdir.Find((tree.Nodes[0] as DTB.NodeString).Text) == null) {
							progress.Progress();
							continue;
						}

						SongsDTA dta = SongsDTA.Create(tree);
						if (dtas.Find(d => d.BaseName == dta.BaseName) != null) {
							progress.Progress();
							continue; // Don't import songs twice
						}

						dtas.Add(dta);

						try {
							SongData song = HarmonixMetadata.GetSongData(data, tree);

							AddSong(data, song, progress);
						} catch (Exception exception) {
							Exceptions.Warning(exception, "Could not import " + dta.Name + " from the Rock Band Wii disc.");
						}

						progress.Progress();
					}
				} catch (Exception exception) {
					Exceptions.Warning(exception, "Unable to parse song list from Rock Band Wii disc: " + songdirname);
				}
				progress.EndTask();
				progress.Progress();
			}

			progress.EndTask();

			return data;
		}
	}
}
