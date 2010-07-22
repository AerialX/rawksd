using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using ConsoleHaxx.Common;
using System.IO;
using System.Drawing;
using ConsoleHaxx.Harmonix;
using System.Drawing.Imaging;

namespace ConsoleHaxx.RawkSD
{
	public class PlatformFretsOnFire : Engine
	{
		public static PlatformFretsOnFire Instance;
		public static void Initialise()
		{
			Instance = new PlatformFretsOnFire();

			PlatformDetection.AddSelection(Instance);
			PlatformDetection.DetectDirectoryNode += new Action<string, DirectoryNode, List<Pair<Engine, Game>>>(PlatformDetection_DetectDirectoryNode);
		}

		static void PlatformDetection_DetectDirectoryNode(string path, DirectoryNode root, List<Pair<Engine, Game>> platforms)
		{
			root = FindFofFolder(root);
			if (root.Find("song.ini", System.IO.SearchOption.AllDirectories, true) != null)
				platforms.Add(new Pair<Engine, Game>(Instance, Game.Unknown));
		}

		private static DirectoryNode FindFofFolder(DirectoryNode root)
		{
			DirectoryNode dir = root.Navigate("data", false, true) as DirectoryNode;
			if (dir != null)
				root = dir;
			dir = root.Navigate("songs", false, true) as DirectoryNode;
			if (dir != null)
				root = dir;
			return root;
		}

		public override int ID { get { throw new NotImplementedException(); } }

		public override string Name { get { return "Frets on Fire folder"; } }

		public override PlatformData Create(string path, Game game, ProgressIndicator progress)
		{
			PlatformData data = new PlatformData(this, game);
			DirectoryNode maindir = data.GetDirectoryStructure(path);

			List<DirectoryNode> dirs = new List<DirectoryNode>(maindir.Directories);
			dirs.Add(maindir);

			progress.NewTask(dirs.Count);
			foreach (DirectoryNode dir in dirs) {
				SongData song = new SongData(data);
				song.Name = dir.Name;
				try {
					data.Session["songdir"] = dir;
					AddSong(data, song, progress);
					data.Session.Remove("songdir");
				} catch (Exception exception) {
					Exceptions.Warning(exception, "Unable to parse the Frets on Fire song from " + song.Name);
				}
				progress.Progress();
			}
			progress.EndTask();

			return data;
		}

