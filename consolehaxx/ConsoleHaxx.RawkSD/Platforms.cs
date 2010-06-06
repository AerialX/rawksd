using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using ConsoleHaxx.Common;
using ConsoleHaxx.Wii;
using ConsoleHaxx.Harmonix;
using System.Text.RegularExpressions;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Threading;

namespace ConsoleHaxx.RawkSD
{
	public abstract class Engine
	{
		public abstract int ID { get; }
		public abstract string Name { get; }

		public abstract PlatformData Create(string path, Game game, ProgressIndicator progress);
		public abstract bool AddSong(PlatformData data, SongData song, ProgressIndicator progress);

		public virtual FormatData CreateSong(PlatformData data, SongData song) { throw new NotImplementedException(); }
		public virtual void SaveSong(PlatformData data, FormatData formatdata, ProgressIndicator progress) { throw new NotImplementedException(); }

		public virtual void DeleteSong(PlatformData data, FormatData formatdata, ProgressIndicator progress) { formatdata.Dispose(); data.RemoveSong(formatdata); }

		public override string ToString()
		{
			return Name;
		}
	}

	public enum FormatType
	{
		Unknown =	0x00,
		Audio =		0x01,
		Chart =		0x02,
		Metadata =	0x03
	}

	public interface IFormat
	{
		int ID { get; }
		FormatType Type { get; }
		string Name { get; }

		bool Writable { get; }
		bool Readable { get; }

		object Decode(FormatData data, ProgressIndicator progress);
		void Encode(object data, FormatData destination, ProgressIndicator progress);

		bool CanRemux(IFormat format);
		void Remux(IFormat format, FormatData data, FormatData destination, ProgressIndicator progress);

		bool CanTransfer(FormatData data);

		bool HasFormat(FormatData format);
	}

	public abstract class IAudioFormat : IFormat
	{
		public abstract int ID { get; }
		public abstract string Name { get; }

		public FormatType Type { get { return FormatType.Audio; } }

		public virtual object Decode(FormatData data, ProgressIndicator progress)
		{
			return DecodeAudio(data, progress);
		}

		public virtual void Encode(object data, FormatData destination, ProgressIndicator progress)
		{
			if (data is AudioFormat)
				EncodeAudio((AudioFormat)data, destination, progress);
			else
				throw new FormatException();
		}

		public abstract AudioFormat DecodeAudio(FormatData data, ProgressIndicator progress);
		public virtual AudioFormat DecodeAudioFormat(FormatData data) { AudioFormat format = DecodeAudio(data, new ProgressIndicator()); format.Decoder.Dispose(); return format; }

		public abstract void EncodeAudio(AudioFormat data, FormatData destination, ProgressIndicator progress);

		public abstract bool Writable { get; }
		public abstract bool Readable { get; }

		public virtual bool CanRemux(IFormat format) { return false; }

		public virtual void Remux(IFormat format, FormatData data, FormatData destination, ProgressIndicator progress) { throw new NotSupportedException(); }

		public virtual bool CanTransfer(FormatData data) { return true; }

		public virtual bool HasFormat(FormatData data) { return data.Formats.Contains(this); }

		public override string ToString()
		{
			return Name;
		}
	}

	public abstract class IChartFormat : IFormat
	{
		public abstract int ID { get; }
		public abstract string Name { get; }

		public FormatType Type { get { return FormatType.Chart; } }

		public virtual object Decode(FormatData data, ProgressIndicator progress)
		{
			return DecodeChart(data, progress);
		}

		public virtual void Encode(object data, FormatData destination, ProgressIndicator progress)
		{
			if (data is ChartFormat)
				EncodeChart((ChartFormat)data, destination, progress);
			else
				throw new FormatException();
		}

		public abstract ChartFormat DecodeChart(FormatData data, ProgressIndicator progress);

		public abstract void EncodeChart(ChartFormat data, FormatData destination, ProgressIndicator progress);

		public abstract bool Writable { get; }
		public abstract bool Readable { get; }

		public virtual bool CanRemux(IFormat format) { return false; }
		public virtual void Remux(IFormat format, FormatData data, FormatData destination, ProgressIndicator progress) { throw new NotSupportedException(); }

		public virtual bool CanTransfer(FormatData data) { return true; }

		public virtual bool HasFormat(FormatData data) { return data.Formats.Contains(this); }

