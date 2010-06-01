using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using ConsoleHaxx.Harmonix;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.RawkSD
{
	public class ChartFormatGH1 : IChartFormat
	{
		public const string ChartFile = "chart";

		public static ChartFormatGH1 Instance;
		public static void Initialise()
		{
			Instance = new ChartFormatGH1();
			Platform.AddFormat(Instance);
		}

		public override int ID {
			get { return 0x42; }
		}

		public override string Name {
			get { return "Guitar Hero 1 MIDI Chart"; }
		}

		public void Create(FormatData data, Stream chart)
		{
			data.SetStream(this, ChartFile, chart);
		}

		public override bool Writable {
			get { return false; }
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
			data.CloseStream(stream);

			ChartFormat chart = new ChartFormat(NoteChart.Create(midi));

			DecodeLeftHandAnimations(chart.Chart, midi);
			ChartFormatGH2.DecodeDrums(chart.Chart, midi, true);
			ChartFormatGH2.DecodeOverdrive(chart.Chart);

			ImportMap.ImportChart(data.Song, chart.Chart);

			return chart;
		}

		public static void DecodeLeftHandAnimations(NoteChart chart, Midi midi)
		{
			Midi.Track track = midi.GetTrack("ANIM");
			foreach (var note in track.Notes) {
				if (note.Note < 60 && note.Note >= 40)
					chart.PartGuitar.FretPosition.Add(new Pair<NoteChart.Note, byte>(new NoteChart.Note(note), (byte)(note.Note - 40)));
			}
		}

		public override void EncodeChart(ChartFormat data, FormatData destination, ProgressIndicator progress)
		{
			throw new NotImplementedException();
		}
	}
}