		public override bool AddSong(PlatformData data, SongData song, ProgressIndicator progress)
		{
			FormatData formatdata = new TemporaryFormatData(song, data);

			DirectoryNode dir = data.Session["songdir"] as DirectoryNode;

			int delay = 0;
			bool eighthhopo = false;
			int hopofreq = -1;
			AudioFormat format = new AudioFormat();
			FileNode songini = dir.Navigate("song.ini", false, true) as FileNode;
			if (songini != null) {
				Ini ini = Ini.Create(songini.Data);
				songini.Data.Close();
				string value = ini.GetValue("song", "name"); if (value != null) song.Name = value;
				value = ini.GetValue("song", "artist"); if (value != null) song.Artist = value;
				value = ini.GetValue("song", "album"); if (value != null) song.Album = value;
				value = ini.GetValue("song", "genre"); if (value != null) song.Genre = value;
				value = ini.GetValue("song", "year"); if (value != null) song.Year = int.Parse(value);
				value = ini.GetValue("song", "version"); if (value != null) song.Version = int.Parse(value);
				value = ini.GetValue("song", "delay"); if (value != null) delay = int.Parse(value);
				value = ini.GetValue("song", "eighthnote_hopo"); if (value != null) eighthhopo = string.Compare(value, "true", true) == 0 ? true : false;
				value = ini.GetValue("song", "hopofreq"); if (value != null) hopofreq = int.Parse(value);
				value = ini.GetValue("song", "tags"); if (value != null) song.Master = string.Compare(value, "cover", true) == 0 ? false : true;
				value = ini.GetValue("song", "icon"); if (value != null) song.Game = GetGameFromIcon(value);
				value = ini.GetValue("song", "diff_band"); if (value != null) song.Difficulty[Instrument.Ambient] = ImportMap.GetBaseRank(Instrument.Ambient, int.Parse(value));
				value = ini.GetValue("song", "diff_bass"); if (value != null) song.Difficulty[Instrument.Bass] = ImportMap.GetBaseRank(Instrument.Bass, int.Parse(value));
				value = ini.GetValue("song", "diff_drums"); if (value != null) song.Difficulty[Instrument.Drums] = ImportMap.GetBaseRank(Instrument.Drums, int.Parse(value));
				value = ini.GetValue("song", "diff_guitar"); if (value != null) song.Difficulty[Instrument.Guitar] = ImportMap.GetBaseRank(Instrument.Guitar, int.Parse(value));
			}

			format.InitialOffset = -delay;

			FileNode album = dir.Navigate("album.png", false, true) as FileNode;
			if (album != null)
				song.AlbumArt = new Bitmap(album.Data);

			FileNode chartfile = dir.Navigate("notes.mid", false, true) as FileNode;
			NoteChart chart = null;
			if (chartfile != null) {
				ChartFormatGH2.Instance.Create(formatdata, chartfile.Data, false); // TODO: Make a separate chart format for this; just don't use ChartFormatRB otherwise it won't fix for quickplay
				chart = NoteChart.Create(Midi.Create(Mid.Create(chartfile.Data)));
				chartfile.Data.Close();
			}

			if (chart != null && eighthhopo) {
				song.HopoThreshold = chart.Division.TicksPerBeat / 2 + 10;
			} else if (hopofreq >= 0) {
				// TODO: This
			}

			List<Stream> streams = new List<Stream>();

			foreach (Node node in dir) {
				FileNode file = node as FileNode;
				if (file == null)
					continue;
				string extension = Path.GetExtension(file.Name).ToLower();
				if (extension != ".ogg")
					continue;

				string name = Path.GetFileNameWithoutExtension(file.Name);
				Instrument instrument = Platform.InstrumentFromString(name);
				RawkAudio.Decoder decoder = new RawkAudio.Decoder(file.Data, RawkAudio.Decoder.AudioFormat.VorbisOgg);
				for (int i = 0; i < decoder.Channels; i++) {
					format.Mappings.Add(new AudioFormat.Mapping(0, 0, instrument));
				}
				decoder.Dispose();
				file.Data.Close();

				streams.Add(file.Data);
			}
			format.AutoBalance();

			if (streams.Count > 0)
				AudioFormatOgg.Instance.Create(formatdata, streams.ToArray(), format);
			else if (chartfile == null)
				return false;

			data.AddSong(formatdata);

			return true;
		}

		public override FormatData CreateSong(PlatformData data, SongData song)
		{
			TemporaryFormatData formatdata = new TemporaryFormatData(song, data);
			return formatdata;
		}

