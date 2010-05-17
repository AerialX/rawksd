using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Text.RegularExpressions;

namespace ConsoleHaxx.Common
{
	public class Ini
	{
		public Dictionary<string, Dictionary<string, string>> Entries;

		public Ini()
		{
			Entries = new Dictionary<string, Dictionary<string, string>>(IgnoreCaseComparer.Instance);
		}

		public static Ini Create(Stream stream)
		{
			StreamReader reader = new StreamReader(stream);

			Ini ini = new Ini();

			Dictionary<string, string> entry = null;

			while (true) {
				string line = reader.ReadLine();
				if (line == null)
					break;
				else
					line = line.Trim();
				Match match = Regex.Match(line, "^\\[(?'header'.+?)\\]$");
				if (match.Success) {
					entry = new Dictionary<string, string>(IgnoreCaseComparer.Instance);
					ini.Entries.Add(match.Groups["header"].Value.Trim(), entry);
				} else {
					match = Regex.Match(line, "^(?'key'.+?)=(?'value'.+)$");
					if (match.Success)
						entry.Add(match.Groups["key"].Value.Trim(), match.Groups["value"].Value.Trim());
				}
			}

			return ini;
		}

		public void Save(Stream stream)
		{
			StreamWriter writer = new StreamWriter(stream);

			foreach (var item in Entries) {
				writer.WriteLine("[" + item.Key + "]");
				foreach (var entry in item.Value) {
					writer.WriteLine(entry.Key + "=" + entry.Value);
				}
			}

			writer.Flush();
		}

		public string GetValue(string header, string key)
		{
			Dictionary<string, string> dict = GetHeader(header);
			if (dict == null || !dict.ContainsKey(key))
				return null;
			return dict[key];
		}

		public IList<string> GetHeaders()
		{
			return Entries.Keys.ToList();
		}

		public Dictionary<string, string> GetHeader(string header, bool create = false)
		{
			if (Entries.ContainsKey(header))
				return Entries[header];
			else if (create) {
				Dictionary<string, string> entry = new Dictionary<string, string>(IgnoreCaseComparer.Instance);
				Entries.Add(header, entry);
				return entry;
			}

			return null;
		}

		public IList<string> GetKeys(string header)
		{
			return Entries[header].Keys.ToList();
		}

		public void SetValue(string header, string key, string value)
		{
			Dictionary<string, string> dict = GetHeader(header, true);
			dict[key] = value;
		}

		protected class IgnoreCaseComparer : IEqualityComparer<string>
		{
			public static IgnoreCaseComparer Instance;
			static IgnoreCaseComparer()
			{
				Instance = new IgnoreCaseComparer();
			}

			public bool Equals(string x, string y)
			{
				return string.Compare(x, y, true) == 0;
			}

			public int GetHashCode(string obj)
			{
				return obj.ToLower().GetHashCode();
			}
		}
	}
}
