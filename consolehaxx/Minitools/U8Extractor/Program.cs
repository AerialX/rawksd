using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using ConsoleHaxx.Wii;
using ConsoleHaxx.Common;

namespace U8Extractor
{
	class Program
	{
		static void Main(string[] args)
		{
			string dir;
			if (args.Length == 1) {
				dir = Path.Combine(Path.GetDirectoryName(args[0]), Path.GetFileNameWithoutExtension(args[0]));
			} else if (args.Length != 2) {
				Console.WriteLine("Usage: u8extractor /path/to/file.arc [/extract/path]");
				return;
			} else
				dir = args[1];

			Stream filestream = new FileStream(args[0], FileMode.Open, FileAccess.Read, FileShare.Read);
			Stream stream = filestream;

			try {
				DlcBin dlc = new DlcBin(filestream);
				stream = dlc.Data;
			} catch (FormatException) { }
			
			U8 u8 = new U8(stream);

			u8.Root.Extract(dir);

			filestream.Close();
		}
	}
}
