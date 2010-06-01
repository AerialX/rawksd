using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Xml;
using System.Drawing;
using ConsoleHaxx.Harmonix;
using ConsoleHaxx.Common;
using System.Collections;
using System.Drawing.Imaging;
using System.Net;

namespace ConsoleHaxx.RawkSD
{
	public class ImportMap
	{
		public static string RootPath;

		public enum NamePrefix
		{
			None = 0,
			Prefix,
			Postfix
		}
		public static NamePrefix ApplyNamePrefix = NamePrefix.Prefix;

		public static Dictionary<string, string> Genres = new Dictionary<string, string>() {
			{ "alternative", "Alternative" },
			{ "blues", "Blues" },
			{ "classicrock", "Classic Rock" },
			{ "country", "Country" },
			{ "emo", "Emo" },
			{ "fusion", "Fusion" },
			{ "glam", "Glam" },
			{ "jazz", "Jazz" },
			{ "metal", "Metal" },
			{ "novelty", "Novelty" },
			{ "numetal", "Nu Metal" },
			{ "poprock", "Pop Rock" },
			{ "prog", "Progressive" },
			{ "punk", "Punk" },
			{ "rock", "Rock" },
			{ "southernrock", "Southern Rock" },
			{ "urban", "Urban" },
			{ "other", "Other" },
			{ "new_wave", "New Wave" },
			{ "indie", "Indie" },
			{ "indierock", "Indie Rock" },
			{ "grunge", "Grunge" }
		};

		public static Dictionary<Instrument, int[]> Tiers = new Dictionary<Instrument, int[]>() {
			{ Instrument.Ambient, new int[] { 0, 159, 219, 275, 328, 384, 459 } },
			{ Instrument.Guitar, new int[] { 1, 145, 194, 248, 302, 355, 406 } },
			{ Instrument.Bass, new int[] { 1, 166, 220, 260, 301, 350, 405 } },
			{ Instrument.Vocals, new int[] { 1, 139, 180, 220, 259, 301, 373 } },
			{ Instrument.Drums, new int[] { 1, 136, 169, 208, 296, 350, 402 } }
		};

		public struct SoloInfo
		{
			public SoloInfo(string start, float starto, string end, float endo)
			{
				StartSection = start;
				StartOffset = starto;

				EndSection = end;
				EndOffset = endo;
			}

			public string StartSection;
			public string EndSection;

			// Beats (positive means into, negative before)
			public float StartOffset;
			public float EndOffset;
		}

		public XmlElement Root;
		public Game Game;

		public ImportMap(Game game)
		{
			Game = game;

			try {
				Stream stream = new FileStream(Path.Combine(RootPath, ((int)Game).ToString() + ".xml"), FileMode.Open, FileAccess.Read, FileShare.Read);
				XmlReader reader = XmlReader.Create(stream);
				XmlDocument doc = new XmlDocument();
				doc.Load(reader);
				Root = doc.DocumentElement;
				reader.Close();
				stream.Close();
			} catch { }
		}

		public bool PopulateChart(SongData song, NoteChart chart)
		{
			if (Root == null)
				return false;

			string idprefix = string.Empty;

			bool ret = false;

			foreach (XmlElement node in Root.GetElementsByTagName("global")) {
				foreach (XmlElement element in node.GetElementsByTagName("idprefix"))
					idprefix = element.InnerText;
			}

			string songid = song.ID;
			if (songid.StartsWith(idprefix))
				songid = songid.Substring(idprefix.Length);

			List<SoloInfo> solos = new List<SoloInfo>();

			foreach (XmlElement element in Root.GetElementsByTagName("song")) {
				if (element.Attributes["game"] != null && int.Parse(element.Attributes["game"].Value) != (int)Game)
					continue;

				if (string.Compare(element.Attributes["id"].Value, songid, true) == 0) {
					foreach (XmlElement soloelement in element.GetElementsByTagName("solo")) {
						string startsection = soloelement.Attributes["start"] != null ? soloelement.Attributes["start"].Value : string.Empty;
						string endsection = soloelement.Attributes["end"] != null ? soloelement.Attributes["end"].Value : startsection;
						float startoffset = float.Parse(soloelement.Attributes["startoffset"] != null ? soloelement.Attributes["startoffset"].Value : "0");
						float endoffset = float.Parse(soloelement.Attributes["endoffset"] != null ? soloelement.Attributes["endoffset"].Value : "0");
						Instrument instrument = soloelement.Attributes["instrument"] != null ? Platform.InstrumentFromString(soloelement.Attributes["instrument"].Value) : Instrument.Guitar;
						if (!startsection.HasValue())
							continue;

						NoteChart.Point start = new NoteChart.Point(chart.Events.Sections.Find(e => string.Compare(e.Value, startsection.Replace(' ', '_'), true) == 0).Key.Time);
						int endindex = chart.Events.Sections.FindIndex(e => string.Compare(e.Value, endsection.Replace(' ', '_'), true) == 0);
						NoteChart.Point end;
						if (endindex + 1 < chart.Events.Sections.Count)
							end = new NoteChart.Point(chart.Events.Sections[endindex + 1].Key.Time);
						else
							end = new NoteChart.Point(chart.Events.End.Time);

						start.Time += (ulong)(startoffset * (float)chart.Division.TicksPerBeat);
						end.Time += (ulong)(endoffset * (float)chart.Division.TicksPerBeat);

						NoteChart.Note note = new NoteChart.Note(start.Time, end.Time - start.Time);

						if (instrument == Instrument.Guitar)
							chart.PartGuitar.SoloSections.Add(note);
						else if (instrument == Instrument.Bass)
							chart.PartBass.SoloSections.Add(note);
						else if (instrument == Instrument.Drums)
							chart.PartDrums.SoloSections.Add(note);
						ret = true;
					}
				}
			}

			return ret;
		}

