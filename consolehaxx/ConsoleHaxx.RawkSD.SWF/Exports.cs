using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using ConsoleHaxx.Wii;
using ConsoleHaxx.Common;
using System.IO;
using ConsoleHaxx.Harmonix;
using ConsoleHaxx.Xbox360;

namespace ConsoleHaxx.RawkSD.SWF
{
	public static class Exports
	{
		public static void ExportRWK(string path, IEnumerable<FormatData> songs, bool audio)
		{
			Program.Form.Progress.QueueTask(progress => {
				progress.NewTask("Exporting to RawkSD archive", 2);

				List<Stream> streams = new List<Stream>();

				U8 u8 = new U8();
				foreach (FormatData data in songs) {
					DirectoryNode dir = new DirectoryNode(data.Song.ID);
					u8.Root.AddChild(dir);
					foreach (string name in data.GetStreamNames()) {
						Stream stream = data.GetStream(name);
						int id;
						if (name.Contains('.') && int.TryParse(Path.GetExtension(name).Substring(1), out id)) {
							IFormat format = Platform.GetFormat(id);
							if (format.Type == FormatType.Audio && (!audio || format == AudioFormatRB2Bink.Instance)) {
								data.CloseStream(stream);
								continue;
							}
						}

						streams.Add(stream);
						dir.AddChild(new FileNode(name, stream));
					}
				}

				progress.Progress();

				Stream ostream = new FileStream(path, FileMode.Create, FileAccess.Write);
				u8.Save(ostream);
				ostream.Close();

				foreach (Stream stream in streams)
					stream.Close();

				progress.Progress();
				progress.EndTask();
			});
		}

		public static void ExportFoF(string path, IEnumerable<FormatData> songs)
		{
			foreach (FormatData data in songs) {
				QueueExportFoF(path, data);
			}
		}

		private static void QueueExportFoF(string path, FormatData data)
		{
			Program.Form.Progress.QueueTask(progress => {
				progress.NewTask("Installing \"" + data.Song.Name + "\" to Frets on Fire", 1);
				FormatData source = data;

				Program.Form.TaskMutex.WaitOne();
				bool local = data.PlatformData.Platform == PlatformLocalStorage.Instance;
				bool ownformat = false;
				if (!local && Configuration.MaxConcurrentTasks > 1) {
					source = new TemporaryFormatData(data.Song, data.PlatformData);
					data.SaveTo(source);
					ownformat = true;
				}
				Program.Form.TaskMutex.ReleaseMutex();

				PlatformData platformdata = new PlatformData(PlatformFretsOnFire.Instance, Game.Unknown);
				platformdata.Session["path"] = path;

				PlatformFretsOnFire.Instance.SaveSong(platformdata, data, progress);
				progress.Progress();

				if (ownformat)
					source.Dispose();

				progress.EndTask();
			});
		}

