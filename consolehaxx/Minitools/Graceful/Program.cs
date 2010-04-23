using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using ConsoleHaxx.Graces;
using ConsoleHaxx.Common;
using System.Net;
using System.Text.RegularExpressions;
using System.Globalization;
using System.Drawing.Imaging;
using System.Drawing;

namespace Graceful
{
	class Program
	{
		static string[] Args;
		static int CurrentArg = 0;

		static void Main(string[] args)
		{
			Console.WriteLine("Graceful v0.07 by AerialX");

			ConsoleSetArgs(args);

			if (args.Length == 1) {
				string filename = args[0];
				if (File.Exists(filename)) {
					string extension = Path.GetExtension(filename).ToLower();
					switch (extension) {
						case ".cpk":
							OperationExtractCPK();
							return;
						case ".tex":
						case ".txv":
						case ".txm":
						case ".ttx":
							OperationTex2Png();
							return;
						case ".png":
							OperationPng2Tex();
							return;
						case ".scs":
							OperationTranslateScs();
							return;
						case ".acf":
						case ".apk":
						case ".adp":
						case ".bin":
						case ".chd":
						case ".mdl":
						case ".eff":
						case ".wmd":
						case ".www":
						default:
							OperationExtractFPS4();
							return;
					}
				} else if (Directory.Exists(filename)) {
					OperationPackFPS4();
					return;
				}
			}

			while (true) {
				int selected = ConsoleMenu("Select an operation...", "Extract FPS4", "Pack FPS4", "Extract CPK", "Patch CPK", "Pack CPK", "TEX -> PNG", "PNG -> TEX", "Google Translate SCS", "Exit");
				switch (selected) {
					case 0:
						OperationExtractFPS4();
						break;
					case 1:
						OperationPackFPS4();
						break;
					case 2:
						OperationExtractCPK();
						break;
					case 3:
						OperationPatchCPK();
						break;
					case 4:
						OperationPackCPK();
						break;
					case 5:
						OperationTex2Png();
						break;
					case 6:
						OperationPng2Tex();
						break;
					case 7:
						OperationTranslateScs();
						break;
					case 8:
						return;
				}
			}
		}

		private static void OperationTranslateScs()
		{
			string filename = ConsolePath("Full path to SCS file..?");
			if (filename.IsEmpty())
				return;

			if (Directory.Exists(filename)) {
				string[] files = Directory.GetFiles(filename, "*.scs", SearchOption.AllDirectories);
				foreach (string file in files)
					OperationTranslateScs(file);
				files = Directory.GetFiles(filename, "*.bin", SearchOption.AllDirectories);
				foreach (string file in files)
					OperationTranslateScs(file);
			} else
				OperationTranslateScs(filename);
		}

