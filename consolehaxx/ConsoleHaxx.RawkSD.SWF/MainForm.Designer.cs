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
			this.components = new System.ComponentModel.Container();
			System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(MainForm));
			this.MenuStrip = new System.Windows.Forms.MenuStrip();
			this.MenuFile = new System.Windows.Forms.ToolStripMenuItem();
			this.MenuFileOpenSD = new System.Windows.Forms.ToolStripMenuItem();
			this.MenuFileSep1 = new System.Windows.Forms.ToolStripSeparator();
			this.MenuFileOpen = new System.Windows.Forms.ToolStripMenuItem();
			this.MenuFileOpenFolder = new System.Windows.Forms.ToolStripMenuItem();
			this.MenuFileSep2 = new System.Windows.Forms.ToolStripSeparator();
			this.MenuFilePreferences = new System.Windows.Forms.ToolStripMenuItem();
			this.MenuFileSep3 = new System.Windows.Forms.ToolStripSeparator();
			this.MenuFileExit = new System.Windows.Forms.ToolStripMenuItem();
			this.MenuSongs = new System.Windows.Forms.ToolStripMenuItem();
			this.MenuSongsCreate = new System.Windows.Forms.ToolStripMenuItem();
			this.MenuSongsSep1 = new System.Windows.Forms.ToolStripSeparator();
			this.MenuSongsSelectAll = new System.Windows.Forms.ToolStripMenuItem();
			this.MenuSongsSep2 = new System.Windows.Forms.ToolStripSeparator();
			this.MenuSongsEdit = new System.Windows.Forms.ToolStripMenuItem();
			this.MenuSongsDelete = new System.Windows.Forms.ToolStripMenuItem();
			this.MenuSongsSep3 = new System.Windows.Forms.ToolStripSeparator();
			this.MenuSongsInstall = new System.Windows.Forms.ToolStripMenuItem();
			this.MenuSongsInstallLocal = new System.Windows.Forms.ToolStripMenuItem();
			this.MenuSongsInstallSD = new System.Windows.Forms.ToolStripMenuItem();
			this.MenuSongsSep4 = new System.Windows.Forms.ToolStripSeparator();
			this.MenuSongsExport = new System.Windows.Forms.ToolStripMenuItem();
			this.MenuSongsExportRawkSD = new System.Windows.Forms.ToolStripMenuItem();
			this.MenuSongsExportRBN = new System.Windows.Forms.ToolStripMenuItem();
			this.MenuSongsExport360DLC = new System.Windows.Forms.ToolStripMenuItem();
			this.MenuSongsExportFoF = new System.Windows.Forms.ToolStripMenuItem();
			this.MenuSongsSep5 = new System.Windows.Forms.ToolStripSeparator();
			this.MenuSongsTranscode = new System.Windows.Forms.ToolStripMenuItem();
			this.MenuSongsRefresh = new System.Windows.Forms.ToolStripMenuItem();
			this.MenuHelp = new System.Windows.Forms.ToolStripMenuItem();
			this.MenuHelpGuide = new System.Windows.Forms.ToolStripMenuItem();
			this.MenuHelpDonate = new System.Windows.Forms.ToolStripMenuItem();
			this.MenuHelpSep1 = new System.Windows.Forms.ToolStripSeparator();
			this.MenuHelpAbout = new System.Windows.Forms.ToolStripMenuItem();
			this.FolderDialog = new System.Windows.Forms.FolderBrowserDialog();
			this.OpenDialog = new System.Windows.Forms.OpenFileDialog();
			this.ToolDock = new System.Windows.Forms.ToolStripContainer();
			this.SongList = new ConsoleHaxx.RawkSD.SWF.MainForm.DoubleBufferedListView();
			this.SongListColumnSong = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
			this.SongListColumnArtist = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
			this.SongListColumnAlbum = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
			this.SongListColumnYear = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
			this.SongListColumnGenre = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
			this.SongListColumnAvailability = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
			this.SongContextMenu = new System.Windows.Forms.ContextMenuStrip(this.components);
			this.ContextMenuEdit = new System.Windows.Forms.ToolStripMenuItem();
			this.ContextMenuDelete = new System.Windows.Forms.ToolStripMenuItem();
			this.ContextMenuSep1 = new System.Windows.Forms.ToolStripSeparator();
			this.ContextMenuInstall = new System.Windows.Forms.ToolStripMenuItem();
			this.ContextMenuInstallLocal = new System.Windows.Forms.ToolStripMenuItem();
			this.ContextMenuInstallSD = new System.Windows.Forms.ToolStripMenuItem();
			this.ContextMenuSep2 = new System.Windows.Forms.ToolStripSeparator();
			this.ContextMenuExport = new System.Windows.Forms.ToolStripMenuItem();
			this.ContextMenuExportRawkSD = new System.Windows.Forms.ToolStripMenuItem();
			this.ContextMenuExportRBA = new System.Windows.Forms.ToolStripMenuItem();
			this.ContextMenuExport360DLC = new System.Windows.Forms.ToolStripMenuItem();
			this.ContextMenuExportFoF = new System.Windows.Forms.ToolStripMenuItem();
			this.ContextMenuSep3 = new System.Windows.Forms.ToolStripSeparator();
			this.ContextMenuTranscode = new System.Windows.Forms.ToolStripMenuItem();
			this.ContextMenuRefresh = new System.Windows.Forms.ToolStripMenuItem();
			this.SongListRowSizeHack = new System.Windows.Forms.ImageList(this.components);
			this.TableLayout = new System.Windows.Forms.TableLayoutPanel();
			this.NameLabel = new System.Windows.Forms.Label();
			this.AlbumArtPicture = new System.Windows.Forms.PictureBox();
			this.ArtistLabel = new System.Windows.Forms.Label();
			this.AlbumLabel = new System.Windows.Forms.Label();
			this.GenreLabel = new System.Windows.Forms.Label();
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
			this.ProgressLayout = new System.Windows.Forms.TableLayoutPanel();
			this.YearLabel = new System.Windows.Forms.Label();
			this.StatusBar = new System.Windows.Forms.StatusStrip();
			this.StatusSongsLabel = new System.Windows.Forms.ToolStripStatusLabel();
			this.StatusRiifsLabel = new System.Windows.Forms.ToolStripStatusLabel();
			this.StatusContactLabel = new System.Windows.Forms.ToolStripStatusLabel();
			this.StatusDonateLabel = new System.Windows.Forms.ToolStripStatusLabel();
			this.Toolbar = new System.Windows.Forms.ToolStrip();
			this.ToolbarOpenSD = new System.Windows.Forms.ToolStripButton();
			this.ToolbarOpen = new System.Windows.Forms.ToolStripButton();
			this.ToolbarOpenFolder = new System.Windows.Forms.ToolStripButton();
			this.ToolbarSep1 = new System.Windows.Forms.ToolStripSeparator();
			this.ToolbarNew = new System.Windows.Forms.ToolStripButton();
			this.ToolbarEdit = new System.Windows.Forms.ToolStripButton();
			this.ToolbarDelete = new System.Windows.Forms.ToolStripButton();
			this.ToolbarSep2 = new System.Windows.Forms.ToolStripSeparator();
			this.ToolbarInstallLocal = new System.Windows.Forms.ToolStripButton();
			this.ToolbarInstallSD = new System.Windows.Forms.ToolStripButton();
			this.SaveDialog = new System.Windows.Forms.SaveFileDialog();
			this.SongListTimer = new System.Windows.Forms.Timer(this.components);
			this.MenuStrip.SuspendLayout();
			this.ToolDock.ContentPanel.SuspendLayout();
			this.ToolDock.TopToolStripPanel.SuspendLayout();
			this.ToolDock.SuspendLayout();
			this.SongContextMenu.SuspendLayout();
			this.TableLayout.SuspendLayout();
			((System.ComponentModel.ISupportInitialize)(this.AlbumArtPicture)).BeginInit();
			this.DifficultyLayout.SuspendLayout();
			this.StatusBar.SuspendLayout();
			this.Toolbar.SuspendLayout();
			this.SuspendLayout();
			// 
			// MenuStrip
			// 
			this.MenuStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.MenuFile,
            this.MenuSongs,
            this.MenuHelp});
			this.MenuStrip.Location = new System.Drawing.Point(0, 0);
			this.MenuStrip.Name = "MenuStrip";
			this.MenuStrip.Size = new System.Drawing.Size(648, 24);
			this.MenuStrip.TabIndex = 0;
			this.MenuStrip.Text = "menuStrip1";
			// 
			// MenuFile
			// 
			this.MenuFile.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.MenuFileOpenSD,
            this.MenuFileSep1,
            this.MenuFileOpen,
            this.MenuFileOpenFolder,
            this.MenuFileSep2,
            this.MenuFilePreferences,
            this.MenuFileSep3,
            this.MenuFileExit});
			this.MenuFile.Name = "MenuFile";
			this.MenuFile.Size = new System.Drawing.Size(35, 20);
			this.MenuFile.Text = "&File";
			// 
			// MenuFileOpenSD
			// 
			this.MenuFileOpenSD.Image = global::ConsoleHaxx.RawkSD.SWF.Properties.Resources.IconRSD;
			this.MenuFileOpenSD.Name = "MenuFileOpenSD";
			this.MenuFileOpenSD.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.B)));
			this.MenuFileOpenSD.Size = new System.Drawing.Size(203, 22);
			this.MenuFileOpenSD.Text = "Open &SD Card...";
			this.MenuFileOpenSD.Click += new System.EventHandler(this.MenuFileOpenSD_Click);
			// 
			// MenuFileSep1
			// 
			this.MenuFileSep1.Name = "MenuFileSep1";
			this.MenuFileSep1.Size = new System.Drawing.Size(200, 6);
			// 
			// MenuFileOpen
			// 
			this.MenuFileOpen.Image = global::ConsoleHaxx.RawkSD.SWF.Properties.Resources.IconOpenFile;
			this.MenuFileOpen.Name = "MenuFileOpen";
			this.MenuFileOpen.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.O)));
			this.MenuFileOpen.Size = new System.Drawing.Size(203, 22);
			this.MenuFileOpen.Text = "&Open...";
			this.MenuFileOpen.Click += new System.EventHandler(this.MenuFileOpen_Click);
			// 
			// MenuFileOpenFolder
			// 
			this.MenuFileOpenFolder.Image = global::ConsoleHaxx.RawkSD.SWF.Properties.Resources.IconOpenFolder;
			this.MenuFileOpenFolder.Name = "MenuFileOpenFolder";
			this.MenuFileOpenFolder.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.F)));
			this.MenuFileOpenFolder.Size = new System.Drawing.Size(203, 22);
			this.MenuFileOpenFolder.Text = "Open &Folder...";
			this.MenuFileOpenFolder.Click += new System.EventHandler(this.MenuFileOpenFolder_Click);
			// 
			// MenuFileSep2
			// 
			this.MenuFileSep2.Name = "MenuFileSep2";
			this.MenuFileSep2.Size = new System.Drawing.Size(200, 6);
			// 
			// MenuFilePreferences
			// 
			this.MenuFilePreferences.Image = global::ConsoleHaxx.RawkSD.SWF.Properties.Resources.IconProperties;
			this.MenuFilePreferences.Name = "MenuFilePreferences";
			this.MenuFilePreferences.Size = new System.Drawing.Size(203, 22);
			this.MenuFilePreferences.Text = "&Preferences...";
			this.MenuFilePreferences.Click += new System.EventHandler(this.MenuFilePreferences_Click);
			// 
			// MenuFileSep3
			// 
			this.MenuFileSep3.Name = "MenuFileSep3";
			this.MenuFileSep3.Size = new System.Drawing.Size(200, 6);
			// 
			// MenuFileExit
			// 
			this.MenuFileExit.Name = "MenuFileExit";
			this.MenuFileExit.Size = new System.Drawing.Size(203, 22);
			this.MenuFileExit.Text = "E&xit";
			this.MenuFileExit.Click += new System.EventHandler(this.MenuFileExit_Click);
			// 
			// MenuSongs
			// 
			this.MenuSongs.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.MenuSongsCreate,
            this.MenuSongsSep1,
            this.MenuSongsSelectAll,
            this.MenuSongsSep2,
            this.MenuSongsEdit,
            this.MenuSongsDelete,
            this.MenuSongsSep3,
            this.MenuSongsInstall,
            this.MenuSongsSep4,
            this.MenuSongsExport,
            this.MenuSongsSep5,
            this.MenuSongsTranscode,
            this.MenuSongsRefresh});
			this.MenuSongs.Name = "MenuSongs";
			this.MenuSongs.Size = new System.Drawing.Size(48, 20);
			this.MenuSongs.Text = "&Songs";
			// 
			// MenuSongsCreate
			// 
			this.MenuSongsCreate.Image = global::ConsoleHaxx.RawkSD.SWF.Properties.Resources.IconNew;
			this.MenuSongsCreate.Name = "MenuSongsCreate";
			this.MenuSongsCreate.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.N)));
			this.MenuSongsCreate.Size = new System.Drawing.Size(205, 22);
			this.MenuSongsCreate.Text = "&Create...";
			this.MenuSongsCreate.Click += new System.EventHandler(this.MenuSongsCreate_Click);
			// 
			// MenuSongsSep1
			// 
			this.MenuSongsSep1.Name = "MenuSongsSep1";
			this.MenuSongsSep1.Size = new System.Drawing.Size(202, 6);
			// 
			// MenuSongsSelectAll
			// 
			this.MenuSongsSelectAll.Name = "MenuSongsSelectAll";
			this.MenuSongsSelectAll.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.A)));
			this.MenuSongsSelectAll.Size = new System.Drawing.Size(205, 22);
			this.MenuSongsSelectAll.Text = "Select &All";
			this.MenuSongsSelectAll.Click += new System.EventHandler(this.MenuSongsSelectAll_Click);
			// 
			// MenuSongsSep2
			// 
			this.MenuSongsSep2.Name = "MenuSongsSep2";
			this.MenuSongsSep2.Size = new System.Drawing.Size(202, 6);
			// 
			// MenuSongsEdit
			// 
			this.MenuSongsEdit.Image = global::ConsoleHaxx.RawkSD.SWF.Properties.Resources.IconEdit;
			this.MenuSongsEdit.Name = "MenuSongsEdit";
			this.MenuSongsEdit.Size = new System.Drawing.Size(205, 22);
			this.MenuSongsEdit.Text = "&Edit...";
			this.MenuSongsEdit.Click += new System.EventHandler(this.MenuSongsEdit_Click);
			// 
			// MenuSongsDelete
			// 
			this.MenuSongsDelete.Image = global::ConsoleHaxx.RawkSD.SWF.Properties.Resources.IconDelete;
			this.MenuSongsDelete.Name = "MenuSongsDelete";
			this.MenuSongsDelete.ShortcutKeys = System.Windows.Forms.Keys.Delete;
			this.MenuSongsDelete.Size = new System.Drawing.Size(205, 22);
			this.MenuSongsDelete.Text = "&Delete";
			this.MenuSongsDelete.Click += new System.EventHandler(this.MenuSongsDelete_Click);
			// 
			// MenuSongsSep3
			// 
			this.MenuSongsSep3.Name = "MenuSongsSep3";
			this.MenuSongsSep3.Size = new System.Drawing.Size(202, 6);
			// 
			// MenuSongsInstall
			// 
			this.MenuSongsInstall.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.MenuSongsInstallLocal,
            this.MenuSongsInstallSD});
			this.MenuSongsInstall.Name = "MenuSongsInstall";
			this.MenuSongsInstall.Size = new System.Drawing.Size(205, 22);
			this.MenuSongsInstall.Text = "&Install";
			// 
			// MenuSongsInstallLocal
			// 
			this.MenuSongsInstallLocal.Name = "MenuSongsInstallLocal";
			this.MenuSongsInstallLocal.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.L)));
			this.MenuSongsInstallLocal.Size = new System.Drawing.Size(179, 22);
			this.MenuSongsInstallLocal.Text = "&Local Cache";
			this.MenuSongsInstallLocal.Click += new System.EventHandler(this.MenuSongsInstallLocal_Click);
			// 
			// MenuSongsInstallSD
			// 
			this.MenuSongsInstallSD.Name = "MenuSongsInstallSD";
			this.MenuSongsInstallSD.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.S)));
			this.MenuSongsInstallSD.Size = new System.Drawing.Size(179, 22);
			this.MenuSongsInstallSD.Text = "&SD Card";
			this.MenuSongsInstallSD.Click += new System.EventHandler(this.MenuSongsInstallSD_Click);
			// 
			// MenuSongsSep4
			// 
			this.MenuSongsSep4.Name = "MenuSongsSep4";
			this.MenuSongsSep4.Size = new System.Drawing.Size(202, 6);
			// 
			// MenuSongsExport
			// 
			this.MenuSongsExport.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.MenuSongsExportRawkSD,
            this.MenuSongsExportRBN,
            this.MenuSongsExport360DLC,
            this.MenuSongsExportFoF});
			this.MenuSongsExport.Name = "MenuSongsExport";
			this.MenuSongsExport.Size = new System.Drawing.Size(205, 22);
			this.MenuSongsExport.Text = "E&xport";
			// 
			// MenuSongsExportRawkSD
			// 
			this.MenuSongsExportRawkSD.Name = "MenuSongsExportRawkSD";
			this.MenuSongsExportRawkSD.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.S)));
			this.MenuSongsExportRawkSD.Size = new System.Drawing.Size(233, 22);
			this.MenuSongsExportRawkSD.Text = "&RawkSD Archive (.rwk)";
			this.MenuSongsExportRawkSD.Click += new System.EventHandler(this.MenuSongsExportRawkSD_Click);
			// 
			// MenuSongsExportRBN
			// 
			this.MenuSongsExportRBN.Name = "MenuSongsExportRBN";
			this.MenuSongsExportRBN.Size = new System.Drawing.Size(233, 22);
			this.MenuSongsExportRBN.Text = "Rock Band &Network (.rba)";
			this.MenuSongsExportRBN.Click += new System.EventHandler(this.MenuSongsExportRBN_Click);
			// 
			// MenuSongsExport360DLC
			// 
			this.MenuSongsExport360DLC.Name = "MenuSongsExport360DLC";
			this.MenuSongsExport360DLC.Size = new System.Drawing.Size(233, 22);
			this.MenuSongsExport360DLC.Text = "Rock Band &360 DLC";
			this.MenuSongsExport360DLC.Click += new System.EventHandler(this.MenuSongsExport360DLC_Click);
			// 
			// MenuSongsExportFoF
			// 
			this.MenuSongsExportFoF.Name = "MenuSongsExportFoF";
			this.MenuSongsExportFoF.Size = new System.Drawing.Size(233, 22);
			this.MenuSongsExportFoF.Text = "&Frets on Fire Folder";
			this.MenuSongsExportFoF.Click += new System.EventHandler(this.MenuSongsExportFoF_Click);
			// 
			// MenuSongsSep5
			// 
			this.MenuSongsSep5.Name = "MenuSongsSep5";
			this.MenuSongsSep5.Size = new System.Drawing.Size(202, 6);
			// 
			// MenuSongsTranscode
			// 
			this.MenuSongsTranscode.Name = "MenuSongsTranscode";
			this.MenuSongsTranscode.Size = new System.Drawing.Size(205, 22);
			this.MenuSongsTranscode.Text = "Transcode for Rock Band";
			this.MenuSongsTranscode.Click += new System.EventHandler(this.MenuSongsTranscode_Click);
			// 
			// MenuSongsRefresh
			// 
			this.MenuSongsRefresh.Image = global::ConsoleHaxx.RawkSD.SWF.Properties.Resources.IconRefresh;
			this.MenuSongsRefresh.Name = "MenuSongsRefresh";
			this.MenuSongsRefresh.Size = new System.Drawing.Size(205, 22);
			this.MenuSongsRefresh.Text = "Refresh Data";
			this.MenuSongsRefresh.Click += new System.EventHandler(this.MenuSongsRefresh_Click);
			// 
			// MenuHelp
			// 
			this.MenuHelp.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.MenuHelpGuide,
            this.MenuHelpDonate,
            this.MenuHelpSep1,
            this.MenuHelpAbout});
			this.MenuHelp.Name = "MenuHelp";
			this.MenuHelp.Size = new System.Drawing.Size(40, 20);
			this.MenuHelp.Text = "&Help";
			// 
			// MenuHelpGuide
			// 
			this.MenuHelpGuide.Name = "MenuHelpGuide";
			this.MenuHelpGuide.ShortcutKeys = System.Windows.Forms.Keys.F1;
			this.MenuHelpGuide.Size = new System.Drawing.Size(176, 22);
			this.MenuHelpGuide.Text = "Usage &Guide...";
			this.MenuHelpGuide.Click += new System.EventHandler(this.MenuHelpGuide_Click);
			// 
			// MenuHelpDonate
			// 
			this.MenuHelpDonate.Name = "MenuHelpDonate";
			this.MenuHelpDonate.Size = new System.Drawing.Size(176, 22);
			this.MenuHelpDonate.Text = "&Donate...";
			this.MenuHelpDonate.Click += new System.EventHandler(this.MenuHelpDonate_Click);
			// 
			// MenuHelpSep1
			// 
			this.MenuHelpSep1.Name = "MenuHelpSep1";
			this.MenuHelpSep1.Size = new System.Drawing.Size(173, 6);
			// 
			// MenuHelpAbout
			// 
			this.MenuHelpAbout.Name = "MenuHelpAbout";
			this.MenuHelpAbout.Size = new System.Drawing.Size(176, 22);
			this.MenuHelpAbout.Text = "&About";
			this.MenuHelpAbout.Click += new System.EventHandler(this.MenuHelpAbout_Click);
			// 
			// ToolDock
			// 
			// 
			// ToolDock.ContentPanel
			// 
			this.ToolDock.ContentPanel.Controls.Add(this.SongList);
			this.ToolDock.ContentPanel.Controls.Add(this.TableLayout);
			this.ToolDock.ContentPanel.Size = new System.Drawing.Size(648, 436);
			this.ToolDock.Dock = System.Windows.Forms.DockStyle.Fill;
			this.ToolDock.Location = new System.Drawing.Point(0, 24);
			this.ToolDock.Name = "ToolDock";
			this.ToolDock.Size = new System.Drawing.Size(648, 461);
			this.ToolDock.TabIndex = 5;
			this.ToolDock.Text = "toolStripContainer1";
			// 
			// ToolDock.TopToolStripPanel
			// 
			this.ToolDock.TopToolStripPanel.Controls.Add(this.Toolbar);
			// 
			// SongList
			// 
			this.SongList.AllowColumnReorder = true;
			this.SongList.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.SongListColumnSong,
            this.SongListColumnArtist,
            this.SongListColumnAlbum,
            this.SongListColumnYear,
            this.SongListColumnGenre,
            this.SongListColumnAvailability});
			this.SongList.ContextMenuStrip = this.SongContextMenu;
			this.SongList.Dock = System.Windows.Forms.DockStyle.Fill;
			this.SongList.FullRowSelect = true;
			this.SongList.Location = new System.Drawing.Point(0, 0);
			this.SongList.Name = "SongList";
			this.SongList.OwnerDraw = true;
			this.SongList.ShowItemToolTips = true;
			this.SongList.Size = new System.Drawing.Size(648, 288);
			this.SongList.SmallImageList = this.SongListRowSizeHack;
			this.SongList.TabIndex = 5;
			this.SongList.UseCompatibleStateImageBehavior = false;
			this.SongList.View = System.Windows.Forms.View.Details;
			this.SongList.ColumnClick += new System.Windows.Forms.ColumnClickEventHandler(this.SongList_ColumnClick);
			this.SongList.DrawColumnHeader += new System.Windows.Forms.DrawListViewColumnHeaderEventHandler(this.SongList_DrawColumnHeader);
			this.SongList.DrawItem += new System.Windows.Forms.DrawListViewItemEventHandler(this.SongList_DrawItem);
			this.SongList.DrawSubItem += new System.Windows.Forms.DrawListViewSubItemEventHandler(this.SongList_DrawSubItem);
			this.SongList.SelectedIndexChanged += new System.EventHandler(this.SongList_SelectedIndexChanged);
			this.SongList.DoubleClick += new System.EventHandler(this.SongList_DoubleClick);
			// 
			// SongListColumnSong
			// 
			this.SongListColumnSong.Text = "Song";
			// 
			// SongListColumnArtist
			// 
			this.SongListColumnArtist.Text = "Artist";
			// 
			// SongListColumnAlbum
			// 
			this.SongListColumnAlbum.Text = "Album";
			// 
			// SongListColumnYear
			// 
			this.SongListColumnYear.Text = "Year";
			// 
			// SongListColumnGenre
			// 
			this.SongListColumnGenre.Text = "Genre";
			// 
			// SongListColumnAvailability
			// 
			this.SongListColumnAvailability.Text = "Availability";
			this.SongListColumnAvailability.Width = 71;
			// 
			// SongContextMenu
			// 
			this.SongContextMenu.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.ContextMenuEdit,
            this.ContextMenuDelete,
            this.ContextMenuSep1,
            this.ContextMenuInstall,
            this.ContextMenuSep2,
            this.ContextMenuExport,
            this.ContextMenuSep3,
            this.ContextMenuTranscode,
            this.ContextMenuRefresh});
			this.SongContextMenu.Name = "ContextMenuStrip";
			this.SongContextMenu.Size = new System.Drawing.Size(206, 176);
			// 
			// ContextMenuEdit
			// 
			this.ContextMenuEdit.Image = global::ConsoleHaxx.RawkSD.SWF.Properties.Resources.IconEdit;
			this.ContextMenuEdit.Name = "ContextMenuEdit";
			this.ContextMenuEdit.Size = new System.Drawing.Size(205, 22);
			this.ContextMenuEdit.Text = "&Edit...";
			this.ContextMenuEdit.Click += new System.EventHandler(this.ContextMenuEdit_Click);
			// 
			// ContextMenuDelete
			// 
			this.ContextMenuDelete.Image = global::ConsoleHaxx.RawkSD.SWF.Properties.Resources.IconDelete;
			this.ContextMenuDelete.Name = "ContextMenuDelete";
			this.ContextMenuDelete.Size = new System.Drawing.Size(205, 22);
			this.ContextMenuDelete.Text = "&Delete";
			this.ContextMenuDelete.Click += new System.EventHandler(this.ContextMenuDelete_Click);
			// 
			// ContextMenuSep1
			// 
			this.ContextMenuSep1.Name = "ContextMenuSep1";
			this.ContextMenuSep1.Size = new System.Drawing.Size(202, 6);
			// 
			// ContextMenuInstall
			// 
			this.ContextMenuInstall.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.ContextMenuInstallLocal,
            this.ContextMenuInstallSD});
			this.ContextMenuInstall.Name = "ContextMenuInstall";
			this.ContextMenuInstall.Size = new System.Drawing.Size(205, 22);
			this.ContextMenuInstall.Text = "&Install";
			// 
			// ContextMenuInstallLocal
			// 
			this.ContextMenuInstallLocal.Name = "ContextMenuInstallLocal";
			this.ContextMenuInstallLocal.Size = new System.Drawing.Size(142, 22);
			this.ContextMenuInstallLocal.Text = "&Local Cache";
			this.ContextMenuInstallLocal.Click += new System.EventHandler(this.ContextMenuInstallLocal_Click);
			// 
			// ContextMenuInstallSD
			// 
			this.ContextMenuInstallSD.Name = "ContextMenuInstallSD";
			this.ContextMenuInstallSD.Size = new System.Drawing.Size(142, 22);
			this.ContextMenuInstallSD.Text = "&SD Card";
			this.ContextMenuInstallSD.Click += new System.EventHandler(this.ContextMenuInstallSD_Click);
			// 
			// ContextMenuSep2
			// 
			this.ContextMenuSep2.Name = "ContextMenuSep2";
			this.ContextMenuSep2.Size = new System.Drawing.Size(202, 6);
			// 
			// ContextMenuExport
			// 
			this.ContextMenuExport.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.ContextMenuExportRawkSD,
            this.ContextMenuExportRBA,
            this.ContextMenuExport360DLC,
            this.ContextMenuExportFoF});
			this.ContextMenuExport.Name = "ContextMenuExport";
			this.ContextMenuExport.Size = new System.Drawing.Size(205, 22);
			this.ContextMenuExport.Text = "E&xport";
			// 
			// ContextMenuExportRawkSD
			// 
			this.ContextMenuExportRawkSD.Name = "ContextMenuExportRawkSD";
			this.ContextMenuExportRawkSD.Size = new System.Drawing.Size(209, 22);
			this.ContextMenuExportRawkSD.Text = "&RawkSD Archive (.rwk)";
			this.ContextMenuExportRawkSD.Click += new System.EventHandler(this.ContextMenuExportRawkSD_Click);
			// 
			// ContextMenuExportRBA
			// 
			this.ContextMenuExportRBA.Name = "ContextMenuExportRBA";
			this.ContextMenuExportRBA.Size = new System.Drawing.Size(209, 22);
			this.ContextMenuExportRBA.Text = "Rock Band &Network (.rba)";
			this.ContextMenuExportRBA.Click += new System.EventHandler(this.ContextMenuExportRBA_Click);
			// 
			// ContextMenuExport360DLC
			// 
			this.ContextMenuExport360DLC.Name = "ContextMenuExport360DLC";
			this.ContextMenuExport360DLC.Size = new System.Drawing.Size(209, 22);
			this.ContextMenuExport360DLC.Text = "Rock Band &360 DLC";
			this.ContextMenuExport360DLC.Click += new System.EventHandler(this.ContextMenuExport360DLC_Click);
			// 
			// ContextMenuExportFoF
			// 
			this.ContextMenuExportFoF.Name = "ContextMenuExportFoF";
			this.ContextMenuExportFoF.Size = new System.Drawing.Size(209, 22);
			this.ContextMenuExportFoF.Text = "&Frets on Fire Folder";
			this.ContextMenuExportFoF.Click += new System.EventHandler(this.ContextMenuExportFoF_Click);
			// 
			// ContextMenuSep3
			// 
			this.ContextMenuSep3.Name = "ContextMenuSep3";
			this.ContextMenuSep3.Size = new System.Drawing.Size(202, 6);
			// 
			// ContextMenuTranscode
			// 
			this.ContextMenuTranscode.Name = "ContextMenuTranscode";
			this.ContextMenuTranscode.Size = new System.Drawing.Size(205, 22);
			this.ContextMenuTranscode.Text = "Transcode for Rock Band";
			this.ContextMenuTranscode.Click += new System.EventHandler(this.ContextMenuTranscode_Click);
			// 
			// ContextMenuRefresh
			// 
			this.ContextMenuRefresh.Image = global::ConsoleHaxx.RawkSD.SWF.Properties.Resources.IconRefresh;
			this.ContextMenuRefresh.Name = "ContextMenuRefresh";
			this.ContextMenuRefresh.Size = new System.Drawing.Size(205, 22);
			this.ContextMenuRefresh.Text = "Refresh Data";
			this.ContextMenuRefresh.Click += new System.EventHandler(this.ContextMenuRefresh_Click);
			// 
			// SongListRowSizeHack
			// 
			this.SongListRowSizeHack.ColorDepth = System.Windows.Forms.ColorDepth.Depth32Bit;
			this.SongListRowSizeHack.ImageSize = new System.Drawing.Size(1, 16);
			this.SongListRowSizeHack.TransparentColor = System.Drawing.Color.Transparent;
			// 
			// TableLayout
			// 
			this.TableLayout.AutoSize = true;
			this.TableLayout.ColumnCount = 4;
			this.TableLayout.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
			this.TableLayout.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
			this.TableLayout.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
			this.TableLayout.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
			this.TableLayout.Controls.Add(this.NameLabel, 1, 0);
			this.TableLayout.Controls.Add(this.AlbumArtPicture, 0, 0);
			this.TableLayout.Controls.Add(this.ArtistLabel, 1, 1);
			this.TableLayout.Controls.Add(this.AlbumLabel, 1, 2);
			this.TableLayout.Controls.Add(this.GenreLabel, 1, 3);
			this.TableLayout.Controls.Add(this.DifficultyLayout, 2, 0);
			this.TableLayout.Controls.Add(this.ProgressLayout, 3, 0);
			this.TableLayout.Controls.Add(this.YearLabel, 1, 4);
			this.TableLayout.Controls.Add(this.StatusBar, 0, 6);
			this.TableLayout.Dock = System.Windows.Forms.DockStyle.Bottom;
			this.TableLayout.Location = new System.Drawing.Point(0, 288);
			this.TableLayout.Name = "TableLayout";
			this.TableLayout.RowCount = 6;
			this.TableLayout.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.TableLayout.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.TableLayout.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.TableLayout.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.TableLayout.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.TableLayout.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.TableLayout.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 20F));
			this.TableLayout.Size = new System.Drawing.Size(648, 148);
			this.TableLayout.TabIndex = 6;
			// 
			// NameLabel
			// 
			this.NameLabel.AutoSize = true;
			this.NameLabel.Dock = System.Windows.Forms.DockStyle.Top;
			this.NameLabel.Location = new System.Drawing.Point(131, 3);
			this.NameLabel.Margin = new System.Windows.Forms.Padding(3, 3, 3, 0);
			this.NameLabel.MinimumSize = new System.Drawing.Size(64, 0);
			this.NameLabel.Name = "NameLabel";
			this.NameLabel.Padding = new System.Windows.Forms.Padding(0, 3, 0, 0);
			this.NameLabel.Size = new System.Drawing.Size(381, 16);
			this.NameLabel.TabIndex = 7;
			this.NameLabel.UseMnemonic = false;
			// 
			// AlbumArtPicture
			// 
			this.AlbumArtPicture.Location = new System.Drawing.Point(0, 0);
			this.AlbumArtPicture.Margin = new System.Windows.Forms.Padding(0);
			this.AlbumArtPicture.Name = "AlbumArtPicture";
			this.TableLayout.SetRowSpan(this.AlbumArtPicture, 5);
			this.AlbumArtPicture.Size = new System.Drawing.Size(128, 128);
			this.AlbumArtPicture.SizeMode = System.Windows.Forms.PictureBoxSizeMode.StretchImage;
			this.AlbumArtPicture.TabIndex = 0;
			this.AlbumArtPicture.TabStop = false;
			// 
			// ArtistLabel
			// 
			this.ArtistLabel.AutoSize = true;
			this.ArtistLabel.Dock = System.Windows.Forms.DockStyle.Top;
			this.ArtistLabel.Location = new System.Drawing.Point(131, 22);
			this.ArtistLabel.Margin = new System.Windows.Forms.Padding(3, 3, 3, 0);
			this.ArtistLabel.MinimumSize = new System.Drawing.Size(64, 0);
			this.ArtistLabel.Name = "ArtistLabel";
			this.ArtistLabel.Padding = new System.Windows.Forms.Padding(0, 3, 0, 0);
			this.ArtistLabel.Size = new System.Drawing.Size(381, 16);
			this.ArtistLabel.TabIndex = 1;
			this.ArtistLabel.UseMnemonic = false;
			// 
			// AlbumLabel
			// 
			this.AlbumLabel.AutoSize = true;
			this.AlbumLabel.Dock = System.Windows.Forms.DockStyle.Top;
			this.AlbumLabel.Location = new System.Drawing.Point(131, 41);
			this.AlbumLabel.Margin = new System.Windows.Forms.Padding(3, 3, 3, 0);
			this.AlbumLabel.MinimumSize = new System.Drawing.Size(64, 0);
			this.AlbumLabel.Name = "AlbumLabel";
			this.AlbumLabel.Padding = new System.Windows.Forms.Padding(0, 3, 0, 0);
			this.AlbumLabel.Size = new System.Drawing.Size(381, 16);
			this.AlbumLabel.TabIndex = 2;
			this.AlbumLabel.UseMnemonic = false;
			// 
			// GenreLabel
			// 
			this.GenreLabel.AutoSize = true;
			this.GenreLabel.Dock = System.Windows.Forms.DockStyle.Top;
			this.GenreLabel.Location = new System.Drawing.Point(131, 60);
			this.GenreLabel.Margin = new System.Windows.Forms.Padding(3, 3, 3, 0);
			this.GenreLabel.MinimumSize = new System.Drawing.Size(64, 0);
			this.GenreLabel.Name = "GenreLabel";
			this.GenreLabel.Padding = new System.Windows.Forms.Padding(0, 3, 0, 0);
			this.GenreLabel.Size = new System.Drawing.Size(381, 16);
			this.GenreLabel.TabIndex = 3;
			this.GenreLabel.UseMnemonic = false;
			// 
			// DifficultyLayout
			// 
			this.DifficultyLayout.AutoSize = true;
			this.DifficultyLayout.ColumnCount = 2;
			this.DifficultyLayout.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
			this.DifficultyLayout.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
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
			this.DifficultyLayout.Location = new System.Drawing.Point(518, 3);
			this.DifficultyLayout.Name = "DifficultyLayout";
			this.DifficultyLayout.RowCount = 6;
			this.TableLayout.SetRowSpan(this.DifficultyLayout, 6);
			this.DifficultyLayout.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.DifficultyLayout.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.DifficultyLayout.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.DifficultyLayout.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.DifficultyLayout.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.DifficultyLayout.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.DifficultyLayout.Size = new System.Drawing.Size(127, 104);
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
			this.DifficultyLabel.Size = new System.Drawing.Size(121, 24);
			this.DifficultyLabel.TabIndex = 6;
			this.DifficultyLabel.Text = "Difficulty";
			// 
			// DifficultyBassValueLabel
			// 
			this.DifficultyBassValueLabel.AutoSize = true;
			this.DifficultyBassValueLabel.Dock = System.Windows.Forms.DockStyle.Top;
			this.DifficultyBassValueLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.DifficultyBassValueLabel.ForeColor = System.Drawing.Color.Red;
			this.DifficultyBassValueLabel.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
			this.DifficultyBassValueLabel.Location = new System.Drawing.Point(54, 88);
			this.DifficultyBassValueLabel.MinimumSize = new System.Drawing.Size(70, 0);
			this.DifficultyBassValueLabel.Name = "DifficultyBassValueLabel";
			this.DifficultyBassValueLabel.Padding = new System.Windows.Forms.Padding(4, 3, 0, 0);
			this.DifficultyBassValueLabel.Size = new System.Drawing.Size(70, 16);
			this.DifficultyBassValueLabel.TabIndex = 14;
			// 
			// DifficultyBassLabel
			// 
			this.DifficultyBassLabel.AutoSize = true;
			this.DifficultyBassLabel.Dock = System.Windows.Forms.DockStyle.Top;
			this.DifficultyBassLabel.Location = new System.Drawing.Point(3, 88);
			this.DifficultyBassLabel.Name = "DifficultyBassLabel";
			this.DifficultyBassLabel.Padding = new System.Windows.Forms.Padding(0, 3, 6, 0);
			this.DifficultyBassLabel.Size = new System.Drawing.Size(45, 16);
			this.DifficultyBassLabel.TabIndex = 13;
			this.DifficultyBassLabel.Text = "Bass";
			this.DifficultyBassLabel.TextAlign = System.Drawing.ContentAlignment.TopRight;
			// 
			// DifficultyVocalsValueLabel
			// 
			this.DifficultyVocalsValueLabel.AutoSize = true;
			this.DifficultyVocalsValueLabel.Dock = System.Windows.Forms.DockStyle.Top;
			this.DifficultyVocalsValueLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.DifficultyVocalsValueLabel.ForeColor = System.Drawing.Color.Red;
			this.DifficultyVocalsValueLabel.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
			this.DifficultyVocalsValueLabel.Location = new System.Drawing.Point(54, 72);
			this.DifficultyVocalsValueLabel.MinimumSize = new System.Drawing.Size(70, 0);
			this.DifficultyVocalsValueLabel.Name = "DifficultyVocalsValueLabel";
			this.DifficultyVocalsValueLabel.Padding = new System.Windows.Forms.Padding(4, 3, 0, 0);
			this.DifficultyVocalsValueLabel.Size = new System.Drawing.Size(70, 16);
			this.DifficultyVocalsValueLabel.TabIndex = 12;
			// 
			// DifficultyVocalsLabel
			// 
			this.DifficultyVocalsLabel.AutoSize = true;
			this.DifficultyVocalsLabel.Dock = System.Windows.Forms.DockStyle.Top;
			this.DifficultyVocalsLabel.Location = new System.Drawing.Point(3, 72);
			this.DifficultyVocalsLabel.Name = "DifficultyVocalsLabel";
			this.DifficultyVocalsLabel.Padding = new System.Windows.Forms.Padding(0, 3, 6, 0);
			this.DifficultyVocalsLabel.Size = new System.Drawing.Size(45, 16);
			this.DifficultyVocalsLabel.TabIndex = 11;
			this.DifficultyVocalsLabel.Text = "Vocals";
			this.DifficultyVocalsLabel.TextAlign = System.Drawing.ContentAlignment.TopRight;
			// 
			// DifficultyDrumsValueLabel
			// 
			this.DifficultyDrumsValueLabel.AutoSize = true;
			this.DifficultyDrumsValueLabel.Dock = System.Windows.Forms.DockStyle.Top;
			this.DifficultyDrumsValueLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.DifficultyDrumsValueLabel.ForeColor = System.Drawing.Color.Red;
			this.DifficultyDrumsValueLabel.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
			this.DifficultyDrumsValueLabel.Location = new System.Drawing.Point(54, 56);
			this.DifficultyDrumsValueLabel.MinimumSize = new System.Drawing.Size(70, 0);
			this.DifficultyDrumsValueLabel.Name = "DifficultyDrumsValueLabel";
			this.DifficultyDrumsValueLabel.Padding = new System.Windows.Forms.Padding(4, 3, 0, 0);
			this.DifficultyDrumsValueLabel.Size = new System.Drawing.Size(70, 16);
			this.DifficultyDrumsValueLabel.TabIndex = 10;
			// 
			// DifficultyDrumsLabel
			// 
			this.DifficultyDrumsLabel.AutoSize = true;
			this.DifficultyDrumsLabel.Dock = System.Windows.Forms.DockStyle.Top;
			this.DifficultyDrumsLabel.Location = new System.Drawing.Point(3, 56);
			this.DifficultyDrumsLabel.Name = "DifficultyDrumsLabel";
			this.DifficultyDrumsLabel.Padding = new System.Windows.Forms.Padding(0, 3, 6, 0);
			this.DifficultyDrumsLabel.Size = new System.Drawing.Size(45, 16);
			this.DifficultyDrumsLabel.TabIndex = 9;
			this.DifficultyDrumsLabel.Text = "Drums";
			this.DifficultyDrumsLabel.TextAlign = System.Drawing.ContentAlignment.TopRight;
			// 
			// DifficultyGuitarValueLabel
			// 
			this.DifficultyGuitarValueLabel.AutoSize = true;
			this.DifficultyGuitarValueLabel.Dock = System.Windows.Forms.DockStyle.Top;
			this.DifficultyGuitarValueLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.DifficultyGuitarValueLabel.ForeColor = System.Drawing.Color.Red;
			this.DifficultyGuitarValueLabel.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
			this.DifficultyGuitarValueLabel.Location = new System.Drawing.Point(54, 40);
			this.DifficultyGuitarValueLabel.MinimumSize = new System.Drawing.Size(70, 0);
			this.DifficultyGuitarValueLabel.Name = "DifficultyGuitarValueLabel";
			this.DifficultyGuitarValueLabel.Padding = new System.Windows.Forms.Padding(4, 3, 0, 0);
			this.DifficultyGuitarValueLabel.Size = new System.Drawing.Size(70, 16);
			this.DifficultyGuitarValueLabel.TabIndex = 7;
			// 
			// DifficultyGuitarLabel
			// 
			this.DifficultyGuitarLabel.AutoSize = true;
			this.DifficultyGuitarLabel.Dock = System.Windows.Forms.DockStyle.Top;
			this.DifficultyGuitarLabel.Location = new System.Drawing.Point(3, 40);
			this.DifficultyGuitarLabel.Name = "DifficultyGuitarLabel";
			this.DifficultyGuitarLabel.Padding = new System.Windows.Forms.Padding(0, 3, 6, 0);
			this.DifficultyGuitarLabel.Size = new System.Drawing.Size(45, 16);
			this.DifficultyGuitarLabel.TabIndex = 8;
			this.DifficultyGuitarLabel.Text = "Guitar";
			this.DifficultyGuitarLabel.TextAlign = System.Drawing.ContentAlignment.TopRight;
			// 
			// DifficultyBandValueLabel
			// 
			this.DifficultyBandValueLabel.AutoSize = true;
			this.DifficultyBandValueLabel.Dock = System.Windows.Forms.DockStyle.Top;
			this.DifficultyBandValueLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.DifficultyBandValueLabel.ForeColor = System.Drawing.Color.Red;
			this.DifficultyBandValueLabel.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
			this.DifficultyBandValueLabel.Location = new System.Drawing.Point(54, 24);
			this.DifficultyBandValueLabel.MinimumSize = new System.Drawing.Size(70, 0);
			this.DifficultyBandValueLabel.Name = "DifficultyBandValueLabel";
			this.DifficultyBandValueLabel.Padding = new System.Windows.Forms.Padding(4, 3, 0, 0);
			this.DifficultyBandValueLabel.Size = new System.Drawing.Size(70, 16);
			this.DifficultyBandValueLabel.TabIndex = 16;
			// 
			// DifficultyBandLabel
			// 
			this.DifficultyBandLabel.AutoSize = true;
			this.DifficultyBandLabel.Dock = System.Windows.Forms.DockStyle.Top;
			this.DifficultyBandLabel.Location = new System.Drawing.Point(3, 24);
			this.DifficultyBandLabel.Name = "DifficultyBandLabel";
			this.DifficultyBandLabel.Padding = new System.Windows.Forms.Padding(0, 3, 6, 0);
			this.DifficultyBandLabel.Size = new System.Drawing.Size(45, 16);
			this.DifficultyBandLabel.TabIndex = 15;
			this.DifficultyBandLabel.Text = "Band";
			this.DifficultyBandLabel.TextAlign = System.Drawing.ContentAlignment.TopRight;
			// 
			// ProgressLayout
			// 
			this.ProgressLayout.AutoSize = true;
			this.ProgressLayout.ColumnCount = 2;
			this.ProgressLayout.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50F));
			this.ProgressLayout.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50F));
			this.ProgressLayout.Location = new System.Drawing.Point(648, 0);
			this.ProgressLayout.Margin = new System.Windows.Forms.Padding(0);
			this.ProgressLayout.Name = "ProgressLayout";
			this.ProgressLayout.RowCount = 2;
			this.TableLayout.SetRowSpan(this.ProgressLayout, 7);
			this.ProgressLayout.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 50F));
			this.ProgressLayout.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 50F));
			this.ProgressLayout.Size = new System.Drawing.Size(0, 0);
			this.ProgressLayout.TabIndex = 8;
			// 
			// YearLabel
			// 
			this.YearLabel.AutoSize = true;
			this.YearLabel.Dock = System.Windows.Forms.DockStyle.Top;
			this.YearLabel.Location = new System.Drawing.Point(131, 79);
			this.YearLabel.Margin = new System.Windows.Forms.Padding(3, 3, 3, 0);
			this.YearLabel.MinimumSize = new System.Drawing.Size(64, 0);
			this.YearLabel.Name = "YearLabel";
			this.YearLabel.Padding = new System.Windows.Forms.Padding(0, 3, 0, 0);
			this.YearLabel.Size = new System.Drawing.Size(381, 16);
			this.YearLabel.TabIndex = 4;
			this.YearLabel.UseMnemonic = false;
			// 
			// StatusBar
			// 
			this.TableLayout.SetColumnSpan(this.StatusBar, 3);
			this.StatusBar.GripMargin = new System.Windows.Forms.Padding(0);
			this.StatusBar.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.StatusSongsLabel,
            this.StatusRiifsLabel,
            this.StatusContactLabel,
            this.StatusDonateLabel});
			this.StatusBar.LayoutStyle = System.Windows.Forms.ToolStripLayoutStyle.Flow;
			this.StatusBar.Location = new System.Drawing.Point(0, 128);
			this.StatusBar.Name = "StatusBar";
			this.StatusBar.ShowItemToolTips = true;
			this.StatusBar.Size = new System.Drawing.Size(648, 20);
			this.StatusBar.SizingGrip = false;
			this.StatusBar.Stretch = false;
			this.StatusBar.TabIndex = 9;
			// 
			// StatusSongsLabel
			// 
			this.StatusSongsLabel.BorderSides = System.Windows.Forms.ToolStripStatusLabelBorderSides.Left;
			this.StatusSongsLabel.BorderStyle = System.Windows.Forms.Border3DStyle.Etched;
			this.StatusSongsLabel.Name = "StatusSongsLabel";
			this.StatusSongsLabel.Size = new System.Drawing.Size(60, 17);
			this.StatusSongsLabel.Text = "Loading...";
			// 
			// StatusRiifsLabel
			// 
			this.StatusRiifsLabel.BorderSides = System.Windows.Forms.ToolStripStatusLabelBorderSides.Left;
			this.StatusRiifsLabel.BorderStyle = System.Windows.Forms.Border3DStyle.Etched;
			this.StatusRiifsLabel.Name = "StatusRiifsLabel";
			this.StatusRiifsLabel.Size = new System.Drawing.Size(112, 17);
			this.StatusRiifsLabel.Text = "RiiFS Server Disabled";
			this.StatusRiifsLabel.ToolTipText = "Click to toggle the RiiFS Server";
			this.StatusRiifsLabel.Click += new System.EventHandler(this.StatusRiifsLabel_Click);
			// 
			// StatusContactLabel
			// 
			this.StatusContactLabel.BorderSides = System.Windows.Forms.ToolStripStatusLabelBorderSides.Left;
			this.StatusContactLabel.BorderStyle = System.Windows.Forms.Border3DStyle.Etched;
			this.StatusContactLabel.IsLink = true;
			this.StatusContactLabel.Name = "StatusContactLabel";
			this.StatusContactLabel.Size = new System.Drawing.Size(49, 17);
			this.StatusContactLabel.Text = "Contact";
			this.StatusContactLabel.ToolTipText = "rawksd@japaneatahand.com";
			this.StatusContactLabel.Click += new System.EventHandler(this.StatusContactLabel_Click);
			// 
			// StatusDonateLabel
			// 
			this.StatusDonateLabel.BorderSides = System.Windows.Forms.ToolStripStatusLabelBorderSides.Left;
			this.StatusDonateLabel.BorderStyle = System.Windows.Forms.Border3DStyle.Etched;
			this.StatusDonateLabel.IsLink = true;
			this.StatusDonateLabel.Name = "StatusDonateLabel";
			this.StatusDonateLabel.Size = new System.Drawing.Size(46, 17);
			this.StatusDonateLabel.Text = "Donate";
			this.StatusDonateLabel.ToolTipText = "Paypal: rawksd@japaneatahand.com";
			this.StatusDonateLabel.Click += new System.EventHandler(this.StatusDonateLabel_Click);
			// 
			// Toolbar
			// 
			this.Toolbar.Dock = System.Windows.Forms.DockStyle.None;
			this.Toolbar.GripStyle = System.Windows.Forms.ToolStripGripStyle.Hidden;
			this.Toolbar.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.ToolbarOpenSD,
            this.ToolbarOpen,
            this.ToolbarOpenFolder,
            this.ToolbarSep1,
            this.ToolbarNew,
            this.ToolbarEdit,
            this.ToolbarDelete,
            this.ToolbarSep2,
            this.ToolbarInstallLocal,
            this.ToolbarInstallSD});
			this.Toolbar.Location = new System.Drawing.Point(3, 0);
			this.Toolbar.Name = "Toolbar";
			this.Toolbar.Size = new System.Drawing.Size(199, 25);
			this.Toolbar.TabIndex = 3;
			// 
			// ToolbarOpenSD
			// 
			this.ToolbarOpenSD.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
			this.ToolbarOpenSD.Image = global::ConsoleHaxx.RawkSD.SWF.Properties.Resources.IconRSD;
			this.ToolbarOpenSD.ImageTransparentColor = System.Drawing.Color.Magenta;
			this.ToolbarOpenSD.Name = "ToolbarOpenSD";
			this.ToolbarOpenSD.Size = new System.Drawing.Size(23, 22);
			this.ToolbarOpenSD.Text = "Open SD Card";
			this.ToolbarOpenSD.Click += new System.EventHandler(this.ToolbarOpenSD_Click);
			// 
			// ToolbarOpen
			// 
			this.ToolbarOpen.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
			this.ToolbarOpen.Image = global::ConsoleHaxx.RawkSD.SWF.Properties.Resources.IconOpenFile;
			this.ToolbarOpen.ImageTransparentColor = System.Drawing.Color.Magenta;
			this.ToolbarOpen.Name = "ToolbarOpen";
			this.ToolbarOpen.Size = new System.Drawing.Size(23, 22);
			this.ToolbarOpen.Text = "Open File";
			this.ToolbarOpen.Click += new System.EventHandler(this.ToolbarOpen_Click);
			// 
			// ToolbarOpenFolder
			// 
			this.ToolbarOpenFolder.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
			this.ToolbarOpenFolder.Image = global::ConsoleHaxx.RawkSD.SWF.Properties.Resources.IconOpenFolder;
			this.ToolbarOpenFolder.ImageTransparentColor = System.Drawing.Color.Magenta;
			this.ToolbarOpenFolder.Name = "ToolbarOpenFolder";
			this.ToolbarOpenFolder.Size = new System.Drawing.Size(23, 22);
			this.ToolbarOpenFolder.Text = "Open Folder";
			this.ToolbarOpenFolder.Click += new System.EventHandler(this.ToolbarOpenFolder_Click);
			// 
			// ToolbarSep1
			// 
			this.ToolbarSep1.Name = "ToolbarSep1";
			this.ToolbarSep1.Size = new System.Drawing.Size(6, 25);
			// 
			// ToolbarNew
			// 
			this.ToolbarNew.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
			this.ToolbarNew.Image = global::ConsoleHaxx.RawkSD.SWF.Properties.Resources.IconNew;
			this.ToolbarNew.ImageTransparentColor = System.Drawing.Color.Magenta;
			this.ToolbarNew.Name = "ToolbarNew";
			this.ToolbarNew.Size = new System.Drawing.Size(23, 22);
			this.ToolbarNew.Text = "New Custom";
			this.ToolbarNew.Click += new System.EventHandler(this.ToolbarNew_Click);
			// 
			// ToolbarEdit
			// 
			this.ToolbarEdit.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
			this.ToolbarEdit.Image = global::ConsoleHaxx.RawkSD.SWF.Properties.Resources.IconEdit;
			this.ToolbarEdit.ImageTransparentColor = System.Drawing.Color.Magenta;
			this.ToolbarEdit.Name = "ToolbarEdit";
			this.ToolbarEdit.Size = new System.Drawing.Size(23, 22);
			this.ToolbarEdit.Text = "Edit";
			this.ToolbarEdit.Click += new System.EventHandler(this.ToolbarEdit_Click);
			// 
			// ToolbarDelete
			// 
			this.ToolbarDelete.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
			this.ToolbarDelete.Image = global::ConsoleHaxx.RawkSD.SWF.Properties.Resources.IconDelete;
			this.ToolbarDelete.ImageTransparentColor = System.Drawing.Color.Magenta;
			this.ToolbarDelete.Name = "ToolbarDelete";
			this.ToolbarDelete.Size = new System.Drawing.Size(23, 22);
			this.ToolbarDelete.Text = "Delete";
			this.ToolbarDelete.Click += new System.EventHandler(this.ToolbarDelete_Click);
			// 
			// ToolbarSep2
			// 
			this.ToolbarSep2.Name = "ToolbarSep2";
			this.ToolbarSep2.Size = new System.Drawing.Size(6, 25);
			// 
			// ToolbarInstallLocal
			// 
			this.ToolbarInstallLocal.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
			this.ToolbarInstallLocal.Image = ((System.Drawing.Image)(resources.GetObject("ToolbarInstallLocal.Image")));
			this.ToolbarInstallLocal.ImageTransparentColor = System.Drawing.Color.Magenta;
			this.ToolbarInstallLocal.Name = "ToolbarInstallLocal";
			this.ToolbarInstallLocal.Size = new System.Drawing.Size(23, 22);
			this.ToolbarInstallLocal.Text = "Install Locally";
			this.ToolbarInstallLocal.Click += new System.EventHandler(this.ToolbarInstallLocal_Click);
			// 
			// ToolbarInstallSD
			// 
			this.ToolbarInstallSD.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
			this.ToolbarInstallSD.Image = global::ConsoleHaxx.RawkSD.SWF.Properties.Resources.IconMoveToFolder;
			this.ToolbarInstallSD.ImageTransparentColor = System.Drawing.Color.Magenta;
			this.ToolbarInstallSD.Name = "ToolbarInstallSD";
			this.ToolbarInstallSD.Size = new System.Drawing.Size(23, 22);
			this.ToolbarInstallSD.Text = "Install to SD";
			this.ToolbarInstallSD.Click += new System.EventHandler(this.ToolbarInstallSD_Click);
			// 
			// SongListTimer
			// 
			this.SongListTimer.Interval = 50;
			this.SongListTimer.Tick += new System.EventHandler(this.SongListTimer_Tick);
			// 
			// MainForm
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.ClientSize = new System.Drawing.Size(648, 485);
			this.Controls.Add(this.ToolDock);
			this.Controls.Add(this.MenuStrip);
			this.Icon = global::ConsoleHaxx.RawkSD.SWF.Properties.Resources.RawkSD;
			this.MainMenuStrip = this.MenuStrip;
			this.MinimumSize = new System.Drawing.Size(512, 256);
			this.Name = "MainForm";
			this.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Show;
			this.Text = "RawkSD v3.00 beta 1.0 [July 24 2010]";
			this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.MainForm_FormClosing);
			this.FormClosed += new System.Windows.Forms.FormClosedEventHandler(this.MainForm_FormClosed);
			this.Load += new System.EventHandler(this.MainForm_Load);
			this.MenuStrip.ResumeLayout(false);
			this.MenuStrip.PerformLayout();
			this.ToolDock.ContentPanel.ResumeLayout(false);
			this.ToolDock.ContentPanel.PerformLayout();
			this.ToolDock.TopToolStripPanel.ResumeLayout(false);
			this.ToolDock.TopToolStripPanel.PerformLayout();
			this.ToolDock.ResumeLayout(false);
			this.ToolDock.PerformLayout();
			this.SongContextMenu.ResumeLayout(false);
			this.TableLayout.ResumeLayout(false);
			this.TableLayout.PerformLayout();
			((System.ComponentModel.ISupportInitialize)(this.AlbumArtPicture)).EndInit();
			this.DifficultyLayout.ResumeLayout(false);
			this.DifficultyLayout.PerformLayout();
			this.StatusBar.ResumeLayout(false);
			this.StatusBar.PerformLayout();
			this.Toolbar.ResumeLayout(false);
			this.Toolbar.PerformLayout();
			this.ResumeLayout(false);
			this.PerformLayout();

		}

		#endregion

		private System.Windows.Forms.MenuStrip MenuStrip;
		private System.Windows.Forms.ToolStripMenuItem MenuHelp;
		private System.Windows.Forms.ToolStripMenuItem MenuHelpGuide;
		private System.Windows.Forms.ToolStripMenuItem MenuHelpDonate;
		private System.Windows.Forms.ToolStripSeparator MenuHelpSep1;
		private System.Windows.Forms.ToolStripMenuItem MenuHelpAbout;
		private System.Windows.Forms.FolderBrowserDialog FolderDialog;
		private System.Windows.Forms.OpenFileDialog OpenDialog;
		private System.Windows.Forms.ToolStripContainer ToolDock;
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
		private DoubleBufferedListView SongList;
		private System.Windows.Forms.ColumnHeader SongListColumnSong;
		private System.Windows.Forms.ColumnHeader SongListColumnArtist;
		private System.Windows.Forms.ColumnHeader SongListColumnAlbum;
		private System.Windows.Forms.ColumnHeader SongListColumnYear;
		private System.Windows.Forms.ColumnHeader SongListColumnGenre;
		private System.Windows.Forms.ColumnHeader SongListColumnAvailability;
		private System.Windows.Forms.ToolStrip Toolbar;
		private System.Windows.Forms.ImageList SongListRowSizeHack;
		private System.Windows.Forms.Label NameLabel;
		private System.Windows.Forms.ToolStripMenuItem MenuFile;
		private System.Windows.Forms.ToolStripMenuItem MenuFileOpenSD;
		private System.Windows.Forms.ToolStripSeparator MenuFileSep1;
		private System.Windows.Forms.ToolStripMenuItem MenuFileOpen;
		private System.Windows.Forms.ToolStripMenuItem MenuFileOpenFolder;
		private System.Windows.Forms.ToolStripSeparator MenuFileSep2;
		private System.Windows.Forms.ToolStripMenuItem MenuFilePreferences;
		private System.Windows.Forms.ToolStripSeparator MenuFileSep3;
		private System.Windows.Forms.ToolStripMenuItem MenuFileExit;
		private System.Windows.Forms.ToolStripMenuItem MenuSongs;
		private System.Windows.Forms.ToolStripMenuItem MenuSongsCreate;
		private System.Windows.Forms.ToolStripSeparator MenuSongsSep1;
		private System.Windows.Forms.ToolStripMenuItem MenuSongsEdit;
		private System.Windows.Forms.ToolStripMenuItem MenuSongsDelete;
		private System.Windows.Forms.ToolStripSeparator MenuSongsSep3;
		private System.Windows.Forms.ToolStripMenuItem MenuSongsInstall;
		private System.Windows.Forms.ToolStripMenuItem MenuSongsInstallLocal;
		private System.Windows.Forms.ToolStripMenuItem MenuSongsInstallSD;
		private System.Windows.Forms.ToolStripSeparator MenuSongsSep4;
		private System.Windows.Forms.ToolStripMenuItem MenuSongsExport;
		private System.Windows.Forms.ToolStripMenuItem MenuSongsExportRawkSD;
		private System.Windows.Forms.ToolStripMenuItem MenuSongsExportRBN;
		private System.Windows.Forms.ToolStripMenuItem MenuSongsExport360DLC;
		private System.Windows.Forms.ContextMenuStrip SongContextMenu;
		private System.Windows.Forms.ToolStripMenuItem MenuSongsExportFoF;
		private System.Windows.Forms.ToolStripMenuItem ContextMenuEdit;
		private System.Windows.Forms.ToolStripMenuItem ContextMenuDelete;
		private System.Windows.Forms.ToolStripSeparator ContextMenuSep1;
		private System.Windows.Forms.ToolStripMenuItem ContextMenuInstall;
		private System.Windows.Forms.ToolStripMenuItem ContextMenuInstallLocal;
		private System.Windows.Forms.ToolStripMenuItem ContextMenuInstallSD;
		private System.Windows.Forms.ToolStripMenuItem ContextMenuExport;
		private System.Windows.Forms.ToolStripMenuItem ContextMenuExportRawkSD;
		private System.Windows.Forms.ToolStripMenuItem ContextMenuExportRBA;
		private System.Windows.Forms.ToolStripMenuItem ContextMenuExport360DLC;
		private System.Windows.Forms.ToolStripMenuItem ContextMenuExportFoF;
		private System.Windows.Forms.ToolStripSeparator ContextMenuSep2;
		private System.Windows.Forms.SaveFileDialog SaveDialog;
		private System.Windows.Forms.ToolStripMenuItem MenuSongsSelectAll;
		private System.Windows.Forms.ToolStripSeparator MenuSongsSep2;
		private System.Windows.Forms.Timer SongListTimer;
		private System.Windows.Forms.TableLayoutPanel ProgressLayout;
		private System.Windows.Forms.StatusStrip StatusBar;
		private System.Windows.Forms.ToolStripStatusLabel StatusSongsLabel;
		private System.Windows.Forms.ToolStripStatusLabel StatusRiifsLabel;
		private System.Windows.Forms.ToolStripButton ToolbarOpenSD;
		private System.Windows.Forms.ToolStripButton ToolbarOpen;
		private System.Windows.Forms.ToolStripSeparator MenuSongsSep5;
		private System.Windows.Forms.ToolStripMenuItem MenuSongsRefresh;
		private System.Windows.Forms.ToolStripSeparator ContextMenuSep3;
		private System.Windows.Forms.ToolStripMenuItem ContextMenuRefresh;
		private System.Windows.Forms.ToolStripButton ToolbarOpenFolder;
		private System.Windows.Forms.ToolStripSeparator ToolbarSep1;
		private System.Windows.Forms.ToolStripButton ToolbarNew;
		private System.Windows.Forms.ToolStripButton ToolbarEdit;
		private System.Windows.Forms.ToolStripButton ToolbarDelete;
		private System.Windows.Forms.ToolStripSeparator ToolbarSep2;
		private System.Windows.Forms.ToolStripButton ToolbarInstallLocal;
		private System.Windows.Forms.ToolStripButton ToolbarInstallSD;
		private System.Windows.Forms.ToolStripStatusLabel StatusContactLabel;
		private System.Windows.Forms.ToolStripStatusLabel StatusDonateLabel;
		private System.Windows.Forms.ToolStripMenuItem MenuSongsTranscode;
		private System.Windows.Forms.ToolStripMenuItem ContextMenuTranscode;
	}
}