		public override string ToString()
		{
			return Name;
		}
	}

	public class PlatformData : IDisposable
	{
		public Engine Platform;
		public Game Game;
		public DelayedStreamCache Cache;

		public Dictionary<string, object> Session;

		protected List<FormatData> Data;

		public Mutex Mutex;

		public PlatformData(Engine platform, Game game)
		{
			Cache = new DelayedStreamCache();
			Session = new Dictionary<string, object>();
			Data = new List<FormatData>();
			Mutex = new Mutex();

			Platform = platform;
			Game = game;
		}

		public IList<FormatData> Songs { get { return Data; } }

		public void AddSong(FormatData data)
		{
			Mutex.WaitOne();

			Data.Add(data);

			Mutex.ReleaseMutex();
		}

		public void RemoveSong(FormatData data)
		{
			Mutex.WaitOne();

			Data.Remove(data);

			Mutex.ReleaseMutex();
		}

		public void Dispose()
		{
			if (Cache != null) {
				Cache.Dispose();
				Cache = null;
			}
		}

		~PlatformData()
		{
			Dispose();
		}
	}

	public static class Platform
	{
		public static List<IFormat> Formats;

		public static IFormat GetFormat(int id) { return Formats.SingleOrDefault(a => a.ID == id); }
		public static void AddFormat(IFormat format) { Formats.Add(format); }

		public static void Initialise()
		{
			Formats = new List<IFormat>();

			PlatformDJHWiiDisc.Initialise();
			PlatformFretsOnFire.Initialise();
			PlatformGH2PS2Disc.Initialise();
			PlatformGH3WiiDisc.Initialise();
			PlatformGH4WiiDLC.Initialise();
			PlatformGH5WiiDisc.Initialise();
			PlatformGH5WiiDLC.Initialise();
			PlatformLocalStorage.Initialise();
			PlatformRawkFile.Initialise();
			PlatformRB2360Disc.Initialise();
			PlatformRB2360DLC.Initialise();
			PlatformRB2360RBN.Initialise();
			PlatformRB2WiiCustomDLC.Initialise();
			PlatformRB2WiiDisc.Initialise();
			PlatformRB2WiiDLC.Initialise();

			ChartFormatChart.Initialise();
			ChartFormatFsgMub.Initialise();
			ChartFormatGH1.Initialise();
			ChartFormatGH2.Initialise();
			ChartFormatGH3.Initialise();
			ChartFormatGH4.Initialise();
			ChartFormatGH5.Initialise();
			ChartFormatRB.Initialise();

			AudioFormatBeatlesBink.Initialise();
			AudioFormatBink.Initialise();
			AudioFormatFFmpeg.Initialise();
			AudioFormatGH3WiiFSB.Initialise();
			AudioFormatMogg.Initialise();
			AudioFormatMp3.Initialise();
			AudioFormatOgg.Initialise();
			AudioFormatRB2Bink.Initialise();
			AudioFormatRB2Mogg.Initialise();
			AudioFormatVGS.Initialise();
		}

		public static Instrument InstrumentFromString(string s)
		{
			switch (s.ToLower()) {
				case "guitar":
					return Instrument.Guitar;
				case "bass":
				case "rhythm":
					return Instrument.Bass;
				case "drums":
				case "drum":
					return Instrument.Drums;
				case "vocals":
				case "vocal":
				case "vox":
					return Instrument.Vocals;
				case "preview":
					return Instrument.Preview;
			}

			return Instrument.Ambient;
		}

