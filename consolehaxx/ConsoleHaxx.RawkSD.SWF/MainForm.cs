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
using ConsoleHaxx.Harmonix;
using System.Collections;
using ConsoleHaxx.Wii;

namespace ConsoleHaxx.RawkSD.SWF
{
	public partial class MainForm : Form
	{
		public string RootPath;
		public string SdPath;
		public string StoragePath;
		public string TemporaryPath;

		public PlatformData Storage;
		public PlatformData SD;
		
		public List<PlatformData> Platforms;

		public Bitmap AlbumImage;

		public Image[] Tiers;

		public MainForm()
		{
			InitializeComponent();

			Platforms = new List<PlatformData>();
			ProgressList = new List<TaskScheduler>();
			ProgressControls = new Dictionary<TaskScheduler, VisualTask>();

			Platform.GetFormat(0); // Force the static constructor to run

			RootPath = Environment.CurrentDirectory;
			StoragePath = Path.Combine(RootPath, "customs");
			SdPath = Path.Combine(RootPath, "rawksd");
			TemporaryPath = Path.Combine(RootPath, "temp");

			Directory.CreateDirectory(TemporaryPath);
			DeleteTemporaryFiles();
			TemporaryStream.BasePath = TemporaryPath;

			Configuration.Load();

			for (int i = 0; i < Configuration.MaxConcurrentTasks; i++)
				AddTaskScheduler();

			Tiers = new Image[7];
			Tiers[0] = Properties.Resources.TierZero;
			Tiers[1] = Properties.Resources.TierOne;
			Tiers[2] = Properties.Resources.TierTwo;
			Tiers[3] = Properties.Resources.TierThree;
			Tiers[4] = Properties.Resources.TierFour;
			Tiers[5] = Properties.Resources.TierFive;
			Tiers[6] = Properties.Resources.TierSix;

			AlbumImage = WiiImage.Create(new EndianReader(new MemoryStream(RawkSD.Properties.Resources.rawksd_albumart), Endianness.LittleEndian)).Bitmap;

			SongList.DoubleBuffer();
		}

		private void DeleteTemporaryFiles()
		{
			foreach (string file in Directory.GetFiles(TemporaryPath)) {
				try {
					File.Delete(file);
				} catch { }
			}
		}

		private void MainForm_Load(object sender, EventArgs e)
		{
			Progress.QueueTask(progress => {
				progress.NewTask("Loading Local Content");
				Storage = PlatformLocalStorage.Instance.Create(StoragePath, Game.Unknown, progress);
				progress.Progress();
				progress.EndTask();
				Invoke((Action)RefreshSongList);
			});

			Progress.QueueTask(progress => {
				progress.NewTask("Loading SD Content");
				SD = PlatformRB2WiiCustomDLC.Instance.Create(SdPath, Game.Unknown, progress);
				progress.Progress();
				progress.EndTask();
				Invoke((Action)UpdateSongList);
			});
		}

		private void MainForm_FormClosing(object sender, FormClosingEventArgs e)
		{
			Configuration.Save();
			foreach (TaskScheduler task in ProgressList)
				task.Exit(true);
		}

		private void MainForm_FormClosed(object sender, FormClosedEventArgs e)
		{
			DeleteTemporaryFiles();
		}

		private void Open(string path)
		{
			Engine platform;
			Game game;
			if (!SelectPlatform(PlatformDetection.DetectPlaform(path), out platform, out game))
				return;

			GetAsyncProgress().QueueTask(progress => {
				progress.NewTask("Loading Content from " + platform.Name, 3);
				PlatformData data = platform.Create(path, game, progress);
				progress.Progress();
				ImportMap map = new ImportMap(data.Game, Path.Combine(RootPath, "importdata"));
				map.AddSongs(data, progress);
				progress.Progress();
				Platforms.Add(data);
				progress.NewTask(data.Songs.Count);
				foreach (FormatData song in data.Songs) {
					map.Populate(song.Song);
					progress.Progress();
				}
				progress.EndTask();
				progress.Progress();
				Invoke((Action)UpdateSongList);
				progress.EndTask();
			});
		}

