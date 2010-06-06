using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using Nanook.QueenBee.Parser;
using ConsoleHaxx.Harmonix;
using ConsoleHaxx.Common;
using ConsoleHaxx.Neversoft;

namespace ConsoleHaxx.RawkSD
{
	public class ChartFormatGH3 : IChartFormat
	{
		public const string ChartName = "chart";
		public const string SectionsName = "sections";

		public static ChartFormatGH3 Instance;
		public static void Initialise()
		{
			Instance = new ChartFormatGH3();
			Platform.AddFormat(Instance);
		}

		public override int ID {
			get { return 0x44; }
		}

		public override string Name {
			get { return "Guitar Hero 3 Chart"; }
		}

		public void Create(FormatData data, Stream chart, Stream sections, bool coop)
		{
			data.SetStream(this, ChartName, chart);
			data.SetStream(this, SectionsName, sections);

			data.Song.Data.SetValue("GH3ChartCoop", coop);
		}

		public override bool Writable {
			get { return false; }
		}

		public override bool Readable {
			get { return true; }
		}

		public override ChartFormat DecodeChart(FormatData data, ProgressIndicator progress)
		{
			if (!data.HasStream(this, ChartName) || !data.HasStream(this, SectionsName))
				throw new FormatException();

			progress.NewTask(6 + 8);

			Stream chartstream = data.GetStream(this, ChartName);
			Stream sectionstream = data.GetStream(this, SectionsName);

			PakFormat format = NeversoftMetadata.GetSongItemType(data.Song);
			SongData song = NeversoftMetadata.GetSongData(data.PlatformData, NeversoftMetadata.GetSongItem(data.Song));
			Pak chartpak = new Pak(new EndianReader(chartstream, Endianness.BigEndian)); // TODO: Endianness based on format?
			FileNode chartfile = chartpak.Root.Find(song.ID + ".mid.qb.ngc", SearchOption.AllDirectories, true) as FileNode;
			QbFile qbsections = new QbFile(sectionstream, format);
			QbFile qbchart = new QbFile(chartfile.Data, format);

			NoteChart chart = new NoteChart();
			chart.PartGuitar = new NoteChart.Guitar(chart);
			chart.PartBass = new NoteChart.Bass(chart);
			chart.Events = new NoteChart.EventsTrack(chart);
			chart.Venue = new NoteChart.VenueTrack(chart);
			chart.Beat = new NoteChart.BeatTrack(chart);

			progress.Progress();

			DecodeChartFretbars(song, qbchart, chart);

			progress.Progress();

			DecodeChartMarkers(song, qbsections, qbchart, chart);

			progress.Progress();

			for (NoteChart.TrackType track = NoteChart.TrackType.Guitar; track <= NoteChart.TrackType.Bass; track++) {
				for (NoteChart.Difficulty difficulty = NoteChart.Difficulty.Easy; difficulty <= NoteChart.Difficulty.Expert; difficulty++) {
					DecodeChartNotes(data, song, qbchart, chart, track, difficulty, data.Song.Data.GetValue<bool>("GH3ChartCoop"));
					progress.Progress();
				}
			}

			progress.Progress();

			DecodeChartDrums(song, qbchart, chart);

			progress.Progress();

			DecodeChartVenue(song, qbchart, chart);

			ImportMap.ImportChart(data.Song, chart);

			progress.Progress();

			data.CloseStream(chartstream);
			data.CloseStream(sectionstream);

			progress.EndTask();

			return new ChartFormat(chart);
		}

