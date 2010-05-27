using System;
using System.Collections.Generic;
using System.IO;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.FreeStyleGames
{
	public class Mub
	{
		// Header
		public int Version;
		public int Checksum;

		public List<Node> Nodes;

		public Mub()
		{
			Nodes = new List<Node>();
		}

		public static Mub Create(EndianReader reader)
		{
			Mub mub = new Mub();
			mub.Version = reader.ReadInt32();

			if (mub.Version != 1 && mub.Version != 2)
				throw new FormatException(); // Unknown version number

			mub.Checksum = reader.ReadInt32();

			int nodes = reader.ReadInt32();
			int datasize = reader.ReadInt32();

			long begin = reader.Position;

			for (int i = 0; i < nodes; i++) {
				Node node = Node.ParseNode(reader);

				mub.Nodes.Add(node);
			}

			foreach (Node node in mub.Nodes) {
				MarkerNode mark = node as MarkerNode;
				if (mark != null) {
					reader.Position = begin + mark.Data;
					mark.Text = Util.ReadCString(reader.Base);
				}
			}

			return mub;
		}

		public void Save(EndianReader writer)
		{
			Nodes.Sort(new NodeComparer());
			int stringoffset = Nodes.Count * 0x10;

			// First build the stringtable
			MemoryStream stringtable = new MemoryStream();
			EndianReader swriter = new EndianReader(stringtable, writer.Endian);
			foreach (Node node in Nodes) {
				MarkerNode marker = node as MarkerNode;
				if (marker != null) {
					marker.Data = stringoffset;
					swriter.Write(Util.Encoding.GetBytes(marker.Text));
					swriter.Write((byte)0);
					stringoffset += marker.Text.Length + 1;
				}
			}

			writer.Write(Version);
			writer.Write(Checksum);
			writer.Write(Nodes.Count);
			writer.Write((int)stringtable.Length);

			foreach (Node node in Nodes)
				node.Save(writer);

			stringtable.Position = 0;
			Util.StreamCopy(writer.Base, stringtable);

			writer.PadToMultiple(0x20);
		}

		public class Node
		{
			public float Time;
			public int Type;
			public float Duration;
			public int Data;

			internal static Node ParseNode(EndianReader reader)
			{
				float time = reader.ReadFloat32();
				int type = reader.ReadInt32();
				float duration = reader.ReadFloat32();
				int data = reader.ReadInt32();

				Node node = null;

				switch (type) {
					case 0x09FFFFFF: // marker_t
						node = new MarkerNode();
						break;
					case 0x0AFFFFFF: // Some sort of XML metadata
						node = new MarkerNode();
						break;
					default:
						node = new Node();
						break;
				}

				node.Time = time;
				node.Type = type;
				node.Duration = duration;
				node.Data = data;

				return node;
			}

			internal void Save(EndianReader writer)
			{
				writer.Write(Time);
				writer.Write(Type);
				writer.Write(Duration);
				writer.Write(Data);
			}
		}

		public class MarkerNode : Node
		{
			public string Text;
		}

		public Midi ToMidi()
		{
			Midi mid = new Midi();
			mid.Division = new Mid.TicksPerBeatDivision(960);
			mid.Signature.Add(new Midi.TimeSignatureEvent(0, 4, 2, 24, 8));

			Midi.Track track = new Midi.Track();
			mid.AddTrack(track);
			track.Name = string.Empty;
			mid.Name = string.Empty;

			foreach (Node node in Nodes) {
				ulong time = (ulong)(node.Time * mid.Division.TicksPerBeat * 4);
				ulong duration = (ulong)(node.Duration * mid.Division.TicksPerBeat * 4);
				if (node is MarkerNode) {
					switch (node.Type) {
						case 0x09FFFFFF:
							track.Markers.Add(new Midi.TextEvent(time, (node as MarkerNode).Text));
							break;
						case 0x0AFFFFFF:
							track.Comments.Add(new Midi.TextEvent(time, (node as MarkerNode).Text));
							break;
						default:
							throw new NotSupportedException();
					}
				} else
					track.Notes.Add(new Midi.NoteEvent(time, 0, (byte)node.Type, 128, duration));
			}

			return mid;
		}

		private static float GetBars(ulong beats, Midi mid)
		{
			Midi.TimeSignatureEvent previous = new Midi.TimeSignatureEvent(0, 4, 2, 24, 8);
			Midi.TimeSignatureEvent next;

			int index = -1;
			ulong t;
			float bars = 0;

			while (beats > 0) {
				if (index + 1 == mid.Signature.Count)
					next = new Midi.TimeSignatureEvent(ulong.MaxValue, 0, 0, 0, 0);
				else
					next = mid.Signature[++index];

				t = Math.Min(next.Time - previous.Time, beats);
				beats -= t;
				bars += (float)t / previous.Numerator;

				previous = next;
			}

			return bars;
		}

		public static Mub ParseMidi(Midi mid)
		{
			Mub mub = new Mub();

			if (mid.Tracks.Count != 1)
				throw new FormatException();

			Midi.Track track = mid.Tracks[0];

			foreach (Midi.NoteEvent note in track.Notes) {
				float time = GetBars(note.Time, mid) / mid.Division.TicksPerBeat;
				float duration = (GetBars(note.Time + note.Duration, mid) - GetBars(note.Time, mid)) / mid.Division.TicksPerBeat;

				mub.Nodes.Add(new Node() { Time = time, Duration = duration, Type = (int)note.Note, Data = 0 });
			}

			foreach (Midi.TextEvent comment in track.Comments) {
				float time = GetBars(comment.Time, mid) / mid.Division.TicksPerBeat;

				mub.Nodes.Add(new MarkerNode() { Time = time, Duration = 0, Type = (int)0x0AFFFFFF, Text = comment.Text });
			}

			foreach (Midi.TextEvent comment in track.Markers) {
				float time = GetBars(comment.Time, mid) / mid.Division.TicksPerBeat;

				mub.Nodes.Add(new MarkerNode() { Time = time, Duration = 0, Type = (int)0x09FFFFFF, Text = comment.Text });
			}

			return mub;
		}

		private class NodeComparer : IComparer<Node>
		{
			public int Compare(Node x, Node y)
			{
				return x.Time.CompareTo(y.Time);
			}
		}

		public enum EndingType
		{
			SinglePlayer,
			Guitar,
			Scene
		}
		private static int GetEndingSize(EndingType type)
		{
			switch (type) {
				case EndingType.SinglePlayer:
					return 2;
				case EndingType.Guitar:
					return 1;
				case EndingType.Scene:
					return 0;
				default:
					throw new NotSupportedException();
			}
		}
	}
}
