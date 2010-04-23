using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using ConsoleHaxx.Wii;
using ConsoleHaxx.Harmonix;
using ConsoleHaxx.Common;
using System.Text.RegularExpressions;

namespace ConsoleHaxx.RawkSD
{
	public class PlatformRB2WiiDisc : Engine
	{
		public static readonly PlatformRB2WiiDisc Instance;
		static PlatformRB2WiiDisc()
		{
			Instance = new PlatformRB2WiiDisc();
		}

		public override int ID { get { throw new NotImplementedException(); } }

		public override string Name { get { return "Rock Band 2 Wii Disc"; } }

		public override bool IsValid(string path)
		{
			if (File.Exists(path)) {
				Stream stream = new FileStream(path, FileMode.Open, FileAccess.Read);
				bool ret = IsValid(stream);
				stream.Close();
				return ret;
			}

			if (Directory.Exists(path)) {
				throw new NotImplementedException();
			}

			return false;
		}

		public bool IsValid(Stream iso)
		{
			try {
				Disc disc = new Disc(iso);
				U8 u8 = disc.Partitions.Find(p => p.Type == PartitionType.Data).Root;
				FileNode hdr = u8.Root.Find("main_wii.hdr", SearchOption.AllDirectories) as FileNode;
				// TODO: main.hdr
				if (hdr != null)
					return true;
			} catch { }

			return false;
		}

		public override bool AddSong(PlatformData data, SongData song)
		{
			FormatData formatdata = new MemoryFormatData(song);
			formatdata.StreamOwnership = false;

			DirectoryNode songdir = data.Session["songdir"] as DirectoryNode;
			
			// SongsDTA dta = MetadataFormatHarmonix.GetSongsDTA(song);

			DirectoryNode songnode = songdir.Find(song.ID) as DirectoryNode;

			FileNode songaudiofile = songnode.Find(song.ID + ".bik") as FileNode;
			if (songaudiofile == null) {
				songaudiofile = songnode.Find(song.ID + ".mogg") as FileNode;
				if (songaudiofile == null)
					return false;
				AudioFormatMogg.Instance.Create(formatdata, songaudiofile.Data);
			} else {
				AudioFormatRB2Bink.Instance.Create(formatdata, songaudiofile.Data);
			}

			// TODO:	SongInfo
			// TODO:	Preview
			// TODO:	Album

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
			PlatformData data = new PlatformData(this, game);

			DirectoryNode dir = data.GetWiiDirectoryStructure(path);
			Ark ark = Platform.GetHarmonixArk(dir);

			data.Game = Platform.DetermineGame(data);

			string[] songdirs = new string[] { "songs", "songs_regional/na", "songs_regional/eu" };
			foreach (string songdirname in songdirs) {
				DirectoryNode songdir = ark.Root.Navigate(songdirname) as DirectoryNode;
				if (songdir == null)
					continue;

				FileNode songsdtbfile = songdir.Navigate("gen/songs.dtb") as FileNode;
				if (songsdtbfile == null)
					continue;

				data.Session["songdir"] = songdir;

				DTB.NodeTree dtb = DTB.Create(new EndianReader(new CryptedDtbStream(new EndianReader(songsdtbfile.Data, Endianness.LittleEndian)), Endianness.LittleEndian));
				List<SongsDTA> dtas = new List<SongsDTA>();
				foreach (DTB.Node node in dtb.Nodes) {
					DTB.NodeTree tree = node as DTB.NodeTree;
					if (tree == null || tree.Nodes[0].Type != 0x00000005 || songdir.Find((tree.Nodes[0] as DTB.NodeString).Text) == null)
						continue;

					SongsDTA dta = SongsDTA.Create(tree);
					if (dtas.Find(d => d.BaseName == dta.BaseName) != null)
						continue; // Don't import songs twice

					dtas.Add(dta);

					SongData song = MetadataFormatHarmonix.GetSongData(tree);

					AddSong(data, song);
				}
			}

			return data;
		}
	}
}
