namespace ConsoleHaxx.RawkSD.SWF
{
	partial class MainForm
	{
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.IContainer components = null;

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		/// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
		protected override void Dispose(bool disposing)
		{
			if (disposing && (components != null)) {
				components.Dispose();
			}
			base.Dispose(disposing);
		}

		#region Windows Form Designer generated code

		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			this.MenuStrip = new System.Windows.Forms.MenuStrip();
			this.FileMenu = new System.Windows.Forms.ToolStripMenuItem();
			this.FileOpenMenu = new System.Windows.Forms.ToolStripMenuItem();
			this.FileSep1Menu = new System.Windows.Forms.ToolStripSeparator();
			this.FileExitMenu = new System.Windows.Forms.ToolStripMenuItem();
			this.HelpMenu = new System.Windows.Forms.ToolStripMenuItem();
			this.HelpGuideMenu = new System.Windows.Forms.ToolStripMenuItem();
			this.HelpDonateMenu = new System.Windows.Forms.ToolStripMenuItem();
			this.HelpSep1Menu = new System.Windows.Forms.ToolStripSeparator();
			this.HelpAboutMenu = new System.Windows.Forms.ToolStripMenuItem();
			this.StatusBar = new System.Windows.Forms.StatusStrip();
			this.Toolbar = new System.Windows.Forms.ToolStrip();
			this.FolderDialog = new System.Windows.Forms.FolderBrowserDialog();
			this.OpenDialog = new System.Windows.Forms.OpenFileDialog();
			this.TableLayout = new System.Windows.Forms.TableLayoutPanel();
			this.AlbumArtPicture = new System.Windows.Forms.PictureBox();
			this.ArtistLabel = new System.Windows.Forms.Label();
			this.AlbumLabel = new System.Windows.Forms.Label();
			this.GenreLabel = new System.Windows.Forms.Label();
			this.YearLabel = new System.Windows.Forms.Label();
			this.DifficultyLayout = new System.Windows.Forms.TableLayoutPanel();
			this.DifficultyLabel = new System.Windows.Forms.Label();
			this.DifficultyBassValueLabel = new System.Windows.Forms.Label();
			this.DifficultyBassLabel = new System.Windows.Forms.Label();
			this.DifficultyVocalsValueLabel = new System.Windows.Forms.Label();
			this.DifficultyVocalsLabel = new System.Windows.Forms.Label();
			this.DifficultyDrumsValueLabel = new System.Windows.Forms.Label();
			this.DifficultyDrumsLabel = new System.Windows.Forms.Label();
			this.DifficultyGuitarValueLabel = new System.Windows.Forms.Label();
			this.DifficultyGuitarLabel = new System.Windows.Forms.Label();
			this.DifficultyBandValueLabel = new System.Windows.Forms.Label();
			this.DifficultyBandLabel = new System.Windows.Forms.Label();
			this.SongList = new System.Windows.Forms.ListView();
			this.columnHeader1 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
			this.columnHeader2 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
			this.columnHeader3 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
			this.columnHeader4 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
			this.columnHeader5 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
			this.columnHeader6 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
			this.MenuStrip.SuspendLayout();
			this.TableLayout.SuspendLayout();
			((System.ComponentModel.ISupportInitialize)(this.AlbumArtPicture)).BeginInit();
			this.DifficultyLayout.SuspendLayout();
			this.SuspendLayout();
			// 
			// MenuStrip
			// 
			this.MenuStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.FileMenu,
            this.HelpMenu});
			this.MenuStrip.Location = new System.Drawing.Point(0, 0);
			this.MenuStrip.Name = "MenuStrip";
			this.MenuStrip.Size = new System.Drawing.Size(512, 24);
			this.MenuStrip.TabIndex = 0;
			this.MenuStrip.Text = "menuStrip1";
			// 
			// FileMenu
			// 
			this.FileMenu.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.FileOpenMenu,
            this.FileSep1Menu,
            this.FileExitMenu});
			this.FileMenu.Name = "FileMenu";
			this.FileMenu.Size = new System.Drawing.Size(35, 20);
			this.FileMenu.Text = "&File";
			// 
			// FileOpenMenu
			// 
			this.FileOpenMenu.Name = "FileOpenMenu";
			this.FileOpenMenu.Size = new System.Drawing.Size(123, 22);
			this.FileOpenMenu.Text = "&Open...";
			this.FileOpenMenu.Click += new System.EventHandler(this.FileOpenMenu_Click);
			// 
			// FileSep1Menu
			// 
			this.FileSep1Menu.Name = "FileSep1Menu";
			this.FileSep1Menu.Size = new System.Drawing.Size(120, 6);
			// 
			// FileExitMenu
			// 
			this.FileExitMenu.Name = "FileExitMenu";
			this.FileExitMenu.Size = new System.Drawing.Size(123, 22);
			this.FileExitMenu.Text = "E&xit";
			// 
			// HelpMenu
			// 
			this.HelpMenu.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.HelpGuideMenu,
            this.HelpDonateMenu,
            this.HelpSep1Menu,
            this.HelpAboutMenu});
			this.HelpMenu.Name = "HelpMenu";
			this.HelpMenu.Size = new System.Drawing.Size(40, 20);
			this.HelpMenu.Text = "&Help";
			// 
			// HelpGuideMenu
			// 
			this.HelpGuideMenu.Name = "HelpGuideMenu";
			this.HelpGuideMenu.Size = new System.Drawing.Size(157, 22);
			this.HelpGuideMenu.Text = "Usage &Guide...";
			// 
			// HelpDonateMenu
			// 
			this.HelpDonateMenu.Name = "HelpDonateMenu";
			this.HelpDonateMenu.Size = new System.Drawing.Size(157, 22);
			this.HelpDonateMenu.Text = "&Donate...";
			// 
			// HelpSep1Menu
			// 
			this.HelpSep1Menu.Name = "HelpSep1Menu";
			this.HelpSep1Menu.Size = new System.Drawing.Size(154, 6);
			// 
			// HelpAboutMenu
			// 
			this.HelpAboutMenu.Name = "HelpAboutMenu";
			this.HelpAboutMenu.Size = new System.Drawing.Size(157, 22);
			this.HelpAboutMenu.Text = "&About";
			// 
			// StatusBar
			// 
			this.StatusBar.Location = new System.Drawing.Point(0, 463);
			this.StatusBar.Name = "StatusBar";
			this.StatusBar.Size = new System.Drawing.Size(512, 22);
			this.StatusBar.TabIndex = 1;
			this.StatusBar.Text = "statusStrip1";
			// 
			// Toolbar
			// 
			this.Toolbar.Location = new System.Drawing.Point(0, 24);
			this.Toolbar.Name = "Toolbar";
			this.Toolbar.Size = new System.Drawing.Size(512, 25);
			this.Toolbar.TabIndex = 2;
			this.Toolbar.Text = "toolStrip1";
			// 
			// TableLayout
			// 
			this.TableLayout.AutoSize = true;
			this.TableLayout.ColumnCount = 3;
			this.TableLayout.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
			this.TableLayout.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
			this.TableLayout.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 128F));
			this.TableLayout.Controls.Add(this.AlbumArtPicture, 0, 0);
			this.TableLayout.Controls.Add(this.ArtistLabel, 1, 0);
			this.TableLayout.Controls.Add(this.AlbumLabel, 1, 1);
			this.TableLayout.Controls.Add(this.GenreLabel, 1, 2);
			this.TableLayout.Controls.Add(this.YearLabel, 1, 3);
			this.TableLayout.Controls.Add(this.DifficultyLayout, 2, 0);
			this.TableLayout.Dock = System.Windows.Forms.DockStyle.Bottom;
			this.TableLayout.Location = new System.Drawing.Point(0, 335);
			this.TableLayout.Name = "TableLayout";
			this.TableLayout.RowCount = 4;
			this.TableLayout.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.TableLayout.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.TableLayout.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.TableLayout.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.TableLayout.Size = new System.Drawing.Size(512, 128);
			this.TableLayout.TabIndex = 3;
			// 
			// AlbumArtPicture
			// 
			this.AlbumArtPicture.Location = new System.Drawing.Point(0, 0);
			this.AlbumArtPicture.Margin = new System.Windows.Forms.Padding(0);
			this.AlbumArtPicture.Name = "AlbumArtPicture";
			this.TableLayout.SetRowSpan(this.AlbumArtPicture, 4);
			this.AlbumArtPicture.Size = new System.Drawing.Size(128, 128);
			this.AlbumArtPicture.SizeMode = System.Windows.Forms.PictureBoxSizeMode.StretchImage;
			this.AlbumArtPicture.TabIndex = 0;
			this.AlbumArtPicture.TabStop = false;
			// 
			// ArtistLabel
			// 
			this.ArtistLabel.AutoSize = true;
			this.ArtistLabel.Dock = System.Windows.Forms.DockStyle.Top;
			this.ArtistLabel.Location = new System.Drawing.Point(131, 3);
			this.ArtistLabel.Margin = new System.Windows.Forms.Padding(3, 3, 3, 0);
			this.ArtistLabel.Name = "ArtistLabel";
			this.ArtistLabel.Padding = new System.Windows.Forms.Padding(0, 3, 0, 0);
			this.ArtistLabel.Size = new System.Drawing.Size(250, 16);
			this.ArtistLabel.TabIndex = 1;
			// 
			// AlbumLabel
			// 
			this.AlbumLabel.AutoSize = true;
			this.AlbumLabel.Dock = System.Windows.Forms.DockStyle.Top;
			this.AlbumLabel.Location = new System.Drawing.Point(131, 22);
			this.AlbumLabel.Margin = new System.Windows.Forms.Padding(3, 3, 3, 0);
			this.AlbumLabel.Name = "AlbumLabel";
			this.AlbumLabel.Padding = new System.Windows.Forms.Padding(0, 3, 0, 0);
			this.AlbumLabel.Size = new System.Drawing.Size(250, 16);
			this.AlbumLabel.TabIndex = 2;
			// 
			// GenreLabel
			// 
			this.GenreLabel.AutoSize = true;
			this.GenreLabel.Dock = System.Windows.Forms.DockStyle.Top;
			this.GenreLabel.Location = new System.Drawing.Point(131, 41);
			this.GenreLabel.Margin = new System.Windows.Forms.Padding(3, 3, 3, 0);
			this.GenreLabel.Name = "GenreLabel";
			this.GenreLabel.Padding = new System.Windows.Forms.Padding(0, 3, 0, 0);
			this.GenreLabel.Size = new System.Drawing.Size(250, 16);
			this.GenreLabel.TabIndex = 3;
			// 
			// YearLabel
			// 
			this.YearLabel.AutoSize = true;
			this.YearLabel.Dock = System.Windows.Forms.DockStyle.Top;
			this.YearLabel.Location = new System.Drawing.Point(131, 60);
			this.YearLabel.Margin = new System.Windows.Forms.Padding(3, 3, 3, 0);
			this.YearLabel.Name = "YearLabel";
			this.YearLabel.Padding = new System.Windows.Forms.Padding(0, 3, 0, 0);
			this.YearLabel.Size = new System.Drawing.Size(250, 16);
			this.YearLabel.TabIndex = 4;
			// 
			// DifficultyLayout
			// 
			this.DifficultyLayout.AutoSize = true;
			this.DifficultyLayout.ColumnCount = 2;
			this.DifficultyLayout.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50F));
			this.DifficultyLayout.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50F));
			this.DifficultyLayout.Controls.Add(this.DifficultyLabel, 0, 0);
			this.DifficultyLayout.Controls.Add(this.DifficultyBassValueLabel, 1, 5);
			this.DifficultyLayout.Controls.Add(this.DifficultyBassLabel, 0, 5);
			this.DifficultyLayout.Controls.Add(this.DifficultyVocalsValueLabel, 1, 4);
			this.DifficultyLayout.Controls.Add(this.DifficultyVocalsLabel, 0, 4);
			this.DifficultyLayout.Controls.Add(this.DifficultyDrumsValueLabel, 1, 3);
			this.DifficultyLayout.Controls.Add(this.DifficultyDrumsLabel, 0, 3);
			this.DifficultyLayout.Controls.Add(this.DifficultyGuitarValueLabel, 1, 2);
			this.DifficultyLayout.Controls.Add(this.DifficultyGuitarLabel, 0, 2);
			this.DifficultyLayout.Controls.Add(this.DifficultyBandValueLabel, 1, 1);
			this.DifficultyLayout.Controls.Add(this.DifficultyBandLabel, 0, 1);
			this.DifficultyLayout.Dock = System.Windows.Forms.DockStyle.Top;
			this.DifficultyLayout.Location = new System.Drawing.Point(387, 3);
			this.DifficultyLayout.Name = "DifficultyLayout";
			this.DifficultyLayout.RowCount = 6;
			this.TableLayout.SetRowSpan(this.DifficultyLayout, 4);
			this.DifficultyLayout.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.DifficultyLayout.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.DifficultyLayout.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.DifficultyLayout.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.DifficultyLayout.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.DifficultyLayout.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.DifficultyLayout.Size = new System.Drawing.Size(122, 104);
			this.DifficultyLayout.TabIndex = 6;
			// 
			// DifficultyLabel
			// 
			this.DifficultyLabel.AutoSize = true;
			this.DifficultyLayout.SetColumnSpan(this.DifficultyLabel, 2);
			this.DifficultyLabel.Dock = System.Windows.Forms.DockStyle.Top;
			this.DifficultyLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 14.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.DifficultyLabel.Location = new System.Drawing.Point(3, 0);
			this.DifficultyLabel.Name = "DifficultyLabel";
			this.DifficultyLabel.Size = new System.Drawing.Size(116, 24);
			this.DifficultyLabel.TabIndex = 6;
			this.DifficultyLabel.Text = "Difficulty";
			// 
			// DifficultyBassValueLabel
			// 
			this.DifficultyBassValueLabel.AutoSize = true;
			this.DifficultyBassValueLabel.Dock = System.Windows.Forms.DockStyle.Top;
			this.DifficultyBassValueLabel.Location = new System.Drawing.Point(64, 88);
			this.DifficultyBassValueLabel.Name = "DifficultyBassValueLabel";
			this.DifficultyBassValueLabel.Padding = new System.Windows.Forms.Padding(4, 3, 0, 0);
			this.DifficultyBassValueLabel.Size = new System.Drawing.Size(55, 16);
			this.DifficultyBassValueLabel.TabIndex = 14;
			// 
			// DifficultyBassLabel
			// 
			this.DifficultyBassLabel.AutoSize = true;
			this.DifficultyBassLabel.Dock = System.Windows.Forms.DockStyle.Top;
			this.DifficultyBassLabel.Location = new System.Drawing.Point(3, 88);
			this.DifficultyBassLabel.Name = "DifficultyBassLabel";
			this.DifficultyBassLabel.Padding = new System.Windows.Forms.Padding(0, 3, 6, 0);
			this.DifficultyBassLabel.Size = new System.Drawing.Size(55, 16);
			this.DifficultyBassLabel.TabIndex = 13;
			this.DifficultyBassLabel.Text = "Bass";
			this.DifficultyBassLabel.TextAlign = System.Drawing.ContentAlignment.TopRight;
			// 
			// DifficultyVocalsValueLabel
			// 
			this.DifficultyVocalsValueLabel.AutoSize = true;
			this.DifficultyVocalsValueLabel.Dock = System.Windows.Forms.DockStyle.Top;
			this.DifficultyVocalsValueLabel.Location = new System.Drawing.Point(64, 72);
			this.DifficultyVocalsValueLabel.Name = "DifficultyVocalsValueLabel";
			this.DifficultyVocalsValueLabel.Padding = new System.Windows.Forms.Padding(4, 3, 0, 0);
			this.DifficultyVocalsValueLabel.Size = new System.Drawing.Size(55, 16);
			this.DifficultyVocalsValueLabel.TabIndex = 12;
			// 
			// DifficultyVocalsLabel
			// 
			this.DifficultyVocalsLabel.AutoSize = true;
			this.DifficultyVocalsLabel.Dock = System.Windows.Forms.DockStyle.Top;
			this.DifficultyVocalsLabel.Location = new System.Drawing.Point(3, 72);
			this.DifficultyVocalsLabel.Name = "DifficultyVocalsLabel";
			this.DifficultyVocalsLabel.Padding = new System.Windows.Forms.Padding(0, 3, 6, 0);
			this.DifficultyVocalsLabel.Size = new System.Drawing.Size(55, 16);
			this.DifficultyVocalsLabel.TabIndex = 11;
			this.DifficultyVocalsLabel.Text = "Vocals";
			this.DifficultyVocalsLabel.TextAlign = System.Drawing.ContentAlignment.TopRight;
			// 
			// DifficultyDrumsValueLabel
			// 
			this.DifficultyDrumsValueLabel.AutoSize = true;
			this.DifficultyDrumsValueLabel.Dock = System.Windows.Forms.DockStyle.Top;
			this.DifficultyDrumsValueLabel.Location = new System.Drawing.Point(64, 56);
			this.DifficultyDrumsValueLabel.Name = "DifficultyDrumsValueLabel";
			this.DifficultyDrumsValueLabel.Padding = new System.Windows.Forms.Padding(4, 3, 0, 0);
			this.DifficultyDrumsValueLabel.Size = new System.Drawing.Size(55, 16);
			this.DifficultyDrumsValueLabel.TabIndex = 10;
			// 
			// DifficultyDrumsLabel
			// 
			this.DifficultyDrumsLabel.AutoSize = true;
			this.DifficultyDrumsLabel.Dock = System.Windows.Forms.DockStyle.Top;
			this.DifficultyDrumsLabel.Location = new System.Drawing.Point(3, 56);
			this.DifficultyDrumsLabel.Name = "DifficultyDrumsLabel";
			this.DifficultyDrumsLabel.Padding = new System.Windows.Forms.Padding(0, 3, 6, 0);
			this.DifficultyDrumsLabel.Size = new System.Drawing.Size(55, 16);
			this.DifficultyDrumsLabel.TabIndex = 9;
			this.DifficultyDrumsLabel.Text = "Drums";
			this.DifficultyDrumsLabel.TextAlign = System.Drawing.ContentAlignment.TopRight;
			// 
			// DifficultyGuitarValueLabel
			// 
			this.DifficultyGuitarValueLabel.AutoSize = true;
			this.DifficultyGuitarValueLabel.Dock = System.Windows.Forms.DockStyle.Top;
			this.DifficultyGuitarValueLabel.Location = new System.Drawing.Point(64, 40);
			this.DifficultyGuitarValueLabel.Name = "DifficultyGuitarValueLabel";
			this.DifficultyGuitarValueLabel.Padding = new System.Windows.Forms.Padding(4, 3, 0, 0);
			this.DifficultyGuitarValueLabel.Size = new System.Drawing.Size(55, 16);
			this.DifficultyGuitarValueLabel.TabIndex = 7;
			// 
			// DifficultyGuitarLabel
			// 
			this.DifficultyGuitarLabel.AutoSize = true;
			this.DifficultyGuitarLabel.Dock = System.Windows.Forms.DockStyle.Top;
			this.DifficultyGuitarLabel.Location = new System.Drawing.Point(3, 40);
			this.DifficultyGuitarLabel.Name = "DifficultyGuitarLabel";
			this.DifficultyGuitarLabel.Padding = new System.Windows.Forms.Padding(0, 3, 6, 0);
			this.DifficultyGuitarLabel.Size = new System.Drawing.Size(55, 16);
			this.DifficultyGuitarLabel.TabIndex = 8;
			this.DifficultyGuitarLabel.Text = "Guitar";
			this.DifficultyGuitarLabel.TextAlign = System.Drawing.ContentAlignment.TopRight;
			// 
			// DifficultyBandValueLabel
			// 
			this.DifficultyBandValueLabel.AutoSize = true;
			this.DifficultyBandValueLabel.Dock = System.Windows.Forms.DockStyle.Top;
			this.DifficultyBandValueLabel.Location = new System.Drawing.Point(64, 24);
			this.DifficultyBandValueLabel.Name = "DifficultyBandValueLabel";
			this.DifficultyBandValueLabel.Padding = new System.Windows.Forms.Padding(4, 3, 0, 0);
			this.DifficultyBandValueLabel.Size = new System.Drawing.Size(55, 16);
			this.DifficultyBandValueLabel.TabIndex = 16;
			// 
			// DifficultyBandLabel
			// 
			this.DifficultyBandLabel.AutoSize = true;
			this.DifficultyBandLabel.Dock = System.Windows.Forms.DockStyle.Top;
			this.DifficultyBandLabel.Location = new System.Drawing.Point(3, 24);
			this.DifficultyBandLabel.Name = "DifficultyBandLabel";
			this.DifficultyBandLabel.Padding = new System.Windows.Forms.Padding(0, 3, 6, 0);
			this.DifficultyBandLabel.Size = new System.Drawing.Size(55, 16);
			this.DifficultyBandLabel.TabIndex = 15;
			this.DifficultyBandLabel.Text = "Band";
			this.DifficultyBandLabel.TextAlign = System.Drawing.ContentAlignment.TopRight;
			// 
			// SongList
			// 
			this.SongList.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.columnHeader1,
            this.columnHeader2,
            this.columnHeader3,
            this.columnHeader4,
            this.columnHeader5,
            this.columnHeader6});
			this.SongList.Dock = System.Windows.Forms.DockStyle.Fill;
			this.SongList.Location = new System.Drawing.Point(0, 49);
			this.SongList.Name = "SongList";
			this.SongList.Size = new System.Drawing.Size(512, 286);
			this.SongList.TabIndex = 4;
			this.SongList.UseCompatibleStateImageBehavior = false;
			this.SongList.View = System.Windows.Forms.View.Details;
			this.SongList.SelectedIndexChanged += new System.EventHandler(this.SongList_SelectedIndexChanged);
			this.SongList.DoubleClick += new System.EventHandler(this.SongList_DoubleClick);
			// 
			// columnHeader1
			// 
			this.columnHeader1.Text = "Song";
			// 
			// columnHeader2
			// 
			this.columnHeader2.Text = "Artist";
			// 
			// columnHeader3
			// 
			this.columnHeader3.Text = "Album";
			// 
			// columnHeader4
			// 
			this.columnHeader4.Text = "Year";
			// 
			// columnHeader5
			// 
			this.columnHeader5.Text = "Genre";
			// 
			// columnHeader6
			// 
			this.columnHeader6.Text = "Availability";
			// 
			// MainForm
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.ClientSize = new System.Drawing.Size(512, 485);
			this.Controls.Add(this.SongList);
			this.Controls.Add(this.TableLayout);
			this.Controls.Add(this.Toolbar);
			this.Controls.Add(this.StatusBar);
			this.Controls.Add(this.MenuStrip);
			this.MainMenuStrip = this.MenuStrip;
			this.Name = "MainForm";
			this.Text = "RawkSD";
			this.MenuStrip.ResumeLayout(false);
			this.MenuStrip.PerformLayout();
			this.TableLayout.ResumeLayout(false);
			this.TableLayout.PerformLayout();
			((System.ComponentModel.ISupportInitialize)(this.AlbumArtPicture)).EndInit();
			this.DifficultyLayout.ResumeLayout(false);
			this.DifficultyLayout.PerformLayout();
			this.ResumeLayout(false);
			this.PerformLayout();

		}

		#endregion

		private System.Windows.Forms.MenuStrip MenuStrip;
		private System.Windows.Forms.StatusStrip StatusBar;
		private System.Windows.Forms.ToolStrip Toolbar;
		private System.Windows.Forms.ToolStripMenuItem FileMenu;
		private System.Windows.Forms.ToolStripMenuItem FileExitMenu;
		private System.Windows.Forms.ToolStripMenuItem HelpMenu;
		private System.Windows.Forms.ToolStripMenuItem FileOpenMenu;
		private System.Windows.Forms.ToolStripSeparator FileSep1Menu;
		private System.Windows.Forms.ToolStripMenuItem HelpGuideMenu;
		private System.Windows.Forms.ToolStripMenuItem HelpDonateMenu;
		private System.Windows.Forms.ToolStripSeparator HelpSep1Menu;
		private System.Windows.Forms.ToolStripMenuItem HelpAboutMenu;
		private System.Windows.Forms.FolderBrowserDialog FolderDialog;
		private System.Windows.Forms.OpenFileDialog OpenDialog;
		private System.Windows.Forms.TableLayoutPanel TableLayout;
		private System.Windows.Forms.PictureBox AlbumArtPicture;
		private System.Windows.Forms.Label ArtistLabel;
		private System.Windows.Forms.Label AlbumLabel;
		private System.Windows.Forms.Label GenreLabel;
		private System.Windows.Forms.Label YearLabel;
		private System.Windows.Forms.TableLayoutPanel DifficultyLayout;
		private System.Windows.Forms.Label DifficultyLabel;
		private System.Windows.Forms.Label DifficultyBassValueLabel;
		private System.Windows.Forms.Label DifficultyBassLabel;
		private System.Windows.Forms.Label DifficultyVocalsValueLabel;
		private System.Windows.Forms.Label DifficultyVocalsLabel;
		private System.Windows.Forms.Label DifficultyDrumsValueLabel;
		private System.Windows.Forms.Label DifficultyDrumsLabel;
		private System.Windows.Forms.Label DifficultyGuitarValueLabel;
		private System.Windows.Forms.Label DifficultyGuitarLabel;
		private System.Windows.Forms.Label DifficultyBandValueLabel;
		private System.Windows.Forms.Label DifficultyBandLabel;
		private System.Windows.Forms.ListView SongList;
		private System.Windows.Forms.ColumnHeader columnHeader1;
		private System.Windows.Forms.ColumnHeader columnHeader2;
		private System.Windows.Forms.ColumnHeader columnHeader3;
		private System.Windows.Forms.ColumnHeader columnHeader4;
		private System.Windows.Forms.ColumnHeader columnHeader5;
		private System.Windows.Forms.ColumnHeader columnHeader6;
	}
}

