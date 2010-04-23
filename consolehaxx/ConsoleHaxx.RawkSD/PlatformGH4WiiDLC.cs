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

		public override bool IsValid(string path)
		{
			throw new NotImplementedException();
		}

		public override bool AddSong(PlatformData data, SongData song)
		{
			throw new NotImplementedException();
		}

		public override PlatformData Create(string path, Game game)
		{
			throw new NotImplementedException();
		}
	}
}
