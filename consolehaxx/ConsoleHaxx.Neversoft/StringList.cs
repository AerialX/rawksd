using System;
using System.Collections.Generic;
using System.IO;
using Nanook.QueenBee.Parser;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.Neversoft
{
	public class StringList
	{
		protected bool StripPrefix;

		public List<KeyValuePair<QbKey, string>> Items;

		public StringList(bool stripprefix = true)
		{
			Items = new List<KeyValuePair<QbKey, string>>();
			
			StripPrefix = stripprefix;
		}

		public string FindItem(QbKey key)
		{
			foreach (KeyValuePair<QbKey, string> k in Items) {
				if (key.Crc == k.Key.Crc) {
					string value = k.Value;
					if (StripPrefix) {
						while (value.StartsWith(@"\L") || value.StartsWith(", "))
							value = value.Substring(2);
					}
					return value;
				}
			}

			return null;
		}

		public void ParseFromStream(Stream stream, bool stripprefix = true)
		{
			ParseFromStream(new StreamReader(stream, Util.Encoding), stripprefix);
		}

		public void ParseFromStream(StreamReader reader, bool stripprefix = true)
		{
			try {
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

					Items.Add(new KeyValuePair<QbKey, string>(key, str));

					reader.Read(chars, 0, 2);
					if (chars[0] != 0x0D || chars[1] != 0x0A)
						break;
				}
			} catch { }
		}
	}
}
