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
	public partial class DeleteForm : Form
	{
		protected IList<Engine> Platforms;

		public DeleteForm()
		{
			InitializeComponent();
		}

		public static IList<FormatData> Show(IList<FormatData> data, Form parent)
		{
			DeleteForm form = new DeleteForm();
			form.Platforms = data.Select(f => f.PlatformData.Platform).Distinct().ToList();
			for (int i = 0; i < form.Platforms.Count; i++) {
				form.PlatformList.Items.Add(form.Platforms[i].Name);
				form.PlatformList.SelectedIndices.Add(i);
			}
			List<FormatData> list = new List<FormatData>();
			if (form.ShowDialog(parent) == DialogResult.Cancel)
				return list;
			foreach (int i in form.PlatformList.SelectedIndices) {
				list.AddRange(data.Where(f => f.PlatformData.Platform == form.Platforms[i]));
			}
			return list;
		}

		private void ButtonCancel_Click(object sender, EventArgs e)
		{
			Close();
		}

		private void CloseButton_Click(object sender, EventArgs e)
		{
			DialogResult = DialogResult.OK;
			Close();
		}
	}
}
