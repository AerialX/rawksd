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
	public partial class ChartForm : Form
	{
		public struct ShowResult
		{
			public DialogResult Result;
			public IChartFormat Format;
			public bool Coop;
			public bool ExpertPlus;
			public bool FixForQuickplay;
			public string Milo;
			public string Weights;
			public string Pan;
		}

		public IList<IChartFormat> Formats;

		public ChartForm()
		{
			InitializeComponent();
		}

		public static ShowResult Show(IList<IChartFormat> formats, Form parent)
		{
			ChartForm form = new ChartForm();
			form.Formats = formats;
			ShowResult result = new ShowResult();
			result.Result = form.ShowDialog(parent);
			result.Format = form.ChartCombo.SelectedItem as IChartFormat;
			result.Coop = form.CoopCheckbox.Checked;
			result.ExpertPlus = form.ExpertPlusCheckbox.Checked;
			result.FixForQuickplay = form.FixCheckbox.Checked;
			result.Milo = form.MiloTextbox.Text;
			result.Weights = form.WeightsTextbox.Text;
			result.Pan = form.PanTextbox.Text;
			return result;
		}

		private void ChartForm_Load(object sender, EventArgs e)
		{
			ChartCombo.Items.AddRange(Formats.ToArray());
			ChartCombo.SelectedIndex = 0;
		}

		private void MiloBrowseButton_Click(object sender, EventArgs e)
		{
			Browse(MiloTextbox, "Select Milo File", "Milo Files (*.milo_wii)|*.milo_wii");
		}

		private void WeightsBrowseButton_Click(object sender, EventArgs e)
		{
			Browse(WeightsTextbox, "Select Weights Bin File", "Weights Files (weights.bin)|*.bin");
		}

		private void PanBrowseButton_Click(object sender, EventArgs e)
		{
			Browse(PanTextbox, "Select Pan File", "Pan Files (*.pan)|*.pan");
		}

		private void Browse(TextBox textbox, string title, string filter)
		{
			OpenFile.Title = title;
			OpenFile.Filter = filter;
			if (OpenFile.ShowDialog(this) == DialogResult.Cancel)
				return;

			MiloTextbox.Text = OpenFile.FileName;
		}

		private void ChartCombo_SelectedIndexChanged(object sender, EventArgs e)
		{
			if (ChartCombo.SelectedItem == ChartFormatGH2.Instance) {
				ExpertPlusCheckbox.Visible = false;
				CoopCheckbox.Visible = true;
				FixCheckbox.Visible = false;
			} else if (ChartCombo.SelectedItem == ChartFormatRB.Instance) {
				CoopCheckbox.Visible = false;
				ExpertPlusCheckbox.Visible = true;
				FixCheckbox.Visible = true;

				MiloTextbox.Visible = true;
				LabelMilo.Visible = true;
				MiloBrowseButton.Visible = true;

				PanTextbox.Visible = true;
				LabelPan.Visible = true;
				PanBrowseButton.Visible = true;

				WeightsTextbox.Visible = true;
				LabelWeights.Visible = true;
				WeightsBrowseButton.Visible = true;
			} else if (ChartCombo.SelectedItem == ChartFormatChart.Instance) {
				ExpertPlusCheckbox.Visible = false;
				CoopCheckbox.Visible = true;
				FixCheckbox.Visible = false;
			}

			if (ChartCombo.SelectedItem != ChartFormatRB.Instance) {
				MiloTextbox.Visible = false;
				LabelMilo.Visible = false;
				MiloBrowseButton.Visible = false;

				PanTextbox.Visible = false;
				LabelPan.Visible = false;
				PanBrowseButton.Visible = false;

				WeightsTextbox.Visible = false;
				LabelWeights.Visible = false;
				WeightsBrowseButton.Visible = false;
			}
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
	}
}
