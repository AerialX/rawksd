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
		public static bool IterateBins = false;

		public static PlatformRB2WiiCustomDLC Instance;
		public static void Initialise()
		{
			Instance = new PlatformRB2WiiCustomDLC();
		}

		public override int ID { get { throw new NotImplementedException(); } }

		public override string Name { get { return "RawkSD Wii DLC"; } }

		public override bool AddSong(PlatformData data, SongData song, ProgressIndicator progress)
		{
			if (IterateBins)
				return PlatformRB2WiiDisc.Instance.AddSong(data, song, progress);
			else {
				FormatData formatdata = new TemporaryFormatData(song, data);

				SongsDTA dta = HarmonixMetadata.GetSongsDTA(song);

				data.AddSong(formatdata);

				return true;
			}
		}

		public override void DeleteSong(PlatformData data, FormatData formatdata, ProgressIndicator progress)
		{
			string path = data.Session["maindirpath"] as string;
			string dtapath = "rawk/rb2/customs/" + formatdata.Song.ID;

			FileStream dtafile = new FileStream(Path.Combine(path, dtapath + "/data"), FileMode.Open, FileAccess.Read, FileShare.Read);

			EndianReader reader = new EndianReader(dtafile, Endianness.LittleEndian);
			DTB.NodeTree dtb = DTB.Create(reader);
			dtafile.Close();

			SongsDTA dta = SongsDTA.Create(dtb);
			string title = dta.Song.Name.Substring(4, 4);
			int index = int.Parse(dta.Song.Name.Substring(9, 3));
			Util.Delete(Path.Combine(path, "private/wii/data/" + title + "/" + Util.Pad((index).ToString(), 3) + ".bin"));
			Util.Delete(Path.Combine(path, "private/wii/data/" + title + "/" + Util.Pad((index + 1).ToString(), 3) + ".bin"));
			Directory.Delete(Path.Combine(path, dtapath), true);

			base.DeleteSong(data, formatdata, progress);

			SaveDTBCache(data);
		}

		public override PlatformData Create(string path, Game game, ProgressIndicator progress)
		{
			progress.NewTask(10);
			progress.SetNextWeight(9);

			if (!Directory.Exists(path))
				Directory.CreateDirectory(path);

			PlatformData data = new PlatformData(this, game);

			data.Session["maindirpath"] = path;

			string customdir = Path.Combine(path, "rawk/rb2/customs");
			if (Directory.Exists(customdir)) {
				string dtapath = Path.Combine(customdir, "data");
				DTB.NodeTree dtb = null;
				if (File.Exists(dtapath)) {
					Stream dtafile = new FileStream(dtapath, FileMode.Open, FileAccess.Read, FileShare.Read);
					try {
						dtb = DTB.Create(new EndianReader(dtafile, Endianness.LittleEndian));
					} catch (Exception exception) {
						Exceptions.Warning(exception, "The rawk/rb2/customs/data cache DTB is corrupt.");
					}
					dtafile.Close();
				}

				if (dtb != null) {
					progress.NewTask(dtb.Nodes.Count);
					foreach (DTB.NodeTree node in dtb.Nodes) {
						try {
							AddSongFromDTB(path, data, node, progress);
						} catch (Exception exception) {
							Exceptions.Warning(exception, "Could not import RawkSD custom from data cache.");
						}
						progress.Progress();
					}
					progress.EndTask();
				} else {
					string[] dirs = Directory.GetDirectories(customdir);
					progress.NewTask(dirs.Length);
					foreach (string folder in dirs) {
						try {
							dtapath = Path.Combine(folder, "data");
							if (File.Exists(dtapath)) {
								dtb = null;
								Stream dtafile = new FileStream(dtapath, FileMode.Open, FileAccess.Read, FileShare.Read);
								try {
									dtb = DTB.Create(new EndianReader(dtafile, Endianness.LittleEndian));
								} catch (Exception exception) {
									Exceptions.Warning(exception, "The data file for \"" + folder + "\" is corrupt.");
								}
								dtafile.Close();

								if (dtb != null)
									AddSongFromDTB(path, data, dtb, progress);
							}
						} catch (Exception exception) {
							Exceptions.Warning(exception, "Could not import RawkSD custom from " + folder);
						}
						progress.Progress();
					}
					progress.EndTask();
				}
				progress.Progress(9);
			}

			RefreshUnusedContent(data);
			progress.Progress();
			progress.EndTask();

			return data;
		}

		private void AddSongFromDTB(string path, PlatformData data, DTB.NodeTree dtb, ProgressIndicator progress)
		{
			SongData song = HarmonixMetadata.GetSongData(data, dtb);
			SongsDTA dta = HarmonixMetadata.GetSongsDTA(song);
			if (song.ID.StartsWith("rwk"))
				song.ID = dta.Song.Name.Split('/').Last();

			string title = dta.Song.Name.Substring(4, 4);
			int index = int.Parse(dta.Song.Name.Substring(9, 3));
			string indexpath = Util.Pad((index + 1).ToString(), 3);
			string contentpath = Path.Combine(path, "private/wii/data/" + title + "/" + indexpath + ".bin");
			string dtapath = Path.Combine(path, "private/wii/data/" + title + "/" + Util.Pad(index.ToString(), 3) + ".bin");

			AddSongFromBins(data, song, dtapath, contentpath, progress);
		}

		private void AddSongFromBins(PlatformData data, SongData song, string dtapath, string contentpath, ProgressIndicator progress, bool replace = false)
		{
			if (File.Exists(contentpath) && File.Exists(dtapath)) {
				Stream contentfile = null;
				Stream dtafile = null;
				if (IterateBins) {
					contentfile = new DelayedStream(data.Cache.GenerateFileStream(contentpath, FileMode.Open));
					DlcBin content = new DlcBin(contentfile);
					U8 u8 = new U8(content.Data);

					// Read album art from the preview bin
					dtafile = new DelayedStream(data.Cache.GenerateFileStream(dtapath, FileMode.Open));
					DlcBin dtabin = new DlcBin(dtafile);
					U8 dtau8 = new U8(dtabin.Data);
					string genpath = "/content/songs/" + song.ID + "/gen";
					DirectoryNode dtagen = dtau8.Root.Navigate(genpath) as DirectoryNode;
					if (dtagen != null) {
						DirectoryNode contentgen = u8.Root.Navigate(genpath) as DirectoryNode;
						if (contentgen != null)
							contentgen.AddChildren(dtagen.Files);
					}

					data.Session["songdir"] = u8.Root;
				}

				if (replace) {
					foreach (FormatData formatdata in data.Songs) {
						if (formatdata.Song.ID == song.ID) {
							base.DeleteSong(data, formatdata, progress);
							break;
						}
					}
				}

				AddSong(data, song, progress); // TODO: Add dta bin to songdir for album art

				if (IterateBins) {
					data.Session.Remove("songdir");
					contentfile.Close();
					dtafile.Close();
				}
			}
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
					if (!formatdata.HasStream(preview))
						cache.AddStream(preview);
					audioformat = AudioFormatRB2Bink.Instance.DecodeAudioFormat(formatdata);
					audioextension = ".bik";
				} else
					throw new NotSupportedException();

				progress.Progress(6);

				ChartFormat chartformat = (formatdata.GetFormat(FormatType.Chart) as IChartFormat).DecodeChart(formatdata, progress);

				Stream album = null;
				Stream chart = formatdata.GetStream(ChartFormatRB.Instance, ChartFormatRB.ChartFile);
				Stream pan = formatdata.GetStream(ChartFormatRB.Instance, ChartFormatRB.PanFile);
				Stream weights = formatdata.GetStream(ChartFormatRB.Instance, ChartFormatRB.WeightsFile);
				Stream milo = formatdata.GetStream(ChartFormatRB.Instance, ChartFormatRB.MiloFile);

				progress.SetNextWeight(2);

				if (chart != null && ChartFormatRB.Instance.NeedsFixing(formatdata)) {
					formatdata.CloseStream(chart);
					chart = null;
				}

				if (chart == null) {
					chart = new TemporaryStream();
					cache.AddStream(chart);
					AdjustChart(formatdata.Song, audioformat, chartformat).ToMidi().ToMid().Save(chart);
					chart.Position = 0;
				}

				if (weights == null) {
					weights = new TemporaryStream();
					cache.AddStream(weights);
					CreateWeights(weights, chartformat);
					weights.Position = 0;
				}

				if (pan == null) {
					pan = new TemporaryStream();
					cache.AddStream(pan);
					CreatePan(pan, chartformat);
					pan.Position = 0;
				}

				if (milo == null) {
					milo = formatdata.GetStream(ChartFormatRB.Instance, ChartFormatRB.Milo3File);
					if (milo == null)
						milo = new MemoryStream(Properties.Resources.rawksd_milo);
					else {
						Stream milostream = new TemporaryStream();
						cache.AddStream(milostream);
						Milo milofile = new Milo(new EndianReader(milo, Endianness.LittleEndian));
						FaceFX fx = new FaceFX(new EndianReader(milofile.Data[0], Endianness.BigEndian));

						TemporaryStream fxstream = new TemporaryStream();
						fx.Save(new EndianReader(fxstream, Endianness.LittleEndian));
						milofile.Data[0] = fxstream;
						milofile.Compressed = true;
						milofile.Save(new EndianReader(milostream, Endianness.LittleEndian));
						fxstream.Close();
						formatdata.CloseStream(milo);
						milo = milostream;
						milo.Position = 0;
					}
				}

				//if (album == null)
				//	album = new MemoryStream(Properties.Resources.rawksd_albumart);

				progress.Progress(2);

				SongData song = new SongData(formatdata.Song);

				SongsDTA dta = GetSongsDTA(song, audioformat);

				if (album == null)
					dta.AlbumArt = false;
				else
					dta.AlbumArt = true;

				DTB.NodeTree dtb = dta.ToDTB(PlatformRawkFile.Instance.IsRawkSD2(song));

				MemoryStream songsdta = new MemoryStream();
				cache.AddStream(songsdta);
				dtb.SaveDTA(songsdta);
				songsdta.Position = 0;

				U8 appdta = new U8();
				DirectoryNode dir = new DirectoryNode("content");
				appdta.Root.AddChild(dir);
				DirectoryNode songsdir = new DirectoryNode("songs");
				dir.AddChild(songsdir);
				DirectoryNode songdir = new DirectoryNode(song.ID);
				songsdir.AddChild(songdir);
				songdir.AddChild(new FileNode(song.ID + "_prev.mogg", preview));
				songdir.AddChild(new FileNode("songs.dta", songsdta));
				DirectoryNode gendir;
				if (dta.AlbumArt.Value && album != null) {
					gendir = new DirectoryNode("gen");
					songdir.AddChild(gendir);
					gendir.AddChild(new FileNode(song.ID + "_nomip_keep.bmp_wii", album));
				}

				U8 appsong = new U8();
				dir = new DirectoryNode("content");
				appsong.Root.AddChild(dir);
				songsdir = new DirectoryNode("songs");
				dir.AddChild(songsdir);
				songdir = new DirectoryNode(song.ID);
				songsdir.AddChild(songdir);
				songdir.AddChild(new FileNode(song.ID + audioextension, audio));
				songdir.AddChild(new FileNode(song.ID + ".mid", chart));
				songdir.AddChild(new FileNode(song.ID + ".pan", pan));
				gendir = new DirectoryNode("gen");
				songdir.AddChild(gendir);
				gendir.AddChild(new FileNode(song.ID + ".milo_wii", milo));
				gendir.AddChild(new FileNode(song.ID + "_weights.bin", weights));

				Stream memoryDta = new TemporaryStream();
				cache.AddStream(memoryDta);
				appdta.Save(memoryDta);

				Stream memorySong = new TemporaryStream();
				cache.AddStream(memorySong);
				appsong.Save(memorySong);

				formatdata.CloseStream(audio);
				formatdata.CloseStream(preview);
				formatdata.CloseStream(chart);
				formatdata.CloseStream(album);
				formatdata.CloseStream(pan);
				formatdata.CloseStream(weights);
				formatdata.CloseStream(milo);

				FindUnusedContent(data, formatdata.Song, out otitle, out oindex);
				TMD tmd = GenerateDummyTMD(otitle);

				dta.Song.Name = "dlc/" + otitle + "/" + Util.Pad(oindex.ToString(), 3) + "/content/songs/" + song.ID + "/" + song.ID;
				dta.Song.MidiFile = "dlc/content/songs/" + song.ID + "/" + song.ID + ".mid";
				dtb = dta.ToDTB(PlatformRawkFile.Instance.IsRawkSD2(song));
				HarmonixMetadata.SetSongsDTA(song, dtb);

				string dirpath = Path.Combine(Path.Combine(path, "rawk"), "rb2");
				if (!Directory.Exists(Path.Combine(dirpath, "customs")))
					Directory.CreateDirectory(Path.Combine(dirpath, "customs"));
				Directory.CreateDirectory(Path.Combine(Path.Combine(dirpath, "customs"), song.ID));
				FileStream savestream = new FileStream(Path.Combine(Path.Combine(Path.Combine(dirpath, "customs"), song.ID), "data"), FileMode.Create);
				dtb.Save(new EndianReader(savestream, Endianness.LittleEndian));
				savestream.Close();

				TmdContent contentDta = new TmdContent();
				contentDta.ContentID = oindex;
				contentDta.Index = oindex;
				contentDta.Type = 0x4001;

				TmdContent contentSong = new TmdContent();
				contentSong.ContentID = oindex + 1U;
				contentSong.Index = (ushort)(oindex + 1);
				contentSong.Type = 0x4001;

				memoryDta.Position = 0;
				contentDta.Hash = Util.SHA1Hash(memoryDta);
				contentDta.Size = memoryDta.Length;

				memorySong.Position = 0;
				contentSong.Hash = Util.SHA1Hash(memorySong);
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

				dirpath = Path.Combine(Path.Combine(Path.Combine(Path.Combine(path, "private"), "wii"), "data"), "sZAE");
				TemporaryStream binstream;
				DlcBin bin;
				if (consoleid != 0 && !File.Exists(Path.Combine(dirpath, "000.bin"))) {
					Directory.CreateDirectory(dirpath);
					binstream = new TemporaryStream();
					binstream.Write(Properties.Resources.rawksd_000bin, 0, Properties.Resources.rawksd_000bin.Length);
					binstream.Position = 8;
					new EndianReader(binstream, Endianness.BigEndian).Write(consoleid);
					binstream.ClosePersist();
					File.Move(binstream.Name, Path.Combine(dirpath, "000.bin"));
					binstream.Close();
				}

				dirpath = Path.Combine(Path.Combine(Path.Combine(Path.Combine(path, "private"), "wii"), "data"), otitle);
				if (!Directory.Exists(dirpath))
					Directory.CreateDirectory(dirpath);

				binstream = new TemporaryStream();
				bin = new DlcBin();
				bin.Bk.ConsoleID = consoleid;
				bin.TMD = tmd;
				bin.Content = tmd.Contents[oindex];
				bin.Data = memoryDta;
				bin.Generate();
				bin.Bk.TitleID = 0x00010000535A4145UL;
				bin.Save(binstream);
				binstream.ClosePersist();
				string dtabinpath = Path.Combine(dirpath, Util.Pad(oindex.ToString(), 3) + ".bin");
				Util.Delete(dtabinpath);
				File.Move(binstream.Name, dtabinpath);
				binstream.Close();

				binstream = new TemporaryStream();
				bin = new DlcBin();
				bin.Bk.ConsoleID = consoleid;
				bin.TMD = tmd;
				bin.Content = tmd.Contents[oindex + 1];
				bin.Data = memorySong;
				bin.Generate();
				bin.Bk.TitleID = 0x00010000535A4145UL;
				bin.Save(binstream);
				binstream.ClosePersist();
				string songbinpath = Path.Combine(dirpath, Util.Pad((oindex + 1).ToString(), 3) + ".bin");
				Util.Delete(songbinpath);
				File.Move(binstream.Name, songbinpath);
				binstream.Close();

				data.Mutex.WaitOne();
				string mainbinpath = Path.Combine(dirpath, "000.bin");
				if (!File.Exists(mainbinpath)) {
					binstream = new TemporaryStream();
					bin = new DlcBin();
					bin.Bk.ConsoleID = consoleid;
					bin.TMD = tmd;
					bin.Content = tmd.Contents[0];
					bin.Data = new MemoryStream(Properties.Resources.rawksd_savebanner, false);
					bin.Generate();
					bin.Bk.TitleID = 0x00010000535A4145UL;
					bin.Save(binstream);
					binstream.ClosePersist();
					File.Move(binstream.Name, mainbinpath);
					binstream.Close();
				}

				AddSongFromBins(data, song, dtabinpath, songbinpath, progress, true);

				SaveDTBCache(data);

				data.Mutex.ReleaseMutex();

				cache.Dispose();

				progress.Progress();
			}

			progress.EndTask();
		}

		public static void CreateWeights(Stream weights, ChartFormat chartformat)
		{
			EndianReader writer = new EndianReader(weights, Endianness.LittleEndian);

			NoteChart chart = chartformat.Chart;
			//if (chart.PartVocals == null)
			//	return;

			ulong duration = chart.GetTime(chart.FindLastNote());
			for (ulong i = 0; i < 60 * duration / 1000000; i++)
			//for (ulong i = 0; i < 0x100; i++)
				//writer.Write((byte)0x00);
				writer.Write((byte)0x90);
			/* foreach (var lyric in chart.PartVocals.Lyrics) {
				if (!lyric.Value.EndsWith("#") && !lyric.Value.EndsWith("^"))
					continue;

				var gem = chart.PartVocals.Gems.FirstOrDefault(g => g.Time == lyric.Key.Time);
				if (gem == null)
					continue;

				ulong time = chart.GetTime(gem.Time);
				time = 60 * time / 1000000;
				for (ulong i = (ulong)writer.Position; i < time; i++)
					writer.Write((byte)0xFE);
				ulong duration = chart.GetTimeDuration(gem.Time, gem.Duration);
				for (ulong i = 0; i < duration; i += 1000000/60)
					writer.Write((byte)0x90);
			} */
		}

		public static void CreatePan(Stream pan, ChartFormat chartformat)
		{
			EndianReader writer = new EndianReader(pan, Endianness.LittleEndian);

			NoteChart chart = chartformat.Chart;
			//if (chart.PartVocals == null)
			//	return;

			ulong duration = chart.GetTime(chart.FindLastNote());
			for (ulong i = 0; i < 8 * 60 * duration / 1000000; i++)
			//for (ulong i = 0; i < 0x100; i++)
				//writer.Write((byte)0x00);
				writer.Write((byte)0xFF);
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

		public static SongsDTA GetSongsDTA(SongData song, AudioFormat audioformat, bool idioticdrums = true)
		{
			SongsDTA dta = HarmonixMetadata.GetSongData(song);

			dta.Downloaded = true;
			if ((dta.Decade == null || dta.Decade.Length == 0) && dta.Year.ToString().Length > 2)
				dta.Decade = "the" + dta.Year.ToString()[2] + "0s";
			dta.BaseName = "rwk" + dta.Version.ToString() + dta.BaseName;
			dta.Genre = ImportMap.GetShortGenre(dta.Genre);

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

			if (idioticdrums) {
				// For safety with customs messing with mix and not knowing what they're doing
				SongsDTA.SongTracks drumtrack = dta.Song.Tracks.FirstOrDefault(t => t.Name == "drum");
				while (drumtrack != null && drumtrack.Tracks.Count > 0 && drumtrack.Tracks.Count < 6)
					drumtrack.Tracks.Add(drumtrack.Tracks[drumtrack.Tracks.Count - 1]);
			}

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

		public static string UnusedTitle = "cRBA";
		private static bool FindUnusedContent(PlatformData data, SongData song, out string title, out ushort index)
		{
			data.Mutex.WaitOne();

			Dictionary<string, List<ushort>> contents = data.Session["contents"] as Dictionary<string, List<ushort>>;

			foreach (FormatData formatdata in data.Songs) {
				SongData subsong = formatdata.Song;
				if (subsong.ID == song.ID && GetSongBin(subsong, out title, out index)) {
					data.Mutex.ReleaseMutex();
					return true;
				}
			}

			for (title = UnusedTitle; (byte)title[3] <= (byte)'Z'; title = title.Substring(0, 3) + (char)(((byte)title[3]) + 1)) {
				if (!contents.ContainsKey(title))
					contents[title] = new List<ushort>();
				var list = contents[title];
				for (index = 1; index < 510; index += 2) {
					if (!list.Contains(index)) {
						list.Add(index);
						data.Mutex.ReleaseMutex();
						return true;
					}
				}
			}

			data.Mutex.ReleaseMutex();

			title = null;
			index = 0;
			return false;
		}

		private static void RefreshUnusedContent(PlatformData data)
		{
			data.Mutex.WaitOne();

			Dictionary<string, List<ushort>> contents = new Dictionary<string,List<ushort>>();

			foreach (FormatData formatdata in data.Songs) {
				string title;
				ushort index;
				if (GetSongBin(formatdata.Song, out title, out index)) {
					if (!contents.ContainsKey(title))
						contents[title] = new List<ushort>();
					contents[title].Add(index);
				}
			}

			data.Session["contents"] = contents;

			data.Mutex.ReleaseMutex();
		}

		private static bool GetSongBin(SongData song, out string title, out ushort index)
		{
			title = null;
			index = 0;
			SongsDTA dta = HarmonixMetadata.GetSongsDTA(song);
			if (dta == null)
				return false;
			title = dta.Song.Name.Substring(4, 4);
			return ushort.TryParse(dta.Song.Name.Substring(9, 3), out index);
		}

		private void SaveDTBCache(PlatformData data)
		{
			data.Mutex.WaitOne();

			string path = Path.Combine(Path.Combine(Path.Combine(Path.Combine(data.Session["maindirpath"] as string, "rawk"), "rb2"), "customs"), "data");

			if (data.Songs.Count == 0) {
				if (File.Exists(path))
					Util.Delete(path);
				data.Mutex.ReleaseMutex();
				return;
			}

			DTB.NodeTree tree = new DTB.NodeTree();

			foreach (FormatData formatdata in data.Songs) {
				SongsDTA dta = HarmonixMetadata.GetSongsDTA(formatdata.Song);
				tree.Nodes.Add(dta.ToDTB(PlatformRawkFile.Instance.IsRawkSD2(formatdata.Song)));
			}

			FileStream file = new FileStream(path, FileMode.Create, FileAccess.Write, FileShare.None);
			tree.Save(new EndianReader(file, Endianness.LittleEndian));
			file.Close();

			data.Mutex.ReleaseMutex();
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
