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
	public partial class SettingsForm : Form
	{
		public SettingsForm()
		{
			InitializeComponent();
		}

		public static void Show(Form parent)
		{
			SettingsForm form = new SettingsForm();
			form.ShowDialog(parent);
		}

		private void CloseButton_Click(object sender, EventArgs e)
		{
			Close();
		}

		private void SettingsForm_Load(object sender, EventArgs e)
		{
			CustomsText.Text = Configuration.LocalPath;
			TemporaryText.Text = Configuration.TemporaryPath;
			TasksNumeric.Value = Configuration.MaxConcurrentTasks;
			PerformanceCombo.SelectedIndex = Configuration.MemorySongData ? 0 : 1;
			switch (Configuration.DefaultAction) {
				case Configuration.DefaultActionType.InstallLocal:
					DefaultActionCombo.SelectedIndex = 1;
					break;
				case Configuration.DefaultActionType.InstallSD:
					DefaultActionCombo.SelectedIndex = 0;
					break;
				case Configuration.DefaultActionType.ExportRawk:
					DefaultActionCombo.SelectedIndex = 2;
					break;
				case Configuration.DefaultActionType.ExportRBA:
					DefaultActionCombo.SelectedIndex = 3;
					break;
				case Configuration.DefaultActionType.ExportFoF:
					DefaultActionCombo.SelectedIndex = 4;
					break;
				default:
					DefaultActionCombo.SelectedIndex = -1;
					break;
			}

			NamePrefixCombo.SelectedIndex = (int)Configuration.NamePrefix;
			GH5ExpertPlusCheckbox.Checked = Configuration.ExpertPlusGH5;
			if (Configuration.LocalTranscode)
				LocalStorageCombo.SelectedIndex = 1;
			else
				LocalStorageCombo.SelectedIndex = 0;
		}

		private void CustomsText_TextChanged(object sender, EventArgs e)
		{
			Configuration.LocalPath = CustomsText.Text;
		}

		private void TemporaryText_TextChanged(object sender, EventArgs e)
		{
			Configuration.TemporaryPath = TemporaryText.Text;
		}

		private void TasksNumeric_ValueChanged(object sender, EventArgs e)
		{
			Configuration.MaxConcurrentTasks = (int)TasksNumeric.Value;
		}

		private void PerformanceCombo_SelectedIndexChanged(object sender, EventArgs e)
		{
			Configuration.MemorySongData = PerformanceCombo.SelectedIndex == 0 ? true : false;
		}

		private void DefaultActionCombo_SelectedIndexChanged(object sender, EventArgs e)
		{
			switch (DefaultActionCombo.SelectedIndex) {
				case 0:
					Configuration.DefaultAction = Configuration.DefaultActionType.InstallSD;
					break;
				case 1:
					Configuration.DefaultAction = Configuration.DefaultActionType.InstallLocal;
					break;
				case 2:
					Configuration.DefaultAction = Configuration.DefaultActionType.ExportRawk;
					break;
				case 3:
					Configuration.DefaultAction = Configuration.DefaultActionType.ExportRBA;
					break;
				case 4:
					Configuration.DefaultAction = Configuration.DefaultActionType.ExportFoF;
					break;
			}
		}

		private void LocalStorageCombo_SelectedIndexChanged(object sender, EventArgs e)
		{
			switch (LocalStorageCombo.SelectedIndex) {
				case 0:
					Configuration.LocalTranscode = false;
					break;
				case 1:
					Configuration.LocalTranscode = true;
					break;
			}
		}

		private void NamePrefixCombo_SelectedIndexChanged(object sender, EventArgs e)
		{
			Configuration.NamePrefix = (ImportMap.NamePrefix)NamePrefixCombo.SelectedIndex;
		}

		private void GH5ExpertPlusCheckbox_CheckedChanged(object sender, EventArgs e)
		{
			Configuration.ExpertPlusGH5 = GH5ExpertPlusCheckbox.Checked;
		}
	}
}
