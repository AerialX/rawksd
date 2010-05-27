using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using ConsoleHaxx.Common;
using ConsoleHaxx.Harmonix;
using ConsoleHaxx.Wii;
using System.Security.Cryptography;

namespace ConsoleHaxx.RawkSD
{
	public class PlatformRB2WiiCustomDLC : Engine
	{
		public static PlatformRB2WiiCustomDLC Instance;
		public static void Initialise()
		{
			Instance = new PlatformRB2WiiCustomDLC();
		}

		public override int ID { get { throw new NotImplementedException(); } }

		public override string Name { get { return "RawkSD Wii DLC"; } }

		public override bool AddSong(PlatformData data, SongData song, ProgressIndicator progress)
		{
			return PlatformRB2WiiDisc.Instance.AddSong(data, song, progress);
		}

		public override void DeleteSong(PlatformData data, FormatData formatdata, ProgressIndicator progress)
		{
			string path = data.Session["maindirpath"] as string;
			DirectoryNode dir = data.Session["maindir"] as DirectoryNode;
			string dtapath = "rawk/rb2/customs/" + formatdata.Song.ID;
			FileNode dtafile = dir.Navigate(dtapath + "/data", false, true) as FileNode;
			if (dtafile == null)
				return;

			EndianReader reader = new EndianReader(dtafile.Data, Endianness.LittleEndian);
			DTB.NodeTree dtb = DTB.Create(reader);
			dtafile.Data.Close();

			SongsDTA dta = SongsDTA.Create(dtb);
			string title = dta.Song.Name.Substring(4, 4);
			int index = int.Parse(dta.Song.Name.Substring(9, 3));
			File.Delete(Path.Combine(path, "private/wii/data/" + title + "/" + Util.Pad((index).ToString(), 3) + ".bin"));
			File.Delete(Path.Combine(path, "private/wii/data/" + title + "/" + Util.Pad((index + 1).ToString(), 3) + ".bin"));
			Directory.Delete(Path.Combine(path, dtapath), true);

			base.DeleteSong(data, formatdata, progress);
		}

		public override PlatformData Create(string path, Game game, ProgressIndicator progress)
		{
			if (!Directory.Exists(path))
				Directory.CreateDirectory(path);

			PlatformData data = new PlatformData(this, game);

			DirectoryNode maindir = DirectoryNode.FromPath(path, data.Cache, FileAccess.Read, FileShare.Read);

			data.Session["maindir"] = maindir;
			data.Session["maindirpath"] = path;

			DirectoryNode customdir = maindir.Navigate("rawk/rb2/customs", false, true) as DirectoryNode;
			if (customdir == null)
				return data;

			foreach (Node node in customdir.Children) {
				DirectoryNode dir = node as DirectoryNode;
				if (dir == null)
					continue;

				FileNode dtafile = dir.Navigate("data", false, true) as FileNode;
				if (dtafile == null)
					continue;

				EndianReader reader = new EndianReader(dtafile.Data, Endianness.LittleEndian);
				DTB.NodeTree dtb = DTB.Create(reader);
				dtafile.Data.Close();

				SongData song = HarmonixMetadata.GetSongData(data, dtb);
				SongsDTA dta = HarmonixMetadata.GetSongsDTA(song);
				FileNode contentbin = maindir.Navigate("private/wii/data/" + dta.Song.Name.Substring(4, 4) + "/" + Util.Pad((int.Parse(dta.Song.Name.Substring(9, 3)) + 1).ToString(), 3) + ".bin", false, true) as FileNode;
				if (contentbin == null)
					continue;
				if (song.ID.StartsWith("rwk"))
					song.ID = song.ID.Substring(3);
				song.ID = song.ID.TrimStart('0', '1', '2', '3', '4', '5', '6', '7', '8', '9');
				DlcBin content = new DlcBin(contentbin.Data);
				U8 u8 = new U8(content.Data);
				//DirectoryNode songdir = u8.Root.Navigate("/content/songs/", false, true) as DirectoryNode;

				data.Session["songdir"] = u8.Root;
				AddSong(data, song, progress); // TODO: Add dta bin to songdir for album art
				data.Session.Remove("songdir");
				contentbin.Data.Close();
			}

			return data;
		}

