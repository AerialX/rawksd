using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using ConsoleHaxx.Common;
using System.IO;

namespace ConsoleHaxx.RawkSD.SWF
{
	public partial class MainForm : Form
	{
		public PlatformData Storage;
		public string StoragePath;
		public List<PlatformData> Platforms;

		public MainForm()
		{
			InitializeComponent();

			StoragePath = Path.Combine(Environment.CurrentDirectory, "customs");

			Storage = PlatformLocalStorage.Instance.Create(StoragePath, null);

			Platforms = new List<PlatformData>();

			FileOpenMenu_Click(null, null);
		}

		private void FileOpenMenu_Click(object sender, EventArgs e)
		{
			PlatformForm.PlatformFormReturn ret = PlatformForm.Show(this);

			if (ret.Result == DialogResult.Cancel)
				return;

			string path;

			switch (ret.Type) {
				case PlatformForm.PlatformFormReturn.PlatformFormReturnEnum.File:
					OpenDialog.Title = "";
					OpenDialog.Filter = "";
					if (OpenDialog.ShowDialog(this) == DialogResult.Cancel)
						return;
					path = OpenDialog.FileName;
					break;
				case PlatformForm.PlatformFormReturn.PlatformFormReturnEnum.Folder:
					FolderDialog.Description = "";
					if (FolderDialog.ShowDialog(this) == DialogResult.Cancel)
						return;
					path = FolderDialog.SelectedPath;
					break;
				default:
					throw new NotSupportedException();
			}
			PlatformData data = ret.Platform.Create(path, null);

			//PlatformData data = PlatformGH3WiiDisc.Instance.Create(@"Z:\home\discs\Guitar Hero 3.iso", null);
			Platforms.Add(data);

			/*
			TreeNode node = new TreeNode(data.Platform.Name);
			node.Tag = data;

			foreach (FormatData song in data.Songs) {
				TreeNode subnode = new TreeNode(song.Song.Name);
				subnode.Tag = song.Song;
				node.Nodes.Add(subnode);
			}

			CustomsTree.Nodes.Add(node);
			*/

			foreach (FormatData song in data.Songs) {
				ListViewItem item = new ListViewItem(song.Song.Name);
				item.Tag = song;
				item.SubItems.Add(song.Song.Artist);
				item.SubItems.Add(song.Song.Album);
				item.SubItems.Add(song.Song.Year.ToString());
				item.SubItems.Add(song.Song.Genre);
				item.SubItems.Add(data.Game.Name);
				
				SongList.Items.Add(item);
			}
		}

		private void PopulateSongInfo(SongData song)
		{
			if (song != null) {
				if (song.AlbumArt != null) {
					AlbumArtPicture.Visible = true;
					AlbumArtPicture.Image = song.AlbumArt;
				} else
					AlbumArtPicture.Visible = false;

				if (song.Artist.HasValue()) {
					ArtistLabel.Visible = true;
					ArtistLabel.Text = song.Artist;
				} else
					ArtistLabel.Visible = false;

				if (song.Album.HasValue()) {
					AlbumLabel.Visible = true;
					AlbumLabel.Text = song.Album;
				} else
					AlbumLabel.Visible = false;

				if (song.Genre.HasValue()) {
					GenreLabel.Visible = true;
					GenreLabel.Text = song.Genre;
				} else
					GenreLabel.Visible = false;

				if (song.Year > 0) {
					YearLabel.Visible = true;
					YearLabel.Text = song.Year.ToString();
				} else
					YearLabel.Visible = false;

				DifficultyLayout.Visible = true;
				foreach (var diff in song.Difficulty) {
					string text = diff.Value.ToString();
					if (diff.Value == 0)
						text = "NO PART";

					switch (diff.Key) {
						case Instrument.Ambient:
							DifficultyBandValueLabel.Text = text;
							break;
						case Instrument.Guitar:
							DifficultyGuitarValueLabel.Text = text;
							break;
						case Instrument.Bass:
							DifficultyBassValueLabel.Text = text;
							break;
						case Instrument.Drums:
							DifficultyDrumsValueLabel.Text = text;
							break;
						case Instrument.Vocals:
							DifficultyVocalsValueLabel.Text = text;
							break;
					}
				}
			} else {
				AlbumArtPicture.Visible = false;
				ArtistLabel.Visible = false;
				AlbumLabel.Visible = false;
				GenreLabel.Visible = false;
				DifficultyLayout.Visible = false;

				// TODO: Show some sort of info...
			}
		}

		private void SongList_SelectedIndexChanged(object sender, EventArgs e)
		{
			if (SongList.SelectedItems.Count > 0 && SongList.SelectedItems[0].Tag is FormatData)
				PopulateSongInfo((SongList.SelectedItems[0].Tag as FormatData).Song);
		}

		private void SongList_DoubleClick(object sender, EventArgs e)
		{
			if (SongList.SelectedItems.Count > 0 && SongList.SelectedItems[0].Tag is FormatData) {
				ProgressIndicator progress = null;

				FormatData data = SongList.SelectedItems[0].Tag as FormatData;
				FormatData destination = PlatformLocalStorage.Instance.CreateSong(Storage, data.Song);
				Platform.Transfer(data, destination, progress);

				Platform.Transcode(FormatType.Chart, data, new IFormat[] { ChartFormatRB.Instance }, destination, progress);
				//Platform.Transcode(FormatType.Audio, data, new IFormat[] { AudioFormatOgg.Instance }, destination, progress);
				PlatformLocalStorage.Instance.SaveSong(Storage, destination);
			}
		}
	}
}
