using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace ConsoleHaxx.RawkSD
{
	public class PlatformRB2360Disc : Engine
	{
		public static readonly PlatformRB2360Disc Instance;
		static PlatformRB2360Disc()
		{
			Instance = new PlatformRB2360Disc();
		}

		public override int ID { get { throw new NotImplementedException(); } }

		public override string Name { get { return "Rock Band 2 360 Disc"; } }

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