		public static void ExportRBN(string path, FormatData original)
		{
			Program.Form.Progress.QueueTask(progress => {
				progress.NewTask("Exporting to RBA file", 16);
				RBA rba = new RBA();

				FormatData data = original;

				Program.Form.TaskMutex.WaitOne();
				bool local = data.PlatformData.Platform == PlatformLocalStorage.Instance;
				bool ownformat = false;
				if (!local && Configuration.MaxConcurrentTasks > 1) {
					data = new TemporaryFormatData(data.Song, data.PlatformData);
					original.SaveTo(data);
					ownformat = true;
				}
				Program.Form.TaskMutex.ReleaseMutex();

				using (DelayedStreamCache cache = new DelayedStreamCache()) {
					AudioFormat audioformat = (data.GetFormat(FormatType.Audio) as IAudioFormat).DecodeAudio(data, progress);

					ushort[] masks = PlatformRB2WiiCustomDLC.RemixAudioTracks(data.Song, audioformat);

					progress.Progress();

					// RBN Required DTA settings
					SongsDTA dta = PlatformRB2WiiCustomDLC.GetSongsDTA(data.Song, audioformat, false);
					dta.AlbumArt = true;
					dta.BaseName = "song";
					dta.Song.Name = "songs/" + dta.BaseName + "/" + dta.BaseName;
					dta.Song.MidiFile = "songs/" + dta.BaseName + "/" + dta.BaseName + ".mid";
					dta.Rating = 4;
					dta.Genre = "rock";
					dta.SubGenre = "subgenre_rock";
					dta.Format = 4;
					dta.SongID = 0;
					dta.Origin = "rb2";
					dta.Ugc = 1;
					dta.SongLength = 300000;
					dta.Preview[0] = 0;
					dta.Preview[1] = 30000;
					dta.Context = 2000;
					dta.BasePoints = 0;
					if (!dta.TuningOffsetCents.HasValue)
						dta.TuningOffsetCents = 0;

					rba.Audio = new TemporaryStream();
					cache.AddStream(rba.Audio);

					progress.Progress();

					progress.SetNextWeight(10);
					long samples = audioformat.Decoder.Samples;
					CryptedMoggStream mogg = new CryptedMoggStream(rba.Audio);
					mogg.WriteHeader();
					RawkAudio.Encoder encoder = new RawkAudio.Encoder(mogg, audioformat.Mappings.Count, audioformat.Decoder.SampleRate);
					progress.NewTask("Transcoding Audio", samples);
					JaggedShortArray buffer = new JaggedShortArray(encoder.Channels, audioformat.Decoder.AudioBuffer.Rank2);
					AudioFormat.ProcessOffset(audioformat.Decoder, encoder, audioformat.InitialOffset);
					while (samples > 0) {
						int read = audioformat.Decoder.Read((int)Math.Min(samples, audioformat.Decoder.AudioBuffer.Rank2));
						if (read <= 0)
							break;

						audioformat.Decoder.AudioBuffer.DownmixTo(buffer, masks, read);

						encoder.Write(buffer, read);
						samples -= read;
						progress.Progress(read);
					}
					progress.EndTask();
					progress.Progress(10);

					encoder.Dispose();
					audioformat.Decoder.Dispose();
					rba.Audio.Position = 0;

					dta.Song.TracksCount.Add(audioformat.Mappings.Count(m => m.Instrument == Instrument.Drums));
					dta.Song.TracksCount.Add(audioformat.Mappings.Count(m => m.Instrument == Instrument.Bass));
					dta.Song.TracksCount.Add(audioformat.Mappings.Count(m => m.Instrument == Instrument.Guitar));
					dta.Song.TracksCount.Add(audioformat.Mappings.Count(m => m.Instrument == Instrument.Vocals));
					dta.Song.TracksCount.Add(audioformat.Mappings.Count(m => m.Instrument == Instrument.Ambient));

					for (int i = 0; i < dta.Song.Cores.Count; i++)
						dta.Song.Cores[i] = -1;

					rba.Data = new TemporaryStream();
					cache.AddStream(rba.Data);
					dta.Save(rba.Data);

					Stream stream = new MemoryStream(Properties.Resources.rbn_metadata);
					rba.Metadata = stream;
					stream = new MemoryStream(Properties.Resources.rbn_album);
					rba.AlbumArt = stream;
					stream = new MemoryStream(Properties.Resources.rbn_milo);
					rba.Milo = stream;
					stream = new MemoryStream(Properties.Resources.rbn_weights);
					rba.Weights = stream;

					if (data.Formats.Contains(ChartFormatRB.Instance) && !ChartFormatRB.Instance.NeedsFixing(data)) {
						rba.Chart = data.GetStream(ChartFormatRB.Instance, ChartFormatRB.ChartFile);
					} else {
						IChartFormat chartformat = data.GetFormat(FormatType.Chart) as IChartFormat;
						NoteChart chart = PlatformRB2WiiCustomDLC.AdjustChart(data.Song, audioformat, chartformat.DecodeChart(data, progress));
						rba.Chart = new TemporaryStream();
						cache.AddStream(rba.Chart);
						chart.ToMidi().ToMid().Save(rba.Chart);
					}

					progress.Progress(3);

					rba.Strings.Add(((char)0x02).ToString());
					rba.Strings.Add(((char)0x22).ToString());
					rba.Strings.Add((char)0x02 + "090923");
					rba.Strings.Add("090923");
					rba.Strings.Add(string.Empty);
					rba.Strings.Add(string.Empty);
					rba.Strings.Add((char)0x02 + "090923");
					rba.Strings.Add("s/rawk/a");

					Stream ostream = new FileStream(path, FileMode.Create);
					rba.Save(ostream);
					ostream.Close();

					data.CloseStream(rba.Chart);

					if (ownformat)
						data.Dispose();

					progress.Progress();
					progress.EndTask();
				}
			});
		}

