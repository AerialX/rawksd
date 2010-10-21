using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.RawkSD
{
	public class ChartFormatRB : IChartFormat
	{
		public const string ChartFile = "chart";
		public const string PanFile = "pan";
		public const string Pan1File = "pan1";
		public const string WeightsFile = "weights";
		public const string MiloFile = "milo";
		public const string Milo3File = "milo3";

		public const int MiloVersion = 0;
		public const int Milo3Version = 1;
		public const int PanVersion = 1;
		public const int Pan1Version = 0;

		public static ChartFormatRB Instance;
		public static void Initialise()
		{
			Instance = new ChartFormatRB();
			Platform.AddFormat(Instance);
		}

		public override int ID {
			get { return 0x47; }
		}

		public override string Name {
			get { return "Rock Band MIDI Chart"; }
		}

		public void Create(FormatData data, Stream chart, Stream pan, Stream weights, Stream milo, bool expertplus, bool fixforquickplay, Game game)
		{
			int panversion = MiloVersion;
			int miloversion = Pan1Version;

			if (!HarmonixMetadata.IsRockBand1(game))
				panversion = PanVersion;
			if (game == Game.RockBandGreenDay)
				miloversion = Milo3Version;
			Create(data, chart, pan, panversion, weights, milo, miloversion, expertplus, fixforquickplay);
		}

		public void Create(FormatData data, Stream chart, Stream pan, int panversion, Stream weights, Stream milo, int miloversion, bool expertplus, bool fixforquickplay)
		{
			data.SetStream(this, ChartFile, chart);
			data.SetStream(this, panversion == Pan1Version ? Pan1File : PanFile, pan);
			data.SetStream(this, WeightsFile, weights);
			data.SetStream(this, miloversion == Milo3Version ? Milo3File : MiloFile, milo);

			data.Song.Data.SetValue("RBChartExpertPlus", expertplus);
			data.Song.Data.SetValue("RBChartFixForQuickplay", fixforquickplay);
		}

		public override bool Writable {
			get { return true; }
		}

		public override bool Readable {
			get { return true; }
		}

		public override ChartFormat DecodeChart(FormatData data, ProgressIndicator progress)
		{
			if (!data.HasStream(this, ChartFile))
				throw new FormatException();

			Stream stream = data.GetStream(this, ChartFile);
			Midi midi = Midi.Create(Mid.Create(stream));
			if (data.Song.Data.GetValue<bool>("RBChartExpertPlus")) {
				Midi.Track track = midi.GetTrack("PART DRUMS");
				if (track != null) {
					foreach (Midi.NoteEvent note in track.Notes) {
						if (note.Note == 95)
							note.Note = 96;
					}
				}
			}
			ChartFormat chart = ChartFormat.Create(midi);
			data.CloseStream(stream);
			return chart;
		}

		public override void EncodeChart(ChartFormat data, FormatData destination, ProgressIndicator progress)
		{
			Stream stream = destination.AddStream(this, ChartFile);
			data.Save(stream);
			destination.CloseStream(stream);
		}

		public bool NeedsFixing(FormatData data)
		{
			if (data.Song.Data.GetValue<bool>("RBChartFixForQuickplay") || data.Song.Data.GetValue<bool>("RBChartExpertPlus"))
				return true;

			ChartFormat chart = DecodeChart(data, new ProgressIndicator());
			if (chart.Chart.PartVocals != null) {
				foreach (var lyric in chart.Chart.PartVocals.Lyrics) {
					if (lyric.Value.EndsWith("*"))
						return true;
				}
			}

			return false;
		}
	}
}
