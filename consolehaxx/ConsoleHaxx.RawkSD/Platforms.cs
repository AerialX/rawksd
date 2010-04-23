using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using ConsoleHaxx.Common;
using ConsoleHaxx.Wii;
using ConsoleHaxx.Harmonix;
using System.Text.RegularExpressions;

namespace ConsoleHaxx.RawkSD
{
	[Serializable]
	public class CancelledException : Exception
	{
		public CancelledException() { }
		public CancelledException(string message) : base(message) { }
		public CancelledException(string message, Exception inner) : base(message, inner) { }
	}

	public abstract class ProgressIndicator
	{
		public abstract void SetTasks(int tasks);
		public abstract void NewTask(string name, int max);
		public abstract void Progress(int increment);
		public abstract bool Warning(string message);
		public abstract void Error(string message);

		public void Progress() { Progress(1); }
	}

	public abstract class Engine
	{
		public abstract int ID { get; }
		public abstract string Name { get; }

		public abstract bool IsValid(string path);

		public abstract PlatformData Create(string path, Game game);
		public abstract bool AddSong(PlatformData data, SongData song);

		public virtual FormatData CreateSong(PlatformData data, SongData song) { throw new NotImplementedException(); }
		public virtual void SaveSong(PlatformData data, FormatData formatdata) { throw new NotImplementedException(); }
		public virtual bool Writable { get { return false; } }

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

		object Decode(FormatData data);
		void Encode(object data, FormatData destination, ProgressIndicator progress);

		bool CanRemux(IFormat format);
		void Remux(IFormat format, FormatData data, FormatData destination, ProgressIndicator progress);
	}

	public abstract class IAudioFormat : IFormat
	{
		public abstract int ID { get; }
		public abstract string Name { get; }

		public FormatType Type { get { return FormatType.Audio; } }

		public virtual object Decode(FormatData data)
		{
			return DecodeAudio(data);
		}

		public virtual void Encode(object data, FormatData destination, ProgressIndicator progress)
		{
			if (data is AudioFormat)
				EncodeAudio((AudioFormat)data, destination, progress);
			else
				throw new FormatException();
		}

		public abstract AudioFormat DecodeAudio(FormatData data);

		public abstract void EncodeAudio(AudioFormat data, FormatData destination, ProgressIndicator progress);

		public abstract bool Writable { get; }
		public abstract bool Readable { get; }

		public virtual bool CanRemux(IFormat format) { return false; }
		public virtual void Remux(IFormat format, FormatData data, FormatData destination, ProgressIndicator progress) { throw new NotSupportedException(); }
	}

	// At the moment, charts are only "remuxable" because there is no generic chart format
	public abstract class IChartFormat : IFormat
	{
		public abstract int ID { get; }
		public abstract string Name { get; }

		public FormatType Type { get { return FormatType.Chart; } }

		public virtual object Decode(FormatData data)
		{
			return DecodeChart(data);
		}

		public virtual void Encode(object data, FormatData destination, ProgressIndicator progress)
		{
			if (data is ChartFormat)
				EncodeChart((ChartFormat)data, destination, progress);
			else
				throw new FormatException();
		}

		public abstract ChartFormat DecodeChart(FormatData data);

		public abstract void EncodeChart(ChartFormat data, FormatData destination, ProgressIndicator progress);

		public abstract bool Writable { get; }
		public abstract bool Readable { get; }

		public virtual bool CanRemux(IFormat format) { return false; }
		public virtual void Remux(IFormat format, FormatData data, FormatData destination, ProgressIndicator progress) { throw new NotSupportedException(); }
	}

	public class PlatformData : IDisposable
	{
		public Engine Platform;
		public Game Game;
		public DelayedStreamCache Cache;

		public Dictionary<string, object> Session;

		protected List<FormatData> Data;

		public PlatformData(Engine platform, Game game)
		{
			Cache = new DelayedStreamCache();
			Session = new Dictionary<string, object>();
			Data = new List<FormatData>();

			Platform = platform;
			Game = game;
		}

		public IList<FormatData> Songs { get { return Data; } }

