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

namespace ConsoleHaxx.RawkSD
{
	public enum Instrument : int
	{
		Ambient = 0,
		Guitar = 1,
		Bass = 2,
		Drums = 3,
		Vocals = 4,
		Preview = 5
	}

	public class SongData
	{
		public const string TreeName = "SongDataTree";

		public DataArray Data;

		public DataArray Tree { get { return Data.GetSubtree(TreeName, true); } }

		public Engine Game { get { return Platform.GetPlatform(Tree.GetValue<int>("GameID")); } set { Tree.SetValue("GameID", value.ID); RaisePropertyChangedEvent(); } }

		public string ID { get { return Tree.GetValue<string>("SongID"); } set { Tree.SetValue("SongID", value); RaisePropertyChangedEvent(); } }

		public string Name { get { return Tree.GetValue<string>("Name"); } set { Tree.SetValue("Name", value); RaisePropertyChangedEvent(); } }
		public string Artist { get { return Tree.GetValue<string>("Artist"); } set { Tree.SetValue("Artist", value); RaisePropertyChangedEvent(); } }

		public string Album { get { return Tree.GetValue<string>("Album"); } set { Tree.SetValue("Album", value); RaisePropertyChangedEvent(); } }
		public int AlbumTrack { get { return Tree.GetValue<int>("Track"); } set { Tree.SetValue("Track", value); RaisePropertyChangedEvent(); } }
		public int Year { get { return Tree.GetValue<int>("Year"); } set { Tree.SetValue("Year", value); RaisePropertyChangedEvent(); } }
		public bool Master { get { return Tree.GetValue<bool>("Master"); } set { Tree.SetValue("Master", value); RaisePropertyChangedEvent(); } }

		public string Genre { get { return Tree.GetValue<string>("Genre"); } set { Tree.SetValue("Genre", value); RaisePropertyChangedEvent(); } }

		public string Pack { get { return Tree.GetValue<string>("Pack"); } set { Tree.SetValue("Pack", value); RaisePropertyChangedEvent(); } }

		public string Vocalist { get { return Tree.GetValue<string>("Vocalist"); } set { Tree.SetValue("Vocalist", value); RaisePropertyChangedEvent(); } } // "male" "female" etc

		public int Version { get { return Tree.GetValue<int>("Version"); } set { Tree.SetValue("Version", value); RaisePropertyChangedEvent(); } }

		public int HopoThreshold { get { return Tree.GetValue<int>("HopoThreshold"); } set { Tree.SetValue("HopoTheshold", value); RaisePropertyChangedEvent(); } }

		public Bitmap AlbumArt {
			get {
				byte[] buffer = Tree.GetValue<byte[]>("AlbumArt");
				if (buffer == null)
					return null;
				return new Bitmap(new MemoryStream(buffer));
			} set {
				MemoryStream stream = new MemoryStream();
				value.Save(stream, ImageFormat.Png);
				Tree.SetValue("AlbumArt", stream.GetBuffer());
				RaisePropertyChangedEvent();
			}
		}

		public class InstrumentDifficulty : IEnumerable<KeyValuePair<Instrument, int>>
		{
			protected SongData Data;

			public InstrumentDifficulty(SongData data)
			{
				Data = data;
			}

			public int this[Instrument instrument] { get { return Data.Tree.GetValue<int>("instrument" + MetadataFormatHarmonix.InstrumentToString(instrument)); } set { Data.Tree.SetValue("instrument" + MetadataFormatHarmonix.InstrumentToString(instrument), value); Data.RaisePropertyChangedEvent(); } }

			protected Dictionary<Instrument, int> GetBaseEnumerator()
			{
				Dictionary<Instrument, int> dictionary = new Dictionary<Instrument, int>();
				for (Instrument instrument = Instrument.Ambient; instrument <= Instrument.Vocals; instrument++) {
					dictionary[instrument] = this[instrument];
				}

				return dictionary;
			}

			public IEnumerator<KeyValuePair<Instrument, int>> GetEnumerator()
			{
				return GetBaseEnumerator().GetEnumerator();
			}

			IEnumerator IEnumerable.GetEnumerator()
			{
				return GetBaseEnumerator().GetEnumerator();
			}
		}

		public InstrumentDifficulty Difficulty;

		public event Action<SongData> PropertyChanged;

		public void RaisePropertyChangedEvent()
		{
			if (PropertyChanged != null)
				PropertyChanged(this);
		}

		public SongData()
		{
			Difficulty = new InstrumentDifficulty(this);
			Data = new DataArray();
		}

		public static SongData Create(Stream stream)
		{
			SongData data = new SongData();
			data.Data = DataArray.Create(stream);
			
			return data;
		}

		public void Save(Stream stream)
		{
			Data.Save(stream);
		}

