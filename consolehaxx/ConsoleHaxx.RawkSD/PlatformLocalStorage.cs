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

		public override bool IsValid(string path)
		{
			return Directory.Exists(path);
		}

		public override bool AddSong(PlatformData data, SongData song)
		{
			throw new NotImplementedException();
		}

		public override PlatformData Create(string path, Game game)
		{
			if (!IsValid(path))
				throw new FormatException();

			PlatformData data = new PlatformData(this, game);

			data.Session["rootpath"] = path;

			DirectoryInfo[] dirs = new DirectoryInfo(path).GetDirectories();
			foreach (DirectoryInfo dir in dirs) {
				FormatData format = new FolderFormatData(dir.FullName);

				data.AddSong(format);
			}

			return data;
		}

		public override bool Writable { get { return true; } }

		public override FormatData CreateSong(PlatformData data, SongData song)
		{
			string path = Path.Combine(data.Session["rootpath"] as string, song.ID);
			song.Data.SetValue("LocalStoragePath", path);
			Directory.CreateDirectory(path);
			return new FolderFormatData(song, path);
		}

		public override void SaveSong(PlatformData data, FormatData formatdata)
		{
			
		}
	}
}
