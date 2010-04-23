using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using Nanook.QueenBee.Parser;
using ConsoleHaxx.Neversoft;
using ConsoleHaxx.Common;
using ConsoleHaxx.Harmonix;

namespace ConsoleHaxx.RawkSD
{
	public class ChartFormatGH5 : IChartFormat
	{
		public const string ChartName = "chart";

		public static readonly ChartFormatGH5 Instance;
		static ChartFormatGH5()
		{
			Instance = new ChartFormatGH5();
		}

		public override int ID {
			get { return 0x05; }
		}

		public override string Name {
			get { return "Guitar Hero 5 / Band Hero"; }
		}

		public void Create(FormatData data, Stream stream, PakFormatType pak)
		{
			data.SetStream(this, ChartName, stream);
		}

		public override bool Writable {
			get { return false; }
		}

		public override bool Readable {
			get { return false; }
		}

		public ChartFormat DecodeChart(FormatData data, Stream chartstream)
		{
			PakFormat format = MetadataFormatNeversoft.GetSongItemType(data.Song);
			SongData song = MetadataFormatNeversoft.GetSongData(MetadataFormatNeversoft.GetSongItem(data.Song));
			Pak chartpak = new Pak(new EndianReader(chartstream, Endianness.BigEndian)); // TODO: Endianness based on format?
			FileNode chartfile = chartpak.Root.Find(song.ID + ".mid.qb.ngc", SearchOption.AllDirectories, true) as FileNode;
			QbFile qbchart = new QbFile(chartfile.Data, format);

			StringList strings = new StringList();
			foreach (Pak.Node n in chartpak.Nodes)
				if (n.Filename.Length == 0)
					strings.ParseFromStream(n.Data);

			QbFile qbsections = null;
			FileNode qbsectionfile = chartpak.Root.Find(song.ID + ".mid_text.qb.ngc", SearchOption.AllDirectories, true) as FileNode;
			if (qbsectionfile != null)
				qbsections = new QbFile(qbsectionfile.Data, format);

			Notes notes = null;
			FileNode notesfile = chartpak.Root.Find(song.ID + ".note.ngc", true) as FileNode;
			if (notesfile != null)
				notes = Notes.Create(new EndianReader(notesfile.Data, Endianness.BigEndian));

			NoteChart chart = new NoteChart();
			chart.PartGuitar = new NoteChart.Guitar(chart);
			chart.PartBass = new NoteChart.Bass(chart);
			chart.PartDrums = new NoteChart.Drums(chart);
			chart.PartVocals = new NoteChart.Vocals(chart);
			chart.Events = new NoteChart.EventsTrack(chart);
			chart.Venue = new NoteChart.VenueTrack(chart);
			chart.Beat = new NoteChart.BeatTrack(chart);

			DecodeChartSections(song, qbchart, strings, qbsections, notes, chart);

			DecodeChartFretbars(song, qbchart, notes, chart);

			for (NoteChart.TrackType track = NoteChart.TrackType.Guitar; track <= NoteChart.TrackType.Drums; ) {
				for (NoteChart.Difficulty difficulty = NoteChart.Difficulty.Easy; difficulty <= NoteChart.Difficulty.Expert; difficulty++)
					DecodeChartNotes(song, qbchart, notes, chart, track, difficulty);
				switch (track) {
					case NoteChart.TrackType.Guitar: track = NoteChart.TrackType.Bass; break;
					case NoteChart.TrackType.Bass: track = NoteChart.TrackType.Drums; break;
					case NoteChart.TrackType.Drums: track = NoteChart.TrackType.Events; break;
				}
			}

			DecodeChartDrumfills(chart);

			if (DecodeChartVocals(song, qbchart, strings, notes, chart))
				DecodeChartVocalPhrases(song, qbchart, notes, chart);

			return new ChartFormat(chart);
		}

