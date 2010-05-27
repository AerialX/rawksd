using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

namespace ConsoleHaxx.RawkSD
{
	public static class SongDataCache
	{
		public static Dictionary<FormatData, SongData> Cache = new Dictionary<FormatData, SongData>();

		public static SongData Load(FormatData data)
		{
			if ((FormatData.LocalSongCache || data is TemporaryFormatData) && Cache.ContainsKey(data))
				return Cache[data];
			Stream stream = data.GetStream("songdata");
			SongData song = SongData.Create(stream);
			data.CloseStream(stream);
			song.PropertyChanged += new Action<SongData>(data.Song_PropertyChanged);
			Cache[data] = song;
			return song;
		}

		public static void Save(FormatData data, SongData song)
		{
			if (!(data is TemporaryFormatData)) {
				Stream stream = data.AddStream("songdata");
				song.Save(stream);
				data.CloseStream(stream);
			}
			SongData songdata = null;
			if (Cache.ContainsKey(data))
				songdata = Cache[data];
			if (FormatData.LocalSongCache && songdata != song) {
				songdata = song;
				songdata.PropertyChanged += new Action<SongData>(data.Song_PropertyChanged);
				Cache[data] = songdata;
			}
		}
	}
}