		private static void OperationTranslateScs(string filename)
		{
			Stream stream = new FileStream(filename, FileMode.Open, FileAccess.Read);
			Scs scs = Scs.Create(stream);
			stream.Close();

			WebClient client = new WebClient();
			client.BaseAddress = "http://translate.google.com";

			// mapXR.cpk trimming:
			int asbelindex = 0;
			int anmaindex = scs.Entries.Count;
			for (int i = 0; i < scs.Entries.Count; i++) {
				if (scs.Entries[i] == "anma")
					anmaindex = i;
				else if (scs.Entries[i] == "アスベル")
					asbelindex = i + 1;
			}

			for (int i = asbelindex; i < anmaindex; i++) {
				string entry = scs.Entries[i];
				bool ascii = true;
				for (int k = 0; k < entry.Length; k++) {
					if ((int)entry[k] > 0xFF) {
						ascii = false;
						break;
					}
				}
				if (ascii)
					continue;

				try {
					MatchCollection varmatches = Regex.Matches(entry, "[\u0001\u0002\u0003\u0004\u0005\u0006\u0007\u0008\u0009\u000A\u000B\u000C\u000D\u000E\u000F\u0011\u0012\u0013\u0014\u0015\u0016\u0017\u0018\u0019\u001A\u001B\u001C\u001D\u001E\u001F]+\\(.+?\\)");
					List<Pair<int, string>> variables = new List<Pair<int, string>>();
					foreach (Match match in varmatches) {
						variables.Add(new Pair<int, string>(match.Index, match.Value));
						entry = entry.Replace(match.Value, "");
					}

					client.Encoding = Encoding.UTF8;
					client.Headers["User-Agent"] = "Mozilla/5.0 (X11; U; Linux x86_64; en-US; rv:1.9.2.3) Gecko/20100402 Namoroka/3.6.3 (.NET CLR 3.5.30729)";
					client.QueryString.Clear();
					client.QueryString["client"] = "t";
					client.QueryString["sl"] = "ja";
					client.QueryString["tl"] = "en";
					client.QueryString["text"] = entry;
					string ret = client.DownloadString("/translate_a/t");
					MatchCollection matches = Regex.Matches(ret, "\"trans\":\"(?'text'.*?)\"");
					ret = "";
					foreach (Match match in matches) {
						ret += match.Groups["text"].Value + "\n";
					}
					while (true) {
						int indexof = ret.IndexOf("\\u");
						if (indexof < 0)
							break;
						int chr = int.Parse(ret.Substring(indexof + 2, 4), NumberStyles.HexNumber);
						ret = ret.Substring(0, indexof) + (char)chr + ret.Substring(indexof + 6);
					}
					while (true) {
						int indexof = ret.IndexOf("\\n");
						if (indexof < 0)
							break;
						ret = ret.Substring(0, indexof) + '\n' + ret.Substring(indexof + 2);
					}
					while (true) {
						int indexof = ret.IndexOf("\n\n");
						if (indexof < 0)
							break;
						ret = ret.Substring(0, indexof) + ret.Substring(indexof + 1);
					}
					if (entry[entry.Length - 1] != '\n')
						ret = ret.TrimEnd('\n');
					if (matches.Count > 0) {
						foreach (Pair<int, string> var in variables) {
							int index = var.Key * 2;
							if (index >= ret.Length)
								ret = ret + var.Value;
							else {
								for (; index > 0 && !char.IsWhiteSpace(ret[index]); index--) // Find previous break
									;
								ret = ret.Substring(0, index) + var.Value + ret.Substring(index);
							}
						}

						scs.Entries[i] = ret;
					} else
						throw new Exception("Something bad happened.");
				} catch (WebException) { }
			}

			Stream ostream = new FileStream(filename + ".new", FileMode.Create, FileAccess.Write);
			scs.Save(ostream);
			ostream.Close();
		}

