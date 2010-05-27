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

		public static PlatformGH3WiiDisc Instance;
		public static void Initialise()
		{
			Instance = new PlatformGH3WiiDisc();

			PlatformDetection.AddSelection(Instance);
			PlatformDetection.DetectDirectoryNode += new Action<string, DirectoryNode, List<Pair<Engine, Game>>>(PlatformDetection_DetectDirectoryNode);
		}

		static void PlatformDetection_DetectDirectoryNode(string path, DirectoryNode root, List<Pair<Engine, Game>> platforms)
		{
			if (root.Navigate("pak/qb.pak.ngc") != null && root.Navigate("pak/qs.pak.ngc") == null)
				platforms.Add(new Pair<Engine,Game>(Instance, Game.Unknown));
		}

		public override int ID { get { throw new NotImplementedException(); } }

		public override string Name { get { return "Guitar Hero 3 Wii Disc"; } }

		public override bool AddSong(PlatformData data, SongData song, ProgressIndicator progress)
		{
			string albumname = null;
			switch (song.ID) {
				case "dontholdback": albumname = "TheSleepingQuestionsAndAnswers"; break;
				case "minuscelsius": albumname = "BackyardBabiesStockholmSyndrome"; break;
				case "thrufireandflames": albumname = "DragonforceInhumanRampage"; break;
				case "fcpremix": albumname = "FallofTroyDoppelganger"; break;
				case "avalancha": albumname = "HeroesDelSilencioAvalancha"; break;
				case "takethislife": albumname = "InFlamesComeClarity"; break;
				case "ruby": albumname = "KaiserChiefsYoursTrulyAngry_Mob"; break;
				case "mycurse": albumname = "KillswitchEngageAsDaylightDies"; break;
				case "closer": albumname = "LacunaCoilKarmaCode"; break;
				case "metalheavylady": albumname = "LionsLions"; break;
				case "mauvaisgarcon": albumname = "NaastAntichambre"; break;
				case "generationrock": albumname = "RevolverheldRevolverheld"; break;
				case "prayeroftherefugee": albumname = "RiseAgainstTheSuffererAndTheWitness"; break;
				case "cantbesaved": albumname = "SensesFailStillSearching"; break;
				case "shebangsadrum": albumname = "StoneRosesStoneRoses"; break;
				case "radiosong": albumname = "SuperbusPopnGum"; break;
				case "bellyofashark": albumname = "TheGallowsOrchestraofWoles"; break;
				case "gothatfar": albumname = "BretMichealsBandGoThatFar"; break;
				case "impulse": albumname = "endlesssporadic"; break;
				case "thewayitends": albumname = "prototype_continuum_cover"; break;
				case "nothingformehere": albumname = "Dope_PosterCover_edsel"; break;
				case "inlove": albumname = "store_song_ScoutsStSebastian"; break;
			}
			if (albumname != null) {
				Pak albumpak = data.Session["albumpak"] as Pak;
				Pak.Node node = albumpak.Nodes.Find(n => n.FilenamePakKey == QbKey.Create(albumname).Crc);
				if (node != null)
					song.AlbumArt = NgcImage.Create(new EndianReader(node.Data, Endianness.BigEndian)).Bitmap;
			}

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

			for (int coop = 0; coop < 2; coop++) {
				if (coop == 1) {
					song = new SongData(song);
					song.ID += "_coop";
					song.Name += " [coop]";
				}
				FormatData formatdata = new TemporaryFormatData(song, data);

				FileNode datfile = dir.Navigate("music/" + song.ID + ".dat.ngc", false, true) as FileNode;
				FileNode wadfile = dir.Navigate("music/" + song.ID + ".wad.ngc", false, true) as FileNode;

				if (datfile == null || wadfile == null)
					continue;
				
				AudioFormatGH3WiiFSB.Instance.Create(formatdata, datfile.Data, wadfile.Data);

				ChartFormatGH3.Instance.Create(formatdata, chartpak.Data, sectionfile.Data, coop == 1);

				data.AddSong(formatdata);
			}

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

			Pak qb = new Pak(new EndianReader(qbpak.Data, Endianness.BigEndian));
			FileNode songlistfile = qb.Root.Find("songlist.qb.ngc", SearchOption.AllDirectories) as FileNode;
			FileNode albumfile = dir.Navigate("pak/album_covers/album_covers.pak.ngc", false, true) as FileNode;
			QbFile songlist = new QbFile(songlistfile.Data, PakFormat);
			QbItemBase list = songlist.FindItem(QbKey.Create(NeversoftMetadata.SonglistKeys[0]), true);

			data.Session["rootdir"] = dir;
			data.Session["rootqb"] = qb;

			if (albumfile != null)
				data.Session["albumpak"] = new Pak(new EndianReader(albumfile.Data, Endianness.BigEndian));

			var items = (list as QbItemStruct).Items;
			progress.NewTask(items.Count);
			foreach (QbItemStruct item in items) {
				// TODO: Coop
				
				SongData song = NeversoftMetadata.GetSongData(data, item);

				AddSong(data, song, progress);

				progress.Progress();
			}
			progress.EndTask();

			return data;
		}
	}
}
