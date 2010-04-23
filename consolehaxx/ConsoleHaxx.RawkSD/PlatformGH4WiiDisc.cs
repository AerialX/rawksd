using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Nanook.QueenBee.Parser;
using ConsoleHaxx.Common;
using ConsoleHaxx.Neversoft;
using System.IO;

namespace ConsoleHaxx.RawkSD
{
	public class PlatformGH4WiiDisc : Engine
	{
		public static readonly PakFormat PakFormat = new PakFormat(null, null, null, PakFormatType.Wii);

		public static readonly PlatformGH4WiiDisc Instance;
		static PlatformGH4WiiDisc()
		{
			Instance = new PlatformGH4WiiDisc();
		}

		public override int ID { get { throw new NotImplementedException(); } }

		public override string Name { get { return "Guitar Hero World Tour / Metallica / Smash Hits / Van Halen Wii Disc"; } }

		public override bool IsValid(string path)
		{
			throw new NotImplementedException();
		}

		public override bool AddSong(PlatformData data, SongData song)
		{
			return PlatformGH5WiiDisc.Instance.AddSong(data, song);
		}

		public override PlatformData Create(string path, Game game)
		{
			if (game == null)
				game = Game.GetGame(Games.GuitarHeroWorldTour);

			PlatformData data = PlatformGH5WiiDisc.Instance.Create(path, game);

			data.Platform = this;

			return data;
		}
	}
}