		private static void OperationPatchCPK()
		{
			string cpkname = ConsolePath("Full path to CPK archive..?");
			if (cpkname.IsEmpty())
				return;

			string parentcpkname = string.Empty;
			Stream parentstream = null;
			uint cpkoffset = 0;

			Stream stream = new CachedReadStream(new FileStream(cpkname, FileMode.Open, FileAccess.Read, FileShare.Read));
			CPK cpk = new CPK(stream);

			switch (Path.GetFileName(cpkname).ToLower()) {
				case "rootr.cpk":
				case "map0r.cpk":
				case "map1r.cpk":
					parentcpkname = cpkname;
					parentstream = stream;
					break;
				default:
					parentcpkname = ConsolePath("Full path to parent CPK archive..?");
					if (parentcpkname.IsEmpty())
						return;
					parentstream = new CachedReadStream(new FileStream(parentcpkname, FileMode.OpenOrCreate, FileAccess.Read, FileShare.Read));

					CPK parentcpk = new CPK(parentstream);
					FileNode file = parentcpk.Root.Find(Path.GetFileName(cpkname), true) as FileNode;
					if (file == null) {
						Console.WriteLine("The CPK is not a child of the parent CPK.");
						return;
					}

					cpkoffset = (uint)(file.Data as Substream).Offset;
					break;
			}

			string filename = ConsolePath("Full path to file to patch in..?");
			if (filename.IsEmpty())
				return;

			string shortname = Path.GetFileName(filename);
			if (shortname.EndsWith(".new"))
				shortname = shortname.Substring(0, shortname.Length - 4);

		rowsearch:
			bool dirstr = cpk.ToC.Columns.Find(r => r.Name == "DirName") != null;

			UTF.Row filerow = cpk.ToC.Rows.Find(r => (r.FindValue("FileName") as UTF.StringValue).Value == shortname && (!dirstr || Path.GetDirectoryName(filename).Replace('\\', '/').EndsWith((r.FindValue("DirName") as UTF.StringValue).Value)));
			if (filerow == null) {
				shortname = ConsolePath("A matching filename was not found. Please enter the original filename now...");
				if (shortname.IsEmpty())
					return;

				goto rowsearch;
			}

			string cpklenfile = parentcpkname + ".patch";

			ulong baseoffset = cpkoffset + (cpk.Header.Rows[0].FindValue("ContentOffset") as UTF.LongValue).Value;
			uint filelen = (uint)new FileInfo(filename).Length;

			ulong fileoffset = baseoffset + (filerow.FindValue("FileOffset") as UTF.LongValue).Value;
			if (filelen > (filerow.FindValue("FileSize") as UTF.IntValue).Value) {
				fileoffset = Util.RoundUp(File.Exists(cpklenfile) ? ulong.Parse(File.ReadAllText(cpklenfile)) : (ulong)parentstream.Length, 0x20);
			}

			long rowoffset = (long)(cpk.Header.Rows[0].FindValue("TocOffset") as UTF.LongValue).Value;
			rowoffset += 0x10;
			EndianReader reader = new EndianReader(stream, Endianness.BigEndian);
			reader.Position = rowoffset + 0x1A;
			uint rowsize = reader.ReadUInt16();
			reader.Position = rowoffset + 0x08;
			rowoffset += reader.ReadUInt32() + 0x08;

			Stream toc = new FileStream(cpkname + ".toc", FileMode.OpenOrCreate);
			EndianReader writer = new EndianReader(toc, Endianness.BigEndian);
			long begin = Util.RoundDown(rowoffset, 4);
			long end = Util.RoundUp(rowoffset + rowsize * cpk.ToC.Rows.Count, 4);

			if (toc.Length == 0) {
				reader.Position = begin;
				Util.StreamCopy(toc, reader, end - begin);
			}

			writer.Position = rowoffset - begin + rowsize * cpk.ToC.Rows.IndexOf(filerow);

			foreach (UTF.Value value in filerow.Values) {
				switch (value.Type.Name) {
					case "FileSize":
						writer.Write(filelen);
						break;
					case "ExtractSize":
						writer.Write(filelen);
						break;
					case "FileOffset":
						writer.Write((ulong)(fileoffset - baseoffset));
						break;
					default:
						writer.Position += value.Type.Size;
						break;
				}
			}

			toc.Close();

			if (fileoffset >= (ulong)parentstream.Length)
				File.WriteAllText(cpklenfile, (cpkoffset + fileoffset + filelen).ToString());

			stream.Close();

			Console.WriteLine("The following patches will need to be applied.");

			Console.WriteLine("<file resize=\"false\" offset=\"0x" + Util.ToString((uint)begin + cpkoffset) + "\" disc=\"" + Path.GetFileName(parentcpkname) + "\" external=\"" + Path.GetFileName(cpkname) + ".toc" + "\" />");
			Console.WriteLine("<file resize=\"false\" offset=\"0x" + Util.ToString((uint)fileoffset) + "\" disc=\"" + Path.GetFileName(parentcpkname) + "\" external=\"" + Path.GetFileName(filename) + "\" />");
		}

		private static void OperationPackCPK()
		{
			Console.WriteLine("This has not been done yet.");
		}

		private static void OperationExtractCPK()
		{
			string filename = ConsolePath("Full path to CPK archive..?");
			if (filename.IsEmpty())
				return;
			Stream stream = new CachedReadStream(new FileStream(filename, FileMode.Open, FileAccess.Read));
			CPK cpk = new CPK(stream);
			string path = Path.Combine(Path.GetDirectoryName(filename), Path.GetFileName(filename) + ".ext");

			cpk.Root.Extract(path);
			stream.Close();
		}

