using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Threading;

namespace ConsoleHaxx.RawkSD
{
	public static class SongDataCache
	{
		public static Dictionary<FormatData, SongData> Cache = new Dictionary<FormatData, SongData>();

		private static Mutex CacheMutex = new Mutex();

		public static SongData Load(FormatData data)
		{
			if (FormatData.LocalSongCache && Cache.ContainsKey(data))
				return Cache[data];
			CacheMutex.WaitOne();
			SongData song;
			try {
				Stream stream = data.GetStream("songdata");
				song = SongData.Create(stream);
				data.CloseStream(stream);
			} catch (Exception ex) {
				CacheMutex.ReleaseMutex();
				throw ex;
			}
			CacheMutex.ReleaseMutex();
			song.PropertyChanged += new Action<SongData>(data.Song_PropertyChanged);
			if (FormatData.LocalSongCache)
				Cache[data] = song;
			return song;
		}

		public static void Save(FormatData data, SongData song)
		{
			if (!FormatData.LocalSongCache || (data.PlatformData != null && data.PlatformData.Platform == PlatformLocalStorage.Instance)) {
				CacheMutex.WaitOne();
				try {
					Stream stream = data.AddStream("songdata");
					song.Save(stream);
					data.CloseStream(stream);
				} catch (Exception ex) {
					CacheMutex.ReleaseMutex();
					throw ex;
				}
				CacheMutex.ReleaseMutex();
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
