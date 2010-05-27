using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using ConsoleHaxx.Wii;
using ConsoleHaxx.Common;
using System.IO;
using ConsoleHaxx.Harmonix;

namespace ConsoleHaxx.RawkSD.SWF
{
	public static class Exports
	{
		public static void ExportRWK(string path, IEnumerable<FormatData> songs)
		{
			Program.Form.Progress.QueueTask(progress => {
				progress.NewTask("Exporting to RawkSD Archive", 2);

				U8 u8 = new U8();
				foreach (FormatData data in songs) {
					DirectoryNode dir = new DirectoryNode(data.Song.ID, u8.Root);
					foreach (string name in data.GetStreamNames()) {
						Stream stream = data.GetStream(name);
						new FileNode(name, dir, (ulong)stream.Length, stream);
					}
				}

				progress.Progress();

				Stream ostream = new FileStream(path, FileMode.Create, FileAccess.Write);
				u8.Save(ostream);
				ostream.Close();

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
				if ((data.PlatformData.Platform != PlatformLocalStorage.Instance || Configuration.LocalTranscode) && Configuration.LocalTransfer) {
					source = PlatformLocalStorage.Instance.CreateSong(Program.Form.Storage, data.Song);
					data.SaveTo(source);
				} else if (data.PlatformData.Platform != PlatformLocalStorage.Instance && Configuration.MaxConcurrentTasks > 1) {
					source = new TemporaryFormatData(data.Song, data.PlatformData);
					data.SaveTo(source);
				}
				Program.Form.TaskMutex.ReleaseMutex();

				PlatformData platformdata = new PlatformData(PlatformFretsOnFire.Instance, Game.Unknown);
				platformdata.Session["path"] = path;

				PlatformFretsOnFire.Instance.SaveSong(platformdata, data, progress);
				progress.Progress();

				if (source != data)
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
				if ((data.PlatformData.Platform != PlatformLocalStorage.Instance || Configuration.LocalTranscode) && Configuration.LocalTransfer) {
					data = PlatformLocalStorage.Instance.CreateSong(Program.Form.Storage, data.Song);
					original.SaveTo(data);
				} else if (data.PlatformData.Platform != PlatformLocalStorage.Instance && Configuration.MaxConcurrentTasks > 1) {
					data = new TemporaryFormatData(data.Song, data.PlatformData);
					original.SaveTo(data);
				}
				Program.Form.TaskMutex.ReleaseMutex();

				using (DelayedStreamCache cache = new DelayedStreamCache()) {
					AudioFormat audioformat = (data.GetFormat(FormatType.Audio) as IAudioFormat).DecodeAudio(data, progress);

					ushort[] masks = PlatformRB2WiiCustomDLC.RemixAudioTracks(data.Song, audioformat);

					progress.Progress();

					// RBN Required DTA settings
					SongsDTA dta = PlatformRB2WiiCustomDLC.GetSongsDTA(data.Song, audioformat);
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
					CryptedMoggStream mogg = new CryptedMoggStream(rba.Audio);
					mogg.WriteHeader();
					RawkAudio.Encoder encoder = new RawkAudio.Encoder(mogg, audioformat.Mappings.Count, audioformat.Decoder.SampleRate);

					progress.Progress();

					progress.SetNextWeight(10);
					long samples = audioformat.Decoder.Samples;
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

					IChartFormat chartformat = data.GetFormat(FormatType.Chart) as IChartFormat;
					NoteChart chart = PlatformRB2WiiCustomDLC.AdjustChart(data.Song, audioformat, chartformat.DecodeChart(data, progress));
					rba.Chart = new TemporaryStream();
					cache.AddStream(rba.Chart);
					chart.ToMidi().ToMid().Save(rba.Chart);

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
					stream.Close();

					if (data != original)
						data.Dispose();

					progress.Progress();
					progress.EndTask();
				}
			});
		}
	}
}
