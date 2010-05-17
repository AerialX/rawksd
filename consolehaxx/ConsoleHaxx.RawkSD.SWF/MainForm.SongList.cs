using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Drawing;
using System.Collections;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.RawkSD.SWF
{
	public partial class MainForm
	{
		public class SongListItem
		{
			public FormatData Data;
			public string SongID;

			public SongListItem(FormatData data)
			{
				Data = data;
				SongID = data.Song.ID;
			}
		}

		private void RefreshSongList()
		{
			SongList.Items.Clear();
			UpdateSongList();
		}

		private void UpdateSongList()
		{
			UpdateSongList(Storage, 0, true);
			UpdateSongList(SD, 1);
			foreach (PlatformData data in Platforms)
				UpdateSongList(data);
		}

		private void UpdateSongList(PlatformData data, int imageindex = 2, bool master = false)
		{
			if (data == null)
				return;

			foreach (FormatData song in data.Songs) {
				SongData songdata = song.Song;
				ListViewItem item = null;
				foreach (ListViewItem songitem in SongList.Items) {
					SongListItem songtag = songitem.Tag as SongListItem;
					if (songtag.SongID == songdata.ID) {
						item = songitem;
						break;
					}
				}
				SongImageList images = null;
				if (item == null) {
					item = new ListViewItem(songdata.Name);
					item.Tag = new SongListItem(song);
					item.SubItems.Add(songdata.Artist);
					item.SubItems.Add(songdata.Album);
					item.SubItems.Add(songdata.Year.ToString());
					item.SubItems.Add(songdata.TidyGenre);
					item.SubItems.Add("");
					item.ToolTipText = songdata.Pack;
					images = new SongImageList();
					SongList.Items.Add(item);
				} else if (master) {
					item.Tag = new SongListItem(song);
					item.Text = songdata.Name;
					item.SubItems[1].Text = songdata.Artist;
					item.SubItems[2].Text = songdata.Album;
					item.SubItems[3].Text = songdata.Year.ToString();
					item.SubItems[4].Text = songdata.TidyGenre;
					item.ToolTipText = songdata.Pack;
				}
				if (images == null)
					images = new SongImageList(item.SubItems[5].Text);
				if (imageindex == 2 || master)
					images.SetGame(songdata.Game);
				if (imageindex < 2)
					images.SetImage(imageindex);
				item.SubItems[5].Text = images.ToString();
			}

			AutoResizeColumns();
		}

		private void AutoResizeColumns()
		{
			foreach (ColumnHeader column in SongList.Columns)
				column.AutoResize(SongList.Items.Count > 0 ? ColumnHeaderAutoResizeStyle.ColumnContent : ColumnHeaderAutoResizeStyle.HeaderSize);
			SongList.Columns[5].AutoResize(ColumnHeaderAutoResizeStyle.HeaderSize);
		}

		private void PopulateSongInfo(SongData song)
		{
			if (song != null) {
				if (song.AlbumArt != null) {
					AlbumArtPicture.Visible = true;
					AlbumArtPicture.Image = song.AlbumArt;
				} else {
					// AlbumArtPicture.Visible = false;
					AlbumArtPicture.Visible = true;
					AlbumArtPicture.Image = AlbumImage;
				}

				if (song.Name.HasValue()) {
					NameLabel.Visible = true;
					NameLabel.Text = song.Name;
				} else {
					NameLabel.Text = "";
					NameLabel.Visible = false;
				}

				if (song.Artist.HasValue()) {
					ArtistLabel.Visible = true;
					ArtistLabel.Text = song.Artist;
				} else {
					ArtistLabel.Text = "";
					ArtistLabel.Visible = false;
				}

				if (song.Album.HasValue()) {
					AlbumLabel.Visible = true;
					AlbumLabel.Text = song.Album;
				} else {
					AlbumLabel.Text = "";
					AlbumLabel.Visible = false;
				}

				if (song.Genre.HasValue()) {
					GenreLabel.Visible = true;
					GenreLabel.Text = song.TidyGenre;
				} else {
					GenreLabel.Text = "";
					GenreLabel.Visible = false;
				}

				if (song.Year > 0) {
					YearLabel.Visible = true;
					YearLabel.Text = song.Year.ToString();
				} else {
					YearLabel.Text = "";
					YearLabel.Visible = false;
				}

				DifficultyLayout.Visible = true;
				foreach (var diff in song.Difficulty) {
					Label label = null;

					switch (diff.Key) {
						case Instrument.Ambient:
							label = DifficultyBandValueLabel;
							break;
						case Instrument.Guitar:
							label = DifficultyGuitarValueLabel;
							break;
						case Instrument.Bass:
							label = DifficultyBassValueLabel;
							break;
						case Instrument.Drums:
							label = DifficultyDrumsValueLabel;
							break;
						case Instrument.Vocals:
							label = DifficultyVocalsValueLabel;
							break;
					}


					if (diff.Value == 0 && diff.Key != Instrument.Ambient) {
						label.Image = null;
						label.Text = "NO PART";
					} else {
						label.Text = string.Empty;
						label.Image = Tiers[ImportMap.GetBaseTier(diff.Key, diff.Value)];
						//label. = diff.Value.ToString();
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
			if (SongList.SelectedItems.Count > 0 && SongList.SelectedItems[0].Tag is SongListItem)
				PopulateSongInfo((SongList.SelectedItems[0].Tag as SongListItem).Data.Song);

			if (SongList.SelectedItems.Count > 0) {
				MenuSongsEdit.Enabled = true;
				MenuSongsDelete.Enabled = true;
				MenuSongsInstall.Enabled = true;
				MenuSongsExport.Enabled = true;

				ContextMenuEdit.Enabled = true;
				ContextMenuDelete.Enabled = true;
				ContextMenuInstall.Enabled = true;
				ContextMenuExport.Enabled = true;

				if (SongList.SelectedItems.Count > 1) {
					MenuSongsExportRBN.Enabled = false;
					ContextMenuExportRBA.Enabled = false;
				} else {
					MenuSongsExportRBN.Enabled = true;
					ContextMenuExportRBA.Enabled = true;
				}
			} else {
				MenuSongsEdit.Enabled = false;
				MenuSongsDelete.Enabled = false;
				MenuSongsInstall.Enabled = false;
				MenuSongsExport.Enabled = false;

				ContextMenuEdit.Enabled = false;
				ContextMenuDelete.Enabled = false;
				ContextMenuInstall.Enabled = false;
				ContextMenuExport.Enabled = false;
			}
		}

		private IEnumerable<FormatData> GetSelectedSongs()
		{
			return SongList.SelectedItems.Cast<ListViewItem>().Select(i => i.Tag as SongListItem).Where(f => f != null).Select(f => f.Data);
		}

		private void SongList_DoubleClick(object sender, EventArgs e)
		{
			switch (Configuration.DefaultAction) {
				case Configuration.DefaultActionType.Unknown:
					break;
				case Configuration.DefaultActionType.InstallLocal:
					MenuSongsInstallLocal_Click(this, EventArgs.Empty);
					break;
				case Configuration.DefaultActionType.InstallSD:
					if (SD == null && Configuration.LocalTranscode)
						MenuSongsInstallLocal_Click(this, EventArgs.Empty);
					else if (SD != null)
						MenuSongsInstallSD_Click(this, EventArgs.Empty);
					break;
				case Configuration.DefaultActionType.ExportRawk:
					MenuSongsExportRawkSD_Click(this, EventArgs.Empty);
					break;
				case Configuration.DefaultActionType.ExportRBA:
					MenuSongsExportRBN_Click(this, EventArgs.Empty);
					break;
				case Configuration.DefaultActionType.Export360:
					MenuSongsExport360DLC_Click(this, EventArgs.Empty);
					break;
				default:
					break;
			}
		}

		private void SongList_DrawSubItem(object sender, DrawListViewSubItemEventArgs e)
		{
			if (e.ColumnIndex == 5) {
				/*if (e.Item.Selected) {
					e.Graphics.FillRectangle(new SolidBrush(SystemColors.Highlight), new Rectangle(e.Bounds.X - 1, e.Bounds.Y + 1, e.Bounds.Width - 1, e.Bounds.Height - 2));
				} else*/
					e.DrawBackground();
				//e.DrawFocusRectangle(e.Bounds);
				SongImageList images = new SongImageList(e.SubItem.Text);
				int x = e.Bounds.X;
				if (images.Images[0])
					e.Graphics.DrawImage(Properties.Resources.IconFolder, x, e.Bounds.Y, 16, 16);

				x += 20;

				if (SD != null) {
					if (images.Images[1])
						e.Graphics.DrawImage(Properties.Resources.IconRSD, x, e.Bounds.Y, 16, 16);
					x += 20;
				}

				if (images.Game != Game.Unknown)
					e.Graphics.DrawImage(images.GameImage, x, e.Bounds.Y, 16, 16);
			} else
				e.DrawDefault = true;
		}

		private void SongList_DrawItem(object sender, DrawListViewItemEventArgs e)
		{

		}

		private void SongList_DrawColumnHeader(object sender, DrawListViewColumnHeaderEventArgs e)
		{
			e.DrawDefault = true;
		}

		private int sortColumn = -1;
		private void SongList_ColumnClick(object sender, ColumnClickEventArgs e)
		{
			if (e.Column != sortColumn) {
				sortColumn = e.Column;
				SongList.Sorting = SortOrder.Ascending;
			} else {
				if (SongList.Sorting == SortOrder.Ascending)
					SongList.Sorting = SortOrder.Descending;
				else
					SongList.Sorting = SortOrder.Ascending;
			}

			SongList.ListViewItemSorter = new ListViewItemComparer(e.Column, SongList.Sorting);
			SongList.Sort();
		}

		private class SongImageList
		{
			public bool[] Images;
			public Game Game;

			public SongImageList()
			{
				Images = new bool[2];
				Game = Game.Unknown;
			}

			public SongImageList(string text) : this()
			{
				while (text.Length > 0) {
					if (text[0] == 'L') {
						Images[0] = true;
						text = text.Substring(1);
					} else if (text[0] == 'S') {
						Images[1] = true;
						text = text.Substring(1);
					} else {
						Game = (Game)int.Parse(text);
						text = string.Empty;
					}
				}
			}

			public override string ToString()
			{
				string str = "";
				if (Images[0])
					str = "L";
				if (Images[1])
					str += 'S';
				str += Util.Pad(((int)Game).ToString(), 3);
				return str;
			}

			public void SetImage(int index)
			{
				Images[index] = true;
			}

			public void SetGame(Game game)
			{
				Game = game;
			}

			public Bitmap GameImage
			{
				get
				{
					switch (Game) {
						case Game.GuitarHero1:
						case Game.GuitarHero2:
						case Game.GuitarHero80s:
							return Properties.Resources.IconGH2;
						case Game.GuitarHero3:
						case Game.GuitarHeroAerosmith:
							return Properties.Resources.IconGH3;
						case Game.GuitarHeroWorldTour:
						case Game.GuitarHeroMetallica:
						case Game.GuitarHeroSmashHits:
						case Game.GuitarHeroVanHalen:
						case Game.GuitarHero5:
						case Game.BandHero:
							return Properties.Resources.IconGH;
						case Game.RockBand:
						case Game.RockBandACDC:
						case Game.RockBandTP1:
						case Game.RockBandTP2:
						case Game.RockBandCountryTP:
						case Game.RockBandClassicTP:
						case Game.RockBandMetalTP:
						case Game.RockBand2:
						case Game.RockBandBeatles:
						case Game.LegoRockBand:
						case Game.RockBandGreenDay:
						case Game.RockBand3:
							return Properties.Resources.IconRB;
					}

					return new Bitmap(16, 16);
				}
			}
		}

		class ListViewItemComparer : IComparer
		{
			private int col;
			private SortOrder order;
			public ListViewItemComparer()
			{
				col = 0;
				order = SortOrder.Ascending;
			}
			public ListViewItemComparer(int column, SortOrder order)
			{
				col = column;
				this.order = order;
			}
			public int Compare(object x, object y)
			{
				int returnVal = -1;
				returnVal = String.Compare(((ListViewItem)x).SubItems[col].Text, ((ListViewItem)y).SubItems[col].Text);
				if (order == SortOrder.Descending)
					returnVal *= -1;
				return returnVal;
			}
		}

		public class DoubleBufferedListView : ListView
		{
			public void DoubleBuffer()
			{
				DoubleBuffered = true;
			}
		}
	}
}