		private void DecodeChartVocalPhrases(SongData song, QbFile qbchart, Notes notes, NoteChart chart)
		{
			uint[] values = GetChartValues(notes, qbchart, QbKey.Create(0x032292A7), QbKey.Create(song.ID + "_vocals_phrases"), 4, 1);
			NoteChart.Note lastvnote = null;
			bool altvphrase = false;
			int odcounter = 1;
			ulong lastvtime = 0;
			for (int k = 0; k < values.Length; k += 2) {
				if (lastvnote != null) {
					lastvnote.Duration = chart.GetTicksDuration(lastvtime, values[k] - lastvtime) - 1;
				}

				lastvtime = values[k];
				lastvnote = new NoteChart.Note() { Time = chart.GetTicks(lastvtime) };

				odcounter++;
				odcounter %= 4;
				if (odcounter == 0)
					chart.PartVocals.Overdrive.Add(lastvnote);

				if (altvphrase)
					chart.PartVocals.Player2.Add(lastvnote);
				else
					chart.PartVocals.Player1.Add(lastvnote);
				altvphrase = !altvphrase;
			}
			if (lastvnote != null)
				lastvnote.Duration = chart.FindLastNote() - lastvnote.Time;
		}

		private bool DecodeChartVocals(SongData song, QbFile qbchart, StringList strings, Notes notes, NoteChart chart)
		{
			uint[] values = GetChartValues(notes, qbchart, QbKey.Create(0xE8E6ADCB), QbKey.Create(song.ID + "_song_vocals"), 4, 2, 1);
			for (int k = 0; k < values.Length; k += 3) {
				byte note = (byte)values[k + 2];
				if (note != 2) {
					if (note > 84)
						note -= 12;
					chart.PartVocals.Gems.Add(new Midi.NoteEvent(chart.GetTicks(values[k]), 0, note, 100, chart.GetTicksDuration(values[k], values[k + 1])));
				}
			}

			List<Pair<NoteChart.Point, string>> vlyrics = new List<Pair<NoteChart.Point, string>>();
			if (notes != null) {
				byte[][] vdata = notes.Find(QbKey.Create(0x1DA27F4E)).Data;
				foreach (byte[] d in vdata) {
					EndianReader reader = new EndianReader(new MemoryStream(d), Endianness.BigEndian);
					uint time = reader.ReadUInt32();
					string lyric = reader.ReadString();
					vlyrics.Add(new Pair<NoteChart.Point, string>(
						new NoteChart.Point(chart.GetTicks(time)),
						lyric
					));
				}
			} else {
				QbItemStructArray lyrics = (qbchart.FindItem(QbKey.Create(song.ID + "_lyrics"), false) as QbItemArray).Items[0] as QbItemStructArray;
				if (lyrics != null) {
					foreach (QbItemBase item in lyrics.Items) {
						string lyric = strings.FindItem((item.Items[1] as QbItemQbKey).Values[0]);
						vlyrics.Add(new Pair<NoteChart.Point, string>(
							new NoteChart.Point(chart.GetTicks((item.Items[0] as QbItemInteger).Values[0])),
							lyric
						));
					}
				}
			}

			Pair<NoteChart.Point, string> lastlyric = null;
			foreach (var item in vlyrics) {
				string lyric = item.Value;
				if (lyric.StartsWith("=") && lastlyric != null) {
					if (lastlyric.Value.EndsWith("-"))
						lastlyric.Value = lastlyric.Value.Replace('-', '=');
					else
						lastlyric.Value += "-";
					lyric = lyric.Substring(1);
				} else
					lyric = lyric.Replace('-', '=');
				lastlyric = new Pair<NoteChart.Point, string>(
					item.Key,
					lyric
				);
				chart.PartVocals.Lyrics.Add(lastlyric);
			}

			foreach (Midi.NoteEvent item in chart.PartVocals.Gems) {
				bool found = false;
				foreach (Pair<NoteChart.Point, string> lyric in chart.PartVocals.Lyrics) {
					if (lyric.Key.Time == item.Time) {
						if (item.Note == 26) {
							lyric.Value += "#";
							item.Note = 36;
						}
						found = true;
						break;
					}
				}
				if (!found)
					chart.PartVocals.Lyrics.Add(new Pair<NoteChart.Point, string>(new NoteChart.Point(item.Time), "+"));
			}
		__goddamnremovals:
			//Midi.NoteEvent lastnote = null;
			foreach (Pair<NoteChart.Point, string> lyric in chart.PartVocals.Lyrics) {
				bool found = false;
				foreach (Midi.NoteEvent item in chart.PartVocals.Gems) {
					if (lyric.Key.Time == item.Time) {
						found = true;
						break;
					}
				}
				if (!found) {
					/*
					ulong duration = chart.GetTicksDuration(chart.GetTime(lyric.Key.Time) / 1000, 30);
					if (lastnote != null && lastnote.Time + lastnote.Duration > lyric.Key.Time)
						lastnote.Duration = lyric.Key.Time - lastnote.Time - 20;
					lastnote = new Midi.NoteEvent(lyric.Key.Time, 0, 36, 100, duration);
					chart.PartVocals.Gems.Add(lastnote);
					lyric.Value += "^";
					*/
					chart.PartVocals.Lyrics.Remove(lyric);
					goto __goddamnremovals;
				}
			}

			if (chart.PartVocals.Gems.Count == 0) {
				chart.PartVocals = null;
				return false;
			}

			return true;
		}

