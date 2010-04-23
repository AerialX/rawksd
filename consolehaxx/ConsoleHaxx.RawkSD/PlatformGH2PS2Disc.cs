using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using ConsoleHaxx.Common;
using ConsoleHaxx.Harmonix;

namespace ConsoleHaxx.RawkSD
{
	public class PlatformGH2PS2Disc : Engine
	{
		public static readonly PlatformGH2PS2Disc Instance;
		static PlatformGH2PS2Disc()
		{
			Instance = new PlatformGH2PS2Disc();
		}

		public override int ID { get { throw new NotImplementedException(); } }

		public override string Name { get { return "Guitar Hero 2 PS2 Disc"; } }

		public override bool IsValid(string path)
		{
			throw new NotImplementedException();
		}

		public override bool AddSong(PlatformData data, SongData song)
		{
			FormatData formatdata = new MemoryFormatData(song);
			formatdata.StreamOwnership = false;

			DirectoryNode songdir = data.Session["songdir"] as DirectoryNode;

			// SongsDTA dta = MetadataFormatHarmonix.GetSongsDTA(song);

			DirectoryNode songnode = songdir.Navigate(song.ID) as DirectoryNode;

			FileNode songaudiofile = songnode.Find(song.ID + ".vgs") as FileNode;
			if (songaudiofile == null) {
				songaudiofile = songnode.Find(song.ID + "_sp.vgs") as FileNode;
				if (songaudiofile == null)
					return false;
			}

			FileNode chartfile = songnode.Find(song.ID + ".mid") as FileNode;
			if (chartfile == null)
				return false;

			if (data.Game.ID == Games.GuitarHero1)
				ChartFormatGH1.Instance.Create(formatdata, chartfile == null ? null : chartfile.Data);
			else
				ChartFormatGH2.Instance.Create(formatdata, chartfile == null ? null : chartfile.Data);

			AudioFormatVGS.Instance.Create(formatdata, songaudiofile.Data);

			data.AddSong(formatdata);

			return true;
		}

		public override PlatformData Create(string path, Game game)
		{
			if (File.Exists(path)) {
				if (String.Compare(Path.GetExtension(path), ".ark", true) == 0 || String.Compare(Path.GetExtension(path), ".hdr", true) == 0)
					path = Path.GetDirectoryName(path);
			}

			PlatformData data = new PlatformData(this, game);

			DirectoryNode dir = data.GetPS2DirectoryStructure(path);

			Ark ark = Platform.GetHarmonixArk(dir);

			DirectoryNode songdir = ark.Root.Find("songs") as DirectoryNode;
			if (songdir == null)
				throw new FormatException();

			FileNode songsdtbfile = songdir.Navigate("gen/songs.dtb") as FileNode;
			if (songsdtbfile == null)
				throw new FormatException();

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
			}

			data.Session["songdir"] = dir;

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

			return data;
		}
	}
}
