using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

namespace ConsoleHaxx.RawkSD
{
	public class PlatformLocalStorage : Engine
	{
		public static PlatformLocalStorage Instance;
		public static void Initialise()
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
			string path = null;
			int i = 0;
			do {
				path = Path.Combine(data.Session["rootpath"] as string, song.ID) + (i == 0 ? "" : i.ToString());
				i++;
			} while (Directory.Exists(path));
			song.Data.SetValue("LocalStoragePath", path);
			Directory.CreateDirectory(path);

			FormatData format = new FolderFormatData(song, data, path);
			data.AddSong(format);

			return format;
		}

		public override void SaveSong(PlatformData data, FormatData formatdata, ProgressIndicator progress)
		{
			
		}

		public override void DeleteSong(PlatformData data, FormatData formatdata, ProgressIndicator progress)
		{
			string path = formatdata.Song.Data.GetValue<string>("LocalStoragePath");
			if (path == null)
				return;
			formatdata.Dispose();
			Directory.Delete(path, true);
			base.DeleteSong(data, formatdata, progress);
		}
	}
}