		private static void DecodeChartDrumfills(NoteChart chart)
		{
			// Automatic Drum Fills - 1-measure fills every 4 measures, if there's no overdrive overlap
			Midi.TimeSignatureEvent previous = new Midi.TimeSignatureEvent(0, 4, 0, 0, 0);
			Midi.TimeSignatureEvent next;
			int index = -1;
			ulong time = chart.FindLastNote();
			long t2;
			while (time > 0) {
				try {
					next = chart.Signature[++index];
				} catch (ArgumentOutOfRangeException) {
					next = new Midi.TimeSignatureEvent(ulong.MaxValue, 4, 0, 0, 0);
				}

				t2 = (long)Math.Min(next.Time - previous.Time, time);
				long fill = (long)chart.Division.TicksPerBeat * (long)previous.Numerator * 3;
				for (long t1 = fill; t2 - t1 >= (long)chart.Division.TicksPerBeat * previous.Numerator; t1 += fill) {
					bool collide = false;
					NoteChart.Note note = new NoteChart.Note() { Time = previous.Time + (ulong)t1, Duration = (ulong)chart.Division.TicksPerBeat * previous.Numerator, Velocity = 100, ReleaseVelocity = 100 };
					foreach (NoteChart.Note n in chart.PartDrums.Overdrive) {
						if (note.IsUnderNote(n, true)) {
							collide = true;
							break;
						}
					}
					if (!collide)
						chart.PartDrums.DrumFills.Add(note);
				}
				time -= (ulong)t2;

				previous = next;
			}
		}

