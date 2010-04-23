using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using ConsoleHaxx.Harmonix;
using ConsoleHaxx.Common;

namespace ArkExtractor
{
	class Program
	{
		// Params
		// Amplitude Mounted ISO: Z:\home\mnt\gen\main.ark Z:\amplitude
		// FreQuency: 
		// - Z:\freq\loading.fix.ark Z:\frequency\loading
		// - Z:\freq\arenas.fix.ark Z:\frequency\arenas
		// - Z:\freq\levels.fix.ark Z:\frequency\levels
		// - Z:\freq\root.fix.ark Z:\frequency\root

		static void Main(string[] args)
		{
			if (args.Length < 2) {
				Console.WriteLine("Usage: ArkExtractor hdrfile [arkfiles] extract_path");
				return;
			}

			FileStream hdr = new FileStream(args[0], FileMode.Open, FileAccess.Read);
			List<FileStream> arks = new List<FileStream>();
			for (int i = 1; i < args.Length - 1; i++) {
				arks.Add(new FileStream(args[i], FileMode.Open, FileAccess.Read));
			}

			Ark ark = new Ark(new EndianReader(hdr, Endianness.LittleEndian), arks.ToArray());

			ark.Root.Extract(args[args.Length - 1]);

			hdr.Close();

			foreach (FileStream file in arks)
				file.Close();
		}
	}
}