		public bool Populate(SongData song)
		{
			bool ret = false;

			string idprefix = string.Empty;
			string nameprefix = string.Empty;

			if (Root == null)
				goto populateend;

			foreach (XmlElement node in Root.GetElementsByTagName("global")) {
				song.PopulateFromXML(node as XmlElement, RootPath);
				ret = true;
				foreach (XmlElement element in node.GetElementsByTagName("idprefix"))
					idprefix = element.InnerText;
				foreach (XmlElement element in node.GetElementsByTagName("nameprefix"))
					nameprefix = element.InnerText;
			}

			foreach (XmlElement element in Root.GetElementsByTagName("song")) {
				if (element.Attributes["game"] != null && int.Parse(element.Attributes["game"].Value) != (int)Game)
					continue;

				if (string.Compare(element.Attributes["id"].Value, song.ID, true) == 0) {
					song.PopulateFromXML(element, RootPath);
					ret = true;
					goto populateend;
				}
			}

		populateend:
			if (idprefix.Length > 0)
				song.ID = idprefix + song.ID;
			if (nameprefix.Length > 0) {
				switch (ApplyNamePrefix) {
					case NamePrefix.Prefix:
						song.Name = nameprefix + " " + song.Name;
						break;
					case NamePrefix.Postfix:
						song.Name = song.Name + " " + nameprefix;
						break;
				}
			}
			if (!song.Pack.HasValue() && song.Game != Game.Unknown)
				song.Pack = Platform.GameName(song.Game);
			return ret;
		}

		public void AddSongs(PlatformData data, ProgressIndicator progress)
		{
			if (Root == null)
				return;

			foreach (XmlNode node in Root.GetElementsByTagName("addsong")) {
				XmlElement element = node as XmlElement;

				if (element.Attributes["game"] != null && int.Parse(element.Attributes["game"].Value) != (int)data.Game)
					continue;

				SongData song = new SongData(data);
				if (element.Attributes["id"] != null)
					song.ID = element.Attributes["id"].Value;
				song.PopulateFromXML(element, RootPath);
				data.Platform.AddSong(data, song, progress);
			}

			data.Mutex.WaitOne();
			IList<FormatData> songs = data.Songs;
			foreach (XmlNode node in Root.GetElementsByTagName("deletesong")) {
				XmlElement element = node as XmlElement;

				if (element.Attributes["game"] != null && int.Parse(element.Attributes["game"].Value) != (int)data.Game)
					continue;

				if (element.Attributes["id"] != null) {
					foreach (FormatData song in songs.Where(f => string.Compare(f.Song.ID, element.Attributes["id"].Value, true) == 0).ToList())
						data.RemoveSong(song);
				}

				if (element.Attributes["name"] != null) {
					foreach (FormatData song in songs.Where(f => string.Compare(f.Song.Name, element.Attributes["name"].Value, true) == 0).ToList())
						data.RemoveSong(song);
				}
			}
			data.Mutex.ReleaseMutex();
		}

		public static int GetBaseRank(Instrument instrument, int tier)
		{
			return Tiers[instrument][tier];
		}

		public static int GetBaseTier(Instrument instrument, int rank)
		{
			return Math.Max(Tiers[instrument].Count(t => t <= rank) - 1, 0);
		}

		public static string GetShortGenre(string genre)
		{
			var items = Genres.Where(p => string.Compare(p.Value, genre, true) == 0);
			if (items.Count() == 0)
				return genre;
			return items.First().Key;
		}

		public static string GetShortName(string name)
		{
			string id = string.Empty;
			foreach (char letter in name) {
				if (char.IsLetterOrDigit(letter))
					id += char.ToLower(letter);
			}
			return id;
		}

		public static bool ImportChart(SongData song, NoteChart chart)
		{
			ImportMap import = new ImportMap(song.Game);
			return import.PopulateChart(song, chart);
		}
	}
}