		public void AddSong(FormatData data)
		{
			Data.Add(data);
		}

		public void Dispose()
		{
			if (Cache != null)
				Cache.Dispose();
		}

		~PlatformData()
		{
			Dispose();
		}
	}

	public static class Platform
	{
		public static List<Engine> Platforms;
		public static List<IFormat> Formats;

		public static Engine GetPlatform(int id) { return Platforms.SingleOrDefault(p => p.ID == id); }
		public static IFormat GetFormat(int id) { return Formats.SingleOrDefault(a => a.ID == id); }

		static Platform()
		{
			Platforms = new List<Engine>();
			Formats = new List<IFormat>();
			
			Formats.Add(AudioFormatBeatlesBink.Instance);
			Formats.Add(AudioFormatBink.Instance);
			Formats.Add(AudioFormatFFmpeg.Instance);
			Formats.Add(AudioFormatGH3WiiFSB.Instance);
			Formats.Add(AudioFormatMogg.Instance);
			Formats.Add(AudioFormatOgg.Instance);
			Formats.Add(AudioFormatRB2Bink.Instance);
			Formats.Add(AudioFormatVGS.Instance);

			Formats.Add(ChartFormatChart.Instance);
			Formats.Add(ChartFormatGH1.Instance);
			Formats.Add(ChartFormatGH2.Instance);
			Formats.Add(ChartFormatGH3.Instance);
			Formats.Add(ChartFormatGH4.Instance);
			Formats.Add(ChartFormatGH5.Instance);
			Formats.Add(ChartFormatRB.Instance);

			Platforms.Add(PlatformLocalStorage.Instance);
			Platforms.Add(PlatformRB2WiiDisc.Instance);
			Platforms.Add(PlatformRB2WiiDLC.Instance);
			Platforms.Add(PlatformGH1PS2Disc.Instance);
			Platforms.Add(PlatformGH2PS2Disc.Instance);
			Platforms.Add(PlatformGH3WiiDisc.Instance);
			Platforms.Add(PlatformGH4WiiDisc.Instance);
			Platforms.Add(PlatformGH5WiiDisc.Instance);
			Platforms.Add(PlatformGH4WiiDLC.Instance);
			Platforms.Add(PlatformGH5WiiDLC.Instance);
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
			IList<IFormat> formats = FindTranscodePath(data.GetFormats(type), targets);

			if (formats.Count == 0)
				throw new NotSupportedException();

			if (formats.Count == 1) {
				Transfer(formats[0], data, destination, progress);
				return;
			}

			FormatData tempdata = null;

			if (formats.Count > 2)
				tempdata = new TemporaryFormatData();

			for (int i = 1; i < formats.Count; i++) {
				Transcode(formats[i - 1], i > 1 ? tempdata : data, formats[i], i < formats.Count - 1 ? tempdata : destination, progress);
				data = destination;
			}
		}

		public static void Transcode(IFormat format, FormatData data, IFormat target, FormatData destination, ProgressIndicator progress)
		{
			if (target.CanRemux(format)) {
				target.Remux(format, data, destination, progress);
			} else if (target.Writable && format.Readable)
				target.Encode(format.Decode(data), destination, progress);
			else
				throw new NotSupportedException();
		}

		public static void Transfer(IFormat format, FormatData data, FormatData destination, ProgressIndicator progress)
		{
			foreach (string fullname in data.GetStreams()) {
				if (fullname.StartsWith(format.ID + ".")) {
					string name = fullname.Split(new char[] { '.' }, 2)[1];

					Stream ostream = destination.AddStream(format, name);
					Stream istream = data.GetStream(format, name);

					Util.StreamCopy(ostream, istream);

					destination.CloseStream(ostream);
					data.CloseStream(istream);
				}
			}
		}

		public static void Transfer(FormatData data, FormatData destination, ProgressIndicator progress)
		{
			foreach (IFormat format in data.Formats)
				Transfer(format, data, destination, progress);
		}

