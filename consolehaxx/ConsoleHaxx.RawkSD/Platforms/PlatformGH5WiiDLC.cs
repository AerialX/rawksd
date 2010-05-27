using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace ConsoleHaxx.RawkSD
{
	public class PlatformGH5WiiDLC : Engine
	{
		public static PlatformGH5WiiDLC Instance;
		public static void Initialise()
		{
			Instance = new PlatformGH5WiiDLC();
		}

		public override int ID { get { throw new NotImplementedException(); } }

		public override string Name { get { return "Guitar Hero 5 DLC"; } }

		public override bool AddSong(PlatformData data, SongData song, ProgressIndicator progress)
		{
			throw new NotImplementedException();
		}

		public override PlatformData Create(string path, Game game, ProgressIndicator progress)
		{
			throw new NotImplementedException();
		}
	}
}