		private bool SelectPlatform(List<Pair<Engine, Game>> platforms, out Engine platform, out Game game)
		{
			game = Game.Unknown;

			if (platforms.Count == 0 || platforms.Count > 1) {
				string message = "The selected file was not recognized. Please select a platform to try to import with.";
				IList<Engine> engines = PlatformDetection.Selections;
				if (platforms.Count > 1) {
					message = "The selected file could be in any of the following formats. Please select the one that matches best.";
					engines = platforms.Select(p => p.Key).ToList();
				}
				var ret = PlatformForm.Show(message, engines, this);
				if (ret.Result == DialogResult.Cancel) {
					platform = null;
					return false;
				}

				platform = ret.Platform;
				return true;
			} else {
				var pair = platforms.First();
				platform = pair.Key;
				game = pair.Value;
			}

			return true;
		}

		private void MenuFileOpenSD_Click(object sender, EventArgs e)
		{
			FolderDialog.Description = "Select the SD card drive.";
			if (FolderDialog.ShowDialog(this) == DialogResult.Cancel)
				return;
			SdPath = FolderDialog.SelectedPath;

			Progress.QueueTask(progress => {
				progress.NewTask("Loading SD Content");
				SD = PlatformRB2WiiCustomDLC.Instance.Create(SdPath, Game.Unknown, progress);
				progress.Progress();
				progress.EndTask();
				Invoke((Action)UpdateSongList);
			});
		}

		private void MenuFileExit_Click(object sender, EventArgs e)
		{
			Close();
		}

		private void MenuFileOpen_Click(object sender, EventArgs e)
		{
			OpenDialog.Title = "Select a file containing songs.";
			OpenDialog.Filter = "Supported Files|*.iso;*.wod;*.mdf;*.ark;*.hdr;*.rwk;*.rba;*.bin|All Files|*";
			if (OpenDialog.ShowDialog(this) == DialogResult.Cancel)
				return;
			Open(OpenDialog.FileName);
		}

		private void MenuFileOpenFolder_Click(object sender, EventArgs e)
		{
			FolderDialog.Description = "Select a folder containing songs.";
			if (FolderDialog.ShowDialog(this) == DialogResult.Cancel)
				return;
			Open(FolderDialog.SelectedPath);
		}

		private void MenuSongsInstallLocal_Click(object sender, EventArgs e)
		{
			foreach (FormatData data in GetSelectedSongs()) {
				if (data.PlatformData.Platform == PlatformLocalStorage.Instance)
					continue;
				Progress.QueueTask(progress => {
					progress.NewTask("Ripping \"" + data.Song.Name + "\" Locally", 2);
					FormatData destination = PlatformLocalStorage.Instance.CreateSong(Storage, data.Song);
					Platform.Transfer(data, destination, progress);

					progress.Progress();
					PlatformLocalStorage.Instance.SaveSong(Storage, destination, progress);

					progress.Progress();

					progress.EndTask();

					Invoke((Action)UpdateSongList);
				});
			}
		}

		private void MenuSongsInstallSD_Click(object sender, EventArgs e)
		{
			foreach (FormatData data in GetSelectedSongs()) {
				Progress.QueueTask(progress => {
					progress.NewTask("Installing \"" + data.Song.Name + "\" to SD", 10);
					FormatData source = data;
					if (data.PlatformData.Platform != PlatformLocalStorage.Instance && Configuration.LocalTransfer) {
						source = PlatformLocalStorage.Instance.CreateSong(Storage, data.Song);
						data.Save(source);
					}
					progress.Progress();

					progress.SetNextWeight(9);

					PlatformRB2WiiCustomDLC.Instance.SaveSong(SD, source, progress);
					progress.Progress(9);

					progress.EndTask();

					Invoke((Action)UpdateSongList);
				});
			}
		}

