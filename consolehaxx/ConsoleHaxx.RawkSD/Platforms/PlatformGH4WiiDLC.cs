using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace ConsoleHaxx.RawkSD
{
	public class PlatformGH4WiiDLC : Engine
	{
		public static readonly PlatformGH4WiiDLC Instance;
		static PlatformGH4WiiDLC()
		{
			Instance = new PlatformGH4WiiDLC();
		}

		public override int ID { get { throw new NotImplementedException(); } }

		public override string Name { get { return "Guitar Hero World Tour DLC"; } }

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