		private class minicostformatitem {
			public List<IFormat> Formats;
			public IFormat Format { get { return Formats.Last(); } }
			public int Cost;
			public minicostformatitem(IList<IFormat> formats, IFormat format, int cost)
			{
				Formats = new List<IFormat>();
				Formats.AddRange(formats);
				Formats.Add(format);
				Cost = cost;
			}
		}
		public static IList<IFormat> FindTranscodePath(IList<IFormat> roots, IList<IFormat> targets)
		{
			Queue<minicostformatitem> queue = new Queue<minicostformatitem>();
			IList<minicostformatitem> list = new List<minicostformatitem>();

			foreach (IFormat root in roots) {
				queue.Enqueue(new minicostformatitem(new IFormat[0], root, 0));

				while (queue.Count > 0) {
					var formatitem = queue.Dequeue();
					if (targets.Contains(formatitem.Format)) {
						list.Add(formatitem);
						continue;
					}

					foreach (IFormat child in Platform.Formats) {
						if (queue.Count(i => i.Formats.Contains(child)) == 0) {
							if (child.CanRemux(formatitem.Format))
								queue.Enqueue(new minicostformatitem(formatitem.Formats, child, formatitem.Cost + 1));
							else if (child.Type == root.Type && formatitem.Format.Readable && child.Writable)
								queue.Enqueue(new minicostformatitem(formatitem.Formats, child, formatitem.Cost + 1000));
						}
					}
				}
			}

			list = list.OrderBy(i => i.Cost).ToList();

			if (list.Count == 0)
				return new List<IFormat>();

			return list.First().Formats;
		}

		public static void Transcode(FormatType type, FormatData data, IList<IFormat> targets, FormatData destination, ProgressIndicator progress)
		{
			foreach (IFormat format in targets) {
				if (format.HasFormat(data)) {
					Transfer(format, data, destination, progress);
					return;
				}
			}

			IList<IFormat> formats = FindTranscodePath(data.GetFormats(type), targets);

			if (formats.Count == 0)
				throw new NotSupportedException();

			if (formats.Count == 1) {
				if (formats[0].CanTransfer(data)) {
					Transfer(formats[0], data, destination, progress);
					return;
				} else
					formats.Add(formats[0]);
			}

			FormatData tempdata = null;

			if (formats.Count > 2)
				tempdata = new TemporaryFormatData();

			progress.NewTask(formats.Count - 1);

			for (int i = 1; i < formats.Count; i++) {
				FormatData dest = i < formats.Count - 1 ? tempdata : destination;
				Transcode(formats[i - 1], data, formats[i], dest, progress);
				data = dest;
				progress.Progress();
			}

			progress.EndTask();
		}

		public static void Transcode(IFormat format, FormatData data, IFormat target, FormatData destination, ProgressIndicator progress)
		{
			if (target.CanRemux(format)) {
				target.Remux(format, data, destination, progress);
			} else if (target.Writable && format.Readable)
				TranscodeOnly(format, data, target, destination, progress);
			else
				throw new NotSupportedException();
		}

		public static void TranscodeOnly(IFormat format, FormatData data, IFormat target, FormatData destination, ProgressIndicator progress)
		{
			object decode = format.Decode(data, progress);
			target.Encode(decode, destination, progress);
			if (decode is IDisposable)
				(decode as IDisposable).Dispose();
		}

		public static void Transfer(IFormat format, FormatData data, FormatData destination, ProgressIndicator progress)
		{
			string[] streams = data.GetStreamNames();
			progress.NewTask(streams.Length);
			foreach (string fullname in streams) {
				if (fullname.StartsWith(format.ID + ".")) {
					string name = fullname.Split(new char[] { '.' }, 2)[1];

					Stream ostream = destination.AddStream(format, name);
					Stream istream = data.GetStream(format, name);

					Util.StreamCopy(ostream, istream);

					destination.CloseStream(ostream);
					data.CloseStream(istream);

					progress.Progress();
				}
			}

			progress.EndTask();
		}

		public static void Transfer(FormatData data, FormatData destination, ProgressIndicator progress)
		{
			foreach (IFormat format in data.Formats)
				Transfer(format, data, destination, progress);
		}

		public static DirectoryNode GetDirectoryStructure(this PlatformData platform, string path)
		{
			platform.Session["path"] = path;

			if (Directory.Exists(path)) {
				DirectoryNode root = DirectoryNode.FromPath(path, platform.Cache, FileAccess.Read, FileShare.Read);
				platform.Session["rootdirnode"] = root;
				return root;
			} else if (File.Exists(path)) {
				Stream stream = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.Read);
				platform.Cache.AddStream(stream);

				try {
					stream.Position = 0;
					Iso9660 iso = new Iso9660(stream);
					platform.Session["iso"] = iso;
					platform.Session["rootdirnode"] = iso.Root;
					return iso.Root;
				} catch (FormatException) { }

				try {
					stream.Position = 0;
					Disc disc = new Disc(stream);

					platform.Session["wiidisc"] = disc;

					Partition partition = disc.DataPartition;
					if (partition == null)
						throw new FormatException();

					platform.Session["rootdirnode"] = partition.Root.Root;
					return partition.Root.Root;
				} catch (FormatException) { }

				try {
					stream.Position = 0;
					U8 u8 = new U8(stream);
					platform.Session["u8"] = u8;
					platform.Session["rootdirnode"] = u8.Root;
					return u8.Root;
				} catch (FormatException) { }
			}