		public override FormatData CreateSong(PlatformData data, SongData song)
		{
			TemporaryFormatData formatdata = new TemporaryFormatData(song, data);
			return formatdata;
		}

		public override void SaveSong(PlatformData data, FormatData formatdata, ProgressIndicator progress)
		{
			string path = data.Session["maindirpath"] as string;

			string otitle;
			ushort oindex;
			FindUnusedContent(data, formatdata.Song, out otitle, out oindex);
			TMD tmd = GenerateDummyTMD(otitle);

			progress.NewTask(10);

			using (DelayedStreamCache cache = new DelayedStreamCache()) {
				progress.SetNextWeight(6);
				string audioextension = ".mogg";
				Stream audio = null;
				Stream preview = null;
				AudioFormat audioformat = null;
				IList<IFormat> formats = formatdata.Formats;
				if (formats.Contains(AudioFormatRB2Mogg.Instance)) {
					audio = AudioFormatRB2Mogg.Instance.GetAudioStream(formatdata);
					preview = AudioFormatRB2Mogg.Instance.GetPreviewStream(formatdata);
					audioformat = AudioFormatRB2Mogg.Instance.DecodeAudioFormat(formatdata);
				} else if (formats.Contains(AudioFormatRB2Bink.Instance)) {
					audio = AudioFormatRB2Bink.Instance.GetAudioStream(formatdata);
					preview = AudioFormatRB2Bink.Instance.GetPreviewStream(formatdata, progress);
					audioformat = AudioFormatRB2Bink.Instance.DecodeAudioFormat(formatdata);
					audioextension = ".bik";
				} else
					throw new NotSupportedException();

				progress.Progress(6);

				Stream album = null;
				Stream chart = formatdata.GetStream(ChartFormatRB.Instance, ChartFormatRB.ChartFile);
				Stream pan = formatdata.GetStream(ChartFormatRB.Instance, ChartFormatRB.PanFile);
				Stream weights = formatdata.GetStream(ChartFormatRB.Instance, ChartFormatRB.WeightsFile);
				Stream milo = formatdata.GetStream(ChartFormatRB.Instance, ChartFormatRB.MiloFile);

				if (preview == null)
					preview = new MemoryStream(Properties.Resources.rawksd_preview);
				if (pan == null)
					pan = new MemoryStream(Properties.Resources.rawksd_pan);
				if (weights == null)
					weights = new MemoryStream(Properties.Resources.rawksd_weights);
				if (milo == null)
					milo = new MemoryStream(Properties.Resources.rawksd_milo);
				//if (album == null)
					//album = new MemoryStream(Properties.Resources.rawksd_albumart);

				progress.SetNextWeight(2);

				if (chart != null && ChartFormatRB.Instance.NeedsFixing(formatdata))
					chart = null;

				if (chart == null) {
					chart = new TemporaryStream();
					cache.AddStream(chart);
					AdjustChart(formatdata.Song, audioformat, (formatdata.GetFormat(FormatType.Chart) as IChartFormat).DecodeChart(formatdata, progress)).ToMidi().ToMid().Save(chart);
					chart.Position = 0;
				}

				progress.Progress(2);

				SongData song = formatdata.Song;

				SongsDTA dta = GetSongsDTA(song, audioformat);
				dta.BaseName = "rwk" + dta.Version.ToString() + dta.BaseName;
				dta.Genre = ImportMap.GetShortGenre(dta.Genre);

				dta.Song.Name = "dlc/" + otitle + "/" + Util.Pad(oindex.ToString(), 3) + "/content/songs/" + song.ID + "/" + song.ID;
				dta.Song.MidiFile = "dlc/content/songs/" + song.ID + "/" + song.ID + ".mid";

				if (album == null)
					dta.AlbumArt = false;
				else
					dta.AlbumArt = true;

				DTB.NodeTree dtb = dta.ToDTB();
				HarmonixMetadata.SetSongsDTA(song, dtb);

				MemoryStream songsdta = new MemoryStream();
				cache.AddStream(songsdta);
				dtb.SaveDTA(songsdta);
				songsdta.Position = 0;

				U8 appdta = new U8();
				DirectoryNode dir = new DirectoryNode("content", appdta.Root);
				dir = new DirectoryNode("songs", dir);
				dir = new DirectoryNode(song.ID, dir);
				new FileNode(song.ID + "_prev.mogg", dir, (ulong)preview.Length, preview);
				new FileNode("songs.dta", dir, (ulong)songsdta.Length, songsdta);
				if (dta.AlbumArt.Value && album != null) {
					dir = new DirectoryNode("gen", dir);
					new FileNode(song.ID + "_nomip_keep.bmp_wii", dir, (ulong)album.Length, album);
				}

				U8 appsong = new U8();
				dir = new DirectoryNode("content", appsong.Root);
				dir = new DirectoryNode("songs", dir);
				dir = new DirectoryNode(song.ID, dir);
				new FileNode(song.ID + audioextension, dir, (ulong)audio.Length, audio);
				new FileNode(song.ID + ".mid", dir, (ulong)chart.Length, chart);
				new FileNode(song.ID + ".pan", dir, (ulong)pan.Length, pan);
				dir = new DirectoryNode("gen", dir);
				new FileNode(song.ID + ".milo_wii", dir, (ulong)milo.Length, milo);
				new FileNode(song.ID + "_weights.bin", dir, (ulong)weights.Length, weights);

				TmdContent contentDta = new TmdContent();
				contentDta.ContentID = oindex;
				contentDta.Index = oindex;
				contentDta.Type = 0x4001;

				TmdContent contentSong = new TmdContent();
				contentSong.ContentID = oindex + 1U;
				contentSong.Index = (ushort)(oindex + 1);
				contentSong.Type = 0x4001;

				SHA1 sha1 = SHA1.Create();

				Stream memoryDta = new TemporaryStream();
				cache.AddStream(memoryDta);
				appdta.Save(memoryDta);
				memoryDta.Position = 0;
				contentDta.Hash = sha1.ComputeHash(memoryDta);
				contentDta.Size = memoryDta.Length;

				Stream memorySong = new TemporaryStream();
				cache.AddStream(memorySong);
				appsong.Save(memorySong);
				memorySong.Position = 0;
				contentSong.Hash = sha1.ComputeHash(memorySong);
				contentSong.Size = memorySong.Length;

				for (int i = 1; i <= oindex + 1; i++) {
					if (i == oindex)
						tmd.Contents.Add(contentDta);
					else if (i == oindex + 1)
						tmd.Contents.Add(contentSong);
					else
						tmd.Contents.Add(new TmdContent() { Index = (ushort)i, ContentID = (uint)i, Size = 1, Type = 0x4001 });
				}

				tmd.Fakesign();

				uint consoleid = GetConsoleID(path);

				progress.Progress();

				string dirpath = Path.Combine(Path.Combine(Path.Combine(Path.Combine(path, "private"), "wii"), "data"), "sZAE");
				FileStream binstream;
				DlcBin bin;
				if (consoleid != 0 && !File.Exists(Path.Combine(dirpath, "000.bin"))) {
					Directory.CreateDirectory(dirpath);
					binstream = new FileStream(Path.GetTempFileName(), FileMode.Create);
					binstream.Write(Properties.Resources.rawksd_000bin, 0, Properties.Resources.rawksd_000bin.Length);
					binstream.Position = 8;
					new EndianReader(binstream, Endianness.BigEndian).Write(consoleid);
					binstream.Close();
					File.Move(binstream.Name, Path.Combine(dirpath, "000.bin"));
				}

				dirpath = Path.Combine(Path.Combine(Path.Combine(Path.Combine(path, "private"), "wii"), "data"), otitle);
				if (!Directory.Exists(dirpath))
					Directory.CreateDirectory(dirpath);

				binstream = new FileStream(Path.GetTempFileName(), FileMode.Create);
				bin = new DlcBin();
				bin.Bk.ConsoleID = consoleid;
				bin.TMD = tmd;
				bin.Content = tmd.Contents[oindex];
				bin.Data = memoryDta;
				bin.Generate();
				bin.Bk.TitleID = 0x00010000535A4145UL;
				bin.Save(binstream);
				binstream.Close();
				string dtabinpath = Path.Combine(dirpath, Util.Pad(oindex.ToString(), 3) + ".bin");
				File.Delete(dtabinpath);
				File.Move(binstream.Name, dtabinpath);

				binstream = new FileStream(Path.GetTempFileName(), FileMode.Create);
				bin = new DlcBin();
				bin.Bk.ConsoleID = consoleid;
				bin.TMD = tmd;
				bin.Content = tmd.Contents[oindex + 1];
				bin.Data = memorySong;
				bin.Generate();
				bin.Bk.TitleID = 0x00010000535A4145UL;
				bin.Save(binstream);
				binstream.Close();
				string songbinpath = Path.Combine(dirpath, Util.Pad((oindex + 1).ToString(), 3) + ".bin");
				File.Delete(songbinpath);
				File.Move(binstream.Name, songbinpath);

				binstream = new FileStream(Path.GetTempFileName(), FileMode.Create);
				bin = new DlcBin();
				bin.Bk.ConsoleID = consoleid;
				bin.TMD = tmd;
				bin.Content = tmd.Contents[0];
				bin.Data = new MemoryStream(Properties.Resources.rawksd_savebanner, false);
				bin.Generate();
				bin.Bk.TitleID = 0x00010000535A4145UL;
				bin.Save(binstream);
				binstream.Close();
				string mainbinpath = Path.Combine(dirpath, "000.bin");
				File.Delete(mainbinpath);
				File.Move(binstream.Name, mainbinpath);

				dirpath = Path.Combine(Path.Combine(path, "rawk"), "rb2");
				if (!Directory.Exists(Path.Combine(dirpath, "customs")))
					Directory.CreateDirectory(Path.Combine(dirpath, "customs"));
				Directory.CreateDirectory(Path.Combine(Path.Combine(dirpath, "customs"), song.ID));
				FileStream savestream = new FileStream(Path.Combine(Path.Combine(Path.Combine(dirpath, "customs"), song.ID), "data"), FileMode.Create);
				dta.ToDTB().Save(new EndianReader(savestream, Endianness.LittleEndian));
				savestream.Close();

				Stream stream = new DelayedStream(data.Cache.GenerateFileStream(songbinpath, FileMode.Open));
				bin = new DlcBin(stream);
				appsong = new U8(bin.Data);

				data.Session["songdir"] = appsong.Root;
				AddSong(data, song, progress);
				stream.Close();

				cache.Dispose();

				progress.Progress();
			}

			progress.EndTask();
		}

