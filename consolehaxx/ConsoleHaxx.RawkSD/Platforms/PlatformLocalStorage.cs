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
			progress.NewTask(dirs.Length);
			foreach (DirectoryInfo dir in dirs) {
				progress.Progress();

				if (!File.Exists(Path.Combine(dir.FullName, "songdata")))
					continue;

				try {
					FormatData format = new FolderFormatData(data, dir.FullName);

					if (format.Song != null)
						data.AddSong(format);
				} catch (Exception exception) {
					Exceptions.Warning(exception, "Unable to open the custom at " + dir.FullName);
				}
			}
			progress.EndTask();

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
			Directory.CreateDirectory(path);

			FormatData format = new FolderFormatData(song, data, path);

			return format;
		}

		public override void SaveSong(PlatformData data, FormatData formatdata, ProgressIndicator progress)
		{
			data.AddSong(formatdata);
		}

		public override void DeleteSong(PlatformData data, FormatData formatdata, ProgressIndicator progress)
		{
			if (!(formatdata is FolderFormatData))
				return;
			string path = (formatdata as FolderFormatData).Pathname;
			formatdata.Dispose();
			Directory.Delete(path, true);
			base.DeleteSong(data, formatdata, progress);
		}
	}
}
