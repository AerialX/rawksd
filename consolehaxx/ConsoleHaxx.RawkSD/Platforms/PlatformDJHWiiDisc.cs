using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using ConsoleHaxx.Common;
using System.Xml;
using System.IO;

namespace ConsoleHaxx.RawkSD
{
	public class PlatformDJHWiiDisc : Engine
	{
		public static PlatformDJHWiiDisc Instance;
		public static void Initialise()
		{
			Instance = new PlatformDJHWiiDisc();

			PlatformDetection.AddSelection(Instance);
			PlatformDetection.DetectDirectoryNode += new Action<string, DirectoryNode, List<Pair<Engine, Game>>>(PlatformDetection_DetectDirectoryNode);
		}

		static void PlatformDetection_DetectDirectoryNode(string path, DirectoryNode root, List<Pair<Engine, Game>> platforms)
		{
			if (root.Navigate("Wii/HomeButton/homeBtn.arc") != null)
				platforms.Add(new Pair<Engine, Game>(Instance, Game.DjHero));
		}

		public override int ID { get { throw new NotImplementedException(); } }

		public override string Name { get { return "DJ Hero Wii Disc"; } }

		public override PlatformData Create(string path, Game game, ProgressIndicator progress)
		{
			PlatformData data = new PlatformData(this, game);

			DirectoryNode dir = data.GetDirectoryStructure(path);

			data.Game = Platform.DetermineGame(data);

			FileNode tracklist = dir.Navigate("/Common/AUDIO/Audiotracks/TrackListing.xml") as FileNode;
			XmlReader reader = XmlReader.Create(tracklist.Data);
			tracklist.Data.Close();
			XmlDocument doc = new XmlDocument();
			doc.Load(reader);

			var elements = doc.DocumentElement.GetElementsByTagName("Track");

			progress.NewTask(elements.Count);

			foreach (XmlElement element in elements) {
				SongData song = FreeStyleGamesMetadata.GetSongData(element);

				AddSong(data, song, progress);

				progress.Progress();
			}

			return data;
		}

		public override bool AddSong(PlatformData data, SongData song, ProgressIndicator progress)
		{
			FormatData formatdata = new TemporaryFormatData(song, data);

			DirectoryNode dir = data.Session["rootdirnode"] as DirectoryNode;

			string path = song.Data.GetValue<string>("FolderLocation");
			DirectoryNode songdir = dir.Navigate("Common/" + path) as DirectoryNode;
			DirectoryNode audiodir = dir.Navigate("Wii/" + path) as DirectoryNode;

			string mode = "SinglePlayer";
			string difficulty = "Medium";
			path = mode + "/" + difficulty;

			//audiodir = audiodir.Navigate(path) as DirectoryNode;

			List<FileNode> audiofiles = new List<FileNode>();
			audiofiles.Add(audiodir.Find("AudioTrack_Main.fsb", SearchOption.AllDirectories) as FileNode);
			audiofiles.Add(audiodir.Find("AudioTrack_Main_P1.fsb", SearchOption.AllDirectories) as FileNode);
			audiofiles.Add(audiodir.Find("AudioTrack_Main_P2.fsb", SearchOption.AllDirectories) as FileNode);
			audiofiles.RemoveAll(f => f == null);

			List<FileNode> chartfiles = new List<FileNode>();
			chartfiles.Add(audiodir.Find("Markup_Main_P1_0.fsgmub", SearchOption.AllDirectories) as FileNode);
			chartfiles.Add(audiodir.Find("Markup_Main_P1_1.fsgmub", SearchOption.AllDirectories) as FileNode);
			chartfiles.Add(audiodir.Find("Markup_Main_P1_2.fsgmub", SearchOption.AllDirectories) as FileNode);
			chartfiles.Add(audiodir.Find("Markup_Main_P1_3.fsgmub", SearchOption.AllDirectories) as FileNode);
			chartfiles.Add(audiodir.Find("Markup_Main_P1_4.fsgmub", SearchOption.AllDirectories) as FileNode);
			chartfiles.Add(audiodir.Find("Markup_Main_P2.fsgmub", SearchOption.AllDirectories) as FileNode);
			chartfiles.Add(audiodir.Find("Markup_Main_P1.fsgmub", SearchOption.AllDirectories) as FileNode);
			chartfiles.RemoveAll(f => f == null);

			if (audiofiles.Count == 0 && chartfiles.Count == 0)
				return false;

			if (audiofiles.Count > 0)
				AudioFormatMp3.Instance.CreateFromFSB(formatdata, audiofiles.Select(a => a.Data).ToArray(), FreeStyleGamesMetadata.GetAudioFormat(song));

			if (chartfiles.Count > 0)
				ChartFormatFsgMub.Instance.Create(formatdata, chartfiles.Select(c => c.Data).ToArray());

			return true;
		}
	}
}