		private static void DecodeChartVenue(SongData song, QbFile qbchart, NoteChart chart)
		{
			QbItemArray cameraitems = qbchart.FindItem(QbKey.Create(song.ID + "_cameras_notes"), false) as QbItemArray;
			if (cameraitems != null && cameraitems.Items.Count > 0)
				cameraitems = cameraitems.Items[0] as QbItemArray;
			if (cameraitems != null) {
				Random random = new Random();
				foreach (QbItemInteger camera in cameraitems.Items) {
					uint time = camera.Values[0];
					NoteChart.Point point = new NoteChart.Point(chart.GetTicks(camera.Values[0]));

					if (random.Next() % 9 == 0)
						chart.Venue.DirectedShots.Add(new Pair<NoteChart.Point, NoteChart.VenueTrack.DirectedCut>(point, NoteChart.VenueTrack.DirectedCut.None));
					else
						chart.Venue.CameraShots.Add(new Pair<NoteChart.Point, NoteChart.VenueTrack.CameraShot>(point, new NoteChart.VenueTrack.CameraShot()));
				}
			}

			QbItemArray crowditems = qbchart.FindItem(QbKey.Create(song.ID + "_crowd_notes"), false) as QbItemArray;
			if (crowditems != null && crowditems.Items.Count > 0)
				crowditems = crowditems.Items[0] as QbItemArray;
			if (crowditems != null) {
				foreach (QbItemInteger crowd in crowditems.Items) {
					NoteChart.Point point = new NoteChart.Point(chart.GetTicks(crowd.Values[0]));
					chart.Events.Crowd.Add(new Pair<NoteChart.Point, NoteChart.EventsTrack.CrowdType>(point, NoteChart.EventsTrack.CrowdType.None));
				}
			}

			QbItemArray lightitems = qbchart.FindItem(QbKey.Create(song.ID + "_lightshow_notes"), false) as QbItemArray;
			if (lightitems != null && lightitems.Items.Count > 0)
				lightitems = lightitems.Items[0] as QbItemArray;
			if (lightitems != null) {
				foreach (QbItemInteger light in lightitems.Items) {
					NoteChart.Point point = new NoteChart.Point(chart.GetTicks(light.Values[0]));
					chart.Venue.Lighting.Add(new Pair<NoteChart.Point, NoteChart.VenueTrack.LightingType>(point, NoteChart.VenueTrack.LightingType.None));
				}
			}
		}

		private static void DecodeChartDrums(SongData song, QbFile qbchart, NoteChart chart)
		{
			QbItemArray drumsitems = qbchart.FindItem(QbKey.Create(song.ID + "_drums_notes"), false) as QbItemArray;
			if (drumsitems != null)
				drumsitems = drumsitems.Items[0] as QbItemArray;
			if (drumsitems != null) {
				chart.PartDrums = new NoteChart.Drums(chart);
				Dictionary<uint, int> drumnotes = new Dictionary<uint, int>() { // Garbage: 65, 70, 48, 64
					{ 60, 0 },
					{ 40, 1 }, { 64, 1 },
					{ 55, 2 }, { 67, 2 }, { 53, 2 },
					{ 39, 3 }, { 38, 3 }, { 63, 3 }, { 62, 3 },
					{ 68, 4 }, { 56, 4 }, { 66, 4 }, { 54, 4 }, { 69, 4 }, { 57, 4 }, { 37, 4 }, { 61, 4 },
				};
				Dictionary<uint, NoteChart.Drums.Animation> drumanimnotes = new Dictionary<uint, NoteChart.Drums.Animation>() { };

				foreach (QbItemInteger drums in drumsitems.Items) {
					NoteChart.Note note = new NoteChart.Note(chart.GetTicks(drums.Values[0]), (ulong)chart.Division.TicksPerBeat / 8);
					uint notenum = drums.Values[1];
					if (drumnotes.ContainsKey(notenum)) {
						int notevalue = drumnotes[notenum];
						chart.PartDrums.Gems[NoteChart.Difficulty.Expert][notevalue].Add(note);
					}
					if (drumanimnotes.ContainsKey(notenum))
						chart.PartDrums.Animations.Add(new Pair<NoteChart.Note, NoteChart.Drums.Animation>(note, drumanimnotes[notenum]));
				}
			}

			ChartFormatGH5.FillSections(chart, 1, 8, 1, chart.PartDrums.Overdrive, null);
			ChartFormatGH5.FillSections(chart, 1, 4, 3, chart.PartDrums.DrumFills, null);
		}

