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

		public static ChartFormatGH2 Instance;
		public static void Initialise()
		{
			Instance = new ChartFormatGH2();
			Platform.AddFormat(Instance);
		}

		public override int ID {
			get { return 0x43; }
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

			DecodeCoop(midi, data.Song.Data.GetValue<bool>("GH2ChartCoop"));

			ChartFormat chart = new ChartFormat(NoteChart.Create(midi));

			DecodeDrums(chart.Chart, midi, false);
			DecodeOverdrive(chart.Chart);

			return chart;
		}

		private void DecodeCoop(Midi midi, bool coop)
		{
			Midi.Track cooptrack = midi.GetTrack("PART GUITAR COOP");
			if (cooptrack != null && (coop || midi.GetTrack("PART GUITAR") == null)) {
				midi.RemoveTracks("PART GUITAR");
				cooptrack.Name = "PART GUITAR";
			}
			cooptrack = midi.GetTrack("PART RHYTHM");
			if (cooptrack != null && (coop || midi.GetTrack("PART BASS") == null)) {
				midi.RemoveTracks("PART BASS");
				cooptrack.Name = "PART BASS";
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

		public static void DecodeDrums(NoteChart chart, Midi midi, bool gh1)
		{
			chart.PartDrums = new NoteChart.Drums(chart);

			Midi.Track track;
			if (gh1)
				track = midi.GetTrack("TRIGGERS");	
			else
				track = midi.GetTrack("BAND DRUMS");

			foreach (Midi.NoteEvent note in track.Notes) {
				switch (note.Note) {
					case 60:
					case 36: // Kick
						chart.PartDrums.Gems[NoteChart.Difficulty.Expert][0].Add(new NoteChart.Note(note));
						break;
					case 61:
					case 37: // Crash
						chart.PartDrums.Gems[NoteChart.Difficulty.Expert][4].Add(new NoteChart.Note(note));
						break;
					case 0x30:
					case 0x31:
					case 0x40:
					case 0x41:
						break;
				}
			}
			
			if (gh1)
				track = midi.GetTrack("EVENTS");

			Midi.TextEvent previouscomment = new Midi.TextEvent(0, "[nobeat]");
			string previoustext = "nobeat";
			foreach (var comment in track.Comments) {
				NoteChart.Point note = new NoteChart.Point(comment.Time);

				string text = comment.Text.Trim('[', ']', ' ');

				if (gh1) {
					if (text.StartsWith("drum_"))
						text = text.Substring(5);
					else
						continue;
				}

				switch (text) {
					case "idle":
					case "off":
					case "noplay":
						chart.PartDrums.CharacterMoods.Add(new Pair<NoteChart.Point, NoteChart.CharacterMood>(note, NoteChart.CharacterMood.Idle));
						break;
					case "play":
					case "normal":
					case "on":
						chart.PartDrums.CharacterMoods.Add(new Pair<NoteChart.Point, NoteChart.CharacterMood>(note, NoteChart.CharacterMood.Play));
						previouscomment = comment;
						break;
					default:
						ulong duration = comment.Time - previouscomment.Time;
						ulong time = previouscomment.Time;
						float fraction = 0;
						switch (previoustext) {
							case "on":
							case "allplay":
							case "play":
							case "allbeat":
							case "allbreat":
							case "all_beat":
							case "normal":
							case "norm":
							case "nomral":
							case "normal_tempo":
								fraction = 1;
								break;
							case "off":
							case "noplay":
							case "nobeat":
							case "no_beat":
								fraction = 0;
								break;
							case "double":
							case "double_time":
							case "doubletime":
							case "double_tempo":
							case "doulbe_time":
								fraction = 0.5f;
								break;
							case "half":
							case "half_time":
							case "halftime":
							case "half_tempo":
								fraction = 2;
								break;
						}
						if (fraction > 0) {
							while (time < comment.Time) {
								chart.PartDrums.Gems[NoteChart.Difficulty.Expert][(previoustext == "play" || previoustext == "normal") ? 2 : 1].Add(new NoteChart.Note(time));

								time += (ulong)(midi.Division.TicksPerBeat * fraction);
							}
						}
						previouscomment = comment;
						previoustext = text;
						break;
				}
			}

			ChartFormatGH5.FillSections(chart, 1, 8, 1, chart.PartDrums.Overdrive, null);
			ChartFormatGH5.FillSections(chart, 1, 4, 3, chart.PartDrums.DrumFills, null);
		}

		public override void EncodeChart(ChartFormat data, FormatData destination, ProgressIndicator progress)
		{
			throw new NotImplementedException();
		}
	}
}
