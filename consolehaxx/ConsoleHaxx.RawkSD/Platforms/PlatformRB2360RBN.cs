using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using ConsoleHaxx.Common;
using ConsoleHaxx.Harmonix;

namespace ConsoleHaxx.RawkSD
{
	public class PlatformRB2360RBN : Engine
	{
		public static PlatformRB2360RBN Instance;
		public static void Initialise()
		{
			Instance = new PlatformRB2360RBN();

			PlatformDetection.AddSelection(Instance);
			PlatformDetection.DetectFile += new Action<string, Stream, List<Pair<Engine, Game>>>(PlatformDetection_DetectFile);
		}

		static void PlatformDetection_DetectFile(string path, Stream stream, List<Pair<Engine, Game>> platforms)
		{
			try {
				stream.Position = 0;
				RBA rba = new RBA(new EndianReader(stream, Endianness.LittleEndian));
				platforms.Add(new Pair<Engine, Game>(Instance, Game.Unknown));
			} catch { }
		}

		public override int ID { get { throw new NotImplementedException(); } }

		public override string Name { get { return "Rock Band Network File"; } }

		public override bool AddSong(PlatformData data, SongData song, ProgressIndicator progress)
		{
			FormatData formatdata = new TemporaryFormatData(song, data);

			RBA rba = data.Session["rba"] as RBA;

			AudioFormat audio = HarmonixMetadata.GetAudioFormat(song);
			AudioFormatMogg.Instance.Create(formatdata, rba.Audio, audio);

			ChartFormatRB.Instance.Create(formatdata, rba.Chart, null, 0, rba.Weights, rba.Milo, ChartFormatRB.Milo3Version, false, false);

			//song.AlbumArt = XboxImage.Create(new EndianReader(albumfile.Data, Endianness.LittleEndian)).Bitmap;

			data.AddSong(formatdata);

			return true;
		}

		public override PlatformData Create(string path, Game game, ProgressIndicator progress)
		{
			PlatformData data = new PlatformData(this, game);
			data.Session["path"] = path;

			if (Directory.Exists(path))
				Exceptions.Error("An RBN archive must be a file.");

			if (File.Exists(path)) {
				try {
					RBA rba = new RBA(new EndianReader(new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.Read), Endianness.LittleEndian));
					data.Session["rba"] = rba;

					SongData song = HarmonixMetadata.GetSongData(data, DTA.Create(rba.Data));

					song.ID = ImportMap.GetShortName(song.Name);

					AddSong(data, song, progress);
				} catch (Exception exception) {
					Exceptions.Error(exception, "An error occurred while opening the RBN archive.");
				}
			}

			return data;
		}

		public override FormatData CreateSong(PlatformData data, SongData song)
		{
			return new TemporaryFormatData(song, data);
		}

		public override void SaveSong(PlatformData data, FormatData formatdata, ProgressIndicator progress)
		{
			base.SaveSong(data, formatdata, progress);
		}
	}
}