		private void DecodeChartNotes(SongData song, QbFile qbchart, Notes notes, NoteChart chart, NoteChart.TrackType track, NoteChart.Difficulty difficulty)
		{
			uint[] values;
			uint[][] jaggedvalues;
			QbKey basetrack = null; QbKey basetrackstar = null; QbKey basetrackfaceoff1 = null; QbKey basetrackfaceoff2 = null; QbKey basetracktapping = null;
			NoteChart.Instrument instrument = null;
			switch (track) {
				case NoteChart.TrackType.Guitar:
					instrument = chart.PartGuitar;
					if (notes != null) {
						// basetracktapping:	0x5F98331C, 0x23D065C7, 0x64C94EA2, 0xE8E24AB6
						// basetrackstar:		0xEA17B930, 0xBEE50CD6, 0x972693B1, 0x0AD32D6D
						switch (difficulty) {
							case NoteChart.Difficulty.Easy:
								basetrack = QbKey.Create(0x9B3C8C1C);
								basetracktapping = QbKey.Create(0xE8E24AB6);
								break;
							case NoteChart.Difficulty.Medium:
								basetrack = QbKey.Create(0x8E0665C1);
								basetracktapping = QbKey.Create(0x64C94EA2);
								break;
							case NoteChart.Difficulty.Hard:
								basetrack = QbKey.Create(0xD20139E4);
								basetracktapping = QbKey.Create(0x23D065C7);
								break;
							case NoteChart.Difficulty.Expert:
								basetrack = QbKey.Create(0x01FE0E80);
								basetrackstar = QbKey.Create(0xEA17B930);
								basetracktapping = QbKey.Create(0x5F98331C);
								break;
						}
					} else {
						basetrack = QbKey.Create(song.ID + "_song_" + difficulty.ToString().ToLower());
						basetrackstar = QbKey.Create(song.ID + "_" + difficulty.ToString().ToLower() + "_star");
						basetrackfaceoff1 = QbKey.Create(song.ID + "_faceoffp1");
						basetrackfaceoff2 = QbKey.Create(song.ID + "_faceoffp2");
						basetracktapping = QbKey.Create(song.ID + "_" + difficulty.ToString().ToLower() + "_tapping");
					}
					break;
				case NoteChart.TrackType.Bass:
					instrument = chart.PartBass;
					if (notes != null) {
						// basetrackstar:	Expert,		??			??			??
						//					0xEF07C4FB, 0xF93DE8E6, 0xADCF5D00, 0x72F27A27
						switch (difficulty) {
							case NoteChart.Difficulty.Easy:
								basetrack = QbKey.Create(0xBDA26454);
								break;
							case NoteChart.Difficulty.Medium:
								basetrack = QbKey.Create(0x1877EC18);
								break;
							case NoteChart.Difficulty.Hard:
								basetrack = QbKey.Create(0xF49FD1AC);
								break;
							case NoteChart.Difficulty.Expert:
								basetrack = QbKey.Create(0x978F8759);
								basetrackstar = QbKey.Create(0xEF07C4FB);
								break;
						}
					} else {
						basetrack = QbKey.Create(song.ID + "_song_rhythm_" + difficulty.ToString().ToLower());
						basetrackstar = QbKey.Create(song.ID + "_rhythm_" + difficulty.ToString().ToLower() + "_star");
						basetrackfaceoff1 = QbKey.Create(song.ID + "_rhythm_faceoffp1");
						basetrackfaceoff2 = QbKey.Create(song.ID + "_rhythm_faceoffp2");
						// basetracktapping = song.ID + "_rhythm_" + difficulty.ToString().ToLower() + "_tapping";
					}
					break;
				case NoteChart.TrackType.Drums:
					instrument = chart.PartDrums;
					if (notes != null) {
						// basetrackstar:	0xA6FAE27E, 0x630E10F8, 0x37FCA51E, 0x3B0F5CA2
						switch (difficulty) {
							case NoteChart.Difficulty.Easy:
								basetrack = QbKey.Create(0x0E0ADF37);
								break;
							case NoteChart.Difficulty.Medium:
								basetrack = QbKey.Create(0x0A140DD0);
								break;
							case NoteChart.Difficulty.Hard:
								basetrack = QbKey.Create(0x47376ACF);
								break;
							case NoteChart.Difficulty.Expert:
								basetrack = QbKey.Create(0x85EC6691);
								basetrackstar = QbKey.Create(0xA6FAE27E);
								break;
						}
					} else {
						basetrack = QbKey.Create(song.ID + "_song_drum_" + difficulty.ToString().ToLower());
						basetrackstar = QbKey.Create(song.ID + "_drum_" + difficulty.ToString().ToLower() + "_star");
						basetrackfaceoff1 = QbKey.Create(song.ID + "_drum_faceoffp1");
						basetrackfaceoff2 = QbKey.Create(song.ID + "_drum_faceoffp2");
						// basetracktapping = song.ID + "_drum_" + difficulty.ToString().ToLower() + "_tapping";
					}
					break;
			}

			if (difficulty == NoteChart.Difficulty.Expert) { // GH4 has SP for each difficulty; RB2 has one OD for all
				jaggedvalues = GetJaggedChartValues(notes, qbchart, basetrackstar, basetrackstar, 4, 2);
				foreach (uint[] star in jaggedvalues)
					instrument.Overdrive.Add(new NoteChart.Note() { Time = chart.GetTicks(star[0]), Duration = chart.GetTicksDuration(star[0], star[1]) });

				if (notes == null) {
					jaggedvalues = GetJaggedChartValues(notes, qbchart, basetrackfaceoff1, basetrackfaceoff1, 1);
					foreach (uint[] faceoff in jaggedvalues)
						instrument.Player1.Add(new NoteChart.Note() { Time = chart.GetTicks(faceoff[0]), Duration = chart.GetTicksDuration(faceoff[0], faceoff[1]) });

					jaggedvalues = GetJaggedChartValues(notes, qbchart, basetrackfaceoff2, basetrackfaceoff2, 2);
					foreach (uint[] faceoff in jaggedvalues) {
						instrument.Player2.Add(new NoteChart.Note() { Time = chart.GetTicks(faceoff[0]), Duration = chart.GetTicksDuration(faceoff[0], faceoff[1]) });
					}
				}
			}

			if (basetracktapping != null) {
				jaggedvalues = GetJaggedChartValues(notes, qbchart, basetracktapping, basetracktapping, 4, 4);
				foreach (uint[] tap in jaggedvalues) {
					if (instrument is NoteChart.IForcedHopo)
						(instrument as NoteChart.IForcedHopo).ForceHammeron[difficulty].Add(new NoteChart.Note() { Time = chart.GetTicks(tap[0]), Duration = chart.GetTicksDuration(tap[0], tap[1]) });
				}
			}

			int previouschordnum = 0;
			int previouschord = 0;
			NoteChart.Note previousnote = new NoteChart.Note() { Time = uint.MaxValue, Duration = 0 };

			values = GetChartValues(notes, qbchart, basetrack, basetrack);
			for (int k = 0; k < values.Length; k += 2) {
				uint fret = values[k + 1];
				uint length = 0;
				if (notes != null) {
					length = fret >> 16;
					fret = fret & 0x0000FFFF;
				} else {
					length = fret & 0x0000FFFF;
					fret >>= 16;
				}
				if (notes != null)
					fret = ((fret & 0xFF00) >> 8) | ((fret & 0x00FF) << 8);

				NoteChart.Note note = new NoteChart.Note() { Time = chart.GetTicks(values[k]), Duration = chart.GetTicksDuration(values[k], length), Velocity = 100, ReleaseVelocity = 100 };
				int chordnum = 0;
				int chord = 0;

				// Cut off sustains to a 32nd note before the next
				previousnote.Duration = Math.Max(Math.Min(previousnote.Duration, note.Time - previousnote.Time - (ulong)chart.Division.TicksPerBeat / 8), (ulong)chart.Division.TicksPerBeat / 8);

				uint numfrets = 5;
				if (track == NoteChart.TrackType.Drums)
					numfrets += 2;

				for (int l = 0; l < numfrets; l++) {
					if (((fret >> l) & 0x01) != 0) {
						chordnum++;
						chord = l;
						int l2 = l;
						if (track == NoteChart.TrackType.Drums) {
							if (l == 0)
								l2 = 4;
							if (l == 5)
								l2 = 0;
							if (notes != null) {
								if (l == 6)
									l2 = 4;
							} else {
								if (l == 4)
									l2 = 3;
							}
							if (l2 == 0 && ((fret & 0x2000) == 0)) {
								// Expert+
							}
						}

						(instrument as NoteChart.IGems).Gems[difficulty][l2].Add(note);

						if (instrument is NoteChart.Drums) 
							ExpandDrumRoll(chart, difficulty, note, l2);
					}
				}

				if (chordnum == 0) { // Bass open note, actually fret bit 6
					chordnum++;
					if (!(instrument is NoteChart.Drums))
						(instrument as NoteChart.IGems).Gems[difficulty][0].Add(note);
					if (instrument is NoteChart.IForcedHopo)
						(instrument as NoteChart.IForcedHopo).ForceHammeron[difficulty].Add(note); // Bass open notes become hopos, lulz
				} else if (chordnum == 1 && chord != previouschord) {
					if (instrument is NoteChart.IForcedHopo) {
						if ((fret & 0x0040) != 0)
							(instrument as NoteChart.IForcedHopo).ForceHammeron[difficulty].Add(note);
						else
							(instrument as NoteChart.IForcedHopo).ForceStrum[difficulty].Add(note);
					}
				}

				previouschord = chord;
				previousnote = note;
				previouschordnum = chordnum;
			}
		}

