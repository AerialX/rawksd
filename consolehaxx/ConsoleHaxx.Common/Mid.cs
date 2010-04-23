using System;
using System.Collections.Generic;
using System.IO;

// Reference: http://www.sonicspot.com/guide/midifiles.html
namespace ConsoleHaxx.Common
{
	public class Mid
	{
		public const uint MicrosecondsPerMinute = 60000000;

		public MidiType Type;
		public List<Track> Tracks;
		public TimeDivision Division;

		public Mid()
		{
			Tracks = new List<Track>();
		}

		public static Mid Create(Stream stream)
		{
			EndianReader reader = new EndianReader(stream, Endianness.BigEndian);

			if (reader.ReadUInt32() != 0x4D546864)
				return null;
			if (reader.ReadUInt32() != 6)
				return null;

			Mid mid = new Mid();
			mid.Type = (MidiType)reader.ReadUInt16();
			ushort tracks = reader.ReadUInt16();
			ushort division = reader.ReadUInt16();
			if ((division & 0x8000) == 0)
				mid.Division = new TicksPerBeatDivision((ushort)(division & 0x7FFF));
			else
				mid.Division = new FramesPerSecondDivision() {
					FramesPerSecond = (byte)((division & 0x7F00) >> 8),
					TicksPerFrame = (byte)(division & 0x00FF)
				};
			for (int i = 0; i < tracks; i++) {
				Track track = new Track();
				if (reader.ReadUInt32() != 0x4D54726B)
					return null;
				uint size = reader.ReadUInt32();
				uint pos = (uint)stream.Position;

				byte oldb = 0;
				while (stream.Position - pos < size) {
					uint delta = (uint)Mid.ReadVariable(stream);
					byte b = reader.ReadByte();

					if (b == 0xFF) { // Meta Event
						MetaEvent e = new MetaEvent();
						e.DeltaTime = delta;
						e.Type = reader.ReadByte();

						uint length = (uint)Mid.ReadVariable(stream);
						e.Data = reader.ReadBytes((int)length);
						track.Events.Add(e);
						//if (e.Type == 0x2F) // End Track meta event
							//break;
					} else if (b == 0xF0) {
						throw new NotImplementedException();
					} else if (b == 0xF7) {
						throw new NotImplementedException();
					} else { // Channel Event
						if ((b & 0x80) == 0) {
							b = oldb;
							stream.Position--;
						} else
							oldb = b;

						ChannelEvent e = new ChannelEvent();
						e.DeltaTime = delta;

						e.Type = (byte)(b >> 4);

						e.Channel = (byte)(b & 0x0F);
						e.Parameter1 = (byte)reader.ReadByte();
						if (e.Type != 0xC && e.Type != 0xD)
							e.Parameter2 = (byte)reader.ReadByte();
						track.Events.Add(e);
					}
				}

				mid.Tracks.Add(track);
			}

			return mid;
		}

		public void Save(Stream stream)
		{
			EndianReader writer = new EndianReader(stream, Endianness.BigEndian);

			writer.Write(0x4D546864);
			writer.Write((uint)6);

			writer.Write((ushort)Type);
			writer.Write((ushort)Tracks.Count);
			if (Division is TicksPerBeatDivision)
				writer.Write((Division as TicksPerBeatDivision).TicksPerBeat);
			else
				writer.Write((ushort)(0x8000 | ((Division as FramesPerSecondDivision).FramesPerSecond << 8) | (Division as FramesPerSecondDivision).TicksPerFrame));
			foreach (Track track in Tracks) {
				writer.Write(0x4D54726B);

				MemoryStream memory = new MemoryStream();
				EndianReader mwriter = new EndianReader(memory, Endianness.BigEndian);

				foreach (Event e in track.Events) {
					Mid.WriteVariable(memory, (int)e.DeltaTime);
					if (e is MetaEvent) {
						mwriter.Write((byte)0xFF);
						mwriter.Write(e.Type);
						Mid.WriteVariable(memory, (int)(e as MetaEvent).Data.Length);
						mwriter.Write((e as MetaEvent).Data);
					} else {
						ChannelEvent c = e as ChannelEvent;
						mwriter.Write((byte)((c.Type << 4) | c.Channel));
						mwriter.Write(c.Parameter1);
						if (e.Type != 0xC && e.Type != 0xD)
							mwriter.Write(c.Parameter2);
					}
				}

				writer.Write((uint)memory.Length);
				memory.Position = 0;
				Util.StreamCopy(stream, memory);
			}
		}

		public class Track
		{
			public List<Event> Events;

			public Track()
			{
				Events = new List<Event>();
			}
		}

		public class Event
		{
			public uint DeltaTime;
			public byte Type;
		}

		public class MetaEvent : Event
		{
			public byte[] Data;
		}

		public class ChannelEvent : Event
		{
			public byte Channel;
			public byte Parameter1;
			public byte Parameter2;
		}

		public abstract class TimeDivision
		{

		}

		public class TicksPerBeatDivision : TimeDivision
		{
			public ushort TicksPerBeat;

			public TicksPerBeatDivision(ushort ticksperbeat)
			{
				TicksPerBeat = ticksperbeat;
			}
		}

		public class FramesPerSecondDivision : TimeDivision
		{
			public byte FramesPerSecond;
			public byte TicksPerFrame;
		}

		public enum MidiType : ushort
		{
			SingleTrack = 0, // One track does everything
			Uniform = 1, // One track controls tempo, time sig, song info, etc.
			Hybrid = 2 // The tracks aren't necessarily meant to be played in sync, together
		}

		public static int ReadVariable(Stream stream)
		{
			int ret = 0;
			bool done = false;
			while (!done) {
				ret <<= 7;
				byte num = (byte)stream.ReadByte();
				done = ((num & 0x80) == 0);
				ret |= num & 0x7F;
			}

			return ret;
		}

		public static void WriteVariable(Stream stream, int value)
		{
			int buffer;
			buffer = value & 0x7f;
			while ((value >>= 7) > 0) {
				buffer <<= 8;
				buffer |= 0x80;
				buffer += (value & 0x7f);
			}

			while (true) {
				stream.WriteByte((byte)buffer);
				if ((buffer & 0x80) != 0)
					buffer >>= 8;
				else
					break;
			}
		}

	}
}
