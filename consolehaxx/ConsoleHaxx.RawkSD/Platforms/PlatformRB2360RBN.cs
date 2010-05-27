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
				platforms.Add(new Pair<Engine, Game>(Instance, Game.RockBand2));
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

			Stream milostream = rba.Milo;
			milostream = new TemporaryStream();
			Milo milo = new Milo(new EndianReader(rba.Milo, Endianness.LittleEndian));
			FaceFX fx = new FaceFX(new EndianReader(milo.Data[0], Endianness.BigEndian));

			TemporaryStream fxstream = new TemporaryStream();
			fx.Save(new EndianReader(fxstream, Endianness.LittleEndian));
			milo.Data[0] = fxstream;
			milo.Compressed = true;
			milo.Save(new EndianReader(milostream, Endianness.LittleEndian));

			ChartFormatRB.Instance.Create(formatdata, rba.Chart, null, rba.Weights, milostream, false, false);

			//song.AlbumArt = XboxImage.Create(new EndianReader(albumfile.Data, Endianness.LittleEndian)).Bitmap;

			data.AddSong(formatdata);

			return true;
		}

		public override PlatformData Create(string path, Game game, ProgressIndicator progress)
		{
			PlatformData data = new PlatformData(this, game);
			data.Session["path"] = path;

			if (Directory.Exists(path))
				throw new FormatException();

			if (File.Exists(path)) {
				RBA rba = new RBA(new EndianReader(new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.Read), Endianness.LittleEndian));
				data.Session["rba"] = rba;

				SongData song = HarmonixMetadata.GetSongData(data, DTA.Create(rba.Data));

				song.ID = ImportMap.GetShortName(song.Name);

				AddSong(data, song, progress);
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
