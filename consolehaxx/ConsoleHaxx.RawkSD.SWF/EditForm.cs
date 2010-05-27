using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using ConsoleHaxx.Harmonix;
using System.IO;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.RawkSD.SWF
{
	public partial class EditForm : Form
	{
		public const uint OggMagic = 0x4f676753;

		public FormatData Data;
		public SongData Song;
		public bool Writable;

		public EditForm()
		{
			InitializeComponent();

			GenreCombo.Items.AddRange(ImportMap.Genres.Select(g => g.Value).ToArray());
			GameCombo.Items.AddRange(Platform.GetGames().Select(g => Platform.GameName(g)).ToArray());

			AnimationTempoCombo.SelectedIndex = 1;
			BankCombo.SelectedIndex = 0;
		}

		public static DialogResult Show(FormatData data, bool writable, bool cancellable, Form parent)
		{
			EditForm form = new EditForm();
			form.Data = data;
			form.Writable = writable;
			if (!cancellable) {
				form.CancelButton = form.CloseButton;
				form.ButtonCancel.Visible = false;
			}
			return form.ShowDialog(parent);
		}

		private void EditForm_Load(object sender, EventArgs e)
		{
			SongData song = Data.Song;

			if (!Writable) {
				IdText.ReadOnly = true;
				NameText.ReadOnly = true;
				ArtistText.ReadOnly = true;
				AlbumText.ReadOnly = true;
				TrackNumeric.Enabled = false;
				YearNumeric.Enabled = false;
				GenreCombo.Enabled = false;
				VocalistCombo.Enabled = false;
				MasterCheckbox.Enabled = false;

				HopoNumeric.Enabled = false;
				GameCombo.Enabled = false;
				PackText.ReadOnly = true;
				VersionNumeric.Enabled = false;
				PreviewStartNumeric.Enabled = false;
				PreviewEndNumeric.Enabled = false;

				AnimationTempoCombo.Enabled = false;
				ScrollSpeedNumeric.Enabled = false;
				BankCombo.Enabled = false;

				BandTierNumeric.Enabled = false;
				GuitarTierNumeric.Enabled = false;
				DrumsTierNumeric.Enabled = false;
				VocalsTierNumeric.Enabled = false;
				BassTierNumeric.Enabled = false;

				ReplaceChartButton.Enabled = false;

				ReplaceAudioButton.Enabled = false;
			}

			// Song Info
			IdText.Text = song.ID;
			NameText.Text = song.Name;
			ArtistText.Text = song.Artist;
			AlbumText.Text = song.Album;
			TrackNumeric.Value = song.AlbumTrack;
			YearNumeric.Value = song.Year;
			GenreCombo.Text = song.TidyGenre;
			VocalistCombo.Text = song.Vocalist;
			MasterCheckbox.Checked = song.Master;
			Bitmap albumart = song.AlbumArt;
			if (albumart != null)
				AlbumArt.Image = albumart;
			else
				AlbumArt.Image = Program.Form.AlbumImage;

			// Engine Data
			HopoNumeric.Value = song.HopoThreshold;
			GameCombo.Text = Platform.GameName(song.Game);
			PackText.Text = song.Pack;
			VersionNumeric.Value = song.Version;
			PreviewStartNumeric.Value = song.PreviewTimes[0];
			PreviewEndNumeric.Value = song.PreviewTimes[1];

			// RB2 Data
			SongsDTA dta = HarmonixMetadata.GetSongsDTA(song);
			if (dta != null) {
				ScrollSpeedNumeric.Value = dta.SongScrollSpeed;
				switch (dta.AnimTempo) {
					case 0x10:
						AnimationTempoCombo.SelectedIndex = 0;
						break;
					case 0x20:
						AnimationTempoCombo.SelectedIndex = 1;
						break;
					case 0x40:
						AnimationTempoCombo.SelectedIndex = 2;
						break;
					default:
						throw new FormatException();
				}
				switch (dta.Bank) {
					case "sfx/tambourine_bank.milo":
						BankCombo.SelectedIndex = 0;
						break;
					case "sfx/cowbell_bank.milo":
						BankCombo.SelectedIndex = 1;
						break;
					case "sfx/triangle_bank.milo":
						BankCombo.SelectedIndex = 2;
						break;
					case "sfx/handclap_bank.milo":
						BankCombo.SelectedIndex = 3;
						break;
					default:
						throw new FormatException();
				}
			}
			BandTierNumeric.Value = song.Difficulty[Instrument.Ambient];
			GuitarTierNumeric.Value = song.Difficulty[Instrument.Guitar];
			DrumsTierNumeric.Value = song.Difficulty[Instrument.Drums];
			VocalsTierNumeric.Value = song.Difficulty[Instrument.Vocals];
			BassTierNumeric.Value = song.Difficulty[Instrument.Bass];

			// Chart
			IFormat chartformat = Data.GetFormatAny(FormatType.Chart);
			if (chartformat != null) {
				ChartNameLabel.Text = chartformat.Name;
				ChartSizeLabel.Text = CalculateFormatSize(chartformat);
			}

			// Audio
			IFormat audioformat = Data.GetFormatAny(FormatType.Audio);
			if (audioformat != null) {
				AudioNameLabel.Text = audioformat.Name;
				AudioSizeLabel.Text = CalculateFormatSize(audioformat);
			}

			Song = song;
		}

		private string CalculateFormatSize(IFormat chartformat)
		{
			long size = 0;
			foreach (Stream stream in Data.GetStreams(chartformat)) {
				size += stream.Length;
				Data.CloseStream(stream);
			}
			return Util.FileSizeToString(size);
		}

		private void AlbumArt_Click(object sender, EventArgs e)
		{
			if (!Writable || Song == null)
				return;

			OpenDialog.Multiselect = false;
			OpenDialog.Title = "Select album art";
			OpenDialog.Filter = "Common Image Files|*.png;*.jpg;*.bmp|All Files|*";
			if (OpenDialog.ShowDialog(this) == DialogResult.Cancel)
				return;

			Bitmap image = null;

			try {
				image = new Bitmap(OpenDialog.FileName);
			} catch { }
			if (image != null)
				Song.AlbumArt = image;
			AlbumArt.Image = image;
		}

		private void BandTierNumeric_ValueChanged(object sender, EventArgs e)
		{
			BandTierPicture.Image = Program.Form.Tiers[ImportMap.GetBaseTier(Instrument.Ambient, (int)BandTierNumeric.Value)];
			if (Song != null)
				Song.Difficulty[Instrument.Ambient] = (int)BandTierNumeric.Value;
		}

		private void GuitarTierNumeric_ValueChanged(object sender, EventArgs e)
		{
			GuitarTierPicture.Image = Program.Form.Tiers[ImportMap.GetBaseTier(Instrument.Guitar, (int)GuitarTierNumeric.Value)];
			if (Song != null)
				Song.Difficulty[Instrument.Guitar] = (int)GuitarTierNumeric.Value;
		}

		private void DrumsTierNumeric_ValueChanged(object sender, EventArgs e)
		{
			DrumsTierPicture.Image = Program.Form.Tiers[ImportMap.GetBaseTier(Instrument.Drums, (int)DrumsTierNumeric.Value)];
			if (Song != null)
				Song.Difficulty[Instrument.Drums] = (int)DrumsTierNumeric.Value;
		}

		private void VocalsTierNumeric_ValueChanged(object sender, EventArgs e)
		{
			VocalsTierPicture.Image = Program.Form.Tiers[ImportMap.GetBaseTier(Instrument.Vocals, (int)VocalsTierNumeric.Value)];
			if (Song != null)
				Song.Difficulty[Instrument.Vocals] = (int)VocalsTierNumeric.Value;
		}

		private void BassTierNumeric_ValueChanged(object sender, EventArgs e)
		{
			BassTierPicture.Image = Program.Form.Tiers[ImportMap.GetBaseTier(Instrument.Bass, (int)BassTierNumeric.Value)];
			if (Song != null)
				Song.Difficulty[Instrument.Bass] = (int)BassTierNumeric.Value;
		}

		private void IdText_TextChanged(object sender, EventArgs e)
		{
			if (IdText.Text.Length == 0) {
				ErrorProvider.SetError(LabelID, "Songs must have an ID assigned to them.");
				return;
			} else
				ErrorProvider.SetError(LabelID, null);

			if (Song != null)
				Song.ID = IdText.Text;
		}

		private void NameText_TextChanged(object sender, EventArgs e)
		{
			if (Song != null)
				Song.Name = NameText.Text;
		}

		private void ArtistText_TextChanged(object sender, EventArgs e)
		{
			if (Song != null)
				Song.Artist = ArtistText.Text;
		}

		private void AlbumText_TextChanged(object sender, EventArgs e)
		{
			if (Song != null)
				Song.Album = AlbumText.Text;
		}

		private void TrackNumeric_ValueChanged(object sender, EventArgs e)
		{
			if (Song != null)
				Song.AlbumTrack = (int)TrackNumeric.Value;
		}

		private void YearNumeric_ValueChanged(object sender, EventArgs e)
		{
			if (Song != null)
				Song.Year = (int)YearNumeric.Value;
		}

		private void MasterCheckbox_CheckedChanged(object sender, EventArgs e)
		{
			if (Song != null)
				Song.Master = MasterCheckbox.Checked;
		}

		private void GenreCombo_SelectedIndexChanged(object sender, EventArgs e)
		{
			if (Song != null)
				Song.Genre = GenreCombo.SelectedItem.ToString();
		}

		private void VocalistCombo_SelectedIndexChanged(object sender, EventArgs e)
		{
			if (Song != null)
				Song.Vocalist = VocalistCombo.Text.ToLower();
		}

		private void HopoNumeric_ValueChanged(object sender, EventArgs e)
		{
			if (Song != null)
				Song.HopoThreshold = (int)HopoNumeric.Value;
		}

		private void PackText_TextChanged(object sender, EventArgs e)
		{
			if (Song != null)
				Song.Pack = PackText.Text;
		}

		private void GameCombo_SelectedIndexChanged(object sender, EventArgs e)
		{
			if (Song != null)
				throw new NotImplementedException();
		}

		private void VersionNumeric_ValueChanged(object sender, EventArgs e)
		{
			if (Song != null)
				Song.Version = (int)VersionNumeric.Value;
		}

		private void PreviewStartNumeric_ValueChanged(object sender, EventArgs e)
		{
			if (Song != null)
				Song.PreviewTimes[0] = (int)PreviewStartNumeric.Value;
		}

		private void PreviewEndNumeric_ValueChanged(object sender, EventArgs e)
		{
			if (Song != null)
				Song.PreviewTimes[1] = (int)PreviewEndNumeric.Value;
		}

		private void CloseButton_Click(object sender, EventArgs e)
		{
			DialogResult = DialogResult.OK;
			Close();
		}

		private void ReplaceChartButton_Click(object sender, EventArgs e)
		{
			OpenDialog.Multiselect = false;
			OpenDialog.Title = "Select a note chart file";
			OpenDialog.Filter = "Supported Files (*.mid, *.chart)|*.mid;*.chart|All Files|*";
			if (OpenDialog.ShowDialog(this) == DialogResult.Cancel)
				return;

			Stream stream = new FileStream(OpenDialog.FileName, FileMode.Open, FileAccess.Read, FileShare.Read);
			Mid mid = Mid.Create(stream);
			stream.Position = 0;
			Chart chart = mid == null ? Chart.Create(stream) : null;

			if (mid == null && chart == null) {
				stream.Close();
				throw new FormatException();
			}

			List<IChartFormat> formats = new List<IChartFormat>();
			if (mid != null) {
				formats.Add(ChartFormatRB.Instance);
				formats.Add(ChartFormatGH2.Instance);
			}
			if (chart != null) {
				formats.Add(ChartFormatChart.Instance);
			}

			ChartForm.ShowResult result = ChartForm.Show(formats, this);
			if (result.Result == DialogResult.Cancel)
				return;

			if (mid != null || chart != null) {
				foreach (IFormat format in Data.GetFormats(FormatType.Chart)) {
					foreach (string chartstream in Data.GetStreamNames(format))
						Data.DeleteStream(chartstream);
				}
			}

			Stream milo = null;
			Stream pan = null;
			Stream weights = null;

			FormatData data = new TemporaryFormatData(Data.Song);
			if (result.Format == ChartFormatRB.Instance) {
				if (result.Milo.HasValue())
					milo = new FileStream(result.Milo, FileMode.Open, FileAccess.Read, FileShare.Read);
				if (result.Weights.HasValue())
					weights = new FileStream(result.Weights, FileMode.Open, FileAccess.Read, FileShare.Read);
				if (result.Pan.HasValue())
					pan = new FileStream(result.Pan, FileMode.Open, FileAccess.Read, FileShare.Read);

				ChartFormatRB.Instance.Create(data, stream, pan, weights, milo, result.ExpertPlus, result.FixForQuickplay);
			} else if (result.Format == ChartFormatGH2.Instance) {
				ChartFormatGH2.Instance.Create(data, stream, result.Coop);
			} else if (result.Format == ChartFormatChart.Instance) {
				ChartFormatChart.Instance.Create(data, stream, result.Coop);
			}
			
			data.SaveTo(Data);

			stream.Close();

			if (milo != null)
				milo.Close();
			if (pan != null)
				pan.Close();
			if (weights != null)
				weights.Close();
		}

		private void ReplaceAudioButton_Click(object sender, EventArgs e)
		{
			OpenDialog.Multiselect = true;
			OpenDialog.Title = "Select audio files";
			OpenDialog.Filter = "Common Audio Files|*.ogg;*.wav;*.flac;*.mp3;*.aiff;*.ac3|All Files|*";
			if (OpenDialog.ShowDialog(this) == DialogResult.Cancel)
				return;

			bool allogg = true;

			using (DelayedStreamCache cache = new DelayedStreamCache()) {
				FormatData data = new TemporaryFormatData();
				AudioFormat format = new AudioFormat();
				List<string> names = new List<string>();
				Stream[] streams = new Stream[OpenDialog.FileNames.Length];
				for (int i = 0; i < streams.Length; i++) {
					string filename = OpenDialog.FileNames[i];
					streams[i] = new FileStream(filename, FileMode.Open, FileAccess.Read, FileShare.Read);

					cache.AddStream(streams[i]);
					IDecoder decoder;
					if (new EndianReader(streams[i], Endianness.BigEndian).ReadUInt32() != OggMagic) {
						allogg = false;
						streams[i].Position = 0;
						decoder = new FFmpegDecoder(streams[i]);
					} else {
						decoder = new RawkAudio.Decoder(streams[i], RawkAudio.Decoder.AudioFormat.VorbisOgg);
					}

					for (int k = 0; k < decoder.Channels; k++) {
						names.Add(Path.GetFileName(filename) + " [Channel #" + (k + 1).ToString("0") + "]");
						float balance = 0;
						if (decoder.Channels == 2)
							balance = k == 0 ? -1 : 1;
						format.Mappings.Add(new AudioFormat.Mapping(0, balance, Platform.InstrumentFromString(Path.GetFileNameWithoutExtension(filename))));
					}

					decoder.Dispose();
					streams[i].Position = 0;
				}

				if (AudioForm.Show(format, names.ToArray(), this) == DialogResult.Cancel)
					return;

				foreach (IFormat audioformat in Data.GetFormats(FormatType.Audio)) {
					foreach (string audiostream in Data.GetStreamNames(audioformat))
						Data.DeleteStream(audiostream);
				}

				if (allogg)
					AudioFormatOgg.Instance.Create(data, streams, format);
				else
					AudioFormatFFmpeg.Instance.Create(data, streams, format);
				data.SaveTo(Data);
			}
		}

		private void ButtonCancel_Click(object sender, EventArgs e)
		{
			Close();
		}

		private void AnimationTempoCombo_SelectedIndexChanged(object sender, EventArgs e)
		{
			if (Song == null)
				return;

			SongsDTA dta = GetOrCreateDTA();
			switch (AnimationTempoCombo.SelectedIndex) {
				case 0:
					dta.AnimTempo = 0x10;
					break;
				case 1:
					dta.AnimTempo = 0x20;
					break;
				case 2:
					dta.AnimTempo = 0x40;
					break;
				default:
					return;
			}
			SaveDTA(dta);
		}

		private void SaveDTA(SongsDTA dta)
		{
			HarmonixMetadata.SetSongsDTA(Song, dta.ToDTB());
		}

		private SongsDTA GetOrCreateDTA()
		{
			SongsDTA dta = HarmonixMetadata.GetSongsDTA(Song);
			return dta ?? new SongsDTA();
		}

		private void ScrollSpeedNumeric_ValueChanged(object sender, EventArgs e)
		{
			if (Song == null)
				return;

			SongsDTA dta = GetOrCreateDTA();
			dta.SongScrollSpeed = (int)ScrollSpeedNumeric.Value;
			SaveDTA(dta);
		}

		private void BankCombo_SelectedIndexChanged(object sender, EventArgs e)
		{
			if (Song == null)
				return;

			SongsDTA dta = GetOrCreateDTA();
			switch (BankCombo.SelectedIndex) {
				case 0:
					dta.Bank = "sfx/tambourine_bank.milo";
					break;
				case 1:
					dta.Bank = "sfx/cowbell_bank.milo";
					break;
				case 2:
					dta.Bank = "sfx/triangle_bank.milo";
					break;
				case 3:
					dta.Bank = "sfx/handclap_bank.milo";
					break;
				default:
					return;
			}
			SaveDTA(dta);
		}
	}
}