		private static void OperationPng2Tex()
		{
			string filename = ConsolePath("Full path to tex or txm/txv or png file..?");
			if (filename.IsEmpty())
				return;

			if (Path.GetExtension(filename).ToLower() == ".txm" || Path.GetExtension(filename).ToLower() == ".txv")
				filename = filename.Substring(0, filename.Length - 4);
			else if (Path.GetExtension(filename).ToLower() != ".tex") {
				filename = Path.GetDirectoryName(filename);
				if (filename.EndsWith(".ext"))
					filename = filename.Substring(0, filename.Length - 4);
			}

			DelayedStreamCache streams = new DelayedStreamCache();
			Txm tex = OpenTex(filename, streams);
			Stream txmstream;
			Stream txvstream;

			if (File.Exists(filename)) {
				txmstream = new TemporaryStream();
				txvstream = new TemporaryStream();
			} else {
				txmstream = new FileStream(filename + ".txm.new", FileMode.Create);
				txvstream = new FileStream(filename + ".txv.new", FileMode.Create);
			}

			streams.AddStream(txmstream);
			streams.AddStream(txvstream);

			string[] files = Directory.GetFiles(filename + ".ext");
			files = files.OrderBy(f => f).ToArray(); // Want to make sure the .dat files come before the pngs (hacky, but meh).
			foreach (string file in files) {
				string name = Path.GetFileName(file);
				string basename = Path.GetFileNameWithoutExtension(name);

				Txm.Image image = tex.Images.FirstOrDefault(i => i.Name == basename);
				if (image == null)
					continue;

				Stream stream = new FileStream(file, FileMode.Open);
				if (Path.GetExtension(file) == ".dat") {
					image.SecondaryData = stream;
				} else if (Path.GetExtension(file) == ".png") {
					Bitmap bitmap = new Bitmap(stream);
					stream.Close();
					stream = new TemporaryStream();
					image.SecondaryData.Position = 0;
					image.PrimaryEncoding.EncodeImage(stream, bitmap, image.SecondaryData, image.Unknown2 >> 4);
					image.Width = (uint)bitmap.Width;
					image.Height = (uint)bitmap.Height;
					image.PrimaryData = stream;
				}

				streams.AddStream(stream);
			}

			tex.Save(txmstream, txvstream);

			if (File.Exists(filename)) {
				Stream texstream = new FileStream(filename + ".new", FileMode.Create);
				streams.AddStream(texstream);
				FPS4Base fps = new FPS4Base();
				fps.DataOffset = 0x60;
				fps.BlockSize = 0x0C;
				fps.Files = 3;
				fps.Flags = FPS4Base.NodeFlags.SectionSize | FPS4Base.NodeFlags.Filesize | FPS4Base.NodeFlags.Offset;
				fps.Type = new byte[] { 0x74, 0x78, 0x6D, 0x76 }; // "txmv"
				fps.TypeOffset = 0x40;
				fps.Nodes.Add(new FPS4Base.Node(0x60, 0x60, 0x60, "", 0, 0, txmstream));
				fps.Nodes.Add(new FPS4Base.Node(0xC0, (uint)txvstream.Length, (uint)txvstream.Length, "", 0, 0, txvstream));
				fps.Nodes.Add(new FPS4Base.Node(0xC0 + (uint)txvstream.Length, 0, 0, "", 0, 0, null));
				fps.Save(texstream);
			}

			streams.Dispose();
		}

		private static void OperationTex2Png()
		{
			string filename = ConsolePath("Full path to .tex file or partial path to txm/txv..?");
			if (filename.IsEmpty())
				return;
			if (Path.GetExtension(filename).ToLower() == ".txm" || Path.GetExtension(filename).ToLower() == ".txv")
				filename = filename.Substring(0, filename.Length - 4);
			DelayedStreamCache streams = new DelayedStreamCache();
			Txm tex = OpenTex(filename, streams);
			string path = Path.Combine(Path.GetDirectoryName(filename), Path.GetFileName(filename) + ".ext");
			Directory.CreateDirectory(path);
			for (int i = 0; i < tex.Images.Count; i++) {
				string file = Path.Combine(path, tex.Images[i].Name);
				tex.Images[i].PrimaryBitmap.Save(file + ".png", ImageFormat.Png);
				if (tex.Images[i].SecondaryData != null) {
					tex.Images[i].SecondaryData.Position = 0;
					FileStream secondary = new FileStream(file + ".dat", FileMode.Create, FileAccess.Write);
					Util.StreamCopy(secondary, tex.Images[i].SecondaryData);
					secondary.Close();
				}
			}
			streams.Dispose();
		}

