using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using ConsoleHaxx.Common;
using ConsoleHaxx.Wii;
using ConsoleHaxx.Neversoft;
using Nanook.QueenBee.Parser;
using System.IO;

namespace ConsoleHaxx.RawkSD
{
	public class PlatformGH4WiiDLC : Engine
	{
		public static readonly PakFormat PakFormat = new PakFormat(null, null, null, PakFormatType.Wii);

		public static PlatformGH4WiiDLC Instance;
		public static void Initialise()
		{
			Instance = new PlatformGH4WiiDLC();
			PlatformDetection.DetectDirectoryNode += new Action<string, DirectoryNode, List<Pair<Engine, Game>>>(PlatformDetection_DetectDirectoryNode);
		}

		static void PlatformDetection_DetectDirectoryNode(string path, DirectoryNode root, List<Pair<Engine, Game>> platforms)
		{
			FileNode file = root.Navigate("001.bin") as FileNode;
			if (file == null)
				return;
			try {
				DlcBin bin = new DlcBin(file.Data);
				if (bin.Bk.TitleID == 0x0001000053584145)
					platforms.Add(new Pair<Engine, Game>(Instance, Game.GuitarHeroWorldTour));
			} catch { }
			file.Data.Close();
		}

		public override int ID { get { throw new NotImplementedException(); } }

		public override string Name { get { return "Guitar Hero World Tour DLC"; } }

		public override bool AddSong(PlatformData data, SongData song, ProgressIndicator progress)
		{
			string songid = song.ID;
			int index = int.Parse(songid.Substring(3));
			song.ID = "dlc" + ImportMap.GetShortName(song.Name);

			FormatData formatdata = new TemporaryFormatData(song, data);

			DirectoryNode dir = data.Session["rootdir"] as DirectoryNode;

			FileNode binfile = dir.Navigate(Util.Pad(index.ToString(), 3) + ".bin") as FileNode;
			if (binfile == null)
				return false;
			DlcBin bin = new DlcBin(binfile.Data);
			U8 u8 = new U8(bin.Data);

			FileNode chartpak = u8.Root.Find(songid + "_song.pak.ngc", SearchOption.AllDirectories) as FileNode;
			FileNode textpak = u8.Root.Find(songid + "_text.pak.ngc", SearchOption.AllDirectories) as FileNode;

			FileNode audiofile = u8.Root.Find(songid + ".bik", SearchOption.AllDirectories) as FileNode;

			if (chartpak == null || textpak == null || audiofile == null)
				return false;

			ChartFormatGH4.Instance.Create(formatdata, new Stream[] { chartpak.Data, textpak.Data }, PlatformGH5WiiDisc.ImportExpertPlus);

			if (audiofile != null)
				AudioFormatBink.Instance.Create(formatdata, audiofile.Data, null, null);

			data.AddSong(formatdata);

			chartpak.Data.Close();

			return true;
		}

		public override PlatformData Create(string path, Game game, ProgressIndicator progress)
		{
			PlatformData data = new PlatformData(this, game);

			DirectoryNode dir = data.GetDirectoryStructure(path);

			FileNode binfile = dir.Navigate("001.bin") as FileNode;
			if (binfile == null)
				Exceptions.Error("Unable to open Guitar Hero World Tour DLC because 001.bin is missing.");

			data.Session["rootdir"] = dir;

			try {
				DlcBin bin = new DlcBin(binfile.Data);
				U8 u8 = new U8(bin.Data);
				FileNode listfile = u8.Root.Navigate("DLC1.pak.ngc") as FileNode;
				Pak qb = new Pak(new EndianReader(listfile.Data, Endianness.BigEndian));
				FileNode songlistfile = qb.Root.Find("catalog_info.qb.ngc", SearchOption.AllDirectories) as FileNode;
				QbFile songlist = new QbFile(songlistfile.Data, PakFormat);

				StringList strings = new StringList();
				foreach (Pak.Node node in qb.Nodes) {
					if (!node.Filename.HasValue())
						strings.ParseFromStream(node.Data);
				}

				List<QbKey> listkeys = new List<QbKey>();
				foreach (uint songlistkey in NeversoftMetadata.SonglistKeys) {
					QbKey key = QbKey.Create(songlistkey);
					QbItemStruct list = songlist.FindItem(key, true) as QbItemStruct;
					if (list != null && list.Items.Count > 0)
						listkeys.Add(key);
				}

				progress.NewTask(listkeys.Count);
				foreach (QbKey songlistkey in listkeys) {
					QbItemStruct list = songlist.FindItem(songlistkey, true) as QbItemStruct;

					progress.NewTask(list.Items.Count);

					foreach (QbItemArray item in list.Items.OfType<QbItemArray>()) {
						item.Items[0].ItemQbKey = item.ItemQbKey;
						SongData song = NeversoftMetadata.GetSongData(data, item.Items[0] as QbItemStruct, strings);

						try {
							AddSong(data, song, progress);
						} catch (Exception exception) {
							Exceptions.Warning(exception, "Unable to properly parse " + song.Name);
						}
						progress.Progress();
					}

					progress.EndTask();
					progress.Progress();
				}
				progress.EndTask();

				binfile.Data.Close();
			} catch (Exception exception) {
				Exceptions.Error(exception, "An error occurred while parsing the Guitar Hero World Tour DLC list.");
			}

			return data;
		}
	}
}