		private static void DecodeChartNotes(FormatData data, SongData song, QbFile qbchart, NoteChart chart, NoteChart.TrackType track, NoteChart.Difficulty difficulty, bool coop)
		{
			string basetrack = string.Empty;
			string basetrackstar = string.Empty;
			string basetrackfaceoff = string.Empty;
			NoteChart.Instrument instrument = null;
			switch (track) {
				case NoteChart.TrackType.Guitar:
					basetrack = song.ID + (coop ? "_song_guitarcoop_" : "_song_") + difficulty.DifficultyToString();
					basetrackstar = song.ID + (coop ? "_guitarcoop_" : "_") + difficulty.DifficultyToString() + "_Star";
					basetrackfaceoff = song.ID + "_FaceOffp";
					instrument = chart.PartGuitar;
					break;
				case NoteChart.TrackType.Bass:
					basetrack = song.ID + (coop ? "_song_rhythmcoop_" : "_song_rhythm_") + difficulty.DifficultyToString();
					basetrackstar = song.ID + (coop ? "_rhythmcoop_" : "_rhythm_") + difficulty.DifficultyToString() + "_Star";
					basetrackfaceoff = song.ID + "_FaceOffP";
					instrument = chart.PartBass;
					break;
			}

			if (instrument == null)
				return;

			if (difficulty == NoteChart.Difficulty.Expert) { // GH3 has SP for each difficulty; RB2 has one OD for all
				QbItemArray faceoff1 = (qbchart.FindItem(QbKey.Create(basetrackfaceoff + "1"), false) as QbItemArray).Items[0] as QbItemArray;
				if (faceoff1 != null) {
					foreach (QbItemInteger faceoff in faceoff1.Items) {
						NoteChart.Note fnote = new NoteChart.Note(chart.GetTicks(faceoff.Values[0]), chart.GetTicksDuration(faceoff.Values[0], faceoff.Values[1]));
						instrument.Player1.Add(fnote);
					}
				}
				QbItemArray faceoff2 = (qbchart.FindItem(QbKey.Create(basetrackfaceoff + "2"), false) as QbItemArray).Items[0] as QbItemArray;
				if (faceoff2 != null) {
					foreach (QbItemInteger faceoff in faceoff2.Items) {
						NoteChart.Note fnote = new NoteChart.Note(chart.GetTicks(faceoff.Values[0]), chart.GetTicksDuration(faceoff.Values[0], faceoff.Values[1]));
						instrument.Player2.Add(fnote);
					}
				}
				QbItemArray starpower = (qbchart.FindItem(QbKey.Create(basetrackstar), false) as QbItemArray).Items[0] as QbItemArray;
				if (starpower != null) {
					foreach (QbItemInteger star in starpower.Items)
						instrument.Overdrive.Add(new NoteChart.Note(chart.GetTicks(star.Values[0]), chart.GetTicksDuration(star.Values[0], star.Values[1])));
				}
			}

			int previouschordnum = 0;
			int previouschord = 0;
			NoteChart.Note previousnote = new NoteChart.Note() { Time = uint.MaxValue, Duration = 0 };
			QbItemInteger notes = (qbchart.FindItem(QbKey.Create(basetrack), false) as QbItemArray).Items[0] as QbItemInteger;
			if (notes == null) {
				if (track == NoteChart.TrackType.Guitar)
					chart.PartGuitar = null;
				else
					chart.PartBass = null;
				return;
			}

			int note32 = chart.Division.TicksPerBeat / 8;
			int note16 = chart.Division.TicksPerBeat / 4;

			for (int k = 0; k < notes.Values.Length; k += 3) {
				NoteChart.Note note = new NoteChart.Note(chart.GetTicks(notes.Values[k]), chart.GetTicksDuration(notes.Values[k], notes.Values[k + 1]));
				int chordnum = 0;
				int chord = 0;

				// Cut off sustains to a 32nd note before the next
				previousnote.Duration = (ulong)Math.Max(Math.Min((long)previousnote.Duration, (long)note.Time - (long)previousnote.Time - note16), note32);

				bool hopo = note.Time - previousnote.Time <= (ulong)data.Song.HopoThreshold;
				bool ishopo = hopo;
				hopo = hopo && previouschordnum == 1;

				uint fret = notes.Values[k + 2];
				for (int l = 0; l < 6; l++) {
					if ((fret & 0x01) != 0) {
						if (l < 5) {
							chord = l;
							chordnum++;
							(instrument as NoteChart.IGems).Gems[difficulty][l].Add(note);
						} else // l == 5; hopo toggle bit
							ishopo = !ishopo;
					}

					fret >>= 1;
				}

				if (chordnum == 0) { // Old TheGHOST bug, should be a green note
					chordnum = 1;
					chord = 0;
					(instrument as NoteChart.IGems).Gems[difficulty][0].Add(note);
				}

				if (chord == previouschord)
					ishopo = false;

				if (ishopo != hopo && chordnum == 1) {
					if (ishopo)
						(instrument as NoteChart.IForcedHopo).ForceHammeron[difficulty].Add(note);
					else
						(instrument as NoteChart.IForcedHopo).ForceStrum[difficulty].Add(note);
				}

				previouschord = chord;
				previousnote = note;
				previouschordnum = chordnum;
			}
		}

