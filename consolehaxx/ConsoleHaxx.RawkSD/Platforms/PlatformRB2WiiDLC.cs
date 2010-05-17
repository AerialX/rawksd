using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using ConsoleHaxx.Common;
using System.IO;
using ConsoleHaxx.Wii;
using ConsoleHaxx.Harmonix;

namespace ConsoleHaxx.RawkSD
{
	public class PlatformRB2WiiDLC : Engine, IFormat
	{
		public const string DtaBinName = "dta";
		public const string ContentBinName = "bin";

		public static readonly PlatformRB2WiiDLC Instance;
		static PlatformRB2WiiDLC()
		{
			Instance = new PlatformRB2WiiDLC();
		}

		public override int ID { get { return 0x81; } }

		public override string Name { get { return "Rock Band 2 Wii DLC"; } }

		public override bool AddSong(PlatformData data, SongData song, ProgressIndicator progress)
		{
			throw new NotImplementedException();
		}

		public override PlatformData Create(string path, Game game, ProgressIndicator progress)
		{
			PlatformData data = new PlatformData(this, game);

			DirectoryNode maindir = DirectoryNode.FromPath(path, data.Cache, FileAccess.Read);

			char[] regions = new char[] { 'E', 'P' };

			DirectoryNode dir = maindir.Navigate("private/wii/data", false, true) as DirectoryNode;
			if (dir == null)
				dir = maindir;

			for (char letter = 'A'; letter <= 'Z'; letter++) {
				foreach (char region in regions) {
					DirectoryNode subdir = dir.Find("SZ" + letter + region, true) as DirectoryNode;
					if (subdir == null)
						continue;

					foreach (FileNode file in subdir.Children.OfType<FileNode>()) {
						if (String.Compare(Path.GetExtension(file.Name), ".bin", true) != 0)
							continue;

						try {
							file.Data.Position = 0;
							DlcBin bin = new DlcBin(file.Data);
							U8 u8;
							try {
								u8 = new U8(bin.Data);
							} catch (FormatException) {
								file.Data.Close();
								continue;
							}
							FileNode songsdta = u8.Root.Find("songs.dta", SearchOption.AllDirectories) as FileNode;
							if (songsdta == null) {
								file.Data.Close();
								continue;
							}

							DTB.NodeTree dtb = DTA.Create(songsdta.Data);
							SongsDTA dta = SongsDTA.Create(dtb);
							file.Data.Close();
							
							SongData song = HarmonixMetadata.GetSongData(data, dtb);

							string contentbin = dta.Song.Name.Substring(9, 3); // Example, "dlc/sZAE/023/content/songs/simpleman/simpleman"

							FileNode contentfile = subdir.Find(contentbin + ".bin", true) as FileNode;
							if (contentfile == null)
								continue;

							FormatData formatdata = new TemporaryFormatData(song, data);

							Create(formatdata, file.Data, contentfile.Data);
							
							data.AddSong(formatdata);

							contentfile.Data.Close();
						} catch (FormatException) { } // Not a DLC bin we care about
					}
				}
			}

			return data;
		}

		public void Create(FormatData data, Stream dtabin, Stream contentbin)
		{
			data.SetStream(this, DtaBinName, dtabin);
			data.SetStream(this, ContentBinName, contentbin);
		}

		public FormatType Type { get { return FormatType.Unknown; } }

		public bool Writable { get { return false; } }
		public bool Readable { get { return false; } }

		public object Decode(FormatData data, ProgressIndicator progress) { throw new NotImplementedException(); }

		public void Encode(object data, FormatData destination, ProgressIndicator progress) { throw new NotImplementedException(); }

		public bool CanRemux(IFormat format) { throw new NotImplementedException(); }

		public void Remux(IFormat format, FormatData data, FormatData destination, ProgressIndicator progress) { throw new NotImplementedException(); }
		
		public bool CanTransfer(FormatData data) { return true; }

		public bool HasFormat(FormatData data)
		{
			return data.Formats.Contains(this);
		}
	}
}
