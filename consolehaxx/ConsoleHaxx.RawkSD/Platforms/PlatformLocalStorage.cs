using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

namespace ConsoleHaxx.RawkSD
{
	public class PlatformLocalStorage : Engine
	{
		public static readonly PlatformLocalStorage Instance;
		static PlatformLocalStorage()
		{
			Instance = new PlatformLocalStorage();
		}

		public override int ID { get { throw new NotImplementedException(); } }

		public override string Name { get { return "Local Storage"; } }

		public override bool AddSong(PlatformData data, SongData song, ProgressIndicator progress)
		{
			throw new NotImplementedException();
		}

		public override PlatformData Create(string path, Game game, ProgressIndicator progress)
		{
			if (!Directory.Exists(path))
				Directory.CreateDirectory(path);

			PlatformData data = new PlatformData(this, game);

			data.Session["rootpath"] = path;

			DirectoryInfo[] dirs = new DirectoryInfo(path).GetDirectories();
			foreach (DirectoryInfo dir in dirs) {
				FormatData format = new FolderFormatData(data, dir.FullName);

				data.AddSong(format);
			}

			return data;
		}

		public override FormatData CreateSong(PlatformData data, SongData song)
		{
			string path = Path.Combine(data.Session["rootpath"] as string, song.ID);
			song.Data.SetValue("LocalStoragePath", path);
			Directory.CreateDirectory(path);

			FormatData format = new FolderFormatData(song, data, path);
			data.AddSong(format);

			return format;
		}

		public override void SaveSong(PlatformData data, FormatData formatdata, ProgressIndicator progress)
		{
			
		}
	}
}
