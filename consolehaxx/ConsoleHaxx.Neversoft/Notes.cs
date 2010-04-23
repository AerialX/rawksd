using System;
using System.Collections.Generic;
using System.IO;
using ConsoleHaxx.Common;
using Nanook.QueenBee.Parser;

namespace ConsoleHaxx.Neversoft
{
	public class Notes
	{
		public byte[] Header;
		public List<Entry> Entries;

		public static QbKey KeyFretbars = QbKey.Create("fretbar");
		public static QbKey KeyTimesignature = QbKey.Create("timesig");
		public static QbKey KeyVocals = QbKey.Create("vocals");

		public Notes()
		{
			Entries = new List<Entry>();
		}

		public static Notes Create(EndianReader stream)
		{
			Notes notes = new Notes();
			notes.Header = stream.ReadBytes(0x1C);

			while (stream.Position < stream.Base.Length) {
				Entry entry = Entry.Create(stream);
				if (entry != null)
					notes.Entries.Add(entry);
			}

			return notes;
		}

		public class Entry
		{
			public QbKey Identifier;
			public QbKey Type;

			public byte[][] Data;

			public static Entry Create(EndianReader stream)
			{
				Entry entry = new Entry();

				uint entries;
				int sizeofentry;

				entry.Identifier = QbKey.Create(stream.ReadUInt32());
				entries = stream.ReadUInt32();
				entry.Type = QbKey.Create(stream.ReadUInt32());
				sizeofentry = stream.ReadInt32();

				entry.Data = new byte[entries][];
				for (int i = 0; i < entries; i++)
					entry.Data[i] = stream.ReadBytes(sizeofentry);

				return entry;
			}
		}

		public Entry Find(QbKey key)
		{
			foreach (Entry entry in Entries)
				if (entry.Identifier == key)
					return entry;
			return null;
		}
	}
}