		private static Txm OpenTex(string filename, DelayedStreamCache streams)
		{
			Txm tex;
			if (File.Exists(filename)) {
				Stream stream = new CachedReadStream(new FileStream(filename, FileMode.Open, FileAccess.Read));
				tex = Tex.Create(stream);
				streams.AddStream(stream);
			} else {
				Stream txmstream = new CachedReadStream(new FileStream(filename + ".txm", FileMode.Open, FileAccess.Read));
				Stream txvstream = new CachedReadStream(new FileStream(filename + ".txv", FileMode.Open, FileAccess.Read));
				tex = new Txm(txmstream, txvstream);
				streams.AddStream(txmstream);
				streams.AddStream(txvstream);
			}
			return tex;
		}

		private static void OperationPackFPS4()
		{
			string dirname = ConsolePath("Full path to FPS4 file contents..?");
			string filename;
			if (dirname.EndsWith(".ext")) {
				filename = dirname.Substring(0, dirname.Length - 4);
			} else
				filename = ConsolePath("Full path to FPS4 archive..?");

			if (!File.Exists(filename)) {
				Console.WriteLine("FPS4 file does not exist.");
				return;
			}

			Stream stream = new CachedReadStream(new FileStream(filename, FileMode.Open, FileAccess.Read));

			FPS4 fps = new FPS4(stream);

			DelayedStreamCache cache = new DelayedStreamCache();

			string[] files = Directory.GetFiles(dirname, "*", SearchOption.TopDirectoryOnly);
			foreach (string file in files) {
				string fname = Path.GetFileName(file);
				FPS4Base.Node node = fps.Base.Nodes.FirstOrDefault(n => n.Filename == fname);

				Stream data = new FileStream(file, FileMode.Open, FileAccess.Read);
				cache.AddStream(data);

				if (node == null) {
					continue;
					/* TODO: Only add when we want to
					node = new FPS4Base.Node(0, (uint)data.Length, (uint)data.Length, fname, 0, 0, data);
					fps.Base.Nodes.Insert((int)fps.Base.Files - 1, node);
					fps.Base.Files++;
					*/
				} else
					node.Data = data;
			}

			fps.Base.Reorder();

			Stream ostream = new FileStream(filename + ".new", FileMode.Create, FileAccess.Write);
			fps.Base.Save(ostream);
			ostream.Close();

			stream.Close();

			cache.Dispose();
		}

		private static void OperationExtractFPS4()
		{
			string filename = ConsolePath("Full path to FPS4 archive..?");
			if (filename.IsEmpty())
				return;
			Stream stream = new CachedReadStream(new FileStream(filename, FileMode.Open, FileAccess.Read));
			FPS4 fps = new FPS4(stream);
			string path = Path.Combine(Path.GetDirectoryName(filename), Path.GetFileName(filename) + ".ext");
			fps.Root.Extract(path);
			stream.Close();

			File.WriteAllText(Path.Combine(path, "fps4.type"), fps.Type);
		}

		static string ConsolePath(string prompt)
		{
			Console.WriteLine(prompt);
			Console.Write("> ");
			return ConsoleGetArg();
		}

		static int ConsoleMenu(string prompt, params string[] options)
		{
			while (true) {
				Console.WriteLine(prompt);
				for (int i = 0; i < options.Length; i++)
					Console.WriteLine("\t" + (i + 1).ToString() + ") " + options[i]);
				Console.Write("> ");

				string choice = ConsoleGetArg();
				int ret;
				if (int.TryParse(choice, out ret) && ret > 0 && ret <= options.Length)
					return ret - 1;

				Console.WriteLine("That is not a valid choice.");
			}
		}

		static string ConsoleGetArg()
		{
			if (Args == null || Args.Length == 0)
				return Console.ReadLine();

			if (CurrentArg < Args.Length) {
				Console.WriteLine(Args[CurrentArg]);
				return Args[CurrentArg++];
			} else
				Environment.Exit(0);

			return string.Empty;
		}

		static void ConsoleSetArgs(string[] args)
		{
			Args = args;
			CurrentArg = 0;
		}
	}
}
