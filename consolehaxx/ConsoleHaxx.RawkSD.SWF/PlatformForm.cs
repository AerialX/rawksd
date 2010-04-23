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
	public partial class PlatformForm : Form
	{
		protected PlatformFormReturn.PlatformFormReturnEnum Selection { get; set; }

		public PlatformForm()
		{
			InitializeComponent();
		}

		private void PlatformForm_Load(object sender, EventArgs e)
		{
			foreach (Engine engine in Platform.Platforms)
				PlatformList.Items.Add(engine);

			PlatformList.SelectedIndex = 0;
		}

		public static PlatformFormReturn Show(Form parent)
		{
			PlatformForm form = new PlatformForm();
			PlatformFormReturn data = new PlatformFormReturn();
			data.Result = form.ShowDialog(parent);
			if (data.Result == DialogResult.OK) {
				data.Platform = form.PlatformList.SelectedItem as Engine;
				data.Type = form.Selection;
			}

			return data;
		}

		public struct PlatformFormReturn
		{
			public DialogResult Result { get; set; }
			public PlatformFormReturnEnum Type { get; set; }
			public Engine Platform { get; set; }

			public enum PlatformFormReturnEnum
			{
				None = 0,
				Folder,
				File
			}
		}

		private void OpenFolderButton_Click(object sender, EventArgs e)
		{
			Selection = PlatformFormReturn.PlatformFormReturnEnum.Folder;
			DialogResult = DialogResult.OK;
			Close();
		}

		private void OpenFileButton_Click(object sender, EventArgs e)
		{
			Selection = PlatformFormReturn.PlatformFormReturnEnum.File;
			DialogResult = DialogResult.OK;
			Close();
		}

		private void CancelButton_Click(object sender, EventArgs e)
		{
			DialogResult = DialogResult.Cancel;
			Close();
		}
	}
}
