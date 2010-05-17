using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using ConsoleHaxx.Common;
using System.IO;
using System.Text.RegularExpressions;

namespace ConsoleHaxx.Harmonix
{
	public class Chart
	{
		public NoteChart NoteChart;

		public Midi.Track EventsTrack;

		public Dictionary<BaseTrackType, NoteChart.Instrument> Tracks;

		public Dictionary<string, string> Song;

		public Chart()
		{
			NoteChart = new NoteChart();
			Song = new Dictionary<string, string>();
		}

		public NoteChart GetChart(bool coop = false)
		{
			Midi eventsmidi = new Midi();
			eventsmidi.BPM = NoteChart.BPM;
			eventsmidi.Signature = NoteChart.Signature;
			eventsmidi.Division = NoteChart.Division;
			eventsmidi.Tracks.Add(EventsTrack);
			if (Song.ContainsKey("Name"))
				eventsmidi.Name = Song["Name"];
			NoteChart chart = NoteChart.Create(eventsmidi);
			if (coop) {
				if (Tracks.ContainsKey(BaseTrackType.DoubleGuitar))
					chart.PartGuitar = Tracks[BaseTrackType.DoubleGuitar] as NoteChart.Guitar;
				if (Tracks.ContainsKey(BaseTrackType.DoubleBass))
					chart.PartBass = Tracks[BaseTrackType.DoubleBass] as NoteChart.Bass;
			} else {
				if (Tracks.ContainsKey(BaseTrackType.Single))
					chart.PartGuitar = Tracks[BaseTrackType.Single] as NoteChart.Guitar;
				//if (Tracks.ContainsKey(BaseTrackType.SingleBass))
				//	chart.PartBass = Tracks[BaseTrackType.SingleBass] as NoteChart.Bass;
			}

			return chart;
		}

		public static string[] ParseLine(string line)
		{
			List<string> symbols = new List<string>();
			string symbol = string.Empty;
			bool quoted = false;
			for (int i = 0; i < line.Length; i++) {
				char chr = line[i];
				if (chr == '"')
					quoted = !quoted;
				else if (chr == ' ') {
					if (symbol.Length > 0) {
						symbols.Add(symbol);
						symbol = string.Empty;
					}
				} else
					symbol += chr;
			}
			if (symbol.Length > 0)
				symbols.Add(symbol);
			return symbols.ToArray();
		}

		public static Chart Create(Stream stream)
		{
			Chart chart = new Chart();

			StreamReader reader = new StreamReader(stream);

			string line;
			string section = null;
			TrackType track = TrackType.Unknown;
			List<NoteChart.Note>[] notes = null;
			NoteChart.Instrument instrument = null;
			while ((line = reader.ReadLine()) != null) {
				if (line == "}") {
					section = null;
				} else if (line == "{")
					continue;
				Match match = Regex.Match(line, "\\[(?'section'.+)\\]");
				if (match.Success) {
					section = match.Groups["section"].Value;
					track = GetTrackType(section);
					BaseTrackType type = GetBaseTrackType(track);
					instrument = chart.CreateInstrument(GetInstrument(type));
					chart.Tracks[type] = instrument;
					notes = (instrument as NoteChart.IGems).Gems[GetDifficulty(track)];
				} else {
					ulong time;
					string[] values = ParseLine(line);
					switch (section) {
						case null:
							throw new FormatException();
						case "Song":
							chart.Song[values[0]] = values[1];
							break;
						case "SyncTrack":
							time = ulong.Parse(values[0]);
							switch (values[1]) {
								case "TS":
									chart.NoteChart.Signature.Add(new Midi.TimeSignatureEvent(time, byte.Parse(values[2]), 2, 24, 8));
									break;
								case "B":
									chart.NoteChart.BPM.Add(new Midi.TempoEvent(time, (uint)(Mid.MicrosecondsPerMinute / ((float)int.Parse(values[2]) / 1000))));
									break;
								default:
									throw new FormatException();
							}
							break;
						case "Events":
							time = ulong.Parse(values[0]);
							switch (values[1]) {
								case "E":
									chart.EventsTrack.Comments.Add(new Midi.TextEvent(time, "[" + values[2] + "]"));
									break;
								default:
									throw new FormatException();
							}
							break;
						default:
							if (track == TrackType.Unknown)
								throw new FormatException();
							time = ulong.Parse(values[0]);
							ulong duration = ulong.Parse(values[2]);
							if (duration == 0)
								duration = chart.NoteChart.Division.TicksPerBeat / 4U;
							switch (values[1]) {
								case "N":
									int fret = int.Parse(values[1]);
									notes[fret].Add(new NoteChart.Note(time, duration));
									break;
								case "S":
									NoteChart.Note note = new NoteChart.Note(time, duration);
									int sectiontype = int.Parse(values[1]);
									switch (sectiontype) {
										case 0:
											instrument.Player1.Add(note);
											break;
										case 1:
											instrument.Player2.Add(note);
											break;
										case 2:
											instrument.Overdrive.Add(note);
											break;
										default:
											throw new FormatException();
									}
									break;
							}
							break;
					}
				}
			}

			return chart;
		}