		public static void TranscodePreview(IList<int> previewtimes, List<AudioFormat.Mapping> maps, IDecoder decoder, Stream stream, ProgressIndicator progress)
		{
			RawkAudio.Encoder encoder;
			if (maps != null && maps.Count(m => m.Instrument == Instrument.Preview) > 0) {
				decoder.Seek(0);
				List<ushort> masks = new List<ushort>();
				foreach (var m in maps) {
					if (m.Instrument == Instrument.Preview)
						masks.Add((ushort)(1 << maps.IndexOf(m)));
				}
				ushort[] mask = masks.ToArray();

				encoder = new RawkAudio.Encoder(stream, mask.Length, decoder.SampleRate, 28000);
				long samples = decoder.Samples;
				progress.NewTask("Transcoding Preview", samples);
				JaggedShortArray buffer = new JaggedShortArray(encoder.Channels, decoder.AudioBuffer.Rank2);
				while (samples > 0) {
					int read = decoder.Read((int)Math.Min(samples, decoder.AudioBuffer.Rank2));
					if (read <= 0)
						break;

					decoder.AudioBuffer.DownmixTo(buffer, mask, read);

					encoder.Write(buffer, read);
					samples -= read;
					progress.Progress(read);
				}
				progress.EndTask();
			} else {
				long start = Math.Min(decoder.Samples, (long)previewtimes[0] * decoder.SampleRate / 1000);
				decoder.Seek(start);
				long duration = Math.Min(decoder.Samples - start, (long)(previewtimes[1] - previewtimes[0]) * decoder.SampleRate / 1000);
				encoder = new RawkAudio.Encoder(stream, 1, decoder.SampleRate);
				AudioFormat.Transcode(encoder, decoder, duration, progress);
			}
			encoder.Dispose();
		}