			throw new NotSupportedException();
		}

		public static Game DetermineGame(PlatformData data)
		{
			Game game = Game.Unknown;

			if (data.Session.ContainsKey("rootdirnode")) {
				DirectoryNode root = data.Session["rootdirnode"] as DirectoryNode;
				game = DetermineGame(root);
			}
			if (data.Session.ContainsKey("wiidisc")) {
				Disc disc = data.Session["wiidisc"] as Disc;
				switch (disc.Console + disc.Gamecode) {
					case "SXA":
						game = Game.GuitarHeroWorldTour;
						break;
					case "SXB":
						game = Game.GuitarHeroMetallica;
						break;
					case "SXC":
						game = Game.GuitarHeroSmashHits;
						break;
					case "SXD":
						game = Game.GuitarHeroVanHalen;
						break;
					case "SXE":
						game = Game.GuitarHero5;
						break;
					case "SXF":
						game = Game.BandHero;
						break;
					case "RKX":
						game = Game.RockBand;
						break;
					case "R33":
						game = Game.RockBandACDC;
						break;
					case "RRE":
						game = Game.RockBandTP1;
						break;
					case "RRD":
						game = Game.RockBandTP2;
						break;
					case "R3Z":
						game = Game.RockBandClassicTP;
						break;
					case "R34":
						game = Game.RockBandCountryTP;
						break;
					case "R37":
						game = Game.RockBandMetalTP;
						break;
					case "SZA":
						game = Game.RockBand2;
						break;
					case "R6L":
						game = Game.LegoRockBand;
						break;
					case "R9J":
						game = Game.RockBandBeatles;
						break;
					case "RGH":
						game = Game.GuitarHero3;
						break;
					case "RGV":
						game = Game.GuitarHeroAerosmith;
						break;
				}
			}

			if (game == Game.Unknown)
				return data.Game;
			return game;
		}

		public static Game DetermineGame(DirectoryNode root)
		{
			if (root.Find("SLES_541.32") != null || root.Find("SLUS_212.24") != null)
				return Game.GuitarHero1;
			else if (root.Find("SLES_544.42") != null || root.Find("SLUS_214.47") != null)
				return Game.GuitarHero2;
			else if (root.Find("SLES_548.59") != null || root.Find("SLUS_215.86") != null || root.Find("SLES_548.60") != null)
				return Game.GuitarHero80s;

			FileNode rawksdnode = root.Find("disc_id") as FileNode;
			if (rawksdnode != null) {
				StreamReader reader = new StreamReader(rawksdnode.Data);
				string name = reader.ReadLine();
				rawksdnode.Data.Close();
				switch (name) {
					case "RB1":
						return Game.RockBand;
					case "RB_TP1":
						return Game.RockBandTP1;
					case "RB_TP2":
						return Game.RockBandTP2;
					case "RB_ACDC":
						return Game.RockBandACDC;
					case "RB_CLASSIC":
						return Game.RockBandClassicTP;
					case "RB_COUNTRY":
						return Game.RockBandCountryTP;
					case "RB_METAL":
						return Game.RockBandMetalTP;
					case "LRB":
						return Game.LegoRockBand;
					case "RB2":
						return Game.RockBand2;
					case "TBRB":
						return Game.RockBandBeatles;
					case "GDRB":
						return Game.RockBandGreenDay;
					case "RB3":
						return Game.RockBand3;

					case "GH":
					case "GH1":
						return Game.GuitarHero1;
					case "GH2":
						return Game.GuitarHero2;
					case "GH80s":
						return Game.GuitarHero80s;
					case "GH3":
						return Game.GuitarHero3;
					case "GHA":
						return Game.GuitarHeroAerosmith;
					case "GHWT":
						return Game.GuitarHeroWorldTour;
					case "GHM":
						return Game.GuitarHeroMetallica;
					case "GHSH":
						return Game.GuitarHeroSmashHits;
					case "GHVH":
						return Game.GuitarHeroVanHalen;
					case "GH5":
						return Game.GuitarHero5;
					case "GH6":
						return Game.GuitarHero6;
					case "BH":
						return Game.BandHero;
				}
			}

			return Game.Unknown;
		}

