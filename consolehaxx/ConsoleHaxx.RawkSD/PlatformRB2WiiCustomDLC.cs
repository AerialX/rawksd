using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using ConsoleHaxx.Common;
using ConsoleHaxx.Harmonix;
using ConsoleHaxx.Wii;

namespace ConsoleHaxx.RawkSD
{
	public class PlatformRB2WiiCustomDLC : Engine
	{
		public static readonly PlatformRB2WiiCustomDLC Instance;
		static PlatformRB2WiiCustomDLC()
		{
			Instance = new PlatformRB2WiiCustomDLC();
		}

		public override int ID { get { throw new NotImplementedException(); } }

		public override string Name { get { return "Rock Band 2 Wii RawkSD DLC"; } }

		public override bool IsValid(string path)
		{
			throw new NotImplementedException();
		}

		public override bool AddSong(PlatformData data, SongData song)
		{
			FormatData formatdata = new MemoryFormatData(song);
			formatdata.StreamOwnership = false;

			DirectoryNode maindir = data.Session["maindir"] as DirectoryNode;

			SongsDTA dta = MetadataFormatHarmonix.GetSongsDTA(song);

			FileNode contentbin = null;

			DlcBin content = new DlcBin(contentbin.Data);

			U8 u8 = new U8(content.Data);

			DirectoryNode songnode = u8.Root.Navigate("/content/songs/" + song.ID, false, true) as DirectoryNode;

			FileNode songaudiofile = songnode.Find(song.ID + ".bik") as FileNode;
			if (songaudiofile == null) {
				songaudiofile = songnode.Find(song.ID + ".mogg") as FileNode;
				if (songaudiofile == null)
					return false;
				AudioFormatMogg.Instance.Create(formatdata, songaudiofile.Data);
			} else {
				AudioFormatRB2Bink.Instance.Create(formatdata, songaudiofile.Data);
			}

			FileNode chartfile = songnode.Find(song.ID + ".mid") as FileNode;
			FileNode panfile = songnode.Find(song.ID + ".pan") as FileNode;
			FileNode weightsfile = songnode.Navigate("gen/" + song.ID + "_weights.bin") as FileNode;
			FileNode milofile = songnode.Navigate("gen/" + song.ID + ".milo_wii") as FileNode;
			FileNode albumfile = songnode.Navigate("gen/" + song.ID + "_keep.png_wii") as FileNode;

			if (chartfile == null)
				return false;

			if (albumfile == null)
				albumfile = songnode.Navigate("gen/" + song.ID + "_nomip_keep.bmp_wii") as FileNode;

			if (albumfile != null) {
				song.AlbumArt = WiiImage.Create(new EndianReader(albumfile.Data, Endianness.LittleEndian)).Bitmap;
			}

			ChartFormatRB.Instance.Create(formatdata, chartfile == null ? null : chartfile.Data, panfile == null ? null : panfile.Data, weightsfile == null ? null : weightsfile.Data, milofile == null ? null : milofile.Data);

			data.AddSong(formatdata);

			return true;
		}

		public override PlatformData Create(string path, Game game)
		{
			if (!Directory.Exists(path))
				throw new FormatException();

			PlatformData data = new PlatformData(this, game);

			DirectoryNode maindir = DirectoryNode.FromPath(path, data.Cache, FileAccess.Read);

			DirectoryNode customdir = maindir.Navigate("rawk/rb2/customs", false, true) as DirectoryNode;
			if (customdir == null)
				throw new FormatException();

			data.Session["maindir"] = maindir;

			foreach (Node node in customdir.Children) {
				DirectoryNode dir = node as DirectoryNode;
				if (dir == null)
					continue;

				FileNode dtafile = dir.Navigate("data", false, true) as FileNode;
				if (dtafile == null)
					continue;

				EndianReader reader = new EndianReader(dtafile.Data, Endianness.LittleEndian);
				DTB.NodeTree dtb = DTB.Create(reader);

				SongData song = MetadataFormatHarmonix.GetSongData(dtb);

				AddSong(data, song);
			}

			return data;
		}

		public override FormatData CreateSong(PlatformData data, SongData song)
		{
			TemporaryFormatData formatdata = new TemporaryFormatData(song);
			return formatdata;
		}

		public override void SaveSong(PlatformData data, FormatData formatdata)
		{
			throw new NotImplementedException();
		}
	}
}