		public static SongsDTA GetSongsDTA(SongData song, AudioFormat audioformat)
		{
			SongsDTA dta = HarmonixMetadata.GetSongData(song);

			dta.Downloaded = true;
			if ((dta.Decade == null || dta.Decade.Length == 0) && dta.Year.ToString().Length > 2)
				dta.Decade = "the" + dta.Year.ToString()[2] + "0s";

			dta.Song.Cores.Clear();
			dta.Song.Pans.Clear();
			dta.Song.Vols.Clear();
			foreach (SongsDTA.SongTracks track in dta.Song.Tracks)
				track.Tracks.Clear();
			dta.Song.TracksCount.Clear();
			var maps = audioformat.Mappings.Where(m => m.Instrument != Instrument.Preview).ToList();
			foreach (AudioFormat.Mapping map in maps) {
				dta.Song.Cores.Add(map.Instrument == Instrument.Guitar ? 1 : -1);
				dta.Song.Pans.Add(map.Balance);
				dta.Song.Vols.Add(map.Volume);
				SongsDTA.SongTracks track = dta.Song.Tracks.FirstOrDefault(t => t.Name == HarmonixMetadata.InstrumentToString(map.Instrument));
				if (track != null)
					track.Tracks.Add(maps.IndexOf(map));
			}

			// For safety with customs messing with mix and not knowing what they're doing
			SongsDTA.SongTracks drumtrack = dta.Song.Tracks.FirstOrDefault(t => t.Name == "drum");
			while (drumtrack != null && drumtrack.Tracks.Count > 0 && drumtrack.Tracks.Count < 6)
				drumtrack.Tracks.Add(drumtrack.Tracks[drumtrack.Tracks.Count - 1]);

			return dta;
		}
		
