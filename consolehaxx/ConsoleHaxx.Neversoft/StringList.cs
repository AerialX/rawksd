using System;
using System.Collections.Generic;
using System.IO;
using Nanook.QueenBee.Parser;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.Neversoft
{
	public class StringList
	{
		public List<KeyValuePair<QbKey, string>> Items;

		public StringList()
		{
			Items = new List<KeyValuePair<QbKey, string>>();
		}

		public string FindItem(QbKey key)
		{
			foreach (KeyValuePair<QbKey, string> k in Items) {
				if (key.Crc == k.Key.Crc)
					return k.Value;
			}

			return null;
		}

		public void ParseFromStream(Stream stream)
		{
			ParseFromStream(new StreamReader(stream, Util.Encoding));
		}

		public void ParseFromStream(Stream stream, bool stripprefix)
		{
			ParseFromStream(new StreamReader(stream, Util.Encoding), stripprefix);
		}

		public void ParseFromStream(StreamReader reader)
		{
			ParseFromStream(reader, true);
		}

		public void ParseFromStream(StreamReader reader, bool stripprefix)
		{
			while (!reader.EndOfStream) {
				char[] chars = new char[8];
				if (reader.Read(chars, 0, 8) != 8)
					break;

				QbKey key = QbKey.Create(uint.Parse(new string(chars), System.Globalization.NumberStyles.HexNumber));
				char c = '\0';
				while (c != '"')
					c = (char)reader.Read();

				c = (char)reader.Read();
				string str = string.Empty;
				while (c != '"') {
					str += c;
					c = (char)reader.Read();
				}

				if (stripprefix) {
					if (str.StartsWith(@"\L, "))
						str = str.Substring(4);
					else if (str.StartsWith(@"\L"))
						str = str.Substring(2);
				}

				Items.Add(new KeyValuePair<QbKey, string>(key, str));

				reader.Read(chars, 0, 2);
				if (chars[0] != 0x0D || chars[1] != 0x0A)
					break;
			}
		}
	}
}
