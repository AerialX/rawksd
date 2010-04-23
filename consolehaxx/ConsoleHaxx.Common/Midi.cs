using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace ConsoleHaxx.Common
{
	public class Midi
	{
		public string Name;

		public List<TempoEvent> BPM;
		public List<TimeSignatureEvent> Signature;
		public Mid.TicksPerBeatDivision Division;

		public List<Track> Tracks;

		public Midi()
		{
			Tracks = new List<Track>();
			BPM = new List<TempoEvent>();
			Signature = new List<TimeSignatureEvent>();
		}

		public Mid ToMid()
		{
			Mid mid = new Mid();
			mid.Type = Mid.MidiType.Uniform;
			mid.Division = Division;

			Mid.Track beat = new Mid.Track();
			beat.Events.Add(new Mid.MetaEvent() {
				DeltaTime = 0,
				Type = 0x03,
				Data = (Name == null || Name.Length == 0) ? Util.Encoding.GetBytes("rawksd") : Util.Encoding.GetBytes(Name)
			});

			List<Event> events = new List<Event>();
			events.AddRange(BPM.ToArray());
			events.AddRange(Signature.ToArray());
			events.Sort(new EventComparer());
			ulong delta = 0;
			foreach (Event e in events) {
				if (e is TimeSignatureEvent) {
					TimeSignatureEvent sig = e as TimeSignatureEvent;
					beat.Events.Add(new Mid.MetaEvent() { DeltaTime = (uint)(sig.Time - delta), Type = 0x58, Data = new byte[] { sig.Numerator, sig.Denominator, sig.Metronome, sig.NumberOf32ndNotes } });
				} else if (e is TempoEvent) {
					TempoEvent bpm = e as TempoEvent;
					byte[] mpqn = new byte[3];
					Array.Copy(BigEndianConverter.GetBytes(bpm.MicrosecondsPerBeat), 1, mpqn, 0, 3);
					beat.Events.Add(new Mid.MetaEvent() { DeltaTime = (uint)(bpm.Time - delta), Type = 0x51, Data = mpqn });
				}
				delta = e.Time;
			}
			beat.Events.Add(new Mid.MetaEvent() { DeltaTime = 0, Type = 0x2F, Data = new byte[0] });
			mid.Tracks.Add(beat);
			foreach (Track t in Tracks) {
				events.Clear();
				t.Comments.ForEach(e => { if (e.Type == TextEvent.TextEventType.Unknown) e.Type = TextEvent.TextEventType.Comment; });
				t.Lyrics.ForEach(e => { if (e.Type == TextEvent.TextEventType.Unknown) e.Type = TextEvent.TextEventType.Lyric; });
				t.Markers.ForEach(e => { if (e.Type == TextEvent.TextEventType.Unknown) e.Type = TextEvent.TextEventType.Marker; });
				events.AddRange(t.Notes.ToArray());
				events.AddRange(t.Comments.ToArray());
				events.AddRange(t.Markers.ToArray());
				events.AddRange(t.Lyrics.ToArray());
				events.Sort(new EventComparer());
				delta = 0;
				Mid.Track track = new Mid.Track();
				track.Events.Add(new Mid.MetaEvent() { DeltaTime = 0, Type = 0x03, Data = Util.Encoding.GetBytes(t.Name) });

				List<NoteEvent> OpenNotes = new List<NoteEvent>();
				foreach (Event e in events) {
				__fuck_labels_opennotesagain:
					foreach (NoteEvent n in OpenNotes) {
						if (n.Time + n.Duration <= e.Time) {
							track.Events.Add(new Mid.ChannelEvent() { DeltaTime = (uint)(n.Time + n.Duration - delta), Channel = n.Channel, Type = 0x8, Parameter1 = n.Note, Parameter2 = n.ReleaseVelocity });
							delta = n.Time + n.Duration;
							OpenNotes.Remove(n);
							goto __fuck_labels_opennotesagain; // Yeah, too lazy to make a ToRemove list
						}
					}
					if (e is NoteEvent) {
						NoteEvent n = e as NoteEvent;

						NoteEvent overlap = OpenNotes.Find(n2 => n2.Note == n.Note);
						if (overlap != null) { // Stretch the open note over the colliding note
							overlap.Duration = Math.Max(overlap.Duration, n.Time + n.Duration - overlap.Time);
							OpenNotes.Sort(new NoteEventComparer());
							continue;
						} else {
							OpenNotes.Insert(0, n);
							OpenNotes.Sort(new NoteEventComparer());
							track.Events.Add(new Mid.ChannelEvent() { DeltaTime = (uint)(n.Time - delta), Channel = n.Channel, Type = 0x9, Parameter1 = n.Note, Parameter2 = n.Velocity });
						}
					} else if (e is TextEvent) {
						TextEvent l = e as TextEvent;
						track.Events.Add(new Mid.MetaEvent() { DeltaTime = (uint)(l.Time - delta), Type = (byte)l.Type, Data = Util.Encoding.GetBytes(l.Text) });
					} else if (e is TextEvent) {
						TextEvent c = e as TextEvent;
						track.Events.Add(new Mid.MetaEvent() { DeltaTime = (uint)(c.Time - delta), Type = 0x1, Data = Util.Encoding.GetBytes(c.Text) });
					}
					delta = e.Time;
				}
				foreach (NoteEvent n in OpenNotes) {
					track.Events.Add(new Mid.ChannelEvent() { DeltaTime = (uint)(n.Time + n.Duration - delta), Channel = n.Channel, Type = 0x8, Parameter1 = n.Note, Parameter2 = n.ReleaseVelocity });
					delta = n.Time + n.Duration;
				}

				track.Events.Add(new Mid.MetaEvent() { DeltaTime = 0, Type = 0x2F, Data = new byte[0] });

				mid.Tracks.Add(track);
			}

			return mid;
		}

		public ulong GetTime(ulong ticks)
		{
			ulong time = 0;
			TempoEvent previous = new TempoEvent(0, Mid.MicrosecondsPerMinute / 120);
			TempoEvent next;

			int index = -1;
			ulong t;

			while (ticks > 0) {
				try {
					next = BPM[++index];
				} catch (ArgumentOutOfRangeException) {
					next = new TempoEvent(ulong.MaxValue, 0);
				}

				t = Math.Min(next.Time - previous.Time, ticks);
				ticks -= t;
				time += t * previous.MicrosecondsPerBeat / Division.TicksPerBeat;

				previous = next;
			}

			return time;
		}

		public ulong GetTicks(ulong time)
		{
			ulong ticks = 0;

			TempoEvent previous = new TempoEvent(0, Mid.MicrosecondsPerMinute / 120);
			TempoEvent next;

			int index = -1;
			ulong t;

			while (time > 0) {
				try {
					next = BPM[++index];
				} catch (IndexOutOfRangeException) {
					next = new TempoEvent(ulong.MaxValue, 0);
				}

				t = Math.Min(GetTime(next.Time - previous.Time), time);
				time -= t;
				ticks += t * 1000 * Division.TicksPerBeat / previous.MicrosecondsPerBeat; // 1000 for microseconds unit

				previous = next;
			}

			return ticks;
		}

		public static Midi Create(Mid mid)
		{
			Midi midi = new Midi();
			midi.Division = mid.Division as Mid.TicksPerBeatDivision;
			List<NoteEvent> OpenNotes = new List<NoteEvent>();
			foreach (Mid.Track t in mid.Tracks) {
				Track track = new Track();
				ulong time = 0;
				foreach (Mid.Event e in t.Events) {
					time += e.DeltaTime;
					if (e is Mid.MetaEvent) {
						Mid.MetaEvent meta = e as Mid.MetaEvent;
						if (meta.Type == 0x3) {
							track.Name = Util.Encoding.GetString(meta.Data);
							if (midi.Name == null)
								midi.Name = track.Name;
						} else if (meta.Type == 0x1) {
							track.Comments.Add(new TextEvent(time, Util.Encoding.GetString(meta.Data)));
						} else if (meta.Type == 0x5) {
							track.Lyrics.Add(new TextEvent(time, Util.Encoding.GetString(meta.Data)));
						} else if (meta.Type == 0x6) {
							track.Markers.Add(new TextEvent(time, Util.Encoding.GetString(meta.Data)));
						} else if (meta.Type == 0x51) {
							byte[] data = new byte[4];
							meta.Data.CopyTo(data, 1);
							midi.BPM.Add(new TempoEvent(time, BigEndianConverter.ToUInt32(data)));
						} else if (meta.Type == 0x58) {
							midi.Signature.Add(new TimeSignatureEvent(time, meta.Data[0], meta.Data[1], meta.Data[2], meta.Data[3]));
						} else if (meta.Type == 0x2F) {

						} //else
						//throw new Exception("OHSHIT");
					} else if (e is Mid.ChannelEvent) {
						Mid.ChannelEvent c = e as Mid.ChannelEvent;
						if (c.Type == 0x8 || (c.Type == 0x9 && c.Parameter2 == 0)) { // Note off
							// ( A note on with velocity 0 acts as a note off )
							NoteEvent note = OpenNotes.Find(n => n.Note == c.Parameter1);
							if (note != null) {
								note.Duration = time - note.Time;
								note.ReleaseVelocity = c.Parameter2;
								OpenNotes.Remove(note);
							}
						} else if (c.Type == 0x9) { // Note on
							NoteEvent note = new NoteEvent(time, c.Channel, c.Parameter1, c.Parameter2, 0);
							OpenNotes.Add(note);
							track.Notes.Add(note);
						} else if (c.Type == 0xC) { // Program Change
							InstrumentEvent change = new InstrumentEvent(time, c.Channel, c.Parameter1, c.Parameter2);
							track.Instruments.Add(change);
						} else if (c.Type == 0xB) { // Controller
							switch (c.Parameter1) {
								case 0x00: // Bank Select
									BankEvent bank = new BankEvent(time, c.Channel, c.Parameter2);
									track.Banks.Add(bank);
									break;
							}
						} //else
						//throw new Exception("OHSHIT");
					}
				}

				if (track.Notes.Count > 0 || track.Comments.Count > 0 || track.Lyrics.Count > 0)
					midi.Tracks.Add(track);
			}

			return midi;
		}

		public class Track
		{
			public string Name;
			public List<TextEvent> Markers;
			public List<TextEvent> Comments;
			public List<TextEvent> Lyrics;
			public List<NoteEvent> Notes;
			public List<InstrumentEvent> Instruments;
			public List<BankEvent> Banks;

			public Track()
			{
				Markers = new List<TextEvent>();
				Comments = new List<TextEvent>();
				Notes = new List<NoteEvent>();
				Lyrics = new List<TextEvent>();
				Instruments = new List<InstrumentEvent>();
				Banks = new List<BankEvent>();
			}

			public ulong FindLastNote()
			{
				ulong last = 0;

				foreach (var s in Notes) {
					if ((s.Time + s.Duration) > last)
						last = s.Time + s.Duration;
				}

				return last;
			}
		}

		public class EventComparer : IComparer<Event>
		{
			public int Compare(Event x, Event y)
			{
				return x.Time.CompareTo(y.Time);
			}
		}

		public class NoteEventComparer : IComparer<NoteEvent>
		{
			public int Compare(NoteEvent x, NoteEvent y)
			{
				return (x.Time + x.Duration).CompareTo(y.Time + y.Duration);
			}
		}

		public class TextEventComparer : IComparer<TextEvent>
		{
			public int Compare(TextEvent x, TextEvent y)
			{
				return x.Time.CompareTo(y.Time);
			}
		}

		public class Event
		{
			public Event(ulong time)
			{
				Time = time;
			}

			public ulong Time;
		}

		public class ChannelEvent : Event
		{
			public ChannelEvent(ulong time, byte channel) : base(time)
			{
				Channel = channel;
			}

			public byte Channel;
		}

		public class NoteEvent : ChannelEvent
		{
			public NoteEvent(ulong time, byte channel, byte note, byte vel, ulong duration) : base(time, channel)
			{
				Note = note;
				Velocity = vel;
				Duration = duration;
			}

			public byte Note;
			public byte Velocity;
			public byte ReleaseVelocity;
			public ulong Duration;
		}

		public class InstrumentEvent : ChannelEvent
		{
			public InstrumentEvent(ulong time, byte channel, byte instrument, byte unused) : base(time, channel)
			{
				Instrument = instrument;
				Unused = unused;
			}

			public byte Instrument;
			public byte Unused;
		}

		public class BankEvent : ChannelEvent
		{
			public BankEvent(ulong time, byte channel, byte bank) : base(time, channel)
			{
				Bank = bank;
			}

			public byte Bank;
		}

		public class TextEvent : Event
		{
			public TextEvent(ulong time, string text) : this(time, text, TextEventType.Unknown) { }

			public TextEvent(ulong time, string text, TextEventType type) : base(time)
			{
				Text = text;
				Type = type;
			}

			public string Text;

			public TextEventType Type;

			public enum TextEventType : byte
			{
				Unknown = 0,
				Comment = 1,
				Lyric = 5,
				Marker = 6
			}
		}

		public class TempoEvent : Event
		{
			public TempoEvent(ulong time, uint perBeat) : base(time)
			{
				MicrosecondsPerBeat = perBeat;
			}

			public uint MicrosecondsPerBeat;
		}

		public class TimeSignatureEvent : Event
		{
			public TimeSignatureEvent(ulong time, byte numerator, byte denominator, byte metronome, byte num32) : base(time)
			{
				Numerator = numerator;
				Denominator = denominator;
				Metronome = metronome;
				NumberOf32ndNotes = num32;
			}

			public byte Numerator;
			public byte Denominator;
			public byte Metronome;
			public byte NumberOf32ndNotes;
		}
	}
}