		public override void SaveSong(PlatformData data, FormatData formatdata, ProgressIndicator progress)
		{
			string path = data.Session["path"] as string;

			SongData song = formatdata.Song;

			progress.NewTask(8);

			int i;
			string songpath = null;
			for (i = 0; i < 0x1000; i++) {
				songpath = Path.Combine(path, song.ID + (i == 0 ? "" : i.ToString()));
				if (!Directory.Exists(songpath))
					break;
			}
			Directory.CreateDirectory(songpath);
			AudioFormat audio = (formatdata.GetFormat(FormatType.Audio) as IAudioFormat).DecodeAudio(formatdata, progress);
			progress.Progress();
			ChartFormat chart = (formatdata.GetFormat(FormatType.Chart) as IChartFormat).DecodeChart(formatdata, progress);
			progress.Progress();

			Stream chartstream = new FileStream(Path.Combine(songpath, "notes.mid"), FileMode.Create, FileAccess.Write);
			chart.Save(chartstream);
			chartstream.Close();

			Ini ini = new Ini();
			ini.SetValue("song", "name", song.Name);
			ini.SetValue("song", "artist", song.Artist);
			ini.SetValue("song", "album", song.Album);
			ini.SetValue("song", "genre", song.Genre);
			ini.SetValue("song", "year", song.Year.ToString());
			ini.SetValue("song", "diff_band", ImportMap.GetBaseTier(Instrument.Ambient, song.Difficulty[Instrument.Ambient]).ToString());
			ini.SetValue("song", "diff_guitar", ImportMap.GetBaseTier(Instrument.Guitar, song.Difficulty[Instrument.Guitar]).ToString());
			ini.SetValue("song", "diff_bass", ImportMap.GetBaseTier(Instrument.Bass, song.Difficulty[Instrument.Bass]).ToString());
			ini.SetValue("song", "diff_drums", ImportMap.GetBaseTier(Instrument.Drums, song.Difficulty[Instrument.Drums]).ToString());
			Stream inistream = new FileStream(Path.Combine(songpath, "song.ini"), FileMode.Create, FileAccess.Write);
			ini.Save(inistream);
			inistream.Close();

			if (song.AlbumArt != null) {
				Stream albumart = new FileStream(Path.Combine(songpath, "album.png"), FileMode.Create, FileAccess.Write);
				song.AlbumArt.Save(albumart, ImageFormat.Png);
				albumart.Close();
			}

			JaggedShortArray encoderdata;

			var instruments = audio.Mappings.GroupBy(m => m.Instrument == Instrument.Vocals ? Instrument.Ambient : m.Instrument);
			encoderdata = new JaggedShortArray(2, RawkAudio.Decoder.BufferSize);
			int count = instruments.Count();

			Stream[] streams = new Stream[count];
			IEncoder[] encoders = new IEncoder[count];
			ushort[][] masks = new ushort[count][];

			i = 0;
			foreach (var item in instruments) {
				string filename = null;
				switch (item.Key) {
					case Instrument.Guitar:
						filename = "guitar.ogg";
						break;
					case Instrument.Bass:
						filename = "rhythm.ogg";
						break;
					case Instrument.Drums:
						filename = "drums.ogg";
						break;
					case Instrument.Ambient:
						filename = "song.ogg";
						break;
					case Instrument.Preview:
						filename = "preview.ogg";
						break;
				}

				streams[i] = new FileStream(Path.Combine(songpath, filename), FileMode.Create, FileAccess.Write);
				masks[i] = new ushort[2];

				foreach (var map in item) {
					int index = audio.Mappings.IndexOf(map);
					if (map.Balance <= 0)
						masks[i][0] |= (ushort)(1 << index);
					if (map.Balance >= 0)
						masks[i][1] |= (ushort)(1 << index);
				}

				encoders[i] = new RawkAudio.Encoder(streams[i], 2, audio.Decoder.SampleRate, 44100);

				i++;
			}

			if (audio.InitialOffset > 0)
				AudioFormat.ProcessOffset(audio.Decoder, encoders[0], audio.InitialOffset);
			else {
				for (i = 0; i < encoders.Length; i++)
					AudioFormat.ProcessOffset(audio.Decoder, encoders[i], audio.InitialOffset);
			}

			long samples = audio.Decoder.Samples;
			progress.NewTask("Transcoding Audio", samples, 6);
			while (samples > 0) {
				int read = audio.Decoder.Read();
				if (read <= 0)
					break;

				// TODO: Apply volumes to each channel
				for (i = 0; i < count; i++) {
					audio.Decoder.AudioBuffer.DownmixTo(encoderdata, masks[i], read, false);
					encoders[i].Write(encoderdata, read);
				}
				
				samples -= read;
				progress.Progress(read);
			}

			for (i = 0; i < count; i++) {
				encoders[i].Dispose();
				streams[i].Close();
			}
			progress.EndTask();
			progress.Progress(6);
			progress.EndTask();
		}

		private static Game GetGameFromIcon(string value)
		{
			switch (value.ToLower()) {
				case "rb1": return Game.RockBand;
				case "rb2": return Game.RockBand2;
				case "rb3": return Game.RockBand3;
				case "rbdlc": return Game.RockBand2;
				case "rbtpk": case "rbtpk1": return Game.RockBandTP1;
				case "rbtpk2": return Game.RockBandTP2;
				case "gdrb": case "rbgd": return Game.RockBandGreenDay;
				case "tbrb": case "rbtb": return Game.RockBandBeatles;
				case "gh1": return Game.GuitarHero1;
				case "gh2": case "gh2dlc": return Game.GuitarHero2;
				case "gh80s": return Game.GuitarHero80s;
				case "gh3": case "gh3dlc": return Game.GuitarHero3;
				case "gha": return Game.GuitarHeroAerosmith;
				case "ghm": return Game.GuitarHeroMetallica;
				case "ghsh": return Game.GuitarHeroSmashHits;
				case "ghwt": case "gh4": return Game.GuitarHeroWorldTour;
				case "ghvh": return Game.GuitarHeroVanHalen;
				case "gh5": return Game.GuitarHero5;
				case "bh": return Game.BandHero;
			}

			return Game.Unknown;
		}
	}
}