		public enum BaseTrackType
		{
			Unknown = 0,
			Single,
			DoubleGuitar,
			DoubleBass
		}

		public enum TrackType
		{
			Unknown = 0,
			EasySingle,
			EasyDoubleGuitar,
			EasyDoubleBass,
			MediumSingle,
			MediumDoubleGuitar,
			MediumDoubleBass,
			HardSingle,
			HardDoubleGuitar,
			HardDoubleBass,
			ExpertSingle,
			ExpertDoubleGuitar,
			ExpertDoubleBass
		}

		public static Dictionary<TrackType, string> TrackNames = new Dictionary<TrackType, string>() {
			{ TrackType.EasySingle, "EasySingle" },
			{ TrackType.EasyDoubleGuitar, "EasyDoubleGuitar" },
			{ TrackType.EasyDoubleBass, "EasyDoubleBass" },
			{ TrackType.MediumSingle, "MediumSingle" },
			{ TrackType.MediumDoubleGuitar, "MediumDoubleGuitar" },
			{ TrackType.MediumDoubleBass, "MediumDoubleBass" },
			{ TrackType.HardSingle, "HardSingle" },
			{ TrackType.HardDoubleGuitar, "HardDoubleGuitar" },
			{ TrackType.HardDoubleBass, "HardDoubleBass" },
			{ TrackType.ExpertSingle, "ExpertSingle" },
			{ TrackType.ExpertDoubleGuitar, "ExpertDoubleGuitar" },
			{ TrackType.ExpertDoubleBass, "ExpertDoubleBass" }
		};

		public static TrackType GetTrackType(string name)
		{
			if (TrackNames.ContainsValue(name))
				return TrackNames.FirstOrDefault(p => p.Value == name).Key;
			return TrackType.Unknown;
		}

		public static string GetTrackType(TrackType type)
		{
			if (TrackNames.ContainsKey(type))
				return TrackNames[type];
			return null;
		}

		public static BaseTrackType GetBaseTrackType(TrackType type)
		{
			switch (type) {
				case TrackType.EasySingle:
				case TrackType.MediumSingle:
				case TrackType.HardSingle:
				case TrackType.ExpertSingle:
					return BaseTrackType.Single;
				case TrackType.EasyDoubleGuitar:
				case TrackType.MediumDoubleGuitar:
				case TrackType.HardDoubleGuitar:
				case TrackType.ExpertDoubleGuitar:
					return BaseTrackType.DoubleGuitar;
				case TrackType.EasyDoubleBass:
				case TrackType.MediumDoubleBass:
				case TrackType.HardDoubleBass:
				case TrackType.ExpertDoubleBass:
					return BaseTrackType.DoubleBass;
			}

			return BaseTrackType.Unknown;
		}

		public static NoteChart.Difficulty GetDifficulty(TrackType type)
		{
			switch (type) {
				case TrackType.EasySingle:
				case TrackType.EasyDoubleGuitar:
				case TrackType.EasyDoubleBass:
					return NoteChart.Difficulty.Easy;
				case TrackType.MediumSingle:
				case TrackType.MediumDoubleGuitar:
				case TrackType.MediumDoubleBass:
					return NoteChart.Difficulty.Medium;
				case TrackType.HardSingle:
				case TrackType.HardDoubleGuitar:
				case TrackType.HardDoubleBass:
					return NoteChart.Difficulty.Hard;
				case TrackType.ExpertSingle:
				case TrackType.ExpertDoubleGuitar:
				case TrackType.ExpertDoubleBass:
					return NoteChart.Difficulty.Expert;
				default:
					break;
			}

			return (NoteChart.Difficulty)(-1);
		}

		public static NoteChart.TrackType GetInstrument(BaseTrackType type)
		{
			switch (type) {
				case BaseTrackType.DoubleGuitar:
				case BaseTrackType.Single:
					return NoteChart.TrackType.Guitar;
				case BaseTrackType.DoubleBass:
					return NoteChart.TrackType.Bass;
			}
			return (NoteChart.TrackType)(0xFF);
		}

		public NoteChart.Instrument CreateInstrument(NoteChart.TrackType instrument)
		{
			switch (instrument) {
				case NoteChart.TrackType.Guitar:
					return new NoteChart.Guitar(NoteChart);
				case NoteChart.TrackType.Bass:
					return new NoteChart.Bass(NoteChart);
				case NoteChart.TrackType.Drums:
					return new NoteChart.Drums(NoteChart);
				case NoteChart.TrackType.Vocals:
					return new NoteChart.Vocals(NoteChart);
			}
			return null;
		}
	}
}
