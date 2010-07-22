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

			public bool Update(FormatData data = null, SongData song = null)
			{
				if (data == null)
					data = PrimaryData;
				if (data == null)
					return false;
				if (song == null)
					song = data.Song;

				Item.SubItems.Clear();
				Item.Text = song.Name;
				Item.SubItems.Add(song.Artist);
				Item.SubItems.Add(song.Album);
				Item.SubItems.Add(song.Year.ToString());
				Item.SubItems.Add(song.TidyGenre);
				Item.ToolTipText = song.Pack;
				SongID = song.ID;

				Item.SubItems.Add("");

				if (Images.Game == Game.Unknown)
					Images.SetGame(song.Game);
				
				UpdateImage();

				return true;
			}

			private void UpdateImage()
			{
				if (Data.FirstOrDefault(d => d.PlatformData == Program.Form.Storage) != null)
					Images.SetImage(0, true);
				else
					Images.SetImage(0, false);
				
				if (Data.FirstOrDefault(d => d.PlatformData == Program.Form.SD) != null)
					Images.SetImage(1, true);
				else
					Images.SetImage(1, false);

				Item.SubItems[5].Text = Images.ToString();
			}

			public void AddData(FormatData data, SongData song = null)
			{
				if (Data.Contains(data))
					return;
				
				Data.Add(data);

				if (PrimaryData == data)
					Update(data, song);
				else
					UpdateImage();
			}

			public bool HasData()
			{
				return Data.Count > 0;
			}

			public bool HasData(FormatData data)
			{
				return Data.Contains(data);
			}

			public void RemoveData(FormatData data)
			{
				if (Data.Remove(data)) {
					Update();
					UpdateImage();
				}
			}

			public SongListItem(ListViewItem item, FormatData data, SongData song = null)
			{
				Data = new List<FormatData>();
				Images = new SongImageList();

				Item = item;
				AddData(data, song);
			}
		}

		private void UpdateSongList()
		{
			SongUpdateMutex.WaitOne();

			UpdateSongList(Storage);
			foreach (PlatformData data in Platforms)
				UpdateSongList(data);
			UpdateSongList(SD);
			Invoke((Action)AutoResizeColumns);

			Invoke((Action<object, EventArgs>)SongList_SelectedIndexChanged, null, EventArgs.Empty);

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
			SongListItem item;
			foreach (FormatData song in data.GetChanges(false)) {
				SongData songdata = song.Song;
				item = null;
				foreach (ListViewItem songitem in SongList.Items) {
					SongListItem songtag = songitem.Tag as SongListItem;
					if (songtag.SongID == songdata.ID) {
						item = songtag;
						break;
					}
				}
				if (item == null) {
					ListViewItem listitem = new ListViewItem("");
					listitem.Tag = new SongListItem(listitem, song, songdata);
					SongList.Items.Add(listitem);
				} else {
					item.AddData(song, songdata);
				}
			}
			foreach (FormatData song in data.GetChanges(true)) {
				foreach (ListViewItem songitem in SongList.Items) {
					item = songitem.Tag as SongListItem;
					if (item.HasData(song)) {
						item.RemoveData(song);
						if (!item.HasData())
							SongList.Items.Remove(songitem);
						break;
					}
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
				AlbumArtPicture.Visible = true;
				AlbumArtPicture.Image = AlbumImage;
				NameLabel.Visible = false;
				ArtistLabel.Visible = false;
				AlbumLabel.Visible = false;
				GenreLabel.Visible = false;
				YearLabel.Visible = false;
				DifficultyLayout.Visible = false;

				// TODO: Show some sort of info...
			}
		}

		private void SongList_SelectedIndexChanged(object sender, EventArgs e)
		{
			SongListTimer.Start(); // Fucking hack because SWF handles selections in a retarded fashion
		}

		private void SongListTimer_Tick(object sender, EventArgs e)
		{
			if (SongList.SelectedItems.Count > 0) {
				if (SongList.SelectedItems.Count > 0 && SongList.SelectedItems[0].Tag is SongListItem) {
					FormatData data = (SongList.SelectedItems[0].Tag as SongListItem).PrimaryData;
					if (data != null)
						PopulateSongInfo(data.Song);
				}

				MenuSongsEdit.Enabled = true;
				MenuSongsDelete.Enabled = true;
				MenuSongsInstall.Enabled = true;
				MenuSongsExport.Enabled = true;
				MenuSongsTranscode.Enabled = true;

				ContextMenuEdit.Enabled = true;
				ContextMenuDelete.Enabled = true;
				ContextMenuInstall.Enabled = true;
				ContextMenuExport.Enabled = true;
				ContextMenuTranscode.Enabled = true;

				ToolbarEdit.Enabled = true;
				ToolbarDelete.Enabled = true;
				ToolbarInstallLocal.Enabled = true;
				if (SD != null)
					ToolbarInstallSD.Enabled = true;
				else
					ToolbarInstallSD.Enabled = false;

				if (SongList.SelectedItems.Count > 1) {
					MenuSongsExportRBN.Enabled = false;
					ContextMenuExportRBA.Enabled = false;
				} else {
					MenuSongsExportRBN.Enabled = true;
					ContextMenuExportRBA.Enabled = true;
				}

				StatusSongsLabel.Text = SongList.SelectedItems.Count.ToString() + " Song" + (SongList.SelectedItems.Count == 1 ? "" : "s") + " Selected";
			} else {
				MenuSongsEdit.Enabled = false;
				MenuSongsDelete.Enabled = false;
				MenuSongsInstall.Enabled = false;
				MenuSongsExport.Enabled = false;
				MenuSongsTranscode.Enabled = false;

				ContextMenuEdit.Enabled = false;
				ContextMenuDelete.Enabled = false;
				ContextMenuInstall.Enabled = false;
				ContextMenuExport.Enabled = false;
				ContextMenuTranscode.Enabled = false;

				ToolbarEdit.Enabled = false;
				ToolbarDelete.Enabled = false;
				ToolbarInstallLocal.Enabled = false;
				ToolbarInstallSD.Enabled = false;

				PopulateSongInfo(null);

				if (Storage != null)
					StatusSongsLabel.Text = Storage.Songs.Count.ToString() + " Local Song" + (Storage.Songs.Count == 1 ? "" : "s");
			}

			SongListTimer.Stop();
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