		public void ExpandDrumRoll(NoteChart chart, NoteChart.Difficulty difficulty, NoteChart.Note note, int gem)
		{
			if (note.Duration >= (ulong)chart.Division.TicksPerBeat) {
				ulong rolllen = 0;
				switch (difficulty) {
					case NoteChart.Difficulty.Expert:
						rolllen = (ulong)chart.Division.TicksPerBeat / 2; // 8th note rolls
						break;
					case NoteChart.Difficulty.Hard:
					case NoteChart.Difficulty.Medium:
						rolllen = (ulong)chart.Division.TicksPerBeat; // 4th note rolls
						break;
					case NoteChart.Difficulty.Easy:
						rolllen = 0; // Single hit
						break;
				}
				if (rolllen > 0) {
					for (ulong pos = rolllen; pos < note.Duration; pos += rolllen) {
						NoteChart.Note newnote = new NoteChart.Note() { Time = note.Time + pos, Duration = (ulong)chart.Division.TicksPerBeat / 8 };
						chart.PartDrums.Gems[difficulty][gem].Add(newnote);
					}
				}
			}

			note.Duration = (ulong)chart.Division.TicksPerBeat / 8;
		}
		private void DecodeChartFretbars(SongData song, QbFile qbchart, Notes notes, NoteChart chart)
		{
			uint[] values = GetChartValues(notes, qbchart, Notes.KeyFretbars, QbKey.Create(song.ID + "_fretbars"));
			ulong ticks = 0;
			uint previousfret = 0;
			uint previousMsPerBeat = 0;
			for (int k = 1; k < values.Length; k++) {
				uint fret = values[k];
				uint msPerBeat = fret - previousfret;
				if (msPerBeat != previousMsPerBeat)
					chart.BPM.Add(new Midi.TempoEvent(ticks, msPerBeat * 1000));

				previousfret = fret;
				previousMsPerBeat = msPerBeat;
				ticks += chart.Division.TicksPerBeat;
			}

			chart.Events.End = new NoteChart.Point(chart.GetTicks(values[values.Length - 1]));

			uint[][] jaggedvalues = GetJaggedChartValues(notes, qbchart, Notes.KeyTimesignature, QbKey.Create(song.ID + "_timesig"), 4, 1, 1);
			foreach (uint[] sig in jaggedvalues)
				chart.Signature.Add(new Midi.TimeSignatureEvent(chart.GetTicks(sig[0]), (byte)sig[1], (byte)Math.Log(sig[2], 2), 24, 8));
		}

