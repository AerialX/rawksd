using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using ConsoleHaxx.FreeStyleGames;
using ConsoleHaxx.Harmonix;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.RawkSD
{
	public class ChartFormatFsgMub : IChartFormat
	{
		public const string ChartName = "chart";

		public static ChartFormatFsgMub Instance;
		public static void Initialise()
		{
			Instance = new ChartFormatFsgMub();
			Platform.AddFormat(Instance);
		}

		public override int ID { get { return 0x41; } }

		public override string Name { get { return "DJ Hero Chart"; } }

		public override bool Writable { get { return false; } }

		public override bool Readable { get { return true; } }

		public override ChartFormat DecodeChart(FormatData data, ProgressIndicator progress)
		{
			IList<Stream> streams = GetChartStreams(data);

			float bpm = 0;

			NoteChart chart = new NoteChart();
			chart.BPM.Add(new Midi.TempoEvent(0, (uint)(Mid.MicrosecondsPerMinute / bpm)));
			chart.Signature.Add(new Midi.TimeSignatureEvent(0, 4, 2, 24, 8));

			Mub mub = null; // TODO: Determine charts by filenames or some shit
			chart.PartGuitar = new NoteChart.Guitar(chart);
			NoteChart.Instrument instrument = chart.PartGuitar;

			NoteChart.IGems gems = instrument as NoteChart.IGems;
			NoteChart.IForcedHopo hopo = instrument as NoteChart.IForcedHopo;

			NoteChart.Difficulty difficulty = NoteChart.Difficulty.Expert;
			int fadeposition = 0;
			foreach (Mub.Node node in mub.Nodes) {
				ulong time = (ulong)(node.Time * chart.Division.TicksPerBeat / 4);
				ulong duration = (ulong)(node.Duration * chart.Division.TicksPerBeat / 4);
				NoteChart.Note fullnote = new NoteChart.Note(time, duration);
				NoteChart.Note note = new NoteChart.Note(time, chart.Division.TicksPerBeat / 4U);
				NoteChart.Note prevnote = new NoteChart.Note(time - chart.Division.TicksPerBeat / 2U, chart.Division.TicksPerBeat / 4U);
				int greennote = fadeposition < 0 ? 0 : 1;
				int bluenote = fadeposition > 0 ? 4 : 3;

				switch (node.Type) {
					case 0x00: // Green dot
						gems.Gems[difficulty][greennote].Add(note);
						hopo.ForceHammeron[difficulty].Add(note);
						break;
					case 0x01: // Blue dot
						gems.Gems[difficulty][bluenote].Add(note);
						hopo.ForceHammeron[difficulty].Add(note);
						break;
					case 0x02: // Red dot
						gems.Gems[difficulty][2].Add(note);
						hopo.ForceHammeron[difficulty].Add(note);
						break;
					case 0x09: // Blue Crossfade right
						fadeposition = 1;
						gems.Gems[difficulty][4].Add(note);
						gems.Gems[difficulty][fadeposition < 0 ? 0 : 3].Add(prevnote);
						hopo.ForceHammeron[difficulty].Add(note);
						break;
					case 0x0A: // Green Crossfade left (revert to rightish normal)
						fadeposition = 0;
						break;
					case 0x0B: // Green Crossfade left
						fadeposition = -1;
						gems.Gems[difficulty][0].Add(note);
						gems.Gems[difficulty][fadeposition > 0 ? 4 : 1].Add(prevnote);
						hopo.ForceHammeron[difficulty].Add(note);
						break;
					case 0x0C: // Weird whammy thing on left green
						gems.Gems[difficulty][greennote].Add(fullnote);
						hopo.ForceHammeron[difficulty].Add(note);
						break;
					case 0x0D: // ?? Not sure, Weird whammy thing on right blue maybe
						gems.Gems[difficulty][bluenote].Add(fullnote);
						hopo.ForceHammeron[difficulty].Add(note);
						break;
					default:
						break;
				}
			}

			return new ChartFormat(chart);
		}

		public void Create(FormatData data, Stream[] streams)
		{
			for (int i = 0; i < streams.Length; i++)
				data.SetStream(this, ChartName + (i == 0 ? string.Empty : ("." + i.ToString())), streams[i]);
		}

		public IList<Stream> GetChartStreams(FormatData data)
		{
			List<Stream> streams = new List<Stream>();

			int i = 0;
			string name = ChartName;
			do {
				streams.Add(data.GetStream(this, name));
				i++;
				name = ChartName + "." + i.ToString();
			} while (data.HasStream(this, name));

			return streams;
		}

		public override void EncodeChart(ChartFormat data, FormatData destination, ProgressIndicator progress)
		{
			throw new NotImplementedException();
		}
	}
}
