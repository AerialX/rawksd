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

		public XmlElement Root;
		public Game Game;
		public string RootPath;

		public ImportMap(Game game, string path)
		{
			Game = game;
			RootPath = path;

			try {
				byte[] xmldata = File.ReadAllBytes(Path.Combine(RootPath, ((int)Game).ToString() + ".xml"));
				XmlReader reader = XmlReader.Create(new MemoryStream(xmldata));
				XmlDocument doc = new XmlDocument();
				doc.Load(reader);
				Root = doc.DocumentElement;
			} catch { }
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

			foreach (XmlNode node in Root.GetElementsByTagName("song")) {
				XmlElement element = node as XmlElement;

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
		}

		public static int GetBaseRank(Instrument instrument, int tier)
		{
			return Tiers[instrument][tier];
		}

		public static int GetBaseTier(Instrument instrument, int rank)
		{
			return Math.Max(Tiers[instrument].Count(t => t <= rank) - 1, 0);
		}
	}
}