		public static NoteChart AdjustChart(SongData song, AudioFormat audioformat, ChartFormat chartformat)
		{
			NoteChart chart = chartformat.Chart;
			if (song.Difficulty[Instrument.Guitar] == 0)
				chart.PartGuitar = null;
			if (song.Difficulty[Instrument.Bass] == 0)
				chart.PartBass = null;
			if (song.Difficulty[Instrument.Drums] == 0)
				chart.PartDrums = null;
			if (song.Difficulty[Instrument.Vocals] == 0)
				chart.PartVocals = null;

			if (chart.PartDrums != null) {
				int drumcount = audioformat.Mappings.Count(m => m.Instrument == Instrument.Drums);
				foreach (var mix in chart.PartDrums.Mixing) {
					if (drumcount == 2 && mix.Value.Value != "drums0" && mix.Value.Value != "drums0d")
						mix.Value.Value = "drums0";
					// TODO: Better validation
				}
			}

			return chart;
		}
		
		public static ushort[] RemixAudioTracks(SongData song, AudioFormat audioformat)
		{
			// Audio tracks must match up with the tiers; disabled instruments (tier 0) must have no audio tracks.
			List<ushort> masks = new List<ushort>();
			List<AudioFormat.Mapping> newmaps = new List<AudioFormat.Mapping>();
			foreach (Instrument instrument in new Instrument[] { Instrument.Drums, Instrument.Bass, Instrument.Guitar, Instrument.Vocals, Instrument.Ambient }) {
				List<AudioFormat.Mapping> maps = new List<AudioFormat.Mapping>();
				foreach (var map in audioformat.Mappings) {
					if (map.Instrument == instrument && song.Difficulty[instrument] == 0)
						map.Instrument = Instrument.Ambient;
					if (map.Instrument == instrument)
						maps.Add(map);
				}

				if (maps.Count == 0 && song.Difficulty[instrument] > 0) {
					newmaps.Add(new AudioFormat.Mapping(0, instrument == Instrument.Drums ? -1 : 0, instrument)); masks.Add(0);
					if (instrument == Instrument.Drums) {
						newmaps.Add(new AudioFormat.Mapping(0, 1, instrument)); masks.Add(0);
					}
				} else if (maps.Count > 2 && instrument != Instrument.Drums) {
					ushort[] submasks = new ushort[2];
					foreach (var map in maps) {
						int index = audioformat.Mappings.IndexOf(map);
						if (map.Balance <= 0)
							submasks[0] |= (ushort)(1 << index);
						if (map.Balance >= 0)
							submasks[1] |= (ushort)(1 << index);
					}
					if (submasks[0] == submasks[1]) {
						newmaps.Add(new AudioFormat.Mapping(0, 0, instrument)); masks.Add(submasks[0]);
					} else {
						newmaps.Add(new AudioFormat.Mapping(0, -1, instrument)); masks.Add(submasks[0]);
						newmaps.Add(new AudioFormat.Mapping(0, 1, instrument)); masks.Add(submasks[1]);
					}
				} else if (maps.Count != 0) {
					foreach (var map in maps) {
						newmaps.Add(map); masks.Add((ushort)(1 << audioformat.Mappings.IndexOf(map)));
					}
				}
			}
			audioformat.Mappings = newmaps;

			return masks.ToArray();
		}

