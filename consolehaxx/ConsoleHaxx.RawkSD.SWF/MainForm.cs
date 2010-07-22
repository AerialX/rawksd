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
using System.Threading;
using ConsoleHaxx.RiiFS;
using System.Diagnostics;

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

		public Mutex SongUpdateMutex;
		public Mutex TaskMutex;

		public Server RiiFS;

		public StreamWriter LogStream;

		public MainForm()
		{
			InitializeComponent();

			SongUpdateMutex = new Mutex();
			TaskMutex = new Mutex();

			Platforms = new List<PlatformData>();
			ProgressList = new List<TaskScheduler>();
			ProgressControls = new Dictionary<TaskScheduler, ProgressItem>();

			RootPath = Environment.CurrentDirectory;
			Configuration.Load(this);

			try { // It may be in use by another instance... So you get no logs, too bad.
				LogStream = new StreamWriter(new FileStream(Path.Combine(RootPath, "rawksd.log"), FileMode.Append, FileAccess.Write, FileShare.Read));
				LogStream.AutoFlush = true;
				LogStream.WriteLine("RawkSD started at " + DateTime.Now.ToString());
				Console.SetOut(LogStream);
			} catch (Exception exception) {
				Exceptions.Warning(exception, "Unable to use the log file; another RawkSD instance is probably still running.");
			}

			ImportMap.RootPath = Path.Combine(RootPath, "importdata");

			if (!Path.IsPathRooted(Configuration.LocalPath))
				StoragePath = Path.Combine(RootPath, Configuration.LocalPath);
			else
				StoragePath = Configuration.LocalPath;
			if (!Path.IsPathRooted(Configuration.TemporaryPath))
				TemporaryPath = Path.Combine(RootPath, Configuration.TemporaryPath);
			else
				TemporaryPath = Configuration.TemporaryPath;
			TemporaryPath = Path.Combine(TemporaryPath, Path.GetRandomFileName());

			Directory.CreateDirectory(TemporaryPath);
			TemporaryStream.BasePath = TemporaryPath;

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

			SongList_SelectedIndexChanged(this, EventArgs.Empty);
		}

		private void OpenSdCard(string path)
		{
			SdPath = path;
			if (SD != null)
				SD.Dispose();

			GetAsyncProgress().QueueTask(progress => {
				progress.NewTask("Loading SD content");
				if (SD != null) {
					SongUpdateMutex.WaitOne();
					Platforms.Add(SD);
					SongUpdateMutex.ReleaseMutex();
				}

				SD = PlatformRB2WiiCustomDLC.Instance.Create(SdPath, Game.Unknown, progress);
				progress.Progress();

				if (RiiFS != null)
					RiiFS.Stop();
				StartRiiFS();
				RefreshRiiFS();

				UpdateSongList();
				progress.EndTask();
			});
		}

		private void StartRiiFS()
		{
			if (!SdPath.HasValue())
				return;
			try {
				RiiFS = new Server(SdPath, 0);
				RiiFS.ReadOnly = false;
				RiiFS.StartBroadcastAsync(5256);
				RiiFS.StartAsync();
			} catch (Exception exception) {
				Exceptions.Warning(exception, "Unable to start the RiiFS server.");
				if (RiiFS != null)
					RiiFS.Stop();
				RiiFS = null;
			}
		}

		private void ToggleRiiFS()
		{
			if (RiiFS != null) {
				RiiFS.Stop();
				RiiFS = null;
			} else
				StartRiiFS();
			RefreshRiiFS();
		}

		private void RefreshRiiFS()
		{
			Invoke((Action)RefreshRiiFsBase);
		}

		private void RefreshRiiFsBase()
		{
			if (RiiFS == null)
				StatusRiifsLabel.Text = "RiiFS Server Disabled";
			else
				StatusRiifsLabel.Text = "RiiFS Server Enabled on Port " + RiiFS.Port.ToString();
		}

		private void DeleteTemporaryFiles()
		{
			if (Directory.Exists(TemporaryPath)) {
				try {
					Directory.Delete(TemporaryPath, true);
				} catch { }
			}
		}

		private void MainForm_Load(object sender, EventArgs e)
		{
			GetAsyncProgress().QueueTask(progress => {
				progress.NewTask("Loading local content");
				Storage = PlatformLocalStorage.Instance.Create(StoragePath, Game.Unknown, progress);
				progress.Progress();
				UpdateSongList();
				progress.EndTask();
			});

			OpenSdCard(Path.Combine(RootPath, "rawksd"));
		}

		private void MainForm_FormClosing(object sender, FormClosingEventArgs e)
		{
			foreach (TaskScheduler task in ProgressList)
				task.Exit(true);
			if (AsyncProgress != null)
				AsyncProgress.Exit(true);
		}

		private void MainForm_FormClosed(object sender, FormClosedEventArgs e)
		{
			try {
				foreach (PlatformData platform in Platforms)
					platform.Dispose();
			} catch { }
			try {
				if (Storage != null)
					Storage.Dispose();
			} catch { }
			try {
				if (SD != null)
					SD.Dispose();
			} catch { }
			try {
				if (RiiFS != null)
					RiiFS.Stop();
			} catch { }
			try {
				Console.SetOut(new StreamWriter(new MemoryStream()));
				if (LogStream != null)
					LogStream.Close();
			} catch { }
			try {
				Configuration.Save();
				DeleteTemporaryFiles();
			} catch { }
		}

		private void Open(string path)
		{
			Engine platform;
			Game game;
			if (!SelectPlatform(PlatformDetection.DetectPlaform(path), out platform, out game))
				return;

			GetAsyncProgress().QueueTask(progress => {
				progress.NewTask("Loading content from " + platform.Name, 3);
				PlatformData data = platform.Create(path, game, progress);
				progress.Progress();
				ImportMap map = new ImportMap(data.Game);
				map.AddSongs(data, progress);
				progress.Progress();
				data.Mutex.WaitOne();
				progress.NewTask(data.Songs.Count);
				foreach (FormatData song in data.Songs) {
					map.Populate(song.Song);
					progress.Progress();
				}
				data.Mutex.ReleaseMutex();
				progress.EndTask();
				progress.Progress();

				if (data.Game == Game.LegoRockBand) {
					progress.NewTask("Loading Rock Band 1 content from Lego Rock Band");
					Exports.ImportRB1Lipsync(data);
					progress.Progress();
					progress.EndTask();
				}

				SongUpdateMutex.WaitOne();
				Platforms.Add(data);
				SongUpdateMutex.ReleaseMutex();

				UpdateSongList();
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
				if (platforms.Count == 0)
					game = Game.Unknown;
				else
					game = platforms.Where(p => p.Key == ret.Platform).First().Value;
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

			OpenSdCard(FolderDialog.SelectedPath);
		}

		private void MenuFileExit_Click(object sender, EventArgs e)
		{
			Close();
		}

		private void MenuFileOpen_Click(object sender, EventArgs e)
		{
			OpenDialog.Title = "Select a file containing songs.";
			OpenDialog.Filter = "Supported Files|*.iso;*.wod;*.mdf;*.ark;*.hdr;*.rwk;*.rba;*.bin|All Files|*";
			OpenDialog.Multiselect = true;
			if (OpenDialog.ShowDialog(this) == DialogResult.Cancel)
				return;
			foreach (string filename in OpenDialog.FileNames) {
				Open(filename);
			}
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
			foreach (FormatData data in GetSelectedSongData()) {
				if (data.PlatformData == Storage)
					continue;

				QueueInstallLocal(data);
			}
		}

		private void QueueInstallLocal(FormatData data)
		{
			Progress.QueueTask(progress => {
				progress.NewTask("Ripping \"" + data.Song.Name + "\" locally", 16);
				FormatData destination = PlatformLocalStorage.Instance.CreateSong(Storage, data.Song);

				progress.SetNextWeight(14);

				bool audio = false;
				if (Configuration.LocalTranscode && !data.Formats.Contains(AudioFormatRB2Mogg.Instance) && !data.Formats.Contains(AudioFormatRB2Bink.Instance)) {
					TaskMutex.WaitOne();
					TemporaryFormatData olddata = new TemporaryFormatData(data.Song, data.PlatformData);
					data.SaveTo(olddata, FormatType.Audio);
					TaskMutex.ReleaseMutex();

					audio = true;
					try {
						Platform.Transcode(FormatType.Audio, olddata, new IFormat[] { AudioFormatRB2Mogg.Instance }, destination, progress);
					} catch (UnsupportedTranscodeException exception) {
						Exceptions.Warning(exception, "Transcoding audio from " + data.Song.Name + " to Rock Band 2 format failed.");
					}
					olddata.Dispose();
				}

				progress.Progress(14);

				TaskMutex.WaitOne();
				data.SaveTo(destination, audio ? FormatType.Chart : FormatType.Unknown);
				TaskMutex.ReleaseMutex();

				progress.Progress();
				PlatformLocalStorage.Instance.SaveSong(Storage, destination, progress);

				progress.Progress();

				UpdateSongList();

				progress.EndTask();
			});
		}

		private void MenuSongsInstallSD_Click(object sender, EventArgs e)
		{
			foreach (FormatData data in GetSelectedSongData()) {
				QueueInstallSD(data);
			}
		}

		private void QueueInstallSD(FormatData data)
		{
			Progress.QueueTask(progress => {
				progress.NewTask("Installing \"" + data.Song.Name + "\" to SD", 10);
				FormatData source = data;

				TaskMutex.WaitOne();
				bool local = data.PlatformData.Platform == PlatformLocalStorage.Instance;
				bool ownformat = false;
				if (!local && Configuration.MaxConcurrentTasks > 1) {
					source = new TemporaryFormatData(data.Song, data.PlatformData);
					data.SaveTo(source);
					ownformat = true;
				}
				TaskMutex.ReleaseMutex();

				progress.Progress();

				FormatData dest = source;

				progress.SetNextWeight(10);

				if (!source.Formats.Contains(AudioFormatRB2Mogg.Instance) && !source.Formats.Contains(AudioFormatRB2Bink.Instance)) {
					if (local && !ownformat) {
						dest = new TemporaryFormatData(source.Song, source.PlatformData);
						source.SaveTo(dest, FormatType.Chart);
						ownformat = true;
					}
					Platform.Transcode(FormatType.Audio, source, new IFormat[] { AudioFormatRB2Mogg.Instance }, dest, progress);
				}

				progress.Progress(10);

				progress.SetNextWeight(3);

				PlatformRB2WiiCustomDLC.Instance.SaveSong(SD, dest, progress);
				progress.Progress(3);

				if (ownformat)
					dest.Dispose();

				UpdateSongList();

				progress.EndTask();
			});
		}

		private void MenuSongsExportRawkSD_Click(object sender, EventArgs e)
		{
			SaveDialog.Title = "Save RawkSD Archive";
			SaveDialog.Filter = "RawkSD Archive (*.rwk)|*.rwk|All Files|*";
			if (SaveDialog.ShowDialog(this) == DialogResult.Cancel)
				return;

			Exports.ExportRWK(SaveDialog.FileName, GetSelectedSongData());
		}

		private void MenuSongsExportRBN_Click(object sender, EventArgs e)
		{
			var songs = GetSelectedSongData();
			if (songs.Count() > 1) {
				return;
			}

			FormatData data = songs.Single();

			SaveDialog.Title = "Save Rock Band Network Archive";
			SaveDialog.Filter = "Rock Band Audition File (*.rba)|*.rba|All Files|*";
			SaveDialog.FileName = data.Song.ID;
			if (SaveDialog.ShowDialog(this) == DialogResult.Cancel)
				return;

			Exports.ExportRBN(SaveDialog.FileName, data);
		}

		private void MenuSongsExport360DLC_Click(object sender, EventArgs e)
		{
			var songs = GetSelectedSongData();
			if (songs.Count() > 1) {
				return;
			}

			FormatData data = songs.Single();

			SaveDialog.Title = "Save Rock Band 360 DLC";
			SaveDialog.Filter = "Rock Band DLC File (*.dlc)|*.dlc|All Files|*";
			SaveDialog.FileName = data.Song.ID;
			if (SaveDialog.ShowDialog(this) == DialogResult.Cancel)
				return;

			Exports.Export360(SaveDialog.FileName, data);
		}

		private void MenuSongsExportFoF_Click(object sender, EventArgs e)
		{
			FolderDialog.Description = "Select your Frets on Fire songs folder.";
			if (FolderDialog.ShowDialog(this) == DialogResult.Cancel)
				return;

			Exports.ExportFoF(FolderDialog.SelectedPath, GetSelectedSongData());
		}

		private void MenuSongsEdit_Click(object sender, EventArgs e)
		{
			foreach (FormatData data in GetSelectedSongData()) {
				EditForm.Show(data, data.PlatformData == Storage, false, this);
			}

			SongList_SelectedIndexChanged(SongList, EventArgs.Empty);
		}

		private void MenuSongsDelete_Click(object sender, EventArgs e)
		{
			List<FormatData> data = new List<FormatData>();
			foreach (SongListItem item in GetSelectedSongs()) {
				data.AddRange(item.Data);
			}
			QueueDelete(data);
		}

		private void QueueDelete(IList<FormatData> datalist)
		{
			if (datalist.Select(f => f.PlatformData.Platform).Distinct().Count() > 1) {
				datalist = DeleteForm.Show(datalist, this);
			}

			if (datalist.Count == 0)
				return;

			GetAsyncProgress().QueueTask(progress => {
				progress.NewTask("Deleting Songs", datalist.Count);
				foreach (FormatData data in datalist) {
					data.PlatformData.Platform.DeleteSong(data.PlatformData, data, progress);
					progress.Progress();
				}
				UpdateSongList();
				progress.EndTask();
			});
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
					progress.NewTask("Installing \"" + data.Song.Name + "\" locally", 2);

					FormatData localdata = PlatformLocalStorage.Instance.CreateSong(Storage, data.Song);
					data.SaveTo(localdata);
					data.Dispose();
					progress.Progress();

					PlatformLocalStorage.Instance.SaveSong(Storage, localdata, progress);
					progress.Progress();

					UpdateSongList();
					progress.EndTask();
				});
			} else
				data.Dispose();
		}

		private void MenuSongsSelectAll_Click(object sender, EventArgs e)
		{
			SongList.SelectedIndices.Clear();
			for (int i = 0; i < SongList.Items.Count; i++)
				SongList.SelectedIndices.Add(i);
		}

		private void MenuFilePreferences_Click(object sender, EventArgs e)
		{
			int tasks = Configuration.MaxConcurrentTasks;
			SettingsForm.Show(this);
			for (int i = tasks; i < Configuration.MaxConcurrentTasks; i++)
				AddTaskScheduler();
			for (int i = Configuration.MaxConcurrentTasks; i < tasks; i++)
				RemoveTaskScheduler(ProgressList.OrderBy(p => p.Tasks.Count).LastOrDefault());
		}

		private void MenuSongsRefresh_Click(object sender, EventArgs e)
		{
			List<FormatData> list = new List<FormatData>();
			foreach (var song in GetSelectedSongs())
				list.AddRange(song.Data);
			if (list.Count == 0) {
				if (Storage != null)
					list.AddRange(Storage.Songs);
				foreach (PlatformData platform in Platforms)
					list.AddRange(platform.Songs);
			}
			QueueRefresh(list);
		}

		private void QueueRefresh(List<FormatData> list)
		{
			Progress.QueueTask(progress => {
				progress.NewTask("Refreshing Song Data", list.Count);
				Dictionary<Game, ImportMap> Maps = new Dictionary<Game, ImportMap>();
				foreach (FormatData data in list) {
					data.PlatformData.Mutex.WaitOne();
					Game game = data.Song.Game;
					if (game != Game.Unknown) {
						if (!Maps.ContainsKey(game))
							Maps[game] = new ImportMap(data.Song.Game);
						Maps[game].Populate(data.Song);
					}
					data.PlatformData.Mutex.ReleaseMutex();
					progress.Progress();
				}
				UpdateSongList();
				progress.EndTask();
			});
		}

		private void MenuSongsTranscode_Click(object sender, EventArgs e)
		{
			foreach (FormatData data in GetSelectedSongData())
				QueueTranscodeRB2(data);
		}

		private void QueueTranscodeRB2(FormatData data)
		{
			Progress.QueueTask(progress => {
				progress.NewTask("Transcoding \"" + data.Song.Name + "\" to Rock Band 2 format", 1);

				if (data.PlatformData != Storage)
					Exceptions.Error("Cannot transcode " + data.Song.Name + " because it isn't stored locally.");

				IList<IFormat> formats = data.Formats;
				if (!formats.Contains(AudioFormatRB2Mogg.Instance) && !formats.Contains(AudioFormatRB2Bink.Instance)) {
					Platform.Transcode(FormatType.Audio, data, new IFormat[] { AudioFormatRB2Mogg.Instance }, data, progress);

					foreach (IFormat format in formats) {
						if (format.Type == FormatType.Audio) {
							foreach (string name in data.GetStreamNames(format))
								data.DeleteStream(name);
						}
					}
				}
				progress.Progress();
				
				progress.EndTask();
			});
		}

		private void StatusRiifsLabel_Click(object sender, EventArgs e)
		{
			ToggleRiiFS();
		}

		private void ContextMenuRefresh_Click(object sender, EventArgs e)
		{
			MenuSongsRefresh_Click(sender, e);
		}

		private void ToolbarOpenSD_Click(object sender, EventArgs e)
		{
			MenuFileOpenSD_Click(sender, e);
		}

		private void ToolbarOpen_Click(object sender, EventArgs e)
		{
			MenuFileOpen_Click(sender, e);
		}

		private void ToolbarOpenFolder_Click(object sender, EventArgs e)
		{
			MenuFileOpenFolder_Click(sender, e);
		}

		private void ToolbarNew_Click(object sender, EventArgs e)
		{
			MenuSongsCreate_Click(sender, e);
		}

		private void ToolbarEdit_Click(object sender, EventArgs e)
		{
			MenuSongsEdit_Click(sender, e);
		}

		private void ToolbarDelete_Click(object sender, EventArgs e)
		{
			MenuSongsDelete_Click(sender, e);
		}

		private void ToolbarInstallLocal_Click(object sender, EventArgs e)
		{
			MenuSongsInstallLocal_Click(sender, e);
		}

		private void ToolbarInstallSD_Click(object sender, EventArgs e)
		{
			MenuSongsInstallSD_Click(sender, e);
		}

		private void MenuHelpDonate_Click(object sender, EventArgs e)
		{
			new Process() { StartInfo = new ProcessStartInfo("http://japaneatahand.com/rawksd/donate.htm") }.Start();
		}

		private void StatusContactLabel_Click(object sender, EventArgs e)
		{
			new Process() { StartInfo = new ProcessStartInfo("mailto:rawksd@japaneatahand.com") }.Start();
		}

		private void MenuHelpGuide_Click(object sender, EventArgs e)
		{
			new Process() { StartInfo = new ProcessStartInfo("http://rawksd.japaneatahand.com/wiki/Installation") }.Start();
		}

		private void MenuHelpAbout_Click(object sender, EventArgs e)
		{
			MessageBox.Show(this, Text + "\n" +
				"A Rock Band 2 Wii Customs tool developed by the RawkSD Team: " +
				"tueidj, Aaron, and Mrkinator." +
				"\nFor more information, visit http://rawksd.japaneatahand.com/" +
				"\n\nQuestions, comments, concerns? Email us at rawksd@japaneatahand.com\nYou can also visit us on IRC: #RawkSD on irc.EFnet.org" +
				"\n\nSpecial thanks goes out to the following people, who have also contributed to RawkSD:\n" +
				" - nanook for his work on Queen Bee and general Guitar Hero hacking\n" +
				" - GameZelda for his quick Guitar Hero hacking skills\n" +
				" - Szalkow, BHK, SFenton, and tw3nz0r for their song tagging work\n" +
				" - Everyone else on IRC for being such great testers, and for putting up with the recent lack of beta builds :)", "RawkSD", MessageBoxButtons.OK, MessageBoxIcon.Information);
		}

		private void StatusDonateLabel_Click(object sender, EventArgs e)
		{
			MenuHelpDonate_Click(sender, e);
		}

		private void ContextMenuTranscode_Click(object sender, EventArgs e)
		{
			MenuSongsTranscode_Click(sender, e);
		}
	}
}
