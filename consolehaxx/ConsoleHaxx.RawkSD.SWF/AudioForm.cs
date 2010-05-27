using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace ConsoleHaxx.RawkSD.SWF
{
	public partial class AudioForm : Form
	{
		public AudioFormat Format { get; set; }
		public string[] Names { get; set; }

		public AudioForm()
		{
			InitializeComponent();

			DialogResult = DialogResult.Cancel;
		}

		public static DialogResult Show(AudioFormat format, string[] names, Form parent)
		{
			AudioForm form = new AudioForm();
			form.Format = format;
			form.Names = names;
			return form.ShowDialog(parent);
		}

		private void AudioForm_Load(object sender, EventArgs e)
		{
			for (int i = 0; i < Format.Mappings.Count; i++)
				AudioList.Items.Add(Names[i]);

			AudioList.SelectedIndex = 0;
		}

		private void AudioList_SelectedIndexChanged(object sender, EventArgs e)
		{
			if (AudioList.SelectedIndex < 0) {
				VolumeNumeric.Enabled = false;
				BalanceNumeric.Enabled = false;
				BandOption.Enabled = false;
				GuitarOption.Enabled = false;
				VocalsOption.Enabled = false;
				BassOption.Enabled = false;
				DrumsOption.Enabled = false;
				return;
			}

			VolumeNumeric.Enabled = true;
			BalanceNumeric.Enabled = true;
			VolumeNumeric.Value = (decimal)Format.Mappings[AudioList.SelectedIndex].Volume;
			BalanceNumeric.Value = (decimal)Format.Mappings[AudioList.SelectedIndex].Balance;

			BandOption.Enabled = true;
			GuitarOption.Enabled = true;
			VocalsOption.Enabled = true;
			BassOption.Enabled = true;
			DrumsOption.Enabled = true;
			switch (Format.Mappings[AudioList.SelectedIndex].Instrument) {
				case Instrument.Ambient:
					BandOption.Checked = true;
					break;
				case Instrument.Guitar:
					GuitarOption.Checked = true;
					break;
				case Instrument.Bass:
					BassOption.Checked = true;
					break;
				case Instrument.Drums:
					DrumsOption.Checked = true;
					break;
				case Instrument.Vocals:
					VocalsOption.Checked = true;
					break;
			}
		}

		private void VolumeNumeric_ValueChanged(object sender, EventArgs e)
		{
			Format.Mappings[AudioList.SelectedIndex].Volume = (float)VolumeNumeric.Value;
		}

		private void BalanceNumeric_ValueChanged(object sender, EventArgs e)
		{
			Format.Mappings[AudioList.SelectedIndex].Balance = (float)BalanceNumeric.Value;
		}

		private void BandOption_CheckedChanged(object sender, EventArgs e)
		{
			if (BandOption.Checked)
				Format.Mappings[AudioList.SelectedIndex].Instrument = Instrument.Ambient;
		}

		private void GuitarOption_CheckedChanged(object sender, EventArgs e)
		{
			if (GuitarOption.Checked)
				Format.Mappings[AudioList.SelectedIndex].Instrument = Instrument.Guitar;
		}

		private void DrumsOption_CheckedChanged(object sender, EventArgs e)
		{
			if (DrumsOption.Checked)
				Format.Mappings[AudioList.SelectedIndex].Instrument = Instrument.Drums;
		}

		private void VocalsOption_CheckedChanged(object sender, EventArgs e)
		{
			if (VocalsOption.Checked)
				Format.Mappings[AudioList.SelectedIndex].Instrument = Instrument.Vocals;
		}

		private void BassOption_CheckedChanged(object sender, EventArgs e)
		{
			if (BassOption.Checked)
				Format.Mappings[AudioList.SelectedIndex].Instrument = Instrument.Bass;
		}

		private void ButtonCancel_Click(object sender, EventArgs e)
		{
			Close();
		}

		private void ButtonOK_Click(object sender, EventArgs e)
		{
			DialogResult = DialogResult.OK;
			Close();
		}

		private void SilenceNumeric_ValueChanged(object sender, EventArgs e)
		{
			Format.InitialOffset = -(int)(SilenceNumeric.Value * 1000);
		}
	}
}
