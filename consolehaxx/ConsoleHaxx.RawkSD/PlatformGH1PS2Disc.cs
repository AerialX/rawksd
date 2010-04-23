using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace ConsoleHaxx.RawkSD
{
	public class PlatformGH1PS2Disc : Engine
	{
		public static readonly PlatformGH1PS2Disc Instance;
		static PlatformGH1PS2Disc()
		{
			Instance = new PlatformGH1PS2Disc();
		}

		public override int ID { get { throw new NotImplementedException(); } }

		public override string Name { get { return "Guitar Hero PS2 Disc"; } }

		public override bool IsValid(string path)
		{
			throw new NotImplementedException();
		}

		public override bool AddSong(PlatformData data, SongData song)
		{
			return PlatformGH2PS2Disc.Instance.AddSong(data, song);
		}

		public override PlatformData Create(string path, Game game)
		{
			if (game == null)
				game = Game.GetGame(Games.GuitarHero1);

			PlatformData data = PlatformGH2PS2Disc.Instance.Create(path, game);
			data.Platform = this;

			return data;
		}
	}
}