		public static SongData CreateFromXML(Stream stream)
		{
			XmlReader reader = XmlReader.Create(stream);
			XmlDocument doc = new XmlDocument();
			doc.Load(reader);

			XmlNode node = doc.DocumentElement;

			SongData data = new SongData();

			foreach (XmlAttribute att in node.Attributes) {
				switch (att.Name.ToLower()) {
					case "id":
						data.ID = att.Value;
						break;
					case "game":
						int gameid = int.Parse(att.Value);
						data.Game = Platform.GetPlatform(gameid);
						break;
					case "name":
						data.Name = att.Value;
						break;
					case "artist":
						data.Artist = att.Value;
						break;
					case "album":
						data.Album = att.Value;
						break;
					case "track":
						data.AlbumTrack = int.Parse(att.Value);
						break;
					case "year":
						data.Year = int.Parse(att.Value);
						break;
					case "master":
						data.Master = string.Compare(att.Value, "true", true) == 0 ? true : false;
						break;
					case "genre":
						data.Genre = att.Value;
						break;
					case "pack":
						data.Pack = att.Value;
						break;
					case "vocalist":
						data.Vocalist = att.Value;
						break;
					case "version":
						data.Version = int.Parse(att.Value);
						break;
				}
			}

			foreach (XmlElement sub in node.ChildNodes.OfType<XmlElement>()) {
				switch (node.Name.ToLower()) {
					case "difficulty": {
							Instrument instrument = Instrument.Ambient;
							int rank = 0;
							foreach (XmlAttribute att in sub.Attributes) {
								switch (att.Name.ToLower()) {
									case "instrument":
										instrument = Platform.InstrumentFromString(att.Value);
										break;
									case "rank":
										rank = int.Parse(att.Value);
										break;
								}
							}
							data.Difficulty[instrument] =  rank;
							break;
						}
				}
			}

			// Path = album art?

			return data;
		}
	}

	public class ImportMap
	{
	}

	public enum Games
	{
		GuitarHero1 = 0x01,
		GuitarHero2 = 0x02,
		GuitarHero80s = 0x03,
		GuitarHero3 = 0x10,
		GuitarHeroAerosmith = 0x11,
		GuitarHeroWorldTour = 0x12,
		GuitarHeroMetallica = 0x13,
		GuitarHeroSmashHits = 0x14,
		GuitarHeroVanHalen = 0x15,
		GuitarHero5 = 0x20,
		BandHero = 0x21,
		RockBand = 0x80,
		RockBandACDC = 0x81,
		RockBandTP1 = 0x82,
		RockBandTP2 = 0x83,
		RockBandMetalTP = 0x84,
		RockBandCountryTP = 0x85,
		RockBand2 = 0x90,
		RockBandBeatles = 0x91,
		LegoRockBand = 0x92,
		RockBandGreenDay = 0x93,
		RockBand3 = 0x100
	}
	
	public class Game
	{
		public static Dictionary<Games, Game> GameList;

		static Game()
		{
			GameList = new Dictionary<Games, Game>();
			AddGame(new Game(Games.GuitarHero1, "Guitar Hero 1", PlatformGH1PS2Disc.Instance));
			AddGame(new Game(Games.GuitarHero2, "Guitar Hero 2", PlatformGH2PS2Disc.Instance));
			AddGame(new Game(Games.GuitarHero80s, "Guitar Hero Encore: Rocks the 80s", PlatformGH2PS2Disc.Instance));

			AddGame(new Game(Games.RockBand, "Rock Band", PlatformRB2WiiDisc.Instance, PlatformRB2360DLC.Instance));
			AddGame(new Game(Games.RockBandACDC, "AC/DC Live: Rock Band Track Pack", PlatformRB2WiiDisc.Instance));
			AddGame(new Game(Games.RockBand2, "Rock Band 2", PlatformRB2WiiDisc.Instance, PlatformRB2WiiDLC.Instance, PlatformRB2360Disc.Instance, PlatformRB2360DLC.Instance));
			AddGame(new Game(Games.RockBandBeatles, "The Beatles: Rock Band", PlatformRB2WiiDisc.Instance, PlatformRB2WiiDLC.Instance, PlatformRB2360Disc.Instance, PlatformRB2360DLC.Instance));
			AddGame(new Game(Games.LegoRockBand, "Lego Rock Band", PlatformRB2WiiDisc.Instance, PlatformRB2WiiDLC.Instance, PlatformRB2360Disc.Instance, PlatformRB2360DLC.Instance));

			AddGame(new Game(Games.GuitarHero3, "Guitar Hero 3: Legends of Rock", PlatformGH3WiiDisc.Instance));
			AddGame(new Game(Games.GuitarHeroAerosmith, "Guitar Hero: Aerosmith", PlatformGH3WiiDisc.Instance));
			AddGame(new Game(Games.GuitarHeroWorldTour, "Guitar Hero: World Tour", PlatformGH4WiiDisc.Instance, PlatformGH4WiiDLC.Instance));
			AddGame(new Game(Games.GuitarHeroMetallica, "Guitar Hero: Metallica", PlatformGH4WiiDisc.Instance, PlatformGH4WiiDLC.Instance));
			AddGame(new Game(Games.GuitarHeroSmashHits, "Guitar Hero: Smash Hits", PlatformGH4WiiDisc.Instance, PlatformGH4WiiDLC.Instance));
			AddGame(new Game(Games.GuitarHeroVanHalen, "Guitar Hero: Van Halen", PlatformGH4WiiDisc.Instance, PlatformGH4WiiDLC.Instance));
			AddGame(new Game(Games.GuitarHero5, "Guitar Hero 5", PlatformGH5WiiDisc.Instance, PlatformGH5WiiDLC.Instance));
			AddGame(new Game(Games.BandHero, "Band Hero", PlatformGH5WiiDisc.Instance, PlatformGH5WiiDLC.Instance));
		}

		private static void AddGame(Game game)
		{
			GameList.Add(game.ID, game);
		}

		public static Game GetGame(Games game)
		{
			return GameList[game];
		}

		public Game(Games id, string name, params Engine[] platforms)
		{
			ID = id;
			Name = name;

			Platforms = new List<Engine>();
			Platforms.AddRange(platforms);
		}

		public Games ID { get; protected set; }
		public string Name { get; protected set; }

		public List<Engine> Platforms;
	}
}