		private void DecodeChartSections(SongData song, QbFile qbchart, StringList strings, QbFile qbsections, Notes notes, NoteChart chart)
		{
			if (notes != null) {
				uint[] values = GetChartValues(notes, null, QbKey.Create(0x92511D84), null);
				for (int k = 0; k < values.Length; k += 2) {
					uint time = values[k];
					string text = strings.FindItem(QbKey.Create(values[k + 1]));

					chart.Events.Sections.Add(new Pair<NoteChart.Point, string>(new NoteChart.Point(chart.GetTicks(time)), text.Replace(' ', '_').Trim().ToLower()));
				}
			} else {
				QbItemStructArray markers = (qbchart.FindItem(QbKey.Create(song.ID + "_guitar_markers"), false) as QbItemArray).Items[0] as QbItemStructArray;
				foreach (QbItemStruct mark in markers.Items) {
					QbItemQbKey section = qbsections.FindItem((mark.Items[1] as QbItemQbKey).Values[0], false) as QbItemQbKey;

					chart.Events.Sections.Add(new Pair<NoteChart.Point, string>(new NoteChart.Point(chart.GetTicks((mark.Items[0] as QbItemInteger).Values[0])), strings.FindItem(section.Values[0]).Replace(' ', '_').Trim().ToLower()));
				}
			}
		}