		public static void ImportRB1Lipsync(PlatformData lrb)
		{
			DirectoryNode root = lrb.Session["songdir"] as DirectoryNode;

			root = root.Navigate("songs/rb1") as DirectoryNode;
			if (root == null)
				return;

			List<PlatformData> platforms = new List<PlatformData>(Program.Form.Platforms);
			platforms.Add(Program.Form.Storage);

			foreach (PlatformData platform in platforms) {
				platform.Mutex.WaitOne();

				foreach (FormatData data in platform.Songs) {
					if (data.Formats.Contains(ChartFormatRB.Instance)) {
						string songid = data.Song.ID;
						if (songid.StartsWith("rb1")) {
							songid = songid.Substring(3);
							DirectoryNode songdir = root.Navigate(songid) as DirectoryNode;
							FileNode node = songdir.Navigate(songid + ".pan") as FileNode;
							if (node != null) {
								node.Data.Position = 0;
								Stream stream = data.AddStream(ChartFormatRB.Instance, ChartFormatRB.PanFile);
								Util.StreamCopy(stream, node.Data);
								data.CloseStream(stream);
								node.Data.Close();
							}
							node = songdir.Navigate("gen/" + songid + ".milo_wii") as FileNode;
							if (node != null) {
								node.Data.Position = 0;
								Stream stream = data.AddStream(ChartFormatRB.Instance, ChartFormatRB.MiloFile);
								Util.StreamCopy(stream, node.Data);
								data.CloseStream(stream);
								node.Data.Close();
							}
							node = songdir.Navigate("gen/" + songid + "_keep.png_wii") as FileNode;
							if (node == null)
								node = songdir.Navigate("gen/" + songid + "_nomip_keep.bmp_wii") as FileNode;
							if (node != null) {
								node.Data.Position = 0;
								data.Song.AlbumArt = WiiImage.Create(new EndianReader(node.Data, Endianness.LittleEndian)).Bitmap;
								node.Data.Close();
							}
						}
					}
				}

				platform.Mutex.ReleaseMutex();
			}
		}

