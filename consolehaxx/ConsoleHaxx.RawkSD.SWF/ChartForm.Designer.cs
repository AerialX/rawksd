namespace ConsoleHaxx.RawkSD.SWF
{
	partial class ChartForm
	{
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.IContainer components = null;

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		/// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
		protected override void Dispose(bool disposing)
		{
			if (disposing && (components != null)) {
				components.Dispose();
			}
			base.Dispose(disposing);
		}

		#region Windows Form Designer generated code

		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			this.MainLayout = new System.Windows.Forms.TableLayoutPanel();
			this.LabelChartType = new System.Windows.Forms.Label();
			this.ChartCombo = new System.Windows.Forms.ComboBox();
			this.CoopCheckbox = new System.Windows.Forms.CheckBox();
			this.ExpertPlusCheckbox = new System.Windows.Forms.CheckBox();
			this.MiloBrowseButton = new System.Windows.Forms.Button();
			this.WeightsBrowseButton = new System.Windows.Forms.Button();
			this.PanBrowseButton = new System.Windows.Forms.Button();
			this.MiloTextbox = new System.Windows.Forms.TextBox();
			this.WeightsTextbox = new System.Windows.Forms.TextBox();
			this.PanTextbox = new System.Windows.Forms.TextBox();
			this.LabelOptions = new System.Windows.Forms.Label();
			this.LabelMilo = new System.Windows.Forms.Label();
			this.LabelWeights = new System.Windows.Forms.Label();
			this.LabelPan = new System.Windows.Forms.Label();
			this.ButtonLayout = new System.Windows.Forms.FlowLayoutPanel();
			this.ButtonOK = new System.Windows.Forms.Button();
			this.ButtonCancel = new System.Windows.Forms.Button();
			this.OpenFile = new System.Windows.Forms.OpenFileDialog();
			this.MainLayout.SuspendLayout();
			this.ButtonLayout.SuspendLayout();
			this.SuspendLayout();
			// 
			// MainLayout
			// 
			this.MainLayout.ColumnCount = 3;
			this.MainLayout.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
			this.MainLayout.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
			this.MainLayout.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
			this.MainLayout.Controls.Add(this.LabelChartType, 0, 0);
			this.MainLayout.Controls.Add(this.ChartCombo, 1, 0);
			this.MainLayout.Controls.Add(this.CoopCheckbox, 1, 1);
			this.MainLayout.Controls.Add(this.ExpertPlusCheckbox, 1, 2);
			this.MainLayout.Controls.Add(this.MiloBrowseButton, 2, 3);
			this.MainLayout.Controls.Add(this.WeightsBrowseButton, 2, 4);
			this.MainLayout.Controls.Add(this.PanBrowseButton, 2, 5);
			this.MainLayout.Controls.Add(this.MiloTextbox, 1, 3);
			this.MainLayout.Controls.Add(this.WeightsTextbox, 1, 4);
			this.MainLayout.Controls.Add(this.PanTextbox, 1, 5);
			this.MainLayout.Controls.Add(this.LabelOptions, 0, 1);
			this.MainLayout.Controls.Add(this.LabelMilo, 0, 3);
			this.MainLayout.Controls.Add(this.LabelWeights, 0, 4);
			this.MainLayout.Controls.Add(this.LabelPan, 0, 5);
			this.MainLayout.Controls.Add(this.ButtonLayout, 0, 6);
			this.MainLayout.Dock = System.Windows.Forms.DockStyle.Fill;
			this.MainLayout.Location = new System.Drawing.Point(0, 0);
			this.MainLayout.Name = "MainLayout";
			this.MainLayout.RowCount = 7;
			this.MainLayout.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.MainLayout.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.MainLayout.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.MainLayout.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.MainLayout.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.MainLayout.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.MainLayout.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.MainLayout.Size = new System.Drawing.Size(292, 192);
			this.MainLayout.TabIndex = 0;
			// 
			// LabelChartType
			// 
			this.LabelChartType.AutoSize = true;
			this.LabelChartType.Location = new System.Drawing.Point(3, 0);
			this.LabelChartType.Name = "LabelChartType";
			this.LabelChartType.Padding = new System.Windows.Forms.Padding(0, 4, 0, 0);
			this.LabelChartType.Size = new System.Drawing.Size(62, 17);
			this.LabelChartType.TabIndex = 0;
			this.LabelChartType.Text = "Chart Type:";
			// 
			// ChartCombo
			// 
			this.MainLayout.SetColumnSpan(this.ChartCombo, 2);
			this.ChartCombo.Dock = System.Windows.Forms.DockStyle.Top;
			this.ChartCombo.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
			this.ChartCombo.FormattingEnabled = true;
			this.ChartCombo.Location = new System.Drawing.Point(71, 3);
			this.ChartCombo.Name = "ChartCombo";
			this.ChartCombo.Size = new System.Drawing.Size(218, 21);
			this.ChartCombo.TabIndex = 1;
			this.ChartCombo.SelectedIndexChanged += new System.EventHandler(this.ChartCombo_SelectedIndexChanged);
			// 
			// CoopCheckbox
			// 
			this.CoopCheckbox.AutoSize = true;
			this.MainLayout.SetColumnSpan(this.CoopCheckbox, 2);
			this.CoopCheckbox.Location = new System.Drawing.Point(71, 30);
			this.CoopCheckbox.Name = "CoopCheckbox";
			this.CoopCheckbox.Size = new System.Drawing.Size(51, 17);
			this.CoopCheckbox.TabIndex = 3;
			this.CoopCheckbox.Text = "Coop";
			this.CoopCheckbox.UseVisualStyleBackColor = true;
			// 
			// ExpertPlusCheckbox
			// 
			this.ExpertPlusCheckbox.AutoSize = true;
			this.MainLayout.SetColumnSpan(this.ExpertPlusCheckbox, 2);
			this.ExpertPlusCheckbox.Location = new System.Drawing.Point(71, 53);
			this.ExpertPlusCheckbox.Name = "ExpertPlusCheckbox";
			this.ExpertPlusCheckbox.Size = new System.Drawing.Size(62, 17);
			this.ExpertPlusCheckbox.TabIndex = 4;
			this.ExpertPlusCheckbox.Text = "Expert+";
			this.ExpertPlusCheckbox.UseVisualStyleBackColor = true;
			// 
			// MiloBrowseButton
			// 
			this.MiloBrowseButton.AutoSize = true;
			this.MiloBrowseButton.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
			this.MiloBrowseButton.Location = new System.Drawing.Point(263, 76);
			this.MiloBrowseButton.Name = "MiloBrowseButton";
			this.MiloBrowseButton.Size = new System.Drawing.Size(26, 23);
			this.MiloBrowseButton.TabIndex = 5;
			this.MiloBrowseButton.Text = "...";
			this.MiloBrowseButton.UseVisualStyleBackColor = true;
			this.MiloBrowseButton.Click += new System.EventHandler(this.MiloBrowseButton_Click);
			// 
			// WeightsBrowseButton
			// 
			this.WeightsBrowseButton.AutoSize = true;
			this.WeightsBrowseButton.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
			this.WeightsBrowseButton.Location = new System.Drawing.Point(263, 105);
			this.WeightsBrowseButton.Name = "WeightsBrowseButton";
			this.WeightsBrowseButton.Size = new System.Drawing.Size(26, 23);
			this.WeightsBrowseButton.TabIndex = 6;
			this.WeightsBrowseButton.Text = "...";
			this.WeightsBrowseButton.UseVisualStyleBackColor = true;
			this.WeightsBrowseButton.Click += new System.EventHandler(this.WeightsBrowseButton_Click);
			// 
			// PanBrowseButton
			// 
			this.PanBrowseButton.AutoSize = true;
			this.PanBrowseButton.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
			this.PanBrowseButton.Location = new System.Drawing.Point(263, 134);
			this.PanBrowseButton.Name = "PanBrowseButton";
			this.PanBrowseButton.Size = new System.Drawing.Size(26, 23);
			this.PanBrowseButton.TabIndex = 7;
			this.PanBrowseButton.Text = "...";
			this.PanBrowseButton.UseVisualStyleBackColor = true;
			this.PanBrowseButton.Click += new System.EventHandler(this.PanBrowseButton_Click);
			// 
			// MiloTextbox
			// 
			this.MiloTextbox.Dock = System.Windows.Forms.DockStyle.Top;
			this.MiloTextbox.Location = new System.Drawing.Point(71, 76);
			this.MiloTextbox.Name = "MiloTextbox";
			this.MiloTextbox.Size = new System.Drawing.Size(186, 20);
			this.MiloTextbox.TabIndex = 8;
			// 
			// WeightsTextbox
			// 
			this.WeightsTextbox.Dock = System.Windows.Forms.DockStyle.Top;
			this.WeightsTextbox.Location = new System.Drawing.Point(71, 105);
			this.WeightsTextbox.Name = "WeightsTextbox";
			this.WeightsTextbox.Size = new System.Drawing.Size(186, 20);
			this.WeightsTextbox.TabIndex = 8;
			// 
			// PanTextbox
			// 
			this.PanTextbox.Dock = System.Windows.Forms.DockStyle.Top;
			this.PanTextbox.Location = new System.Drawing.Point(71, 134);
			this.PanTextbox.Name = "PanTextbox";
			this.PanTextbox.Size = new System.Drawing.Size(186, 20);
			this.PanTextbox.TabIndex = 8;
			// 
			// LabelOptions
			// 
			this.LabelOptions.AutoSize = true;
			this.LabelOptions.Location = new System.Drawing.Point(3, 27);
			this.LabelOptions.Name = "LabelOptions";
			this.LabelOptions.Padding = new System.Windows.Forms.Padding(0, 4, 0, 0);
			this.LabelOptions.Size = new System.Drawing.Size(46, 17);
			this.LabelOptions.TabIndex = 0;
			this.LabelOptions.Text = "Options:";
			// 
			// LabelMilo
			// 
			this.LabelMilo.AutoSize = true;
			this.LabelMilo.Location = new System.Drawing.Point(3, 73);
			this.LabelMilo.Name = "LabelMilo";
			this.LabelMilo.Padding = new System.Windows.Forms.Padding(0, 4, 0, 0);
			this.LabelMilo.Size = new System.Drawing.Size(29, 17);
			this.LabelMilo.TabIndex = 0;
			this.LabelMilo.Text = "Milo:";
			// 
			// LabelWeights
			// 
			this.LabelWeights.AutoSize = true;
			this.LabelWeights.Location = new System.Drawing.Point(3, 102);
			this.LabelWeights.Name = "LabelWeights";
			this.LabelWeights.Padding = new System.Windows.Forms.Padding(0, 4, 0, 0);
			this.LabelWeights.Size = new System.Drawing.Size(49, 17);
			this.LabelWeights.TabIndex = 0;
			this.LabelWeights.Text = "Weights:";
			// 
			// LabelPan
			// 
			this.LabelPan.AutoSize = true;
			this.LabelPan.Location = new System.Drawing.Point(3, 131);
			this.LabelPan.Name = "LabelPan";
			this.LabelPan.Padding = new System.Windows.Forms.Padding(0, 4, 0, 0);
			this.LabelPan.Size = new System.Drawing.Size(29, 17);
			this.LabelPan.TabIndex = 0;
			this.LabelPan.Text = "Pan:";
			// 
			// ButtonLayout
			// 
			this.MainLayout.SetColumnSpan(this.ButtonLayout, 3);
			this.ButtonLayout.Controls.Add(this.ButtonOK);
			this.ButtonLayout.Controls.Add(this.ButtonCancel);
			this.ButtonLayout.Dock = System.Windows.Forms.DockStyle.Fill;
			this.ButtonLayout.FlowDirection = System.Windows.Forms.FlowDirection.RightToLeft;
			this.ButtonLayout.Location = new System.Drawing.Point(3, 163);
			this.ButtonLayout.Name = "ButtonLayout";
			this.ButtonLayout.Size = new System.Drawing.Size(286, 26);
			this.ButtonLayout.TabIndex = 9;
			// 
			// ButtonOK
			// 
			this.ButtonOK.Location = new System.Drawing.Point(208, 3);
			this.ButtonOK.Name = "ButtonOK";
			this.ButtonOK.Size = new System.Drawing.Size(75, 23);
			this.ButtonOK.TabIndex = 2;
			this.ButtonOK.Text = "&OK";
			this.ButtonOK.UseVisualStyleBackColor = true;
			this.ButtonOK.Click += new System.EventHandler(this.ButtonOK_Click);
			// 
			// ButtonCancel
			// 
			this.ButtonCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
			this.ButtonCancel.Location = new System.Drawing.Point(127, 3);
			this.ButtonCancel.Name = "ButtonCancel";
			this.ButtonCancel.Size = new System.Drawing.Size(75, 23);
			this.ButtonCancel.TabIndex = 3;
			this.ButtonCancel.Text = "&Cancel";
			this.ButtonCancel.UseVisualStyleBackColor = true;
			this.ButtonCancel.Click += new System.EventHandler(this.ButtonCancel_Click);
			// 
			// ChartForm
			// 
			this.AcceptButton = this.ButtonOK;
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.CancelButton = this.ButtonCancel;
			this.ClientSize = new System.Drawing.Size(292, 192);
			this.Controls.Add(this.MainLayout);
			this.Name = "ChartForm";
			this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
			this.Text = "Chart Configuration";
			this.Load += new System.EventHandler(this.ChartForm_Load);
			this.MainLayout.ResumeLayout(false);
			this.MainLayout.PerformLayout();
			this.ButtonLayout.ResumeLayout(false);
			this.ResumeLayout(false);

		}

		#endregion

		private System.Windows.Forms.TableLayoutPanel MainLayout;
		private System.Windows.Forms.Label LabelChartType;
		private System.Windows.Forms.ComboBox ChartCombo;
		private System.Windows.Forms.CheckBox CoopCheckbox;
		private System.Windows.Forms.CheckBox ExpertPlusCheckbox;
		private System.Windows.Forms.Button MiloBrowseButton;
		private System.Windows.Forms.Button WeightsBrowseButton;
		private System.Windows.Forms.Button PanBrowseButton;
		private System.Windows.Forms.TextBox MiloTextbox;
		private System.Windows.Forms.TextBox WeightsTextbox;
		private System.Windows.Forms.TextBox PanTextbox;
		private System.Windows.Forms.Label LabelOptions;
		private System.Windows.Forms.Label LabelMilo;
		private System.Windows.Forms.Label LabelWeights;
		private System.Windows.Forms.Label LabelPan;
		private System.Windows.Forms.FlowLayoutPanel ButtonLayout;
		private System.Windows.Forms.Button ButtonOK;
		private System.Windows.Forms.Button ButtonCancel;
		private System.Windows.Forms.OpenFileDialog OpenFile;
	}
}