		protected uint[] GetChartValues(Notes notes, QbFile qbchart, QbKey notekey, QbKey qbkey, params int[] split)
		{
			Notes.Entry entry;
			List<uint> nums = new List<uint>();
			if (notes == null) {
				QbItemArray data = qbchart.FindItem(qbkey, false) as QbItemArray;
				while (data.Items[0] is QbItemArray)
					data = data.Items[0] as QbItemArray;
				foreach (QbItemBase num in data.Items) {
					if (num is QbItemInteger)
						nums.AddRange((num as QbItemInteger).Values);
					else if (num is QbItemFloat)
						nums.AddRange((num as QbItemFloat).Values.Cast<uint>().ToArray());
				}
			} else {
				entry = notes.Find(notekey);

				if (split.Length == 0)
					split = new int[] { 4 };

				for (int i = 0; i < entry.Data.Length; i++) {
					int pos = 0;
					for (int k = 0; k < split.Length; k++) {
						byte[] data = new byte[4];
						Array.Copy(entry.Data[i], pos, data, 4 - split[k], split[k]);
						nums.Add(BigEndianConverter.ToUInt32(data));
						pos += split[k];
					}
				}
			}

			return nums.ToArray();
		}

		protected uint[][] GetJaggedChartValues(Notes notes, QbFile qbchart, QbKey notekey, QbKey qbkey, params int[] split)
		{
			Notes.Entry entry;
			List<uint[]> nums = new List<uint[]>();
			if (notes == null) {
				QbItemArray data = qbchart.FindItem(qbkey, false) as QbItemArray;
				while (data.Items[0] is QbItemArray)
					data = data.Items[0] as QbItemArray;
				foreach (QbItemBase num in data.Items) {
					if (num is QbItemInteger)
						nums.Add((num as QbItemInteger).Values);
					else if (num is QbItemFloat)
						nums.Add((num as QbItemFloat).Values.Cast<uint>().ToArray());
				}
			} else {
				entry = notes.Find(notekey);

				if (split.Length == 0)
					split = new int[] { 4 };

				for (int i = 0; i < entry.Data.Length; i++) {
					uint[] num = new uint[split.Length];
					int pos = 0;
					for (int k = 0; k < split.Length; k++) {
						byte[] data = new byte[4];
						Array.Copy(entry.Data[i], pos, data, 4 - split[k], split[k]);
						num[k] = BigEndianConverter.ToUInt32(data);
						pos += split[k];
					}
					nums.Add(num);
				}
			}

			return nums.ToArray();
		}

		public override ChartFormat DecodeChart(FormatData data)
		{
			if (!data.HasStream(this, ChartName))
				throw new FormatException();

			Stream chartstream = data.GetStream(this, ChartName);

			ChartFormat format = DecodeChart(data, chartstream);

			data.CloseStream(this, ChartName);

			return format;
		}

		public override void EncodeChart(ChartFormat data, FormatData destination, ProgressIndicator progress)
		{
			throw new NotImplementedException();
		}
	}
}
