using System;
using System.Collections.Generic;
using System.Text;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.Harmonix
{
	public class NoteChart
	{
		string Name;

		public Guitar PartGuitar;
		public Bass PartBass;
		public Drums PartDrums;
		public Vocals PartVocals;
		public BeatTrack Beat;
		public VenueTrack Venue;
		public EventsTrack Events;

		public List<Midi.TempoEvent> BPM;
		public List<Midi.TimeSignatureEvent> Signature;
		public Mid.TicksPerBeatDivision Division;

		public Point BigRockEnding;

		public Dictionary<string, string> Metadata;

		//UnisonBonus;

		public const ushort DefaultTicksPerBeat = 480;

		public NoteChart()
		{
			Metadata = new Dictionary<string, string>();
			BPM = new List<Midi.TempoEvent>();
			Signature = new List<Midi.TimeSignatureEvent>();
			Division = new Mid.TicksPerBeatDivision(DefaultTicksPerBeat);
		}

		public ulong GetTime(ulong ticks)
		{
			ulong time = 0;
			Midi.TempoEvent previous = new Midi.TempoEvent(0, Mid.MicrosecondsPerMinute / 120);
			Midi.TempoEvent next;

			int index = -1;
			ulong t;

			while (ticks > 0) {
				if (index + 1 == BPM.Count)
					next = new Midi.TempoEvent(ulong.MaxValue, 0);
				else
					next = BPM[++index];

				t = Math.Min(next.Time - previous.Time, ticks);
				ticks -= t;
				time += t * previous.MicrosecondsPerBeat / Division.TicksPerBeat;

				previous = next;
			}

			return time;
		}

		public ulong GetTimeDuration(ulong time, ulong duration)
		{
			return GetTime(time + duration) - GetTime(time);
		}

		public ulong GetTicks(ulong time)
		{
			time *= 1000; // To microseconds
			ulong ticks = 0;

			Midi.TempoEvent previous = new Midi.TempoEvent(0, Mid.MicrosecondsPerMinute / 120);
			Midi.TempoEvent next;

			int index = -1;
			ulong t;

			while (time > 0) {
				if (index + 1 == BPM.Count)
					next = new Midi.TempoEvent(ulong.MaxValue, 0);
				else
					next = BPM[++index];

				t = Math.Min((next.Time - previous.Time) * previous.MicrosecondsPerBeat / Division.TicksPerBeat, time);
				time -= t;
				ticks += t * Division.TicksPerBeat / previous.MicrosecondsPerBeat;

				previous = next;
			}

			return ticks;
		}

		public ulong GetTicksDuration(ulong time, ulong duration)
		{
			return GetTicks(time + duration) - GetTicks(time);
		}

		public Midi ToMidi()
		{
			Midi midi = new Midi();
			midi.BPM = BPM;
			midi.Division = Division;
			midi.Signature = Signature;

			if (PartDrums != null)
				midi.Tracks.Add(PartDrums.ToMidiTrack());
			if (PartBass != null)
				midi.Tracks.Add(PartBass.ToMidiTrack());
			if (PartGuitar != null)
				midi.Tracks.Add(PartGuitar.ToMidiTrack());
			if (PartVocals != null)
				midi.Tracks.Add(PartVocals.ToMidiTrack());
			if (Events != null)
				midi.Tracks.Add(Events.ToMidiTrack());
			if (Beat != null)
				midi.Tracks.Add(Beat.ToMidiTrack());
			if (Venue != null)
				midi.Tracks.Add(Venue.ToMidiTrack());
			midi.Name = Name;

			Random rand = new Random();
			foreach (KeyValuePair<string, string> meta in Metadata) {
				Midi.Track track = midi.Tracks[rand.Next(0, midi.Tracks.Count)];
				track.Comments.Add(new Midi.TextEvent((ulong)rand.Next(0, (int)track.FindLastNote()), "{" + meta.Key + " " + meta.Value + "}"));
			}

			return midi;
		}

		public static NoteChart Create(Midi midi)
		{
			NoteChart chart = new NoteChart();

			chart.Name = midi.Name;
			chart.BPM = midi.BPM;
			chart.Signature = midi.Signature;
			chart.Division = midi.Division;

			if (chart.Signature.Count == 0)
				chart.Signature.Add(new Midi.TimeSignatureEvent(0, 4, 2, 24, 8));

			chart.Venue = new VenueTrack(chart);
			chart.Events = new EventsTrack(chart);
			chart.Beat = new BeatTrack(chart);

			foreach (Midi.Track t in midi.Tracks) {
				Track track = null;
				string tname = t.Name;
				if (tname == null)
					tname = string.Empty;
				tname = tname.ToUpper();
				switch (tname) {
					case "PART GUITAR":
					case "T1 GEMS": // What the fuck is GH1 on?
						chart.PartGuitar = new Guitar(chart);
						track = chart.PartGuitar;
						break;
					case "PART BASS":
						chart.PartBass = new Bass(chart);
						track = chart.PartBass;
						break;
					case "PART DRUMS":
					case "PART DRUM":
						chart.PartDrums = new Drums(chart);
						track = chart.PartDrums;
						break;
					case "PART VOCALS":
						chart.PartVocals = new Vocals(chart);
						track = chart.PartVocals;
						break;
					case "VENUE":
						track = chart.Venue;
						break;
					case "EVENTS":
						track = chart.Events;
						break;
					case "BEAT":
						track = chart.Beat;
						break;
					default:
						if (midi.Tracks.Count == 1) {
							chart.PartGuitar = new Guitar(chart);
							track = chart.PartGuitar;
						}
						break;
				}

				if (track == null)
					continue;

				foreach (Midi.NoteEvent note in t.Notes) {
					for (var iter = NoteTypes.Keys.GetEnumerator(); iter.MoveNext(); ) {
						if ((iter.Current.Track & track.Type) != track.Type)
							continue;

						foreach (byte n in iter.Current.Notes) {
							if (n == note.Note) {
								NoteTypes[iter.Current](note, track); // Invoke "parser"
								break;
							}
						}
					}
				}

				foreach (Midi.TextEvent comment in t.Comments) {
					ParseComment(track, comment);
				}

				if (track is Vocals) {
					Vocals vox = track as Vocals;
					foreach (Midi.TextEvent lyric in t.Lyrics) {
						vox.Lyrics.Add(new Pair<Point, string>(new Point(lyric.Time), lyric.Text));
					}
				}
			}

			return chart;
		}

		private static void ParseComment(Track track, Midi.TextEvent comment)
		{
			string text = comment.Text.Trim();

			if (text.StartsWith("{")) {
				text = text.Substring(1, text.Length - 2);
				string[] split = text.Split(new char[] { ' ' }, 2, StringSplitOptions.RemoveEmptyEntries);
				track.Chart.Metadata.Add(split[0], split[1]);
			}

			if (text.StartsWith("["))
				text = text.Substring(1, text.Length - 2);

			switch (track.Type) {
				case TrackType.Guitar:
					Guitar guitar = track as Guitar;
					if (text.StartsWith("map HandMap_"))
						guitar.HandMap.Add(new Pair<Point, LeftHandMap>(new Point(comment.Time), NoteChartHelper.LeftHandMapFromString(text.Substring(12))));
					else
						guitar.CharacterMoods.Add(new Pair<Point, CharacterMood>(new Point(comment.Time), NoteChartHelper.CharacterMoodFromString(text)));
					break;
				case TrackType.Bass:
					Bass bass = track as Bass;
					if (text.StartsWith("map HandMap_"))
						bass.HandMap.Add(new Pair<Point, LeftHandMap>(new Point(comment.Time), NoteChartHelper.LeftHandMapFromString(text.Substring(12))));
					else if (text.StartsWith("map StrumMap_"))
						bass.StrumMap.Add(new Pair<Point, RightHandMap>(new Point(comment.Time), NoteChartHelper.RightHandMapFromString(text.Substring(13))));
					else
						bass.CharacterMoods.Add(new Pair<Point, CharacterMood>(new Point(comment.Time), NoteChartHelper.CharacterMoodFromString(text)));
					break;
				case TrackType.Drums:
					Drums drums = track as Drums;
					if (text.StartsWith("mix ")) {
						drums.Mixing[(Difficulty)int.Parse(text.Substring(4, 1))] = text.Substring(6);
					} else
						drums.CharacterMoods.Add(new Pair<Point, CharacterMood>(new Point(comment.Time), NoteChartHelper.CharacterMoodFromString(text)));
					break;
				case TrackType.Vocals:
					Vocals vocals = track as Vocals;
					if (text.StartsWith("tambourine") || text.StartsWith("cowbell") || text.StartsWith("clap"))
						vocals.PercussionSectionComments.Add(new Pair<Point, string>(new Point(comment.Time), text));
					else if (comment.Text.Trim().StartsWith("["))
						vocals.CharacterMoods.Add(new Pair<Point, CharacterMood>(new Point(comment.Time), NoteChartHelper.CharacterMoodFromString(text)));
					else // The game apparently uses comments as lyrics too >__>
						vocals.Lyrics.Add(new Pair<Point, string>(new Point(comment.Time), text));
					break;
				case TrackType.Beat:
					break;
				case TrackType.Events:
					EventsTrack events = track as EventsTrack;
					if (text.StartsWith("coda"))
						events.Chart.BigRockEnding = new Point(comment.Time);
					else if (text.StartsWith("section"))
						events.Sections.Add(new Pair<Point, string>(new Point(comment.Time), text.Substring(8)));
					else if (text.StartsWith("crowd"))
						events.Crowd.Add(new Pair<Point, EventsTrack.CrowdType>(new Point(comment.Time), NoteChartHelper.CrowdTypeFromString(text)));
					else if (text.StartsWith("music_start"))
						events.MusicStart = new Point(comment.Time);
					else if (text.StartsWith("music_end"))
						events.MusicEnd = new Point(comment.Time);
					else if (text.StartsWith("end"))
						events.End = new Point(comment.Time);
					break;
				case TrackType.Venue:
					VenueTrack venue = track as VenueTrack;
					if (text.StartsWith("lighting"))
						//venue.Lighting.Add(new Pair<Point, VenueTrack.LightingType>(new Point(comment.Time), VenueTrack.LightingFromText(text.Substring(9))));
						venue.Lighting.Add(new Pair<Point, VenueTrack.LightingType>(new Point(comment.Time), NoteChartHelper.LightingTypeFromString(text.Substring(10, text.Length - 10 - 1))));
					else if (text.StartsWith("do_directed_cut"))
						venue.DirectedShots.Add(new Pair<Point, VenueTrack.DirectedCut>(new Point(comment.Time), NoteChartHelper.DirectedCutFromString(text.Substring(16))));
					else if (text.StartsWith("do_optional_cut"))
						venue.OptionalShots.Add(new Pair<Point, VenueTrack.DirectedCut>(new Point(comment.Time), NoteChartHelper.DirectedCutFromString(text.Substring(16))));
					else if (text.StartsWith("verse"))
						venue.Verse.Add(new Point(comment.Time));
					else if (text.StartsWith("chorus"))
						venue.Chorus.Add(new Point(comment.Time));
					break;
				default:
					break;
			}
		}

		public enum TrackType : byte
		{
			Guitar = 1,
			Bass = 2,
			Drums = 4,
			Vocals = 8,
			Beat = 16,
			Events = 32,
			Venue = 64
		}

		public enum Difficulty
		{
			Easy = 0,
			Medium,
			Hard,
			Expert
		}

		public interface ISolo
		{ // Instruments that support solo sections
			List<Note> SoloSections { get; }
		}

		public interface IForcedHopo
		{ // Instruments that can have forced hammerons/pulloffs, or strums
			Dictionary<Difficulty, List<Note>> ForceHammeron { get; }
			Dictionary<Difficulty, List<Note>> ForceStrum { get; }
		}

		public interface IGems
		{
			Dictionary<Difficulty, List<Note>[]> Gems { get; }
		}

		public interface ICharacterMood
		{
			List<Pair<Point, CharacterMood>> CharacterMoods { get; }
		}

		public interface IHandMap
		{
			List<Pair<Point, LeftHandMap>> HandMap { get; }
		}

		public interface IStrumMap
		{
			List<Pair<Point, RightHandMap>> StrumMap { get; }
		}

		public interface IFretPosition
		{
			/// <summary>
			/// Must have a value from 0 to 19. 0 is the top of the nexk, 19 is the bottom nearest the guitar body.
			/// </summary>
			List<Pair<Note, byte>> FretPosition { get; }
		}

		public enum LeftHandMap
		{
			Default,
			NoChords,
			AllChords,
			Solo,
			DropD,
			DropD2,
			AllBend,
			ChordC,
			ChordD,
			ChordA
		}

		public enum RightHandMap
		{
			Default,
			Pick,
			SlapBass
		}

		public enum CharacterMood
		{
			None,
			Idle,
			IdleRealtime,
			IdleIntense,
			Mellow,
			Play,
			Intense
		}

		public abstract class Track
		{
			public Track(NoteChart chart)
			{
				Chart = chart;
			}

			public TrackType Type;
			public NoteChart Chart;

			public abstract Midi.Track ToMidiTrack();
		}

		public class BeatTrack : Track
		{
			public List<Note> High;
			public List<Note> Low;

			public BeatTrack(NoteChart chart)
				: base(chart)
			{
				Type = TrackType.Beat;

				High = new List<Note>();
				Low = new List<Note>();
			}

			public override Midi.Track ToMidiTrack()
			{
				Midi.Track track = new Midi.Track();
				track.Name = "BEAT";

				if (High.Count == 0 && Low.Count == 0) {
					for (int i = 0; i < Chart.Signature.Count; i++) {
						Midi.TimeSignatureEvent sig = Chart.Signature[i];

						uint k = 0;
						while (true) {
							Note note = new Note() { Time = sig.Time + Chart.Division.TicksPerBeat * k, Duration = (ulong)Chart.Division.TicksPerBeat / sig.Numerator };
							if ((i < Chart.Signature.Count - 1 && note.Time + note.Duration >= Chart.Signature[i + 1].Time) ||
								(note.Time >= Chart.Events.End.Time))
								break;

							if (k % sig.Numerator == 0)
								Low.Add(note);
							else
								High.Add(note);
							k++;
						}
					}
				}

				track.Notes.AddRange(Low.ConvertAll(n => n.ToMidiNote(12)));
				track.Notes.AddRange(High.ConvertAll(n => n.ToMidiNote(13)));
				return track;
			}
		}

		public class VenueTrack : Track
		{
			public List<Pair<Point, LightingType>> Lighting;
			public List<Pair<Point, DirectedCut>> DirectedShots;
			public List<Pair<Point, DirectedCut>> OptionalShots;
			public List<Pair<Point, PostProcess>> PostProcesses;
			public List<Pair<Point, CameraShot>> CameraShots;
			public List<Pair<Note, NoteChart.TrackType>> Spotlights;
			public List<Pair<Note, NoteChart.TrackType>> Singalong;
			public List<Pair<Point, LightingKeyframe>> LightingKeyframes;
			public List<Point> Pyrotechnics;
			public List<Point> OptionalPyrotechnics;
			public List<Point> Verse;
			public List<Point> Chorus;

			public VenueTrack(NoteChart chart)
				: base(chart)
			{
				Type = TrackType.Venue;

				Lighting = new List<Pair<Point, LightingType>>();
				DirectedShots = new List<Pair<Point, DirectedCut>>();
				OptionalShots = new List<Pair<Point, DirectedCut>>();
				PostProcesses = new List<Pair<Point, PostProcess>>();
				CameraShots = new List<Pair<Point, CameraShot>>();
				Spotlights = new List<Pair<Note, TrackType>>();
				Singalong = new List<Pair<Note, TrackType>>();
				LightingKeyframes = new List<Pair<Point, LightingKeyframe>>();

				Pyrotechnics = new List<Point>();
				OptionalPyrotechnics = new List<Point>();
				Verse = new List<Point>();
				Chorus = new List<Point>();
			}

			public override Midi.Track ToMidiTrack()
			{
				Midi.Track track = new Midi.Track();
				track.Name = "VENUE";

				Random random = new Random();
				if (CameraShots.Count == 0 && DirectedShots.Count == 0) {
					int end = (int)Chart.GetTime(Chart.Events.MusicEnd.Time);
					int time = 0;
					while (time < end) {
						Point point = new Point(Chart.GetTicks((ulong)time / 1000));
						if (random.Next() % 2 == 0)
							Lighting.Add(new Pair<Point, LightingType>(point, LightingType.None));

						if (random.Next() % 8 == 0)
							DirectedShots.Add(new Pair<Point, DirectedCut>(point, DirectedCut.None));
						else if (random.Next() % 2 == 0)
							CameraShots.Add(new Pair<Point, CameraShot>(point, new CameraShot()));

						time += random.Next(1000, 4000) * 1000;
					}
				}
				if (Lighting.Count == 0) {
					int end = (int)Chart.GetTime(Chart.Events.MusicEnd.Time);
					int time = 0;
					while (time < end) {
						Point point = new Point(Chart.GetTicks((ulong)time / 1000));
						if (random.Next() % 2 == 0)
							Lighting.Add(new Pair<Point, LightingType>(point, LightingType.None));

						time += random.Next(1000, 4000) * 1000;
					}
				}
				foreach (var light in Lighting) {
					if (light.Value == LightingType.None)
						light.Value = (LightingType)random.Next((int)LightingType.BlackoutFast, (int)LightingType.BigRockEnding);
				}
				foreach (var camera in CameraShots) {
					if (!camera.Value.Bassist && !camera.Value.Closeup && !camera.Value.Drummer && !camera.Value.Far && !camera.Value.Guitarist && !camera.Value.NoBehind && !camera.Value.NoCloseup && !camera.Value.Vocalist) {
						if (random.Next() % 2 == 0)
							camera.Value.Bassist = true;
						if (random.Next() % 2 == 0)
							camera.Value.Closeup = true;
						if (random.Next() % 2 == 0)
							camera.Value.Drummer = true;
						if (random.Next() % 2 == 0)
							camera.Value.Far = true;
						if (random.Next() % 2 == 0)
							camera.Value.Guitarist = true;
						if (random.Next() % 2 == 0)
							camera.Value.NoBehind = true;
						if (random.Next() % 2 == 0)
							camera.Value.NoCloseup = true;
						if (random.Next() % 2 == 0)
							camera.Value.Vocalist = true;
					}

					if (camera.Value.Drummer && ((camera.Value.Guitarist && camera.Value.Bassist) || (camera.Value.Guitarist && camera.Value.Vocalist) || (camera.Value.Bassist && camera.Value.Vocalist))) {
						// RB2 does not allow 3-member shots including the drummer
						camera.Value.Drummer = false;
					}
				}
				foreach (var shot in DirectedShots) {
					if (shot.Value == DirectedCut.None) {
						shot.Value = (DirectedCut)random.Next((int)DirectedCut.DuoGuitarist, (int)DirectedCut.BassistCrowd);

						// Blacklist
						switch (shot.Value) {
							case DirectedCut.Stagedive:
							case DirectedCut.Crowdsurf:
								shot.Value = DirectedCut.VocalistCloseup;
								break;
						}
					}
				}

				// Default to [verse] at the beginning of the song
				if (Verse.Count == 0 && Chorus.Count == 0)
					Verse.Add(new Point(0));

				track.Comments.AddRange(Lighting.ConvertAll(l => new Midi.TextEvent(l.Key.Time, "[lighting (" + l.Value.ToShortString() + ")]")));
				track.Comments.AddRange(DirectedShots.ConvertAll(l => new Midi.TextEvent(l.Key.Time, "[do_directed_cut " + l.Value.ToShortString() + "]")));
				track.Comments.AddRange(OptionalShots.ConvertAll(l => new Midi.TextEvent(l.Key.Time, "[do_optional_cut " + l.Value.ToShortString() + "]")));
				track.Comments.AddRange(Verse.ConvertAll(v => new Midi.TextEvent(v.Time, "[verse]")));
				track.Comments.AddRange(Chorus.ConvertAll(c => new Midi.TextEvent(c.Time, "[chorus]")));
				track.Comments.AddRange(Pyrotechnics.ConvertAll(p => new Midi.TextEvent(p.Time, "[bonusfx]")));
				track.Comments.AddRange(OptionalPyrotechnics.ConvertAll(p => new Midi.TextEvent(p.Time, "[bonusfx_optional]")));
				CameraShots.ForEach(s => {
					if (s.Value.Bassist)
						track.Notes.Add(s.Key.ToMidiNote(61));
					if (s.Value.Drummer)
						track.Notes.Add(s.Key.ToMidiNote(62));
					if (s.Value.Guitarist)
						track.Notes.Add(s.Key.ToMidiNote(63));
					if (s.Value.Vocalist)
						track.Notes.Add(s.Key.ToMidiNote(64));
					if (s.Value.NoBehind)
						track.Notes.Add(s.Key.ToMidiNote(70));
					if (s.Value.Far)
						track.Notes.Add(s.Key.ToMidiNote(71));
					if (s.Value.Closeup)
						track.Notes.Add(s.Key.ToMidiNote(72));
					if (s.Value.NoCloseup)
						track.Notes.Add(s.Key.ToMidiNote(73));
					track.Notes.Add(s.Key.ToMidiNote(60));
				});
				Singalong.ForEach(s => {
					if (s.Value == TrackType.Bass)
						track.Notes.Add(s.Key.ToMidiNote(85));
					if (s.Value == TrackType.Drums)
						track.Notes.Add(s.Key.ToMidiNote(86));
					if (s.Value == TrackType.Guitar)
						track.Notes.Add(s.Key.ToMidiNote(87));
				});
				Spotlights.ForEach(s => {
					if (s.Value == TrackType.Drums)
						track.Notes.Add(s.Key.ToMidiNote(37));
					if (s.Value == TrackType.Bass)
						track.Notes.Add(s.Key.ToMidiNote(38));
					if (s.Value == TrackType.Guitar)
						track.Notes.Add(s.Key.ToMidiNote(39));
					if (s.Value == TrackType.Vocals)
						track.Notes.Add(s.Key.ToMidiNote(40));
				});
				LightingKeyframes.ForEach(s => {
					switch (s.Value) {
						case LightingKeyframe.Next:
							track.Notes.Add(s.Key.ToMidiNote(48));
							break;
						case LightingKeyframe.Previous:
							track.Notes.Add(s.Key.ToMidiNote(49));
							break;
						case LightingKeyframe.First:
							track.Notes.Add(s.Key.ToMidiNote(50));
							break;
						default:
							break;
					}
				});
				track.Notes.AddRange(PostProcesses.ConvertAll(s => s.Key.ToMidiNote((byte)s.Value)));

				return track;
			}

			public class CameraShot
			{
				public bool Bassist;
				public bool Drummer;
				public bool Guitarist;
				public bool Vocalist;

				public bool NoBehind;
				public bool NoCloseup;
				public bool Far;
				public bool Closeup;
			}

			public enum LightingType
			{
				None,
				Default,
				BlackoutFast,
				BlackoutSlow,
				Dischord,
				FlareFast,
				FlareSlow,
				Frenzy,
				Harmony,
				ManualCool,
				ManualWarm,
				Searchlights,
				Silhouettes,
				SilhouettesSpot,
				Stomp,
				StrobeFast,
				StrobeSlow,
				Sweep,
				LoopCool,
				LoopWarm,
				WinBigRockEnding,
				BigRockEnding
			}

			public enum DirectedCut
			{
				None,
				DuoGuitarist,
				DuoBassist,
				DuoDrummer,
				DuoGuitarBass,
				FullBand,
				FullBandCamera,
				FullBandLongShot,
				FullBandJump,
				Bassist,
				BassistIdle,
				BassistCamera,
				BassistFretboard,
				BigRockEnding,
				BigRockEndingEnd,
				Drummer,
				DrummerPoint,
				DrummerIdle,
				DrummerOverhead,
				DrummerPedal,
				Guitarist,
				GuitaristIdle,
				GuitaristCamera,
				GuitaristFretboard,
				Stagedive,
				Crowdsurf,
				Vocalist,
				VocalistIdle,
				VocalistCamera,
				VocalistCloseup,
				GuitaristCrowd,
				BassistCrowd
			}

			public enum PostProcess
			{
				Default = 96,
				ContrastA = 97,
				Film16mm = 98,
				FilmSepia = 99,
				Silvertone = 100,
				Negative = 101,
				Photocopy = 102,
				ProFilmA = 103,
				ProFilmB = 104,
				ProFilmMirrored = 105,
				BlueTint = 106,
				VideoA = 107,
				VideoBlackAndWhite = 108,
				VideoSecurity = 109,
				VideoTrails = 110
			}

			public enum LightingKeyframe
			{
				Next,
				Previous,
				First
			}
		}

		public class EventsTrack : Track
		{
			public List<Note> Kick;
			public List<Note> Snare;
			public List<Note> Hat;

			public Point MusicStart;
			public Point MusicEnd;
			public Point End;

			//public List<Point> Coda; // Beginning of a BRE
			public List<Pair<Point, CrowdType>> Crowd;

			public List<Pair<Point, string>> Sections;

			public override Midi.Track ToMidiTrack()
			{
				Midi.Track track = new Midi.Track();
				track.Name = "EVENTS";

				if (MusicStart == null)
					MusicStart = new Point(0);
				ulong lastnote = Chart.FindLastNote();
				if (MusicEnd == null)
					MusicEnd = new Point(lastnote - 200);
				if (End == null)
					End = new Point(lastnote + 800);
				if (Sections.Count == 0)
					Sections.Add(new Pair<Point, string>(new Point(0), "rawk'd"));
				if (Crowd.Count == 0)
					Crowd.Add(new Pair<Point, CrowdType>(new Point(0), CrowdType.Normal));
				if (Kick.Count == 0 && Snare.Count == 0 && Hat.Count == 0) {
					/*foreach (Pair<Point, string> section in Sections) {
						Kick.Add(new Note() { Time = section.Key.Time + 0, Duration = 120, Velocity = 100, ReleaseVelocity = 100 });
						Snare.Add(new Note() { Time = section.Key.Time + 120, Duration = 120, Velocity = 100, ReleaseVelocity = 100 });
						Hat.Add(new Note() { Time = section.Key.Time + 240, Duration = 120, Velocity = 100, ReleaseVelocity = 100 });
						Kick.Add(new Note() { Time = section.Key.Time + 360, Duration = 120, Velocity = 100, ReleaseVelocity = 100 });
					}*/
				}

				Random random = new Random();
				foreach (var crowd in Crowd) {
					if (crowd.Value == CrowdType.None)
						crowd.Value = (CrowdType)random.Next((int)CrowdType.NoClap, (int)CrowdType.Intense);
				}

				track.Comments.Add(new Midi.TextEvent(MusicStart.Time, "[music_start]"));
				track.Comments.Add(new Midi.TextEvent(MusicEnd.Time, "[music_end]"));
				track.Comments.Add(new Midi.TextEvent(End.Time, "[end]"));
				track.Comments.AddRange(Crowd.ConvertAll(c => new Midi.TextEvent(c.Key.Time, "[" + c.Value.ToShortString() + "]")));
				track.Comments.AddRange(Sections.ConvertAll(c => new Midi.TextEvent(c.Key.Time, "[section " + c.Value + "]")));
				if (Chart.BigRockEnding != null)
					track.Comments.Add(new Midi.TextEvent(Chart.BigRockEnding.Time, "[coda]"));

				track.Notes.AddRange(Kick.ConvertAll(n => n.ToMidiNote(24)));
				track.Notes.AddRange(Snare.ConvertAll(n => n.ToMidiNote(25)));
				track.Notes.AddRange(Hat.ConvertAll(n => n.ToMidiNote(26)));

				return track;
			}

			public enum CrowdType
			{
				None,
				NoClap,
				Clap,
				Realtime,
				Mellow,
				Normal,
				Intense
			}

			public static CrowdType FromText(string crowd)
			{
				switch (crowd.ToLower()) {
					case "realtime":
						return CrowdType.Realtime;
					case "mellow":
						return CrowdType.Mellow;
					case "normal":
						return CrowdType.Normal;
					case "intense":
						return CrowdType.Intense;
					case "noclap":
						return CrowdType.NoClap;
					case "clap":
						return CrowdType.Clap;
				}
				throw new InvalidOperationException("Crowd mood \"" + crowd + "\" doesn't exist.");
			}
			public static string ToText(CrowdType crowd)
			{
				switch (crowd) {
					case CrowdType.Realtime:
						return "realtime";
					case CrowdType.Mellow:
						return "mellow";
					case CrowdType.Normal:
						return "normal";
					case CrowdType.Intense:
						return "intense";
					case CrowdType.NoClap:
						return "noclap";
					case CrowdType.Clap:
						return "clap";
					default:
						return null;
				}
			}

			public EventsTrack(NoteChart chart)
				: base(chart)
			{
				Type = TrackType.Events;

				Kick = new List<Note>();
				Snare = new List<Note>();
				Hat = new List<Note>();

				Crowd = new List<Pair<Point, CrowdType>>();
				Sections = new List<Pair<Point, string>>();
			}
		}

		public enum BandMember
		{
			Guitarist,
			Bassist,
			Drummer,
			Vocalist
		}

		public abstract class Instrument : Track
		{
			public List<Note> Overdrive;
			public List<Note> Player1;
			public List<Note> Player2;

			public Instrument(NoteChart chart)
				: base(chart)
			{
				Overdrive = new List<Note>();
				Player1 = new List<Note>();
				Player2 = new List<Note>();
			}

			protected void ToMidiTrack(Midi.Track track)
			{
				track.Notes.AddRange(Overdrive.ConvertAll(o => o.ToMidiNote(116)));
				track.Notes.AddRange(Player1.ConvertAll(o => o.ToMidiNote(105)));
				track.Notes.AddRange(Player2.ConvertAll(o => o.ToMidiNote(106)));

				if (this is ISolo) {
					ISolo solo = this as ISolo;
					track.Notes.AddRange(solo.SoloSections.ConvertAll(n => n.ToMidiNote(103)));
				}
				if (this is IGems) {
					IGems gems = this as IGems;
					foreach (KeyValuePair<Difficulty, List<Note>[]> g in gems.Gems) {
						byte notebase = 0;
						switch (g.Key) {
							case Difficulty.Easy:
								notebase = 60;
								break;
							case Difficulty.Medium:
								notebase = 72;
								break;
							case Difficulty.Hard:
								notebase = 84;
								break;
							case Difficulty.Expert:
								notebase = 96;
								break;
							default:
								break;
						}
						int num = 0;
						for (byte i = 0; i < g.Value.Length; i++) {
							num += g.Value[i].Count;
						}

						if (num == 0) { // Add empty fallback notes
							track.Notes.Add(new Note() { Time = Chart.Division.TicksPerBeat * 4U, Duration = 1, Velocity = 100, ReleaseVelocity = 100 }.ToMidiNote(notebase));
						} else {
							for (byte i = 0; i < g.Value.Length; i++)
								track.Notes.AddRange(g.Value[i].ConvertAll(n => n.ToMidiNote((byte)(notebase + i))));
						}
					}
				}
				if (this is ICharacterMood) {
					ICharacterMood mood = this as ICharacterMood;
					if (mood.CharacterMoods.Count == 0)
						mood.CharacterMoods.Add(new Pair<Point, CharacterMood>(new Point(0), CharacterMood.Play));

					foreach (var m in mood.CharacterMoods) {
						if (m.Value != CharacterMood.None)
							track.Comments.Add(new Midi.TextEvent(m.Key.Time, "[" + m.Value.ToShortString() + "]"));
					}
				}
				if (this is IHandMap) {
					IHandMap hand = this as IHandMap;
					track.Comments.AddRange(hand.HandMap.ConvertAll(h => new Midi.TextEvent(h.Key.Time, "[map HandMap_" + h.Value.ToShortString() + "]")));
				}
				if (this is IStrumMap) {
					IStrumMap strum = this as IStrumMap;
					track.Comments.AddRange(strum.StrumMap.ConvertAll(s => new Midi.TextEvent(s.Key.Time, "[map StrumMap_" + s.Value.ToShortString() + "]")));
				}
				if (this is IFretPosition) {
					IFretPosition fret = this as IFretPosition;
					track.Notes.AddRange(fret.FretPosition.ConvertAll(f => f.Key.ToMidiNote((byte)(f.Value + 40))));
				}
				if (this is IForcedHopo) {
					IForcedHopo hopo = this as IForcedHopo;
					foreach (KeyValuePair<Difficulty, List<Note>> h in hopo.ForceHammeron) {
						byte notebase = 0;
						switch (h.Key) {
							case Difficulty.Easy:
								notebase = 60 + 5;
								break;
							case Difficulty.Medium:
								notebase = 72 + 5;
								break;
							case Difficulty.Hard:
								notebase = 84 + 5;
								break;
							case Difficulty.Expert:
								notebase = 96 + 5;
								break;
							default:
								break;
						}
						track.Notes.AddRange(h.Value.ConvertAll(n => n.ToMidiNote(notebase)));
					}
					foreach (KeyValuePair<Difficulty, List<Note>> h in hopo.ForceStrum) {
						byte notebase = 0;
						switch (h.Key) {
							case Difficulty.Easy:
								notebase = 60 + 6;
								break;
							case Difficulty.Medium:
								notebase = 72 + 6;
								break;
							case Difficulty.Hard:
								notebase = 84 + 6;
								break;
							case Difficulty.Expert:
								notebase = 96 + 6;
								break;
							default:
								break;
						}
						// Give Hammerons priority over Strums
						foreach (var h2 in h.Value) {
							if (hopo.ForceHammeron[h.Key].Find(c => h2.IsUnderNote(c, false)) != null)
								continue;
							track.Notes.Add(h2.ToMidiNote(notebase));
						}
						//track.Notes.AddRange(h.Value.ConvertAll(n => n.ToMidiNote(notebase)));
					}
				}
			}
		}

		public class Vocals : Instrument, ICharacterMood
		{
			public List<Midi.NoteEvent> Gems;
			public List<Note> PercussionSections;
			public List<Point> PercussionGems;
			public List<Point> PercussionSound;
			public List<Pair<Point, string>> PercussionSectionComments;
			public List<Pair<Point, string>> Lyrics;
			public List<Pair<Point, CharacterMood>> CharacterMoods { get; set; }

			public Vocals(NoteChart chart)
				: base(chart)
			{
				Gems = new List<Midi.NoteEvent>();

				PercussionSections = new List<Note>();
				PercussionGems = new List<Point>();
				Lyrics = new List<Pair<Point, string>>();

				PercussionSound = new List<Point>();
				PercussionSectionComments = new List<Pair<Point, string>>();

				CharacterMoods = new List<Pair<Point, CharacterMood>>();

				Type = TrackType.Vocals;
			}

			public override Midi.Track ToMidiTrack()
			{
				Midi.Track track = new Midi.Track();
				track.Name = "PART VOCALS";

				track.Notes.AddRange(Gems);
				track.Notes.AddRange(PercussionSections.ConvertAll(p => p.ToMidiNote(103)));
				track.Notes.AddRange(PercussionGems.ConvertAll(p => p.ToMidiNote(96)));
				track.Notes.AddRange(PercussionSound.ConvertAll(p => p.ToMidiNote(97)));
				track.Comments.AddRange(PercussionSectionComments.ConvertAll(s => new Midi.TextEvent(s.Key.Time, s.Value)));
				track.Lyrics.AddRange(Lyrics.ConvertAll(l => new Midi.TextEvent(l.Key.Time, l.Value)));

				ToMidiTrack(track);

				return track;
			}
		}

		public class Drums : Instrument, ISolo, IGems, ICharacterMood
		{
			public List<Note> DrumFills;
			public Dictionary<Difficulty, List<Note>[]> Gems { get; set; }
			public List<Note> SoloSections { get; set; }
			public Dictionary<Difficulty, string> Mixing;
			public List<Pair<Point, CharacterMood>> CharacterMoods { get; set; }
			public List<Pair<Note, Animation>> Animations { get; set; }

			public Drums(NoteChart chart)
				: base(chart)
			{
				DrumFills = new List<Note>();
				Gems = new Dictionary<Difficulty, List<Note>[]>();
				SoloSections = new List<Note>();
				Mixing = new Dictionary<Difficulty, string>();

				CharacterMoods = new List<Pair<Point, CharacterMood>>();

				Animations = new List<Pair<Note, Animation>>();

				Mixing[Difficulty.Easy] = "drums0";
				Mixing[Difficulty.Medium] = "drums0";
				Mixing[Difficulty.Hard] = "drums0";
				Mixing[Difficulty.Expert] = "drums0";

				Gems.Add(Difficulty.Easy, new List<Note>[] { new List<Note>(), new List<Note>(), new List<Note>(), new List<Note>(), new List<Note>() });
				Gems.Add(Difficulty.Medium, new List<Note>[] { new List<Note>(), new List<Note>(), new List<Note>(), new List<Note>(), new List<Note>() });
				Gems.Add(Difficulty.Hard, new List<Note>[] { new List<Note>(), new List<Note>(), new List<Note>(), new List<Note>(), new List<Note>() });
				Gems.Add(Difficulty.Expert, new List<Note>[] { new List<Note>(), new List<Note>(), new List<Note>(), new List<Note>(), new List<Note>() });

				Type = TrackType.Drums;
			}

			public override Midi.Track ToMidiTrack()
			{
				Midi.Track track = new Midi.Track();
				track.Name = "PART DRUMS";

				if (Animations.Count == 0) {
					for (int i = 0; i < Gems[Difficulty.Expert].Length; i++) {
						Note previous = null;
						AnimationHandedness lasthand = AnimationHandedness.LeftFoot;
						foreach (Note n in Gems[Difficulty.Expert][i]) {
							AnimationHandedness hand = AnimationHandedness.BothHands;
							Animation animation = Animation.HiHatPedalLeftFoot;
							switch (i) {
								case 1: // Red
									hand = AnimationHandedness.LeftHand;
									break;
								case 2: // Yellow
									hand = AnimationHandedness.RightHand;
									break;
								case 3: // Blue
									hand = AnimationHandedness.RightHand;
									break;
								case 4: // Green
									hand = AnimationHandedness.RightHand;
									break;
							}

							if (previous != null && (n.Time - previous.Time + previous.Duration) < 40) {
								if (lasthand == AnimationHandedness.LeftHand)
									hand = AnimationHandedness.RightHand;
								else
									hand = AnimationHandedness.LeftHand;
							}


							List<Pair<Note, Animation>> animations = Animations.FindAll(a => a.Key.Time == n.Time);
							if (animations.Count != 0) { // There might be a conflicting animation...
								foreach (Pair<Note, Animation> ani in animations) {
									if (ani.Value.GetAnimationHandedness() == hand) { // Swap hands if conflict
										if (hand == AnimationHandedness.LeftHand)
											hand = AnimationHandedness.RightHand;
										else
											hand = AnimationHandedness.LeftHand;
									}
								}
							}

							switch (i) {
								case 0: // Bass
									animation = Animation.KickRightFoot;
									break;
								case 1: // Red
									if (hand == AnimationHandedness.LeftHand)
										animation = Animation.SnareLeftHand;
									else
										animation = Animation.SnareRightHand;
									break;
								case 2: // Yellow
									if (hand == AnimationHandedness.RightHand)
										animation = Animation.HiHatRightHand;
									else
										animation = Animation.HiHatLeftHand;
									break;
								case 3: // Blue
									if (hand == AnimationHandedness.RightHand)
										animation = Animation.RideCymbalRightHand;
									else
										animation = Animation.Tom1LeftHand;
									break;
								case 4: // Green
									if (hand == AnimationHandedness.RightHand)
										animation = Animation.Crash1HardRightHand;
									else
										animation = Animation.Crash1HardLeftHand;
									break;
							}

							lasthand = hand;

							Animations.Add(new Pair<Note, Animation>(n, animation));

							previous = n;
						}
					}
				}

				DrumFills.ForEach(n => {
					for (byte i = 120; i <= 124; i++)
						track.Notes.Add(n.ToMidiNote(i));
				});

				ToMidiTrack(track);

				foreach (KeyValuePair<Difficulty, string> m in Mixing) {
					track.Comments.Add(new Midi.TextEvent(0, "[mix " + (int)m.Key + " " + m.Value + "]"));
				}

				track.Notes.AddRange(Animations.ConvertAll(c => c.Key.ToMidiNote((byte)c.Value)));

				return track;
			}

			public enum Animation
			{
				KickRightFoot = 24,
				HiHatPedalLeftFoot = 25,
				SnareLeftHand = 26,
				SnareRightHand = 27,
				HiHatLeftHand = 30,
				HiHatRightHand = 31,
				PercussionRightHand = 32,
				Crash1HardLeftHand = 34,
				Crash1SoftLeftHand = 35,
				Crash1HardRightHand = 36,
				Crash1SoftRightHand = 37,
				Crash2HardRightHand = 38,
				Crash2SoftRightHand = 39,
				Crash1Choke = 40,
				Crash2Choke = 41,
				RideCymbalRightHand = 42,
				Tom1LeftHand = 46,
				Tom1RightHand = 47,
				Tom2LeftHand = 48,
				Tom2RightHand = 49,
				FloorTomLeftHand = 50,
				FloorTomRightHand = 51
			}

			public enum AnimationHandedness
			{
				LeftHand,
				RightHand,
				LeftFoot,
				RightFoot,
				BothHands
			}
		}

		public class Guitar : Instrument, ISolo, IGems, IForcedHopo, ICharacterMood, IHandMap, IFretPosition
		{
			public Dictionary<Difficulty, List<Note>[]> Gems { get; set; }

			public List<Note> SoloSections { get; set; }

			public Dictionary<Difficulty, List<Note>> ForceHammeron { get; set; }
			public Dictionary<Difficulty, List<Note>> ForceStrum { get; set; }

			public List<Pair<Note, byte>> FretPosition { get; set; }

			public List<Pair<Point, LeftHandMap>> HandMap { get; set; }

			public List<Pair<Point, CharacterMood>> CharacterMoods { get; set; }

			public Guitar(NoteChart chart)
				: base(chart)
			{
				Gems = new Dictionary<Difficulty, List<Note>[]>();
				SoloSections = new List<Note>();

				Gems.Add(Difficulty.Easy, new List<Note>[] { new List<Note>(), new List<Note>(), new List<Note>(), new List<Note>(), new List<Note>() });
				Gems.Add(Difficulty.Medium, new List<Note>[] { new List<Note>(), new List<Note>(), new List<Note>(), new List<Note>(), new List<Note>() });
				Gems.Add(Difficulty.Hard, new List<Note>[] { new List<Note>(), new List<Note>(), new List<Note>(), new List<Note>(), new List<Note>() });
				Gems.Add(Difficulty.Expert, new List<Note>[] { new List<Note>(), new List<Note>(), new List<Note>(), new List<Note>(), new List<Note>() });

				ForceHammeron = new Dictionary<Difficulty, List<Note>>();
				ForceStrum = new Dictionary<Difficulty, List<Note>>();

				FretPosition = new List<Pair<Note, byte>>();

				HandMap = new List<Pair<Point, LeftHandMap>>();

				ForceHammeron.Add(Difficulty.Easy, new List<Note>());
				ForceHammeron.Add(Difficulty.Medium, new List<Note>());
				ForceHammeron.Add(Difficulty.Hard, new List<Note>());
				ForceHammeron.Add(Difficulty.Expert, new List<Note>());
				ForceStrum.Add(Difficulty.Easy, new List<Note>());
				ForceStrum.Add(Difficulty.Medium, new List<Note>());
				ForceStrum.Add(Difficulty.Hard, new List<Note>());
				ForceStrum.Add(Difficulty.Expert, new List<Note>());

				CharacterMoods = new List<Pair<Point, CharacterMood>>();

				Type = TrackType.Guitar;
			}

			public override Midi.Track ToMidiTrack()
			{
				Midi.Track track = new Midi.Track();
				track.Name = "PART GUITAR";

				ToMidiTrack(track);

				return track;
			}
		}

		public class Bass : Instrument, IGems, IForcedHopo, ICharacterMood, ISolo, IHandMap, IStrumMap, IFretPosition
		{
			public Dictionary<Difficulty, List<Note>[]> Gems { get; set; }

			public List<Note> SoloSections { get; set; }

			public Dictionary<Difficulty, List<Note>> ForceHammeron { get; set; }
			public Dictionary<Difficulty, List<Note>> ForceStrum { get; set; }

			public List<Pair<Point, LeftHandMap>> HandMap { get; set; }
			public List<Pair<Point, RightHandMap>> StrumMap { get; set; }

			public List<Pair<Note, byte>> FretPosition { get; set; }

			public List<Pair<Point, CharacterMood>> CharacterMoods { get; set; }

			public Bass(NoteChart chart)
				: base(chart)
			{
				Gems = new Dictionary<Difficulty, List<Note>[]>();
				SoloSections = new List<Note>();

				Gems.Add(Difficulty.Easy, new List<Note>[] { new List<Note>(), new List<Note>(), new List<Note>(), new List<Note>(), new List<Note>() });
				Gems.Add(Difficulty.Medium, new List<Note>[] { new List<Note>(), new List<Note>(), new List<Note>(), new List<Note>(), new List<Note>() });
				Gems.Add(Difficulty.Hard, new List<Note>[] { new List<Note>(), new List<Note>(), new List<Note>(), new List<Note>(), new List<Note>() });
				Gems.Add(Difficulty.Expert, new List<Note>[] { new List<Note>(), new List<Note>(), new List<Note>(), new List<Note>(), new List<Note>() });

				ForceHammeron = new Dictionary<Difficulty, List<Note>>();
				ForceStrum = new Dictionary<Difficulty, List<Note>>();

				FretPosition = new List<Pair<Note, byte>>();

				HandMap = new List<Pair<Point, LeftHandMap>>();
				StrumMap = new List<Pair<Point, RightHandMap>>();

				ForceHammeron.Add(Difficulty.Easy, new List<Note>());
				ForceHammeron.Add(Difficulty.Medium, new List<Note>());
				ForceHammeron.Add(Difficulty.Hard, new List<Note>());
				ForceHammeron.Add(Difficulty.Expert, new List<Note>());
				ForceStrum.Add(Difficulty.Easy, new List<Note>());
				ForceStrum.Add(Difficulty.Medium, new List<Note>());
				ForceStrum.Add(Difficulty.Hard, new List<Note>());
				ForceStrum.Add(Difficulty.Expert, new List<Note>());

				CharacterMoods = new List<Pair<Point, CharacterMood>>();

				Type = TrackType.Bass;
			}

			public override Midi.Track ToMidiTrack()
			{
				Midi.Track track = new Midi.Track();
				track.Name = "PART BASS";

				ToMidiTrack(track);

				return track;
			}
		}

		public class Point : Midi.Event
		{
			public Point() : base(0) { }

			public Point(ulong time) : base(time) { }

			internal Midi.NoteEvent ToMidiNote(byte p)
			{
				return new Midi.NoteEvent(Time, 0, p, 100, 1) { ReleaseVelocity = 100 };
			}
		}

		public class Note : Point
		{
			public byte Velocity = 127;
			public byte ReleaseVelocity = 127;
			public ulong Duration = 60;

			public static Note FromMidiNote(Midi.NoteEvent note)
			{
				return new Note() { Time = note.Time, Duration = note.Duration, Velocity = note.Velocity, ReleaseVelocity = note.ReleaseVelocity };
			}

			public new Midi.NoteEvent ToMidiNote(byte note)
			{
				return new Midi.NoteEvent(Time, 0, note, Velocity, Duration) { ReleaseVelocity = ReleaseVelocity };
			}
		}

		static NoteChart()
		{
			NoteTypes = new Dictionary<NoteDescriptor, NoteProducer>();

			// H2H Camera Cuts and Focus Notes
			/* NoteTypes.Add(new NoteDescriptor() {
				Notes = new byte[] { 12, 13, 14, 15 },
				Track = TrackType.Bass | TrackType.Drums | TrackType.Guitar | TrackType.Vocals },
				(n, t) => {
					BandMember member;
					switch (n.Note) {
						case 12:
							break;
						case 13:
							break;
						case 14:
							break;
						case 15:
							break;
					}
					(t as Instrument).H2H.Add(new Instrument.H2HNote() { Note = Note.FromMidiNote(n), Member = member });
				}
			); */
			// Fret Animations
			/* NoteTypes.Add(new NoteDescriptor() {
				Notes = new byte[] { 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59 },
				Track = TrackType.Bass | TrackType.Guitar },
				null
			); */
			// Overdrive
			NoteTypes.Add(new NoteDescriptor() {
				Notes = new byte[] { 116 },
				Track = TrackType.Bass | TrackType.Guitar | TrackType.Drums | TrackType.Vocals
			},
				(n, t) => (t as Instrument).Overdrive.Add(Note.FromMidiNote(n))
			);
			// Easy
			NoteTypes.Add(new NoteDescriptor() { // Gems
				Notes = new byte[] { 60, 61, 62, 63, 64 },
				Track = TrackType.Bass | TrackType.Guitar | TrackType.Drums
			},
				(n, t) => (t as IGems).Gems[Difficulty.Easy][n.Note - 60].Add(Note.FromMidiNote(n))
			);
			NoteTypes.Add(new NoteDescriptor() { // Force Hopo
				Notes = new byte[] { 60 + 5 },
				Track = TrackType.Bass | TrackType.Guitar
			},
				(n, t) => (t as IForcedHopo).ForceHammeron[Difficulty.Easy].Add(Note.FromMidiNote(n))
			);
			NoteTypes.Add(new NoteDescriptor() { // Force Strum
				Notes = new byte[] { 60 + 6 },
				Track = TrackType.Bass | TrackType.Guitar
			},
				(n, t) => (t as IForcedHopo).ForceStrum[Difficulty.Easy].Add(Note.FromMidiNote(n))
			);
			// Medium
			NoteTypes.Add(new NoteDescriptor() { // Gems
				Notes = new byte[] { 72, 73, 74, 75, 76 },
				Track = TrackType.Bass | TrackType.Guitar | TrackType.Drums
			},
				(n, t) => (t as IGems).Gems[Difficulty.Medium][n.Note - 72].Add(Note.FromMidiNote(n))
			);
			NoteTypes.Add(new NoteDescriptor() { // Force Hopo
				Notes = new byte[] { 72 + 5 },
				Track = TrackType.Bass | TrackType.Guitar
			},
				(n, t) => (t as IForcedHopo).ForceHammeron[Difficulty.Medium].Add(Note.FromMidiNote(n))
			);
			NoteTypes.Add(new NoteDescriptor() { // Force Strum
				Notes = new byte[] { 72 + 6 },
				Track = TrackType.Bass | TrackType.Guitar
			},
				(n, t) => (t as IForcedHopo).ForceStrum[Difficulty.Medium].Add(Note.FromMidiNote(n))
			);
			// Hard
			NoteTypes.Add(new NoteDescriptor() { // Gems
				Notes = new byte[] { 84, 85, 86, 87, 88 },
				Track = TrackType.Bass | TrackType.Guitar | TrackType.Drums
			},
				(n, t) => (t as IGems).Gems[Difficulty.Hard][n.Note - 84].Add(Note.FromMidiNote(n))
			);
			NoteTypes.Add(new NoteDescriptor() { // Force Hopo
				Notes = new byte[] { 84 + 5 },
				Track = TrackType.Bass | TrackType.Guitar
			},
				(n, t) => (t as IForcedHopo).ForceHammeron[Difficulty.Hard].Add(Note.FromMidiNote(n))
			);
			NoteTypes.Add(new NoteDescriptor() { // Force Strum
				Notes = new byte[] { 84 + 6 },
				Track = TrackType.Bass | TrackType.Guitar
			},
				(n, t) => (t as IForcedHopo).ForceStrum[Difficulty.Hard].Add(Note.FromMidiNote(n))
			);
			// Expert
			NoteTypes.Add(new NoteDescriptor() { // Gems
				Notes = new byte[] { 96, 97, 98, 99, 100 },
				Track = TrackType.Bass | TrackType.Guitar | TrackType.Drums
			},
				(n, t) => (t as IGems).Gems[Difficulty.Expert][n.Note - 96].Add(Note.FromMidiNote(n))
			);
			NoteTypes.Add(new NoteDescriptor() { // Force Hopo
				Notes = new byte[] { 96 + 5 },
				Track = TrackType.Bass | TrackType.Guitar
			},
				(n, t) => (t as IForcedHopo).ForceHammeron[Difficulty.Expert].Add(Note.FromMidiNote(n))
			);
			NoteTypes.Add(new NoteDescriptor() { // Force Strum
				Notes = new byte[] { 96 + 6 },
				Track = TrackType.Bass | TrackType.Guitar
			},
				(n, t) => (t as IForcedHopo).ForceStrum[Difficulty.Expert].Add(Note.FromMidiNote(n))
			);
			// Fretboard Position
			NoteTypes.Add(new NoteDescriptor() {
				Notes = new byte[] { 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59 },
				Track = TrackType.Bass | TrackType.Guitar
			},
				(n, t) => (t as IFretPosition).FretPosition.Add(new Pair<Note, byte>(Note.FromMidiNote(n), (byte)(n.Note - 40)))
			);
			// Player Demarkation
			NoteTypes.Add(new NoteDescriptor() {
				Notes = new byte[] { 105 },
				Track = TrackType.Bass | TrackType.Guitar | TrackType.Drums | TrackType.Vocals
			},
				(n, t) => (t as Instrument).Player1.Add(Note.FromMidiNote(n))
			);
			NoteTypes.Add(new NoteDescriptor() {
				Notes = new byte[] { 106 },
				Track = TrackType.Bass | TrackType.Guitar | TrackType.Drums | TrackType.Vocals
			},
				(n, t) => (t as Instrument).Player2.Add(Note.FromMidiNote(n))
			);
			// Big Rock Ending / Drum Fills
			/*
			NoteTypes.Add(new NoteDescriptor() {
				//Notes = new byte[] { 120, 121, 122, 123, 124 },
				Notes = new byte[] { 120, 121, 122, 123, 124 },
				Track = TrackType.Bass | TrackType.Guitar | TrackType.Vocals },
				(n, t) => { if (n.Note == 120) t.Chart.BigRockEnding = Note.FromMidiNote(n); }
			);
			*/
			NoteTypes.Add(new NoteDescriptor() {
				//Notes = new byte[] { 120, 121, 122, 123, 124 },
				Notes = new byte[] { 120 },
				Track = TrackType.Drums
			},
				//(n, t) => { if (n.Note == 120) (t as Drums).DrumFills.Add(Note.FromMidiNote(n)); }
				(n, t) => (t as Drums).DrumFills.Add(Note.FromMidiNote(n))
			);
			// Solo
			NoteTypes.Add(new NoteDescriptor() {
				Notes = new byte[] { 103 },
				Track = TrackType.Guitar | TrackType.Bass | TrackType.Drums
			},
				(n, t) => (t as ISolo).SoloSections.Add(Note.FromMidiNote(n))
			);
			// Drums
			NoteTypes.Add(new NoteDescriptor() {
				Notes = new byte[] { 24, 25, 26, 27, 30, 31, 32, 34, 35, 36, 37, 38, 39, 40, 41, 42, 46, 47, 48, 49, 50, 51 },
				Track = TrackType.Drums
			},
				(n, t) => (t as Drums).Animations.Add(new Pair<Note, Drums.Animation>(Note.FromMidiNote(n), (Drums.Animation)n.Note))
			);
			// Beat Track
			NoteTypes.Add(new NoteDescriptor() {
				Notes = new byte[] { 12 },
				Track = TrackType.Beat
			},
				(n, t) => (t as BeatTrack).Low.Add(Note.FromMidiNote(n))
			);
			NoteTypes.Add(new NoteDescriptor() {
				Notes = new byte[] { 13 },
				Track = TrackType.Beat
			},
				(n, t) => (t as BeatTrack).High.Add(Note.FromMidiNote(n))
			);
			// Events Track
			NoteTypes.Add(new NoteDescriptor() {
				Notes = new byte[] { 24 },
				Track = TrackType.Events
			},
				(n, t) => (t as EventsTrack).Kick.Add(Note.FromMidiNote(n))
			);
			NoteTypes.Add(new NoteDescriptor() {
				Notes = new byte[] { 25 },
				Track = TrackType.Events
			},
				(n, t) => (t as EventsTrack).Snare.Add(Note.FromMidiNote(n))
			);
			NoteTypes.Add(new NoteDescriptor() {
				Notes = new byte[] { 26 },
				Track = TrackType.Events
			},
				(n, t) => (t as EventsTrack).Hat.Add(Note.FromMidiNote(n))
			);
			// Vocals
			byte[] notes = new byte[84 - 36 + 1];
			for (byte i = 0; i < notes.Length; i++)
				notes[i] = (byte)(36 + i);
			NoteTypes.Add(new NoteDescriptor() {
				Notes = notes,
				Track = TrackType.Vocals
			},
				(n, t) => (t as Vocals).Gems.Add(n)
			);
			NoteTypes.Add(new NoteDescriptor() {
				Notes = new byte[] { 103 },
				Track = TrackType.Vocals
			},
				(n, t) => (t as Vocals).PercussionSections.Add(Note.FromMidiNote(n))
			);
			NoteTypes.Add(new NoteDescriptor() {
				Notes = new byte[] { 96 },
				Track = TrackType.Vocals
			},
				(n, t) => (t as Vocals).PercussionGems.Add(new Point(n.Time))
			);
			NoteTypes.Add(new NoteDescriptor() {
				Notes = new byte[] { 97 },
				Track = TrackType.Vocals
			},
				(n, t) => (t as Vocals).PercussionSound.Add(new Point(n.Time))
			);
			// Venue Track
			NoteTypes.Add(new NoteDescriptor() {
				Notes = new byte[] { 61, 62, 63, 64, 70, 71, 72, 73 },
				Track = TrackType.Venue
			},
				(n, t) => {
					VenueTrack v = t as VenueTrack;
					// Find a shot at the same time...
					VenueTrack.CameraShot shot = new VenueTrack.CameraShot();
					bool newshot = false;
					var shotpair = v.CameraShots.Find(s => s.Key.Time == n.Time);
					if (shotpair == null) {
						newshot = true;
						shot = new VenueTrack.CameraShot();
					} else
						shot = shotpair.Value;

					switch (n.Note) {
						case 61:
							shot.Bassist = true; break;
						case 62:
							shot.Drummer = true; break;
						case 63:
							shot.Guitarist = true; break;
						case 64:
							shot.Vocalist = true; break;
						case 70:
							shot.NoBehind = true; break;
						case 71:
							shot.Far = true; break;
						case 72:
							shot.Closeup = true; break;
						case 73:
							shot.NoCloseup = true; break;
					}
					if (newshot)
						v.CameraShots.Add(new Pair<Point, VenueTrack.CameraShot>(new Point(n.Time), shot));
				}
			);
			NoteTypes.Add(new NoteDescriptor() {
				Notes = new byte[] { 48, 49, 50 },
				Track = TrackType.Venue
			},
				(n, t) => {
					VenueTrack v = t as VenueTrack;
					switch (n.Note) {
						case 48:
							v.LightingKeyframes.Add(new Pair<Point, VenueTrack.LightingKeyframe>(new Point(n.Time), VenueTrack.LightingKeyframe.Next));
							break;
						case 49:
							v.LightingKeyframes.Add(new Pair<Point, VenueTrack.LightingKeyframe>(new Point(n.Time), VenueTrack.LightingKeyframe.Previous));
							break;
						case 50:
							v.LightingKeyframes.Add(new Pair<Point, VenueTrack.LightingKeyframe>(new Point(n.Time), VenueTrack.LightingKeyframe.First));
							break;
					}
				}
			);
			NoteTypes.Add(new NoteDescriptor() {
				Notes = new byte[] { 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110 },
				Track = TrackType.Venue
			},
				(n, t) => (t as VenueTrack).PostProcesses.Add(new Pair<Point, VenueTrack.PostProcess>(new Point(n.Time), (VenueTrack.PostProcess)n.Note))
			);
		}

		public static Dictionary<NoteDescriptor, NoteProducer> NoteTypes;

		public class NoteDescriptor
		{
			public byte[] Notes;
			public TrackType Track;
		}

		public delegate void NoteProducer(Midi.NoteEvent note, Track track);

		public ulong FindLastNote()
		{
			ulong last = 0;

			if (PartGuitar != null) {
				foreach (KeyValuePair<Difficulty, List<Note>[]> s in PartGuitar.Gems) {
					foreach (List<Note> l in s.Value) {
						foreach (Note n in l) {
							if ((n.Time + n.Duration) > last)
								last = n.Time + n.Duration;
						}
					}
				}
			}
			if (PartBass != null) {
				foreach (KeyValuePair<Difficulty, List<Note>[]> s in PartBass.Gems) {
					foreach (List<Note> l in s.Value) {
						foreach (Note n in l) {
							if ((n.Time + n.Duration) > last)
								last = n.Time + n.Duration;
						}
					}
				}
			}
			if (PartDrums != null) {
				foreach (KeyValuePair<Difficulty, List<Note>[]> s in PartDrums.Gems) {
					foreach (List<Note> l in s.Value) {
						foreach (Note n in l) {
							if ((n.Time + n.Duration) > last)
								last = n.Time + n.Duration;
						}
					}
				}
			}
			if (PartVocals != null) {
				foreach (Midi.NoteEvent n in PartVocals.Gems) {
					if ((n.Time + n.Duration) > last)
						last = n.Time + n.Duration;
				}
			}

			return last;
		}
	}

	public static class NoteChartHelper
	{
		public static List<Pair<NoteChart.VenueTrack.LightingType, string>> LightingTypes = new List<Pair<NoteChart.VenueTrack.LightingType, string>> {
			new Pair<NoteChart.VenueTrack.LightingType, string>(NoteChart.VenueTrack.LightingType.Default, string.Empty),
			new Pair<NoteChart.VenueTrack.LightingType, string>(NoteChart.VenueTrack.LightingType.BlackoutFast, "blackout_fast"),
			new Pair<NoteChart.VenueTrack.LightingType, string>(NoteChart.VenueTrack.LightingType.BlackoutSlow, "blackout_slow"),
			new Pair<NoteChart.VenueTrack.LightingType, string>(NoteChart.VenueTrack.LightingType.Dischord, "dischord"),
			new Pair<NoteChart.VenueTrack.LightingType, string>(NoteChart.VenueTrack.LightingType.FlareFast, "flare_fast"),
			new Pair<NoteChart.VenueTrack.LightingType, string>(NoteChart.VenueTrack.LightingType.FlareSlow, "flare_slow"),
			new Pair<NoteChart.VenueTrack.LightingType, string>(NoteChart.VenueTrack.LightingType.Frenzy, "frenzy"),
			new Pair<NoteChart.VenueTrack.LightingType, string>(NoteChart.VenueTrack.LightingType.Harmony, "harmony"),
			new Pair<NoteChart.VenueTrack.LightingType, string>(NoteChart.VenueTrack.LightingType.ManualCool, "manual_cool"),
			new Pair<NoteChart.VenueTrack.LightingType, string>(NoteChart.VenueTrack.LightingType.ManualWarm, "manual_warm"),
			new Pair<NoteChart.VenueTrack.LightingType, string>(NoteChart.VenueTrack.LightingType.Searchlights, "searchlights"),
			new Pair<NoteChart.VenueTrack.LightingType, string>(NoteChart.VenueTrack.LightingType.Silhouettes, "silhouettes"),
			new Pair<NoteChart.VenueTrack.LightingType, string>(NoteChart.VenueTrack.LightingType.SilhouettesSpot, "silhouettes_spot"),
			new Pair<NoteChart.VenueTrack.LightingType, string>(NoteChart.VenueTrack.LightingType.Stomp, "stomp"),
			new Pair<NoteChart.VenueTrack.LightingType, string>(NoteChart.VenueTrack.LightingType.StrobeFast, "strobe_fast"),
			new Pair<NoteChart.VenueTrack.LightingType, string>(NoteChart.VenueTrack.LightingType.StrobeSlow, "strobe_slow"),
			new Pair<NoteChart.VenueTrack.LightingType, string>(NoteChart.VenueTrack.LightingType.Sweep, "sweep"),
			new Pair<NoteChart.VenueTrack.LightingType, string>(NoteChart.VenueTrack.LightingType.LoopCool, "loop_cool"),
			new Pair<NoteChart.VenueTrack.LightingType, string>(NoteChart.VenueTrack.LightingType.LoopWarm, "loop_warm"),
			new Pair<NoteChart.VenueTrack.LightingType, string>(NoteChart.VenueTrack.LightingType.WinBigRockEnding, "win_bre"),
			new Pair<NoteChart.VenueTrack.LightingType, string>(NoteChart.VenueTrack.LightingType.BigRockEnding, "bre"),
		};

		public static string ToShortString(this NoteChart.VenueTrack.LightingType lighting)
		{
			var found = LightingTypes.Find(p => p.Key == lighting);
			if (found != null)
				return found.Value;
			else
				return String.Empty;
		}

		public static NoteChart.VenueTrack.LightingType LightingTypeFromString(string shortname)
		{
			var found = LightingTypes.Find(p => p.Value == shortname);
			if (found != null)
				return found.Key;
			else
				return NoteChart.VenueTrack.LightingType.None;
		}

		public static List<Pair<NoteChart.VenueTrack.DirectedCut, string>> DirectedCuts = new List<Pair<NoteChart.VenueTrack.DirectedCut, string>> {
			new Pair<NoteChart.VenueTrack.DirectedCut, string>(NoteChart.VenueTrack.DirectedCut.DuoGuitarist, "duo_guitar"),
			new Pair<NoteChart.VenueTrack.DirectedCut, string>(NoteChart.VenueTrack.DirectedCut.DuoBassist, "duo_bass"),
			new Pair<NoteChart.VenueTrack.DirectedCut, string>(NoteChart.VenueTrack.DirectedCut.DuoDrummer, "duo_drums"),
			new Pair<NoteChart.VenueTrack.DirectedCut, string>(NoteChart.VenueTrack.DirectedCut.DuoGuitarBass, "duo_gb"),
			new Pair<NoteChart.VenueTrack.DirectedCut, string>(NoteChart.VenueTrack.DirectedCut.FullBand, "all"),
			new Pair<NoteChart.VenueTrack.DirectedCut, string>(NoteChart.VenueTrack.DirectedCut.FullBandCamera, "all_cam"),
			new Pair<NoteChart.VenueTrack.DirectedCut, string>(NoteChart.VenueTrack.DirectedCut.FullBandLongShot, "all_lt"),
			new Pair<NoteChart.VenueTrack.DirectedCut, string>(NoteChart.VenueTrack.DirectedCut.FullBandJump, "all_yeah"), // "all_yeah" on their documentation, I have "all_year"?
			new Pair<NoteChart.VenueTrack.DirectedCut, string>(NoteChart.VenueTrack.DirectedCut.Bassist, "bass"),
			new Pair<NoteChart.VenueTrack.DirectedCut, string>(NoteChart.VenueTrack.DirectedCut.BassistIdle, "bass_np"),
			new Pair<NoteChart.VenueTrack.DirectedCut, string>(NoteChart.VenueTrack.DirectedCut.BassistCamera, "bass_cam"),
			new Pair<NoteChart.VenueTrack.DirectedCut, string>(NoteChart.VenueTrack.DirectedCut.BassistFretboard, "bass_cls"),
			new Pair<NoteChart.VenueTrack.DirectedCut, string>(NoteChart.VenueTrack.DirectedCut.BigRockEnding, "bre"),
			new Pair<NoteChart.VenueTrack.DirectedCut, string>(NoteChart.VenueTrack.DirectedCut.BigRockEndingEnd, "brej"),
			new Pair<NoteChart.VenueTrack.DirectedCut, string>(NoteChart.VenueTrack.DirectedCut.Drummer, "drums"),
			new Pair<NoteChart.VenueTrack.DirectedCut, string>(NoteChart.VenueTrack.DirectedCut.DrummerPoint, "drums_pnt"),
			new Pair<NoteChart.VenueTrack.DirectedCut, string>(NoteChart.VenueTrack.DirectedCut.DrummerIdle, "drums_np"),
			new Pair<NoteChart.VenueTrack.DirectedCut, string>(NoteChart.VenueTrack.DirectedCut.DrummerOverhead, "drums_lt"),
			new Pair<NoteChart.VenueTrack.DirectedCut, string>(NoteChart.VenueTrack.DirectedCut.DrummerPedal, "drums_kd"),
			new Pair<NoteChart.VenueTrack.DirectedCut, string>(NoteChart.VenueTrack.DirectedCut.Guitarist, "guitar"),
			new Pair<NoteChart.VenueTrack.DirectedCut, string>(NoteChart.VenueTrack.DirectedCut.GuitaristIdle, "guitar_np"),
			new Pair<NoteChart.VenueTrack.DirectedCut, string>(NoteChart.VenueTrack.DirectedCut.GuitaristCamera, "guitar_cam"),
			new Pair<NoteChart.VenueTrack.DirectedCut, string>(NoteChart.VenueTrack.DirectedCut.GuitaristFretboard, "guitar_cls"),
			new Pair<NoteChart.VenueTrack.DirectedCut, string>(NoteChart.VenueTrack.DirectedCut.Stagedive, "stagedive"),
			new Pair<NoteChart.VenueTrack.DirectedCut, string>(NoteChart.VenueTrack.DirectedCut.Crowdsurf, "crowdsurf"),
			new Pair<NoteChart.VenueTrack.DirectedCut, string>(NoteChart.VenueTrack.DirectedCut.Vocalist, "vocals"),
			new Pair<NoteChart.VenueTrack.DirectedCut, string>(NoteChart.VenueTrack.DirectedCut.VocalistIdle, "vocals_np"),
			new Pair<NoteChart.VenueTrack.DirectedCut, string>(NoteChart.VenueTrack.DirectedCut.VocalistCamera, "vocals_cam"),
			new Pair<NoteChart.VenueTrack.DirectedCut, string>(NoteChart.VenueTrack.DirectedCut.VocalistCloseup, "vocals_cls"),
			new Pair<NoteChart.VenueTrack.DirectedCut, string>(NoteChart.VenueTrack.DirectedCut.GuitaristCrowd, "crowd_g"),
			new Pair<NoteChart.VenueTrack.DirectedCut, string>(NoteChart.VenueTrack.DirectedCut.BassistCrowd, "crowd_b")
		};

		public static string ToShortString(this NoteChart.VenueTrack.DirectedCut cut)
		{
			var found = DirectedCuts.Find(p => p.Key == cut);
			if (found != null)
				return "directed_" + found.Value;
			else
				return String.Empty;
		}

		public static NoteChart.VenueTrack.DirectedCut DirectedCutFromString(string shortname)
		{
			var found = DirectedCuts.Find(p => "directed_" + p.Value == shortname);
			if (found != null)
				return found.Key;
			else
				return NoteChart.VenueTrack.DirectedCut.Guitarist;
		}

		public static List<Pair<NoteChart.EventsTrack.CrowdType, string>> CrowdTypes = new List<Pair<NoteChart.EventsTrack.CrowdType, string>> {
			new Pair<NoteChart.EventsTrack.CrowdType, string>(NoteChart.EventsTrack.CrowdType.Mellow, "mellow"),
			new Pair<NoteChart.EventsTrack.CrowdType, string>(NoteChart.EventsTrack.CrowdType.Normal, "normal"),
			new Pair<NoteChart.EventsTrack.CrowdType, string>(NoteChart.EventsTrack.CrowdType.Realtime, "realtime"),
			new Pair<NoteChart.EventsTrack.CrowdType, string>(NoteChart.EventsTrack.CrowdType.Intense, "intense"),
			new Pair<NoteChart.EventsTrack.CrowdType, string>(NoteChart.EventsTrack.CrowdType.Clap, "clap"),
			new Pair<NoteChart.EventsTrack.CrowdType, string>(NoteChart.EventsTrack.CrowdType.NoClap, "noclap"),
		};

		public static string ToShortString(this NoteChart.EventsTrack.CrowdType crowd)
		{
			var found = CrowdTypes.Find(p => p.Key == crowd);
			if (found != null)
				return "crowd_" + found.Value;
			else
				return String.Empty;
		}

		public static NoteChart.EventsTrack.CrowdType CrowdTypeFromString(string shortname)
		{
			var found = CrowdTypes.Find(p => "crowd_" + p.Value == shortname);
			if (found != null)
				return found.Key;
			else
				return NoteChart.EventsTrack.CrowdType.Normal;
		}

		public static List<Pair<NoteChart.CharacterMood, string>> CharacterMoods = new List<Pair<NoteChart.CharacterMood, string>> {
			new Pair<NoteChart.CharacterMood, string>(NoteChart.CharacterMood.Idle, "idle"),
			new Pair<NoteChart.CharacterMood, string>(NoteChart.CharacterMood.IdleRealtime, "idle_realtime"),
			new Pair<NoteChart.CharacterMood, string>(NoteChart.CharacterMood.IdleIntense, "idle_intense"),
			new Pair<NoteChart.CharacterMood, string>(NoteChart.CharacterMood.Intense, "intense"),
			new Pair<NoteChart.CharacterMood, string>(NoteChart.CharacterMood.Mellow, "mellow"),
			new Pair<NoteChart.CharacterMood, string>(NoteChart.CharacterMood.Play, "play")
		};

		public static string ToShortString(this NoteChart.CharacterMood mood)
		{
			var found = CharacterMoods.Find(p => p.Key == mood);
			if (found != null)
				return found.Value;
			else
				return String.Empty;
		}

		public static NoteChart.CharacterMood CharacterMoodFromString(string shortname)
		{
			var found = CharacterMoods.Find(p => p.Value == shortname);
			if (found != null)
				return found.Key;
			else
				return NoteChart.CharacterMood.None;
		}

		public static List<Pair<NoteChart.LeftHandMap, string>> LeftHandMaps = new List<Pair<NoteChart.LeftHandMap, string>> {
			new Pair<NoteChart.LeftHandMap, string>(NoteChart.LeftHandMap.AllBend, "AllBend"),
			new Pair<NoteChart.LeftHandMap, string>(NoteChart.LeftHandMap.AllChords, "AllChords"),
			new Pair<NoteChart.LeftHandMap, string>(NoteChart.LeftHandMap.ChordA, "Chord_A"),
			new Pair<NoteChart.LeftHandMap, string>(NoteChart.LeftHandMap.ChordC, "Chord_C"),
			new Pair<NoteChart.LeftHandMap, string>(NoteChart.LeftHandMap.ChordD, "Chord_D"),
			new Pair<NoteChart.LeftHandMap, string>(NoteChart.LeftHandMap.Default, "Default"),
			new Pair<NoteChart.LeftHandMap, string>(NoteChart.LeftHandMap.DropD, "DropD"),
			new Pair<NoteChart.LeftHandMap, string>(NoteChart.LeftHandMap.DropD2, "DropD2"),
			new Pair<NoteChart.LeftHandMap, string>(NoteChart.LeftHandMap.NoChords, "NoChords"),
			new Pair<NoteChart.LeftHandMap, string>(NoteChart.LeftHandMap.Solo, "Solo")
		};

		public static string ToShortString(this NoteChart.LeftHandMap map)
		{
			var found = LeftHandMaps.Find(p => p.Key == map);
			if (found != null)
				return found.Value;
			else
				return String.Empty;
		}

		public static NoteChart.LeftHandMap LeftHandMapFromString(string shortname)
		{
			var found = LeftHandMaps.Find(p => p.Value == shortname);
			if (found != null)
				return found.Key;
			else
				return NoteChart.LeftHandMap.Default;
		}

		public static List<Pair<NoteChart.RightHandMap, string>> RightHandMaps = new List<Pair<NoteChart.RightHandMap, string>> {
			new Pair<NoteChart.RightHandMap, string>(NoteChart.RightHandMap.Default, "Default"),
			new Pair<NoteChart.RightHandMap, string>(NoteChart.RightHandMap.Pick, "Pick"),
			new Pair<NoteChart.RightHandMap, string>(NoteChart.RightHandMap.SlapBass, "SlapBass")
		};

		public static string ToShortString(this NoteChart.RightHandMap map)
		{
			var found = RightHandMaps.Find(p => p.Key == map);
			if (found != null)
				return found.Value;
			else
				return String.Empty;
		}

		public static NoteChart.RightHandMap RightHandMapFromString(string shortname)
		{
			var found = RightHandMaps.Find(p => p.Value == shortname);
			if (found != null)
				return found.Key;
			else
				return NoteChart.RightHandMap.Default;
		}

		public static NoteChart.Drums.AnimationHandedness GetAnimationHandedness(this NoteChart.Drums.Animation animation)
		{
			switch (animation) {
				case NoteChart.Drums.Animation.KickRightFoot:
					return NoteChart.Drums.AnimationHandedness.RightFoot;

				case NoteChart.Drums.Animation.HiHatPedalLeftFoot:
					return NoteChart.Drums.AnimationHandedness.LeftFoot;

				case NoteChart.Drums.Animation.SnareLeftHand:
				case NoteChart.Drums.Animation.HiHatLeftHand:
				case NoteChart.Drums.Animation.Crash1HardLeftHand:
				case NoteChart.Drums.Animation.Crash1SoftLeftHand:
				case NoteChart.Drums.Animation.Tom1LeftHand:
				case NoteChart.Drums.Animation.Tom2LeftHand:
				case NoteChart.Drums.Animation.FloorTomLeftHand:
					return NoteChart.Drums.AnimationHandedness.LeftHand;

				case NoteChart.Drums.Animation.SnareRightHand:
				case NoteChart.Drums.Animation.HiHatRightHand:
				case NoteChart.Drums.Animation.PercussionRightHand:
				case NoteChart.Drums.Animation.Crash1HardRightHand:
				case NoteChart.Drums.Animation.Crash1SoftRightHand:
				case NoteChart.Drums.Animation.Crash2HardRightHand:
				case NoteChart.Drums.Animation.Crash2SoftRightHand:
				case NoteChart.Drums.Animation.RideCymbalRightHand:
				case NoteChart.Drums.Animation.Tom1RightHand:
				case NoteChart.Drums.Animation.Tom2RightHand:
				case NoteChart.Drums.Animation.FloorTomRightHand:
					return NoteChart.Drums.AnimationHandedness.RightHand;

				case NoteChart.Drums.Animation.Crash1Choke:
				case NoteChart.Drums.Animation.Crash2Choke:
				default:
					return NoteChart.Drums.AnimationHandedness.BothHands;
			}
		}

		public static List<NoteChart.Note> FindGemsUnderNote(this NoteChart.IGems gems, NoteChart.Difficulty difficulty, NoteChart.Note note)
		{
			List<NoteChart.Note> ret = new List<NoteChart.Note>();

			foreach (var g in gems.Gems[difficulty]) {
				foreach (var n in g) {
					if (n.IsUnderNote(note))
						ret.Add(n);
				}
			}

			return ret;
		}

		public static bool IsUnderNote(this NoteChart.Note note, NoteChart.Note parent)
		{
			return note.IsUnderNote(parent, true);
		}

		public static bool IsUnderNote(this NoteChart.Note note, NoteChart.Note parent, bool alsocheckduration)
		{
			return (note.Time >= parent.Time && note.Time < parent.Time + parent.Duration) || (alsocheckduration && (note.Time + note.Duration > parent.Time && note.Time + note.Duration <= parent.Time + parent.Duration));
		}

		public static List<NoteChart.Note> SplitNote(this NoteChart.Note note, NoteChart.Note remove)
		{
			List<NoteChart.Note> notes = new List<NoteChart.Note>();
			NoteChart.Note nnote = new NoteChart.Note() { Time = note.Time, Duration = note.Duration, Velocity = note.Velocity, ReleaseVelocity = note.ReleaseVelocity };
			notes.Add(nnote);

			if (remove.Time >= note.Time && remove.Time <= note.Time + note.Duration) {
				// remove starts inside note; shorten the first and split
				nnote.Duration = remove.Time - note.Time;
				// create another note starting at the end of remove to the end of note
				if (remove.Time + remove.Duration < note.Time + note.Duration) {
					nnote = new NoteChart.Note() { Time = remove.Time + remove.Duration, Duration = note.Time + note.Duration - (remove.Time + remove.Duration), Velocity = note.Velocity, ReleaseVelocity = note.ReleaseVelocity };
					notes.Add(nnote);
				}
			} else if (remove.Time + remove.Duration > note.Time && remove.Time + remove.Duration <= note.Time + note.Duration) {
				// remove start outside the note; just shorten it
				ulong diff = remove.Time + remove.Duration - note.Time;
				nnote.Time = remove.Time + remove.Duration;
				nnote.Duration -= remove.Time + remove.Duration - note.Time;
			}

		__fucking_collections_and_shit:
			foreach (var n in notes) {
				if (n.Duration == 0) {
					notes.Remove(n);
					goto __fucking_collections_and_shit;
				}
			}

			return notes;
		}
	}
}

// Missing:
//   PART DRUM
//     - 110 - 112: Tom gems
//     - H2H