		private static void DecodeChartFretbars(SongData song, QbFile qbchart, NoteChart chart)
		{
			QbItemInteger fretbars = (qbchart.FindItem(QbKey.Create(song.ID + "_fretbars"), false) as QbItemArray).Items[0] as QbItemInteger;
			ulong ticks = 0;
			uint previousfret = 0;
			uint previousMsPerBeat = 0;
			for (int k = 1; k < fretbars.Values.Length; k++) {
				uint fret = fretbars.Values[k];
				uint msPerBeat = fret - previousfret;
				if (msPerBeat != previousMsPerBeat)
					chart.BPM.Add(new Midi.TempoEvent(ticks, msPerBeat * 1000));

				previousfret = fret;
				previousMsPerBeat = msPerBeat;
				ticks += chart.Division.TicksPerBeat;
			}

			chart.Events.End = new NoteChart.Point(chart.GetTicks(fretbars.Values[fretbars.Values.Length - 1]));

			QbItemArray timesig = (qbchart.FindItem(QbKey.Create(song.ID + "_timesig"), false) as QbItemArray).Items[0] as QbItemArray;
			foreach (QbItemInteger sig in timesig.Items)
				chart.Signature.Add(new Midi.TimeSignatureEvent(chart.GetTicks(sig.Values[0]), (byte)sig.Values[1], (byte)Math.Log(sig.Values[2], 2), 24, 8));
		}

		private static void DecodeChartMarkers(SongData song, QbFile qbsections, QbFile qbchart, NoteChart chart)
		{
			QbItemStructArray markers = (qbchart.FindItem(QbKey.Create(song.ID + "_markers"), false) as QbItemArray).Items[0] as QbItemStructArray;
			if (markers != null) {
				foreach (QbItemStruct mark in markers.Items) {
					QbItemString section = qbsections.FindItem((mark.Items[1] as QbItemQbKey).Values[0], false) as QbItemString;

					chart.Events.Sections.Add(new Pair<NoteChart.Point, string>(new NoteChart.Point(chart.GetTicks((mark.Items[0] as QbItemInteger).Values[0])), section.Strings[0]));
				}
			}
		}

		public override void EncodeChart(ChartFormat data, FormatData destination, ProgressIndicator progress)
		{
			throw new NotImplementedException();
		}
	}
}