		public static Ark GetHarmonixArk(DirectoryNode dir)
		{
			DirectoryNode gen = dir.Navigate("gen") as DirectoryNode;
			if (gen == null) // Just in case we're given the "wrong" directory that directly contains the ark
				gen = dir;

			List<Pair<int, Stream>> arkfiles = new List<Pair<int, Stream>>();
			Stream hdrfile = null;
			foreach (FileNode file in gen.Children.Where(n => n is FileNode)) {
				if (file.Name.EndsWith(".hdr"))
					hdrfile = file.Data;
				else if (file.Name.EndsWith(".ark")) {
					Match match = Regex.Match(file.Name, @"_(\d+).ark");
					if (match.Success)
						arkfiles.Add(new Pair<int, Stream>(int.Parse(match.Groups[1].Value), file.Data));
					else
						arkfiles.Add(new Pair<int, Stream>(0, file.Data));
				}
			}

			// FreQuency/Amplitude where the header is the ark
			if (hdrfile == null) {
				if (arkfiles.Count == 1) {
					hdrfile = arkfiles[0].Value;
					arkfiles.Clear();
				} else
					throw new FormatException();
			}

			return new Ark(new EndianReader(hdrfile, Endianness.LittleEndian), arkfiles.OrderBy(f => f.Key).Select(f => f.Value).ToArray());
		}

		public static DirectoryNode GetWiiDirectoryStructure(this PlatformData platform, string path)
		{
			if (File.Exists(path)) {
				Stream stream = new FileStream(path, FileMode.Open, FileAccess.Read);

				platform.Cache.AddStream(stream);

				Disc disc = new Disc(stream);

				platform.Session["wiidisc"] = disc;

				Partition partition = disc.Partitions.Find(p => p.Type == PartitionType.Data);
				if (partition == null)
					return null;
				U8 u8 = partition.Root;
				if (u8 == null)
					return null;
				return u8.Root;
			} else if (Directory.Exists(path))
				return DirectoryNode.FromPath(path, platform.Cache, FileAccess.Read);
			else
				throw new NotSupportedException();
		}

		public static DirectoryNode GetPS2DirectoryStructure(this PlatformData data, string path)
		{
			if (File.Exists(path))
				throw new NotImplementedException(); // ISO
			else if (Directory.Exists(path))
				return DirectoryNode.FromPath(path, data.Cache, FileAccess.Read);
			else
				throw new NotSupportedException();
		}

		public static IFormat GetPreferredFormat(IFormat[] formats, FormatType type)
		{
			// TODO: Have some sort of priority system set up
			foreach (IFormat format in formats)
				if (format.Type == type)
					return format;

			return null;
		}

		public static IFormat GetPreferredFormat(IFormat[] formats, FormatType type, object priorities)
		{
			throw new NotImplementedException();
		}

		public static Game DetermineGame(PlatformData data)
		{
			if (data.Session.ContainsKey("wiidisc")) {
				Disc disc = data.Session["wiidisc"] as Disc;
				Games game;
				switch (disc.Console + disc.Gamecode) {
					case "SXA":
						game = Games.GuitarHeroWorldTour;
						break;
					case "SXB":
						game = Games.GuitarHeroMetallica;
						break;
					case "SXC":
						game = Games.GuitarHeroSmashHits;
						break;
					case "SXD":
						game = Games.GuitarHeroVanHalen;
						break;
					case "SXE":
						game = Games.GuitarHero5;
						break;
					case "SXF":
						game = Games.BandHero;
						break;
					case "RKX":
						game = Games.RockBand;
						break;
					case "R33":
						game = Games.RockBandACDC;
						break;
					case "RRD":
						game = Games.RockBandTP2;
						break;
					case "SZA":
						game = Games.RockBand2;
						break;
					case "R6L":
						game = Games.LegoRockBand;
						break;
					case "R9J":
						game = Games.RockBandBeatles;
						break;
					case "RGH":
						game = Games.GuitarHero3;
						break;
					case "RGV":
						game = Games.GuitarHeroAerosmith;
						break;
					default:
						return data.Game;
				}

				return Game.GetGame(game);
			}

			return data.Game;
		}
	}
}