		public static string GameName(Game game)
		{
			switch (game) {
				case Game.GuitarHero1:
					return "Guitar Hero";
				case Game.GuitarHero2:
					return "Guitar Hero 2";
				case Game.GuitarHero80s:
					return "Guitar Hero Encore: Rocks the 80s";
				case Game.GuitarHero3:
					return "Guitar Hero 3: Legends of Rock";
				case Game.GuitarHeroAerosmith:
					return "Guitar Hero: Aerosmith";
				case Game.GuitarHeroWorldTour:
					return "Guitar Hero World Tour";
				case Game.GuitarHeroMetallica:
					return "Guitar Hero: Metallica";
				case Game.GuitarHeroSmashHits:
					return "Guitar Hero Smash Hits";
				case Game.GuitarHeroVanHalen:
					return "Guitar Hero: Van Halen";
				case Game.GuitarHero5:
					return "Guitar Hero 5";
				case Game.BandHero:
					return "Band Hero";
				case Game.GuitarHero6:
					return "Guitar Hero 6";
				case Game.DjHero:
					return "DJ Hero";
				case Game.RockBand:
					return "Rock Band";
				case Game.RockBandACDC:
					return "AC/DC Live: Rock Band Track Pack";
				case Game.RockBandTP1:
					return "Rock Band: Track Pack Volume 1";
				case Game.RockBandTP2:
					return "Rock Band: Track Pack Volume 2";
				case Game.RockBandCountryTP:
					return "Rock Band: Country Track Pack";
				case Game.RockBandClassicTP:
					return "Rock Band Track Pack: Classic Rock";
				case Game.RockBandMetalTP:
					return "Rock Band: Metal Track Pack";
				case Game.RockBand2:
					return "Rock Band 2";
				case Game.RockBandBeatles:
					return "The Beatles: Rock Band";
				case Game.LegoRockBand:
					return "Lego Rock Band";
				case Game.RockBandGreenDay:
					return "Green Day: Rock Band";
				case Game.RockBand3:
					return "Rock Band 3";
			}

			return null;
		}

		public static IList<Game> GetGames()
		{
			return new List<Game>() {
				Game.GuitarHero1, Game.GuitarHero2, Game.GuitarHero80s,
				Game.GuitarHero3, Game.GuitarHeroAerosmith,
				Game.GuitarHeroWorldTour, Game.GuitarHeroMetallica, Game.GuitarHeroSmashHits, Game.GuitarHeroVanHalen,
				Game.GuitarHero5, Game.BandHero,
				Game.GuitarHero6,
				Game.DjHero,
				Game.RockBand, Game.RockBandACDC, Game.RockBandTP1, Game.RockBandTP2, Game.RockBandCountryTP, Game.RockBandClassicTP, Game.RockBandMetalTP, 
				Game.RockBand2, Game.RockBandBeatles, Game.LegoRockBand, Game.RockBandGreenDay, 
				Game.RockBand3
			};
		}
	}

	public enum Game
	{
		Unknown = 0x00,
		GuitarHero1 = 0x01,
		GuitarHero2 = 0x02,
		GuitarHero80s = 0x03,

		GuitarHero3 = 0x10,
		GuitarHeroAerosmith = 0x11,

		GuitarHeroWorldTour = 0x20,
		GuitarHeroMetallica = 0x21,
		GuitarHeroSmashHits = 0x22,
		GuitarHeroVanHalen = 0x23,

		GuitarHero5 = 0x30,
		BandHero = 0x31,

		GuitarHero6 = 0x40,

		DjHero = 0x70,

		RockBand = 0x80,
		RockBandACDC = 0x81,
		RockBandTP1 = 0x82,
		RockBandTP2 = 0x83,
		RockBandCountryTP = 0x84,
		RockBandClassicTP = 0x85,
		RockBandMetalTP = 0x86,

		RockBand2 = 0x90,
		RockBandBeatles = 0x91,
		LegoRockBand = 0x92,
		RockBandGreenDay = 0x93,

		RockBand3 = 0xA0,
	}
}