		private void MenuSongsExportRawkSD_Click(object sender, EventArgs e)
		{
			SaveDialog.Title = "Save RawkSD Archive";
			SaveDialog.Filter = "RawkSD Archive (*.rwk)|*.rwk|All Files|*";
			if (SaveDialog.ShowDialog(this) == DialogResult.Cancel)
				return;

			Exports.ExportRWK(SaveDialog.FileName, GetSelectedSongs());
		}

		private void MenuSongsExportRBN_Click(object sender, EventArgs e)
		{
			var songs = GetSelectedSongs();
			if (songs.Count() > 1) {

				return;
			}

			SaveDialog.Title = "Save Rock Band Network Archive";
			SaveDialog.Filter = "Rock Band Audition File (*.rba)|*.rba|All Files|*";
			if (SaveDialog.ShowDialog(this) == DialogResult.Cancel)
				return;

			Exports.ExportRBN(SaveDialog.FileName, songs.Single());
		}

		private void MenuSongsExport360DLC_Click(object sender, EventArgs e)
		{

		}

		private void MenuSongsExportFoF_Click(object sender, EventArgs e)
		{
			FolderDialog.Description = "Select your Frets on Fire songs/ folder.";
			if (FolderDialog.ShowDialog(this) == DialogResult.Cancel)
				return;

			Exports.ExportFoF(FolderDialog.SelectedPath, GetSelectedSongs());
		}

		private void MenuSongsEdit_Click(object sender, EventArgs e)
		{
			foreach (FormatData data in GetSelectedSongs()) {
				EditForm.Show(data, data.PlatformData == Storage, false, this);
			}

			SongList_SelectedIndexChanged(SongList, EventArgs.Empty);
		}

		private void MenuSongsDelete_Click(object sender, EventArgs e)
		{

		}

		private void ContextMenuEdit_Click(object sender, EventArgs e)
		{
			MenuSongsEdit_Click(sender, e);
		}

		private void ContextMenuDelete_Click(object sender, EventArgs e)
		{
			MenuSongsDelete_Click(sender, e);
		}

		private void ContextMenuInstallLocal_Click(object sender, EventArgs e)
		{
			MenuSongsInstallLocal_Click(sender, e);
		}

		private void ContextMenuInstallSD_Click(object sender, EventArgs e)
		{
			MenuSongsInstallSD_Click(sender, e);
		}

		private void ContextMenuExportRawkSD_Click(object sender, EventArgs e)
		{
			MenuSongsExportRawkSD_Click(sender, e);
		}

		private void ContextMenuExportRBA_Click(object sender, EventArgs e)
		{
			MenuSongsExportRBN_Click(sender, e);
		}

		private void ContextMenuExport360DLC_Click(object sender, EventArgs e)
		{
			MenuSongsExport360DLC_Click(sender, e);
		}

		private void ContextMenuExportFoF_Click(object sender, EventArgs e)
		{
			MenuSongsExportFoF_Click(sender, e);
		}

		private void MenuSongsCreate_Click(object sender, EventArgs e)
		{
			SongData song = new SongData(Storage);
			FormatData data = new TemporaryFormatData(song, Storage);
			if (EditForm.Show(data, true, true, this) != DialogResult.Cancel) {
				Progress.QueueTask(progress => {
					progress.NewTask("Installing \"" + data.Song.Name + "\" Locally", 2);

					FormatData localdata = PlatformLocalStorage.Instance.CreateSong(Storage, data.Song);
					data.Save(localdata);
					data.Dispose();
					progress.Progress();

					PlatformLocalStorage.Instance.SaveSong(Storage, localdata, progress);
					progress.Progress();

					progress.EndTask();
					Invoke((Action)UpdateSongList);
				});
			} else
				data.Dispose();
		}
	}
}