		internal static void Export360(string path, FormatData original)
		{
			Program.Form.Progress.QueueTask(progress => {
				progress.NewTask("Exporting to 360 DLC file", 16);

				SongData song = original.Song;

				StfsArchive stfs = new StfsArchive();
				stfs.Stfs.ContentType = StfsFile.ContentTypes.Marketplace;
				stfs.Stfs.TitleID = 0x45410829;
				for (int i = 0; i < stfs.Stfs.DisplayName.Length; i++)
					stfs.Stfs.DisplayName[i] = song.Artist + " - " + song.Name;
				for (int i = 0; i < stfs.Stfs.DisplayDescription.Length; i++)
					stfs.Stfs.DisplayDescription[i] = "RawkSD DLC - http://rawksd.japaneatahand.com";
				stfs.Stfs.TitleName = "Rawk Band 2";
				stfs.Stfs.Publisher = "RawkSD";
				stfs.Stfs.TitleThumbnail = WiiImage.Create(new EndianReader(new MemoryStream(ConsoleHaxx.RawkSD.Properties.Resources.rawksd_albumart), Endianness.LittleEndian)).Bitmap;
				if (song.AlbumArt != null)
					stfs.Stfs.Thumbnail = song.AlbumArt;
				else
					stfs.Stfs.Thumbnail = stfs.Stfs.TitleThumbnail;
				StfsFile.SignedHeader header = new StfsFile.SignedHeader(StfsFile.FileTypes.LIVE);
				stfs.Stfs.Header = header;
				header.Licenses[0].ID = 0xFFFFFFFFFFFFFFFF;
				stfs.Stfs.MetadataVersion = 2;
				StfsFile.PackageDescriptor desc = new StfsFile.PackageDescriptor();
				stfs.Stfs.Descriptor = desc;
				desc.StructSize = 0x24;
				desc.BlockSeparation = 1;
				desc.FileTableBlockCount = 0x100; // byte apparently
				stfs.Stfs.TransferFlags = 0xC0;

				FormatData data = original;

				Program.Form.TaskMutex.WaitOne();
				bool local = data.PlatformData.Platform == PlatformLocalStorage.Instance;
				bool ownformat = false;
				if (!local && Configuration.MaxConcurrentTasks > 1) {
					data = new TemporaryFormatData(data.Song, data.PlatformData);
					original.SaveTo(data);
					ownformat = true;
				}
				Program.Form.TaskMutex.ReleaseMutex();

				Stream songs = null;
				Stream audio = null;
				Stream chart = data.GetStream(ChartFormatRB.Instance, ChartFormatRB.ChartFile);
				Stream pan = data.GetStream(ChartFormatRB.Instance, ChartFormatRB.PanFile);
				Stream weights = data.GetStream(ChartFormatRB.Instance, ChartFormatRB.WeightsFile);
				Stream milo = data.GetStream(ChartFormatRB.Instance, ChartFormatRB.MiloFile);
				milo = new MemoryStream(Properties.Resources.rbn_milo); // TODO: properly convert milo
				
				if (milo == null)
					milo = new MemoryStream(ConsoleHaxx.RawkSD.Properties.Resources.rawksd_milo);

				using (DelayedStreamCache cache = new DelayedStreamCache()) {
					AudioFormat audioformat = (data.GetFormat(FormatType.Audio) as IAudioFormat).DecodeAudio(data, progress);

					ushort[] masks = PlatformRB2WiiCustomDLC.RemixAudioTracks(data.Song, audioformat);

					progress.Progress();

					// RBN Required DTA settings
					SongsDTA dta = PlatformRB2WiiCustomDLC.GetSongsDTA(song, audioformat, false);
					dta.AlbumArt = false;
					dta.Song.Name = "songs/" + song.ID + "/" + song.ID;
					dta.Song.MidiFile = "songs/" + song.ID + "/" + song.ID + ".mid";
					dta.Rating = 4;
					dta.SubGenre = "subgenre_core";
					dta.Format = 4;
					//dta.SongID = 0;
					dta.Origin = "rb2";
					//dta.Ugc = 1;
					//dta.SongLength = 300000;
					dta.Context = 2000;
					dta.BasePoints = 0;
					if (!dta.TuningOffsetCents.HasValue)
						dta.TuningOffsetCents = 0;

					audio = new TemporaryStream();
					cache.AddStream(audio);

					progress.Progress();

					progress.SetNextWeight(10);
					long samples = audioformat.Decoder.Samples;
					CryptedMoggStream mogg = new CryptedMoggStream(audio);
					mogg.WriteHeader();
					RawkAudio.Encoder encoder = new RawkAudio.Encoder(mogg, audioformat.Mappings.Count, audioformat.Decoder.SampleRate);
					progress.NewTask("Transcoding Audio", samples);
					JaggedShortArray buffer = new JaggedShortArray(encoder.Channels, audioformat.Decoder.AudioBuffer.Rank2);
					AudioFormat.ProcessOffset(audioformat.Decoder, encoder, audioformat.InitialOffset);
					while (samples > 0) {
						int read = audioformat.Decoder.Read((int)Math.Min(samples, audioformat.Decoder.AudioBuffer.Rank2));
						if (read <= 0)
							break;

						audioformat.Decoder.AudioBuffer.DownmixTo(buffer, masks, read);

						encoder.Write(buffer, read);
						samples -= read;
						progress.Progress(read);
					}
					progress.EndTask();
					progress.Progress(10);

					encoder.Dispose();
					audioformat.Decoder.Dispose();
					audio.Position = 0;

					dta.Song.TracksCount.Add(audioformat.Mappings.Count(m => m.Instrument == Instrument.Drums));
					dta.Song.TracksCount.Add(audioformat.Mappings.Count(m => m.Instrument == Instrument.Bass));
					dta.Song.TracksCount.Add(audioformat.Mappings.Count(m => m.Instrument == Instrument.Guitar));
					dta.Song.TracksCount.Add(audioformat.Mappings.Count(m => m.Instrument == Instrument.Vocals));
					dta.Song.TracksCount.Add(audioformat.Mappings.Count(m => m.Instrument == Instrument.Ambient));

					for (int i = 0; i < dta.Song.Cores.Count; i++)
						dta.Song.Cores[i] = -1;

					songs = new TemporaryStream();
					cache.AddStream(songs);
					dta.ToDTB().SaveDTA(songs);

					ChartFormat chartformat = (data.GetFormat(FormatType.Chart) as IChartFormat).DecodeChart(data, progress);
					if (chart != null && ChartFormatRB.Instance.NeedsFixing(data)) {
						data.CloseStream(chart);
						chart = null;
					}

					if (chart == null) {
						chart = new TemporaryStream();
						cache.AddStream(chart);
						PlatformRB2WiiCustomDLC.AdjustChart(song, audioformat, chartformat).ToMidi().ToMid().Save(chart);
						chart.Position = 0;
					}

					if (weights == null) {
						weights = new TemporaryStream();
						cache.AddStream(weights);
						PlatformRB2WiiCustomDLC.CreateWeights(weights, chartformat);
						weights.Position = 0;
					}

					if (pan == null) {
						pan = new TemporaryStream();
						cache.AddStream(pan);
						PlatformRB2WiiCustomDLC.CreatePan(pan, chartformat);
						pan.Position = 0;
					}

					progress.Progress(3);

					DirectoryNode dir = new DirectoryNode("songs");
					stfs.Root.AddChild(dir);
					dir.AddChild(new FileNode("songs.dta", songs));
					DirectoryNode songdir = new DirectoryNode(song.ID);
					dir.AddChild(songdir);
					songdir.AddChild(new FileNode(song.ID + ".mogg", audio));
					songdir.AddChild(new FileNode(song.ID + ".mid", chart));
					DirectoryNode gendir = new DirectoryNode("gen");
					songdir.AddChild(gendir);
					gendir.AddChild(new FileNode(song.ID + ".milo_xbox", milo));
					gendir.AddChild(new FileNode(song.ID + "_weights.bin", weights));

					Stream ostream = new FileStream(path, FileMode.Create);
					stfs.Save(ostream);
					ostream.Close();

					data.CloseStream(audio);
					data.CloseStream(chart);
					data.CloseStream(pan);
					data.CloseStream(weights);
					data.CloseStream(milo);

					if (ownformat)
						data.Dispose();

					progress.Progress();
					progress.EndTask();
				}
			});
		}
	}
}
