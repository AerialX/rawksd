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
		public IList<Engine> Platforms;

		public PlatformForm()
		{
			InitializeComponent();

			DialogResult = DialogResult.Cancel;
		}

		private void PlatformForm_Load(object sender, EventArgs e)
		{
			foreach (Engine engine in Platforms)
				PlatformList.Items.Add(engine);

			PlatformList.SelectedIndex = 0;
		}

		public static PlatformFormReturn Show(string message, IList<Engine> platforms, Form parent)
		{
			PlatformForm form = new PlatformForm();
			form.Platforms = platforms;
			form.MessageLabel.Text = message;
			PlatformFormReturn data = new PlatformFormReturn();
			data.Result = form.ShowDialog(parent);
			if (data.Result == DialogResult.OK)
				data.Platform = form.PlatformList.SelectedItem as Engine;

			return data;
		}

		public struct PlatformFormReturn
		{
			public DialogResult Result { get; set; }
			public Engine Platform { get; set; }
		}

		private void OpenFileButton_Click(object sender, EventArgs e)
		{
			DialogResult = DialogResult.OK;
			Close();
		}

		private void CancelButton_Click(object sender, EventArgs e)
		{
			Close();
		}
	}
}
