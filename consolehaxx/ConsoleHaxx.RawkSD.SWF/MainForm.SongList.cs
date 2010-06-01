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
			public ListViewItem Item;
			public SongImageList Images;
			public string SongID;
			public List<FormatData> Data;
			public FormatData PrimaryData
			{
				get
				{
					return Data.FirstOrDefault(d => d.PlatformData == Program.Form.Storage) ?? Data.FirstOrDefault(d => d.PlatformData != Program.Form.SD) ?? Data.FirstOrDefault();
				}
			}

			public void AddData(FormatData data)
			{
				if (!Data.Contains(data))
					Data.Add(data);

				SongData song = data.Song;
				if (PrimaryData == data) {
					Item.SubItems.Clear();
					Item.Text = song.Name;
					Item.SubItems.Add(song.Artist);
					Item.SubItems.Add(song.Album);
					Item.SubItems.Add(song.Year.ToString());
					Item.SubItems.Add(song.TidyGenre);
					Item.ToolTipText = song.Pack;
					SongID = song.ID;

					Item.SubItems.Add("");
				}

				if (song.Game != Game.Unknown)
					Images.SetGame(song.Game);
				if (data.PlatformData == Program.Form.Storage)
					Images.SetImage(0, true);
				if (data.PlatformData == Program.Form.SD)
					Images.SetImage(1, true);
				Item.SubItems[5].Text = Images.ToString();
			}

			public SongListItem(ListViewItem item, FormatData data)
			{
				Data = new List<FormatData>();
				Images = new SongImageList();

				Item = item;
				AddData(data);
			}
		}

		private void RefreshSongList()
		{
			Invoke((Action)ClearSongList);
			UpdateSongList();
		}

		private void ClearSongList()
		{
			SongUpdateMutex.WaitOne();

			SongList.Items.Clear();

			SongUpdateMutex.ReleaseMutex();
		}

		private void UpdateSongList()
		{
			SongUpdateMutex.WaitOne();

			UpdateSongList(Storage);
			UpdateSongList(SD);
			foreach (PlatformData data in Platforms)
				UpdateSongList(data);
			Invoke((Action)AutoResizeColumns);

			SongUpdateMutex.ReleaseMutex();
		}

		private void UpdateSongList(PlatformData data)
		{
			if (data == null)
				return;

			data.Mutex.WaitOne();
			Invoke((Action<PlatformData>)UpdateSongListBase, data);
			data.Mutex.ReleaseMutex();
		}

		private void UpdateSongListBase(PlatformData data)
		{
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
				if (item == null) {
					item = new ListViewItem(songdata.Name);
					item.Tag = new SongListItem(item, song);
					SongList.Items.Add(item);
				} else {
					(item.Tag as SongListItem).AddData(song);
				}
			}
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
				PopulateSongInfo((SongList.SelectedItems[0].Tag as SongListItem).PrimaryData.Song);

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

		private IList<SongListItem> GetSelectedSongs()
		{
			return SongList.SelectedItems.Cast<ListViewItem>().Select(i => i.Tag as SongListItem).Where(f => f != null).ToList();
		}

		private IList<FormatData> GetSelectedSongData()
		{
			return GetSelectedSongs().Select(s => s.PrimaryData).ToList();
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
				e.DrawBackground();
				SongListItem item = e.Item.Tag as SongListItem;
				int x = e.Bounds.X;
				if (item.Images.Images[0])
					e.Graphics.DrawImage(Properties.Resources.IconFolder, x, e.Bounds.Y, 16, 16);

				x += 20;

				if (SD != null) {
					if (item.Images.Images[1])
						e.Graphics.DrawImage(Properties.Resources.IconRSD, x, e.Bounds.Y, 16, 16);
					x += 20;
				}

				if (item.Images.Game != Game.Unknown)
					e.Graphics.DrawImage(item.Images.GameImage, x, e.Bounds.Y, 16, 16);
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

		public class SongImageList
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

			public void SetImage(int index, bool value)
			{
				Images[index] = value;
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
