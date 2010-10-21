using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Drawing;
using System.IO;
using System.Drawing.Imaging;
using System.Collections;
using System.Xml;
using ConsoleHaxx.Harmonix;
using ConsoleHaxx.Common;

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

		public Game Game { get { return (Game)Tree.GetValue<int>("GameID"); } set { Tree.SetValue("GameID", (int)value); RaisePropertyChangedEvent(); } }

		private string idcache = null;
		//public string ID { get { return Tree.GetValue<string>("SongID"); } set { Tree.SetValue("SongID", value); RaisePropertyChangedEvent(); } }
		public string ID { get { return idcache ?? Tree.GetValue<string>("SongID"); } set { Tree.SetValue("SongID", value); idcache = value; RaisePropertyChangedEvent(); } }

		public string Name { get { return Tree.GetValue<string>("Name"); } set { Tree.SetValue("Name", value); if (ID == null) ID = SongNameToID(value); else RaisePropertyChangedEvent(); } }

		public string Artist { get { return Tree.GetValue<string>("Artist"); } set { Tree.SetValue("Artist", value); RaisePropertyChangedEvent(); } }

		public string Album { get { return Tree.GetValue<string>("Album"); } set { Tree.SetValue("Album", value); RaisePropertyChangedEvent(); } }
		public int AlbumTrack { get { return Tree.GetValue<int>("Track"); } set { Tree.SetValue("Track", value); RaisePropertyChangedEvent(); } }
		public int Year { get { return Tree.GetValue<int>("Year"); } set { Tree.SetValue("Year", value); RaisePropertyChangedEvent(); } }
		public bool Master { get { return Tree.GetValue<bool>("Master"); } set { Tree.SetValue("Master", value); RaisePropertyChangedEvent(); } }

		public string Genre { get { return Tree.GetValue<string>("Genre"); } set { Tree.SetValue("Genre", value); RaisePropertyChangedEvent(); } }
		public string TidyGenre { get { return ImportMap.Genres.ContainsKey(Genre) ? ImportMap.Genres[Genre] : Genre; } }

		public string Charter { get { return Tree.GetValue<string>("Charter"); } set { Tree.SetValue("Charter", value); RaisePropertyChangedEvent(); } }
		public string Pack { get { return Tree.GetValue<string>("Pack"); } set { Tree.SetValue("Pack", value); RaisePropertyChangedEvent(); } }

		public string Vocalist { get { return Tree.GetValue<string>("Vocalist"); } set { Tree.SetValue("Vocalist", value); RaisePropertyChangedEvent(); } } // "male" "female" etc

		public int Version { get { return Tree.GetValue<int>("Version"); } set { Tree.SetValue("Version", value); RaisePropertyChangedEvent(); } }

		public int HopoThreshold { get { return Tree.GetValue<int>("HopoThreshold"); } set { Tree.SetValue("HopoThreshold", value); RaisePropertyChangedEvent(); } }

		public IList<int> PreviewTimes { get { return Tree.GetArray<int>("Preview"); } set { Tree.SetArray("Preview", value); RaisePropertyChangedEvent(); } }

		public Bitmap AlbumArt
		{
			get
			{
				byte[] buffer = Tree.GetValue<byte[]>("AlbumArt");
				if (buffer == null)
					return null;
				return new Bitmap(new MemoryStream(buffer));
			}
			set
			{
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

			public int this[Instrument instrument] { get { return Data.Tree.GetValue<int>("instrument" + HarmonixMetadata.InstrumentToString(instrument)); } set { Data.Tree.SetValue("instrument" + HarmonixMetadata.InstrumentToString(instrument), value); Data.RaisePropertyChangedEvent(); } }

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

			Data.PropertyChanged += new Action<DataArray>(Data_PropertyChanged);

			PreviewTimes = new int[] { 30000, 60000 };
			HopoThreshold = 170;
			Version = 1;
			Vocalist = "male";
			Master = true;
			Year = 2010;
			Genre = "rock";
		}

		public void SetPreviewTime(int index, int value)
		{
			IList<int> preview = PreviewTimes;
			preview[index] = value;
			PreviewTimes = preview;
		}

		void Data_PropertyChanged(DataArray data)
		{
			RaisePropertyChangedEvent();
		}

		public SongData(PlatformData data) : this()
		{
			Game = data.Game;
		}

		public SongData(SongData song) : this()
		{
			Stream stream = new MemoryStream();
			song.Data.Save(stream);
			stream.Position = 0;
			Data.Options = DTB.Create(new EndianReader(stream, Endianness.LittleEndian));
		}

		public static SongData Create(Stream stream)
		{
			SongData data = new SongData();
			data.Data = DataArray.Create(stream);

			data.Data.PropertyChanged += new Action<DataArray>(data.Data_PropertyChanged);

			return data;
		}

		public void Save(Stream stream)
		{
			Data.Save(stream);
		}

		public static SongData CreateFromXML(Stream stream, string rootpath)
		{
			SongData data = new SongData();
			XmlReader reader = XmlReader.Create(stream);
			XmlDocument doc = new XmlDocument();
			doc.Load(reader);
			data.PopulateFromXML(doc.DocumentElement, rootpath);
			return data;
		}

		public void PopulateFromXML(XmlElement node, string rootpath)
		{
			foreach (XmlNode element in node.ChildNodes) {
				if (!(element is XmlElement))
					continue;

				switch (element.Name.ToLower()) {
					case "id":
						ID = element.InnerText;
						break;
					case "game":
						int gameid = int.Parse(element.InnerText);
						Game = (Game)gameid;
						break;
					case "name":
						Name = element.InnerText;
						break;
					case "artist":
						Artist = element.InnerText;
						break;
					case "album":
						Album = element.InnerText;
						break;
					case "albumart":
						string path = element.InnerText;
						if (!Path.IsPathRooted(path))
							path = Path.Combine(rootpath, path);
						try {
							Bitmap bitmap = new Bitmap(path);
							if (bitmap != null)
								AlbumArt = bitmap;
						} catch { }
						break;
					case "track":
						AlbumTrack = int.Parse(element.InnerText);
						break;
					case "year":
						Year = int.Parse(element.InnerText);
						break;
					case "master":
						Master = string.Compare(element.InnerText, "true", true) == 0 ? true : false;
						break;
					case "genre":
						Genre = element.InnerText;
						break;
					case "pack":
						Pack = element.InnerText;
						break;
					case "vocalist":
						Vocalist = element.InnerText;
						break;
					case "version":
						Version = int.Parse(element.InnerText);
						break;
				}
			}

			foreach (XmlElement sub in node.GetElementsByTagName("difficulty")) {
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
				Difficulty[instrument] = rank;
			}

			// Path = album art?
		}

		public static string SongNameToID(string name)
		{
			StringBuilder id = new StringBuilder();
			for (int i = 0; i < name.Length; i++)
				if (char.IsLetterOrDigit(name[i]))
					id.Append(name[i]);
			return id.ToString().ToLower();
		}
	}
}
