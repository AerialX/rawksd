using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using ConsoleHaxx.Wii;
using System.IO;
using ConsoleHaxx.Common;
using System.Text.RegularExpressions;
using ConsoleHaxx.Harmonix;

namespace WiidiscExtractor
{
	class Program
	{
		static void Main(string[] args)
		{
			string dir = string.Empty;

			if (args.Length == 2)
				dir = args[1];
			else if (args.Length == 1)
				dir = args[0] + ".ext";
			else {
				Console.WriteLine("Usage: wiidiscextractor /path/to/disc.iso /extract/path");
				return;
			}

			Directory.CreateDirectory(dir);

			try {
				if (!Directory.Exists(args[0]))
					throw new FormatException();
				DelayedStreamCache cache = new DelayedStreamCache();
				DirectoryNode dirn = DirectoryNode.FromPath(args[0], cache, FileAccess.Read, FileShare.Read);
				DirectoryNode gen = dirn.Navigate("gen", false, true) as DirectoryNode;
				if (gen == null) // Just in case we're given the "wrong" directory that directly contains the ark
					gen = dirn;

				List<Pair<int, Stream>> arkfiles = new List<Pair<int, Stream>>();
				Stream hdrfile = null;
				foreach (FileNode file in gen.Files) {
					if (file.Name.ToLower().EndsWith(".hdr"))
						hdrfile = file.Data;
					else if (file.Name.ToLower().EndsWith(".ark")) {
						Match match = Regex.Match(file.Name.ToLower(), @"_(\d+).ark");
						if (match.Success)
							arkfiles.Add(new Pair<int, Stream>(int.Parse(match.Groups[1].Value), file.Data));
						else
							arkfiles.Add(new Pair<int, Stream>(0, file.Data));
					}
				}

				// FreQuency/Amplitude where the header is the ark
				if (hdrfile == null) {
					if (arkfiles.Count == 1) {
						hdrfile = arkfiles[0].Value;
						arkfiles.Clear();
					} else
						throw new FormatException();
				}

				Ark ark = new Ark(new EndianReader(hdrfile, Endianness.LittleEndian), arkfiles.OrderBy(f => f.Key).Select(f => f.Value).ToArray());
				ark.Root.Extract(dir);
				cache.Dispose();
			} catch (FormatException) {
				Stream stream = new FileStream(args[0], FileMode.Open, FileAccess.Read);
				try {
					Iso9660 iso = new Iso9660(stream);
					iso.Root.Extract(dir);
				} catch (Exception) {
					try {
						stream.Position = 0;
						Disc disc = new Disc(stream);

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
					} catch {
						try {
							stream.Position = 0;
							DlcBin bin = new DlcBin(stream);
							U8 u8 = new U8(bin.Data);
							u8.Root.Extract(dir);
						} catch {
							try {
								stream.Position = 0;
								U8 u8 = new U8(stream);
								u8.Root.Extract(dir);
							} catch {
								stream.Position = 0;
								Rarc rarc = new Rarc(stream);
								rarc.Root.Extract(dir);
							}
						}
					}
				}
				stream.Close();
			}
		}
	}
}
