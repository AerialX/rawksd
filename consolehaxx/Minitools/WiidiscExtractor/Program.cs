using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using ConsoleHaxx.Wii;
using System.IO;
using ConsoleHaxx.Common;

namespace WiidiscExtractor
{
	class Program
	{
		static void Main(string[] args)
		{
			if (args.Length != 2) {
				Console.WriteLine("Usage: wiidiscextractor /path/to/disc.iso /extract/path");
				return;
			}

			Stream stream = new FileStream(args[0], FileMode.Open, FileAccess.Read);
			Disc disc = new Disc(stream);

			string dir = args[1];
			Directory.CreateDirectory(dir);
			File.WriteAllText(Path.Combine(dir, "title"), disc.Title);

			foreach (var partition in disc.Partitions) {
				string path = Path.Combine(dir, "partition" + disc.Partitions.IndexOf(partition).ToString());
				Directory.CreateDirectory(path);

				partition.Root.Root.Extract(Path.Combine(path, "data"));

				FileStream file = new FileStream(Path.Combine(path, "partition.tik"), FileMode.Create, FileAccess.Write);
				partition.Ticket.Save(file);
				file.Close();

				file = new FileStream(Path.Combine(path, "partition.tmd"), FileMode.Create, FileAccess.Write);
				partition.TMD.Save(file);
				file.Close();

				file = new FileStream(Path.Combine(path, "partition.certs"), FileMode.Create, FileAccess.Write);
				file.Write(partition.CertificateChain);
				file.Close();
			}

			stream.Close();
		}
	}
}
