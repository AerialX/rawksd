using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Drawing;

namespace ConsoleHaxx.RawkSD.SWF
{
	public class ProgressItem : Control
	{
		public ProgressBar ProgressBar;
		public Label Label;
		public ToolTip ToolTip;

		public int Maximum { get { return ProgressBar.Maximum; } set { ProgressBar.Maximum = value; } }
		public int Minimum { get { return ProgressBar.Minimum; } set { ProgressBar.Minimum = value; } }
		public int Value { get { return ProgressBar.Value; } set { ProgressBar.Value = value; } }

		public override string Text { get { return Label.Text; } set { Label.Text = value; AutoResize(); } }
		public string ToolTipText { get { return ToolTip.GetToolTip(this); } set { ToolTip.SetToolTip(this, value); ToolTip.SetToolTip(ProgressBar, value); ToolTip.SetToolTip(Label, value); } }

		public ProgressItem()
		{
			ProgressBar = new ProgressBar();
			Label = new Label();
			ToolTip = new ToolTip();

			DoubleBuffered = true;

			Label.UseMnemonic = false;
			Label.AutoSize = false;
			Label.Dock = DockStyle.Top;
			Label.TextAlign = ContentAlignment.MiddleCenter;
			Label.Padding = new Padding(3, 0, 0, 3);
			ProgressBar.Style = ProgressBarStyle.Continuous;
			ProgressBar.Dock = DockStyle.Fill;
			ProgressBar.Margin = new Padding(0, 0, 0, 0);

			Controls.Add(Label);
			Controls.Add(ProgressBar);

			ProgressBar.BringToFront();

			Padding = new Padding(0, 0, 0, 0);
			Margin = new Padding(0, 0, 0, 0);

			AutoResize();
		}

		public void AutoResize()
		{
			Label.Height = Label.PreferredHeight;
			MinimumSize = new Size(64, Label.PreferredHeight + 18);
			PreferredWidth = Math.Max(64, Label.PreferredWidth + 4);
			Size = new Size(PreferredWidth, MinimumSize.Height);
		}

		public int PreferredWidth { get; protected set; }
	}
}
