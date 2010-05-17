namespace ConsoleHaxx.RawkSD.SWF
{
	partial class AudioForm
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
			this.AudioList = new System.Windows.Forms.ListBox();
			this.LabelInstruments = new System.Windows.Forms.Label();
			this.BandOption = new System.Windows.Forms.RadioButton();
			this.GuitarOption = new System.Windows.Forms.RadioButton();
			this.DrumsOption = new System.Windows.Forms.RadioButton();
			this.VocalsOption = new System.Windows.Forms.RadioButton();
			this.BassOption = new System.Windows.Forms.RadioButton();
			this.LabelBalance = new System.Windows.Forms.Label();
			this.LabelVolume = new System.Windows.Forms.Label();
			this.VolumeNumeric = new System.Windows.Forms.NumericUpDown();
			this.BalanceNumeric = new System.Windows.Forms.NumericUpDown();
			this.LabelChannel = new System.Windows.Forms.Label();
			this.ButtonLayout = new System.Windows.Forms.FlowLayoutPanel();
			this.ButtonOK = new System.Windows.Forms.Button();
			this.ButtonCancel = new System.Windows.Forms.Button();
			this.MainLayout.SuspendLayout();
			((System.ComponentModel.ISupportInitialize)(this.VolumeNumeric)).BeginInit();
			((System.ComponentModel.ISupportInitialize)(this.BalanceNumeric)).BeginInit();
			this.ButtonLayout.SuspendLayout();
			this.SuspendLayout();
			// 
			// MainLayout
			// 
			this.MainLayout.ColumnCount = 4;
			this.MainLayout.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
			this.MainLayout.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 32F));
			this.MainLayout.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
			this.MainLayout.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
			this.MainLayout.Controls.Add(this.AudioList, 0, 0);
			this.MainLayout.Controls.Add(this.LabelInstruments, 0, 1);
			this.MainLayout.Controls.Add(this.BandOption, 0, 2);
			this.MainLayout.Controls.Add(this.GuitarOption, 0, 3);
			this.MainLayout.Controls.Add(this.DrumsOption, 0, 4);
			this.MainLayout.Controls.Add(this.VocalsOption, 0, 5);
			this.MainLayout.Controls.Add(this.BassOption, 0, 6);
			this.MainLayout.Controls.Add(this.LabelBalance, 2, 3);
			this.MainLayout.Controls.Add(this.LabelVolume, 2, 2);
			this.MainLayout.Controls.Add(this.VolumeNumeric, 3, 2);
			this.MainLayout.Controls.Add(this.BalanceNumeric, 3, 3);
			this.MainLayout.Controls.Add(this.LabelChannel, 2, 1);
			this.MainLayout.Controls.Add(this.ButtonLayout, 0, 7);
			this.MainLayout.Dock = System.Windows.Forms.DockStyle.Fill;
			this.MainLayout.Location = new System.Drawing.Point(0, 0);
			this.MainLayout.Name = "MainLayout";
			this.MainLayout.Padding = new System.Windows.Forms.Padding(0, 4, 0, 0);
			this.MainLayout.RowCount = 8;
			this.MainLayout.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.MainLayout.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.MainLayout.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.MainLayout.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.MainLayout.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.MainLayout.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.MainLayout.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.MainLayout.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.MainLayout.Size = new System.Drawing.Size(253, 280);
			this.MainLayout.TabIndex = 0;
			// 
			// AudioList
			// 
			this.MainLayout.SetColumnSpan(this.AudioList, 4);
			this.AudioList.Dock = System.Windows.Forms.DockStyle.Fill;
			this.AudioList.FormattingEnabled = true;
			this.AudioList.Location = new System.Drawing.Point(3, 7);
			this.AudioList.Name = "AudioList";
			this.AudioList.Size = new System.Drawing.Size(247, 95);
			this.AudioList.TabIndex = 0;
			this.AudioList.SelectedIndexChanged += new System.EventHandler(this.AudioList_SelectedIndexChanged);
			// 
			// LabelInstruments
			// 
			this.LabelInstruments.AutoSize = true;
			this.LabelInstruments.Location = new System.Drawing.Point(3, 105);
			this.LabelInstruments.Name = "LabelInstruments";
			this.LabelInstruments.Padding = new System.Windows.Forms.Padding(0, 4, 0, 4);
			this.LabelInstruments.Size = new System.Drawing.Size(100, 21);
			this.LabelInstruments.TabIndex = 1;
			this.LabelInstruments.Text = "Instrument Mapping";
			// 
			// BandOption
			// 
			this.BandOption.AutoSize = true;
			this.BandOption.Location = new System.Drawing.Point(3, 129);
			this.BandOption.Name = "BandOption";
			this.BandOption.Size = new System.Drawing.Size(83, 17);
			this.BandOption.TabIndex = 2;
			this.BandOption.TabStop = true;
			this.BandOption.Text = "Background";
			this.BandOption.UseVisualStyleBackColor = true;
			this.BandOption.CheckedChanged += new System.EventHandler(this.BandOption_CheckedChanged);
			// 
			// GuitarOption
			// 
			this.GuitarOption.AutoSize = true;
			this.GuitarOption.Location = new System.Drawing.Point(3, 155);
			this.GuitarOption.Name = "GuitarOption";
			this.GuitarOption.Size = new System.Drawing.Size(53, 17);
			this.GuitarOption.TabIndex = 3;
			this.GuitarOption.TabStop = true;
			this.GuitarOption.Text = "Guitar";
			this.GuitarOption.UseVisualStyleBackColor = true;
			this.GuitarOption.CheckedChanged += new System.EventHandler(this.GuitarOption_CheckedChanged);
			// 
			// DrumsOption
			// 
			this.DrumsOption.AutoSize = true;
			this.DrumsOption.Location = new System.Drawing.Point(3, 181);
			this.DrumsOption.Name = "DrumsOption";
			this.DrumsOption.Size = new System.Drawing.Size(55, 17);
			this.DrumsOption.TabIndex = 4;
			this.DrumsOption.TabStop = true;
			this.DrumsOption.Text = "Drums";
			this.DrumsOption.UseVisualStyleBackColor = true;
			this.DrumsOption.CheckedChanged += new System.EventHandler(this.DrumsOption_CheckedChanged);
			// 
			// VocalsOption
			// 
			this.VocalsOption.AutoSize = true;
			this.VocalsOption.Location = new System.Drawing.Point(3, 204);
			this.VocalsOption.Name = "VocalsOption";
			this.VocalsOption.Size = new System.Drawing.Size(57, 17);
			this.VocalsOption.TabIndex = 5;
			this.VocalsOption.TabStop = true;
			this.VocalsOption.Text = "Vocals";
			this.VocalsOption.UseVisualStyleBackColor = true;
			this.VocalsOption.CheckedChanged += new System.EventHandler(this.VocalsOption_CheckedChanged);
			// 
			// BassOption
			// 
			this.BassOption.AutoSize = true;
			this.BassOption.Location = new System.Drawing.Point(3, 227);
			this.BassOption.Name = "BassOption";
			this.BassOption.Size = new System.Drawing.Size(48, 17);
			this.BassOption.TabIndex = 6;
			this.BassOption.TabStop = true;
			this.BassOption.Text = "Bass";
			this.BassOption.UseVisualStyleBackColor = true;
			this.BassOption.CheckedChanged += new System.EventHandler(this.BassOption_CheckedChanged);
			// 
			// LabelBalance
			// 
			this.LabelBalance.AutoSize = true;
			this.LabelBalance.Location = new System.Drawing.Point(141, 152);
			this.LabelBalance.Name = "LabelBalance";
			this.LabelBalance.Padding = new System.Windows.Forms.Padding(0, 4, 0, 0);
			this.LabelBalance.Size = new System.Drawing.Size(49, 17);
			this.LabelBalance.TabIndex = 10;
			this.LabelBalance.Text = "Balance:";
			// 
			// LabelVolume
			// 
			this.LabelVolume.AutoSize = true;
			this.LabelVolume.Location = new System.Drawing.Point(141, 126);
			this.LabelVolume.Name = "LabelVolume";
			this.LabelVolume.Padding = new System.Windows.Forms.Padding(0, 4, 0, 0);
			this.LabelVolume.Size = new System.Drawing.Size(45, 17);
			this.LabelVolume.TabIndex = 8;
			this.LabelVolume.Text = "Volume:";
			// 
			// VolumeNumeric
			// 
			this.VolumeNumeric.DecimalPlaces = 1;
			this.VolumeNumeric.Dock = System.Windows.Forms.DockStyle.Top;
			this.VolumeNumeric.Location = new System.Drawing.Point(196, 129);
			this.VolumeNumeric.Minimum = new decimal(new int[] {
            100,
            0,
            0,
            -2147483648});
			this.VolumeNumeric.Name = "VolumeNumeric";
			this.VolumeNumeric.Size = new System.Drawing.Size(54, 20);
			this.VolumeNumeric.TabIndex = 9;
			this.VolumeNumeric.ValueChanged += new System.EventHandler(this.VolumeNumeric_ValueChanged);
			// 
			// BalanceNumeric
			// 
			this.BalanceNumeric.DecimalPlaces = 1;
			this.BalanceNumeric.Dock = System.Windows.Forms.DockStyle.Top;
			this.BalanceNumeric.Location = new System.Drawing.Point(196, 155);
			this.BalanceNumeric.Minimum = new decimal(new int[] {
            100,
            0,
            0,
            -2147483648});
			this.BalanceNumeric.Name = "BalanceNumeric";
			this.BalanceNumeric.Size = new System.Drawing.Size(54, 20);
			this.BalanceNumeric.TabIndex = 11;
			this.BalanceNumeric.ValueChanged += new System.EventHandler(this.BalanceNumeric_ValueChanged);
			// 
			// LabelChannel
			// 
			this.LabelChannel.AutoSize = true;
			this.MainLayout.SetColumnSpan(this.LabelChannel, 2);
			this.LabelChannel.Location = new System.Drawing.Point(141, 105);
			this.LabelChannel.Name = "LabelChannel";
			this.LabelChannel.Padding = new System.Windows.Forms.Padding(0, 4, 0, 4);
			this.LabelChannel.Size = new System.Drawing.Size(87, 21);
			this.LabelChannel.TabIndex = 7;
			this.LabelChannel.Text = "Channel Settings";
			// 
			// ButtonLayout
			// 
			this.MainLayout.SetColumnSpan(this.ButtonLayout, 4);
			this.ButtonLayout.Controls.Add(this.ButtonOK);
			this.ButtonLayout.Controls.Add(this.ButtonCancel);
			this.ButtonLayout.Dock = System.Windows.Forms.DockStyle.Fill;
			this.ButtonLayout.FlowDirection = System.Windows.Forms.FlowDirection.RightToLeft;
			this.ButtonLayout.Location = new System.Drawing.Point(3, 250);
			this.ButtonLayout.Name = "ButtonLayout";
			this.ButtonLayout.Size = new System.Drawing.Size(247, 27);
			this.ButtonLayout.TabIndex = 12;
			// 
			// ButtonOK
			// 
			this.ButtonOK.Location = new System.Drawing.Point(169, 3);
			this.ButtonOK.Name = "ButtonOK";
			this.ButtonOK.Size = new System.Drawing.Size(75, 23);
			this.ButtonOK.TabIndex = 0;
			this.ButtonOK.Text = "&OK";
			this.ButtonOK.UseVisualStyleBackColor = true;
			this.ButtonOK.Click += new System.EventHandler(this.ButtonOK_Click);
			// 
			// ButtonCancel
			// 
			this.ButtonCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
			this.ButtonCancel.Location = new System.Drawing.Point(88, 3);
			this.ButtonCancel.Name = "ButtonCancel";
			this.ButtonCancel.Size = new System.Drawing.Size(75, 23);
			this.ButtonCancel.TabIndex = 1;
			this.ButtonCancel.Text = "&Cancel";
			this.ButtonCancel.UseVisualStyleBackColor = true;
			this.ButtonCancel.Click += new System.EventHandler(this.ButtonCancel_Click);
			// 
			// AudioForm
			// 
			this.AcceptButton = this.ButtonOK;
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.CancelButton = this.ButtonCancel;
			this.ClientSize = new System.Drawing.Size(253, 280);
			this.Controls.Add(this.MainLayout);
			this.Name = "AudioForm";
			this.Text = "Audio Configuration";
			this.Load += new System.EventHandler(this.AudioForm_Load);
			this.MainLayout.ResumeLayout(false);
			this.MainLayout.PerformLayout();
			((System.ComponentModel.ISupportInitialize)(this.VolumeNumeric)).EndInit();
			((System.ComponentModel.ISupportInitialize)(this.BalanceNumeric)).EndInit();
			this.ButtonLayout.ResumeLayout(false);
			this.ResumeLayout(false);

		}

		#endregion

		private System.Windows.Forms.TableLayoutPanel MainLayout;
		private System.Windows.Forms.ListBox AudioList;
		private System.Windows.Forms.Label LabelInstruments;
		private System.Windows.Forms.RadioButton BandOption;
		private System.Windows.Forms.RadioButton GuitarOption;
		private System.Windows.Forms.RadioButton DrumsOption;
		private System.Windows.Forms.RadioButton VocalsOption;
		private System.Windows.Forms.RadioButton BassOption;
		private System.Windows.Forms.Label LabelBalance;
		private System.Windows.Forms.Label LabelVolume;
		private System.Windows.Forms.NumericUpDown VolumeNumeric;
		private System.Windows.Forms.NumericUpDown BalanceNumeric;
		private System.Windows.Forms.Label LabelChannel;
		private System.Windows.Forms.FlowLayoutPanel ButtonLayout;
		private System.Windows.Forms.Button ButtonOK;
		private System.Windows.Forms.Button ButtonCancel;
	}
}