		private static uint GetConsoleID(string sdpath)
		{
			string idpath = Path.Combine(Path.Combine(sdpath, "rawk"), "id");
			if (File.Exists(idpath)) {
				FileStream stream = new FileStream(idpath, FileMode.Open);
				if (stream.Length < 4) {
					stream.Close();
					return 0;
				}

				uint cid = new EndianReader(stream, Endianness.BigEndian).ReadUInt32();
				stream.Close();
				return cid;
			}

			return 0;
		}

		private static bool FindUnusedContent(PlatformData data, SongData song, out string title, out ushort index)
		{
			for (title = "cRBA"; (byte)title[3] <= (byte)'Z'; title = title.Substring(0, 3) + (char)(((byte)title[3]) + 1)) {
				for (index = 1; index <= 510; index += 2) {
					bool flag = false;
					data.Mutex.WaitOne();
					foreach (FormatData formatdata in data.Songs) {
						SongsDTA dta = HarmonixMetadata.GetSongsDTA(formatdata.Song);
						if (String.Compare(title, dta.Song.Name.Substring(4, 4), true) == 0 && index == ushort.Parse(dta.Song.Name.Substring(9, 3))) {
							if (song.ID == formatdata.Song.ID)
								return true;
							flag = true;
							break;
						}
					}
					data.Mutex.ReleaseMutex();

					if (!flag)
						return true;
				}
			}

			title = null;
			index = 0;
			return false;
		}

		private static IList<SongsDTA> GetInstalledCustoms(string path, string sdpath)
		{
			if (!Directory.Exists(path))
				return new List<SongsDTA>();

			List<SongsDTA> list = new List<SongsDTA>();
			string[] dirs = Directory.GetDirectories(path, "*", SearchOption.TopDirectoryOnly);
			foreach (string file in dirs) {
				if (File.Exists(Path.Combine(file, "data"))) {
					FileStream fstream = null;
					try {
						fstream = new FileStream(Path.Combine(file, "data"), FileMode.Open);
						list.Add(SongsDTA.Create(DTB.Create(new EndianReader(fstream, Endianness.LittleEndian))));
					} catch { }
					if (fstream != null)
						fstream.Close();
				}
			}
			return list;
		}

		private static TMD GenerateDummyTMD(string title)
		{
			TMD tmd;
			tmd = TMD.Create(new MemoryStream(Properties.Resources.rawksd_tmd));
			tmd.TitleID = ((0x00010005UL << 32) | (BigEndianConverter.ToUInt32(Util.Encoding.GetBytes(title))));
			tmd.Contents.RemoveAll(c => c.Index != 0);
			tmd.Contents[0].ContentID = 0;
			tmd.Contents[0].Size = Properties.Resources.rawksd_savebanner.Length;
			tmd.Contents[0].Hash = SHA1.Create().ComputeHash(Properties.Resources.rawksd_savebanner);
			tmd.Fakesign();

			return tmd;
		}
	}
}
