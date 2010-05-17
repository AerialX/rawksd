using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using ConsoleHaxx.Common;
using ConsoleHaxx.Harmonix;

namespace ConsoleHaxx.RawkSD
{
	public class ChartFormatGH2 : IChartFormat
	{
		public const string ChartFile = "chart";

		public static readonly ChartFormatGH2 Instance;
		static ChartFormatGH2()
		{
			Instance = new ChartFormatGH2();
			Platform.AddFormat(Instance);
		}

		public override int ID {
			get { return 0x02; }
		}

		public override string Name {
			get { return "Guitar Hero 2 / 80s MIDI Chart"; }
		}

		public void Create(FormatData data, Stream chart, bool coop)
		{
			data.SetStream(this, ChartFile, chart);

			data.Song.Data.SetValue("GH2ChartCoop", coop);
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

			if (data.Song.Data.GetValue<bool>("GH2ChartCoop"))
				DecodeCoop(midi);

			ChartFormat chart = new ChartFormat(NoteChart.Create(midi));

			DecodeDrums(chart.Chart, midi);
			DecodeOverdrive(chart.Chart);

			return chart;
		}

		private void DecodeCoop(Midi midi)
		{
			Midi.Track coop = midi.Tracks.Find(t => t.Name == "PART GUITAR COOP");
			if (coop != null) {
				midi.Tracks.RemoveAll(t => t.Name == "PART GUITAR");
				coop.Name = "PART GUITAR";
			}
			coop = midi.Tracks.Find(t => t.Name == "PART RHYTHM");
			if (coop != null) {
				midi.Tracks.RemoveAll(t => t.Name == "PART BASS");
				coop.Name = "PART BASS";
			}
		}

		public static void DecodeOverdrive(NoteChart chart)
		{
			if (chart.PartGuitar != null) {
				chart.PartGuitar.Overdrive.AddRange(chart.PartGuitar.SoloSections);
				chart.PartGuitar.SoloSections.Clear();
			}
			if (chart.PartBass != null) {
				chart.PartBass.Overdrive.AddRange(chart.PartBass.SoloSections);
				chart.PartBass.SoloSections.Clear();
			}
		}

		private void DecodeDrums(NoteChart chart, Midi midi)
		{
			chart.PartDrums = new NoteChart.Drums(chart);

			Midi.Track track = midi.Tracks.Find(t => t.Name == "BAND DRUMS");
			foreach (Midi.NoteEvent note in track.Notes) {
				switch (note.Note) {
					case 36: // Kick
						chart.PartDrums.Gems[NoteChart.Difficulty.Expert][0].Add(new NoteChart.Note(note));
						break;
					case 37: // Crash
						chart.PartDrums.Gems[NoteChart.Difficulty.Expert][4].Add(new NoteChart.Note(note));
						break;
					default:
						throw new FormatException();
				}
			}

			Midi.TextEvent previouscomment = new Midi.TextEvent(0, "[nobeat]");
			foreach (var comment in track.Comments) {
				NoteChart.Point note = new NoteChart.Point(comment.Time);

				switch (comment.Text.Trim()) {
					case "[idle]":
						chart.PartDrums.CharacterMoods.Add(new Pair<NoteChart.Point, NoteChart.CharacterMood>(note, NoteChart.CharacterMood.Idle));
						break;
					case "[play]":
						chart.PartDrums.CharacterMoods.Add(new Pair<NoteChart.Point, NoteChart.CharacterMood>(note, NoteChart.CharacterMood.Play));
						previouscomment = comment;
						break;
					default:
						ulong duration = comment.Time - previouscomment.Time;
						ulong time = previouscomment.Time;
						float fraction;
						switch (previouscomment.Text.Trim()) {
							case "[play]":
							case "[allbeat]":
								fraction = 1;
								break;
							case "[nobeat]":
								fraction = 0;
								break;
							case "[double_time]":
								fraction = 0.5f;
								break;
							case "[half_time]":
								fraction = 2;
								break;
							default:
								throw new FormatException();
						}
						if (fraction > 0) {
							while (time < comment.Time) {
								chart.PartDrums.Gems[NoteChart.Difficulty.Expert][previouscomment.Text.Trim() == "[play]" ? 2 : 1].Add(new NoteChart.Note(time));

								time += (ulong)(midi.Division.TicksPerBeat * fraction);
							}
						}
						previouscomment = comment;
						break;
				}
			}
		}

		public override void EncodeChart(ChartFormat data, FormatData destination, ProgressIndicator progress)
		{
			throw new NotImplementedException();
		}
	}
}
