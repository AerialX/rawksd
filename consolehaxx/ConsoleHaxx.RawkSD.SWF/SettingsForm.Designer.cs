namespace ConsoleHaxx.RawkSD.SWF
{
	partial class SettingsForm
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
			this.Tab = new System.Windows.Forms.TabControl();
			this.tabPage1 = new System.Windows.Forms.TabPage();
			this.tabPage2 = new System.Windows.Forms.TabPage();
			this.tableLayoutPanel1 = new System.Windows.Forms.TableLayoutPanel();
			this.LabelCustoms = new System.Windows.Forms.Label();
			this.LabelTemporary = new System.Windows.Forms.Label();
			this.LabelPerformance = new System.Windows.Forms.Label();
			this.LabelDefaultAction = new System.Windows.Forms.Label();
			this.CustomsText = new System.Windows.Forms.TextBox();
			this.TemporaryText = new System.Windows.Forms.TextBox();
			this.PerformanceCombo = new System.Windows.Forms.ComboBox();
			this.DefaultActionCombo = new System.Windows.Forms.ComboBox();
			this.tableLayoutPanel2 = new System.Windows.Forms.TableLayoutPanel();
			this.LabelStorage = new System.Windows.Forms.Label();
			this.LocalStorageCombo = new System.Windows.Forms.ComboBox();
			this.LabelTasks = new System.Windows.Forms.Label();
			this.TasksNumeric = new System.Windows.Forms.NumericUpDown();
			this.LabelTags = new System.Windows.Forms.Label();
			this.GH5ExpertPlusCheckbox = new System.Windows.Forms.CheckBox();
			this.NamePrefixCombo = new System.Windows.Forms.ComboBox();
			this.ButtonLayout = new System.Windows.Forms.FlowLayoutPanel();
			this.CloseButton = new System.Windows.Forms.Button();
			this.Tab.SuspendLayout();
			this.tabPage1.SuspendLayout();
			this.tabPage2.SuspendLayout();
			this.tableLayoutPanel1.SuspendLayout();
			this.tableLayoutPanel2.SuspendLayout();
			((System.ComponentModel.ISupportInitialize)(this.TasksNumeric)).BeginInit();
			this.ButtonLayout.SuspendLayout();
			this.SuspendLayout();
			// 
			// Tab
			// 
			this.Tab.Controls.Add(this.tabPage1);
			this.Tab.Controls.Add(this.tabPage2);
			this.Tab.Dock = System.Windows.Forms.DockStyle.Fill;
			this.Tab.Location = new System.Drawing.Point(0, 0);
			this.Tab.Name = "Tab";
			this.Tab.SelectedIndex = 0;
			this.Tab.Size = new System.Drawing.Size(310, 166);
			this.Tab.TabIndex = 0;
			// 
			// tabPage1
			// 
			this.tabPage1.Controls.Add(this.tableLayoutPanel1);
			this.tabPage1.Location = new System.Drawing.Point(4, 22);
			this.tabPage1.Name = "tabPage1";
			this.tabPage1.Padding = new System.Windows.Forms.Padding(3);
			this.tabPage1.Size = new System.Drawing.Size(302, 140);
			this.tabPage1.TabIndex = 0;
			this.tabPage1.Text = "Options";
			this.tabPage1.UseVisualStyleBackColor = true;
			// 
			// tabPage2
			// 
			this.tabPage2.Controls.Add(this.tableLayoutPanel2);
			this.tabPage2.Location = new System.Drawing.Point(4, 22);
			this.tabPage2.Name = "tabPage2";
			this.tabPage2.Padding = new System.Windows.Forms.Padding(3);
			this.tabPage2.Size = new System.Drawing.Size(302, 140);
			this.tabPage2.TabIndex = 1;
			this.tabPage2.Text = "Imports";
			this.tabPage2.UseVisualStyleBackColor = true;
			// 
			// tableLayoutPanel1
			// 
			this.tableLayoutPanel1.ColumnCount = 2;
			this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
			this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
			this.tableLayoutPanel1.Controls.Add(this.LabelCustoms, 0, 0);
			this.tableLayoutPanel1.Controls.Add(this.LabelTemporary, 0, 1);
			this.tableLayoutPanel1.Controls.Add(this.LabelPerformance, 0, 3);
			this.tableLayoutPanel1.Controls.Add(this.LabelDefaultAction, 0, 4);
			this.tableLayoutPanel1.Controls.Add(this.CustomsText, 1, 0);
			this.tableLayoutPanel1.Controls.Add(this.TemporaryText, 1, 1);
			this.tableLayoutPanel1.Controls.Add(this.PerformanceCombo, 1, 3);
			this.tableLayoutPanel1.Controls.Add(this.DefaultActionCombo, 1, 4);
			this.tableLayoutPanel1.Controls.Add(this.LabelTasks, 0, 2);
			this.tableLayoutPanel1.Controls.Add(this.TasksNumeric, 1, 2);
			this.tableLayoutPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
			this.tableLayoutPanel1.Location = new System.Drawing.Point(3, 3);
			this.tableLayoutPanel1.Name = "tableLayoutPanel1";
			this.tableLayoutPanel1.RowCount = 5;
			this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.tableLayoutPanel1.Size = new System.Drawing.Size(296, 134);
			this.tableLayoutPanel1.TabIndex = 0;
			// 
			// LabelCustoms
			// 
			this.LabelCustoms.AutoSize = true;
			this.LabelCustoms.Location = new System.Drawing.Point(3, 0);
			this.LabelCustoms.Name = "LabelCustoms";
			this.LabelCustoms.Padding = new System.Windows.Forms.Padding(0, 4, 0, 0);
			this.LabelCustoms.Size = new System.Drawing.Size(75, 17);
			this.LabelCustoms.TabIndex = 1;
			this.LabelCustoms.Text = "Customs Path:";
			// 
			// LabelTemporary
			// 
			this.LabelTemporary.AutoSize = true;
			this.LabelTemporary.Location = new System.Drawing.Point(3, 26);
			this.LabelTemporary.Name = "LabelTemporary";
			this.LabelTemporary.Padding = new System.Windows.Forms.Padding(0, 4, 0, 0);
			this.LabelTemporary.Size = new System.Drawing.Size(85, 17);
			this.LabelTemporary.TabIndex = 1;
			this.LabelTemporary.Text = "Temporary Path:";
			// 
			// LabelPerformance
			// 
			this.LabelPerformance.AutoSize = true;
			this.LabelPerformance.Location = new System.Drawing.Point(3, 78);
			this.LabelPerformance.Name = "LabelPerformance";
			this.LabelPerformance.Padding = new System.Windows.Forms.Padding(0, 4, 0, 0);
			this.LabelPerformance.Size = new System.Drawing.Size(70, 17);
			this.LabelPerformance.TabIndex = 1;
			this.LabelPerformance.Text = "Performance:";
			// 
			// LabelDefaultAction
			// 
			this.LabelDefaultAction.AutoSize = true;
			this.LabelDefaultAction.Location = new System.Drawing.Point(3, 105);
			this.LabelDefaultAction.Name = "LabelDefaultAction";
			this.LabelDefaultAction.Padding = new System.Windows.Forms.Padding(0, 4, 0, 0);
			this.LabelDefaultAction.Size = new System.Drawing.Size(77, 17);
			this.LabelDefaultAction.TabIndex = 1;
			this.LabelDefaultAction.Text = "Default Action:";
			// 
			// CustomsText
			// 
			this.CustomsText.Dock = System.Windows.Forms.DockStyle.Top;
			this.CustomsText.Location = new System.Drawing.Point(103, 3);
			this.CustomsText.Name = "CustomsText";
			this.CustomsText.Size = new System.Drawing.Size(190, 20);
			this.CustomsText.TabIndex = 2;
			this.CustomsText.TextChanged += new System.EventHandler(this.CustomsText_TextChanged);
			// 
			// TemporaryText
			// 
			this.TemporaryText.Dock = System.Windows.Forms.DockStyle.Top;
			this.TemporaryText.Location = new System.Drawing.Point(103, 29);
			this.TemporaryText.Name = "TemporaryText";
			this.TemporaryText.Size = new System.Drawing.Size(190, 20);
			this.TemporaryText.TabIndex = 2;
			this.TemporaryText.TextChanged += new System.EventHandler(this.TemporaryText_TextChanged);
			// 
			// PerformanceCombo
			// 
			this.PerformanceCombo.Dock = System.Windows.Forms.DockStyle.Top;
			this.PerformanceCombo.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
			this.PerformanceCombo.FormattingEnabled = true;
			this.PerformanceCombo.Items.AddRange(new object[] {
            "Optimize Performance",
            "Optimize Memory"});
			this.PerformanceCombo.Location = new System.Drawing.Point(103, 81);
			this.PerformanceCombo.Name = "PerformanceCombo";
			this.PerformanceCombo.Size = new System.Drawing.Size(190, 21);
			this.PerformanceCombo.TabIndex = 3;
			this.PerformanceCombo.SelectedIndexChanged += new System.EventHandler(this.PerformanceCombo_SelectedIndexChanged);
			// 
			// DefaultActionCombo
			// 
			this.DefaultActionCombo.Dock = System.Windows.Forms.DockStyle.Top;
			this.DefaultActionCombo.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
			this.DefaultActionCombo.FormattingEnabled = true;
			this.DefaultActionCombo.Items.AddRange(new object[] {
            "Install to SD",
            "Install Locally",
            "Export to .rwk",
            "Export to .rba",
            "Export to Frets on Fire"});
			this.DefaultActionCombo.Location = new System.Drawing.Point(103, 108);
			this.DefaultActionCombo.Name = "DefaultActionCombo";
			this.DefaultActionCombo.Size = new System.Drawing.Size(190, 21);
			this.DefaultActionCombo.TabIndex = 3;
			this.DefaultActionCombo.SelectedIndexChanged += new System.EventHandler(this.DefaultActionCombo_SelectedIndexChanged);
			// 
			// tableLayoutPanel2
			// 
			this.tableLayoutPanel2.ColumnCount = 2;
			this.tableLayoutPanel2.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
			this.tableLayoutPanel2.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
			this.tableLayoutPanel2.Controls.Add(this.LabelStorage, 0, 0);
			this.tableLayoutPanel2.Controls.Add(this.LocalStorageCombo, 1, 0);
			this.tableLayoutPanel2.Controls.Add(this.LabelTags, 0, 1);
			this.tableLayoutPanel2.Controls.Add(this.GH5ExpertPlusCheckbox, 1, 2);
			this.tableLayoutPanel2.Controls.Add(this.NamePrefixCombo, 1, 1);
			this.tableLayoutPanel2.Dock = System.Windows.Forms.DockStyle.Fill;
			this.tableLayoutPanel2.Location = new System.Drawing.Point(3, 3);
			this.tableLayoutPanel2.Name = "tableLayoutPanel2";
			this.tableLayoutPanel2.RowCount = 3;
			this.tableLayoutPanel2.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.tableLayoutPanel2.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.tableLayoutPanel2.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.tableLayoutPanel2.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 20F));
			this.tableLayoutPanel2.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 20F));
			this.tableLayoutPanel2.Size = new System.Drawing.Size(296, 134);
			this.tableLayoutPanel2.TabIndex = 0;
			// 
			// LabelStorage
			// 
			this.LabelStorage.AutoSize = true;
			this.LabelStorage.Location = new System.Drawing.Point(3, 0);
			this.LabelStorage.Name = "LabelStorage";
			this.LabelStorage.Padding = new System.Windows.Forms.Padding(0, 4, 0, 0);
			this.LabelStorage.Size = new System.Drawing.Size(76, 17);
			this.LabelStorage.TabIndex = 4;
			this.LabelStorage.Text = "Local Storage:";
			// 
			// LocalStorageCombo
			// 
			this.LocalStorageCombo.Dock = System.Windows.Forms.DockStyle.Top;
			this.LocalStorageCombo.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
			this.LocalStorageCombo.FormattingEnabled = true;
			this.LocalStorageCombo.Items.AddRange(new object[] {
            "None",
            "Store Original Format Locally",
            "Store Rock Band 2 Format Locally"});
			this.LocalStorageCombo.Location = new System.Drawing.Point(106, 3);
			this.LocalStorageCombo.Name = "LocalStorageCombo";
			this.LocalStorageCombo.Size = new System.Drawing.Size(187, 21);
			this.LocalStorageCombo.TabIndex = 5;
			this.LocalStorageCombo.SelectedIndexChanged += new System.EventHandler(this.LocalStorageCombo_SelectedIndexChanged);
			// 
			// LabelTasks
			// 
			this.LabelTasks.AutoSize = true;
			this.LabelTasks.Location = new System.Drawing.Point(3, 52);
			this.LabelTasks.Name = "LabelTasks";
			this.LabelTasks.Padding = new System.Windows.Forms.Padding(0, 4, 0, 0);
			this.LabelTasks.Size = new System.Drawing.Size(94, 17);
			this.LabelTasks.TabIndex = 1;
			this.LabelTasks.Text = "Concurrent Tasks:";
			// 
			// TasksNumeric
			// 
			this.TasksNumeric.Dock = System.Windows.Forms.DockStyle.Top;
			this.TasksNumeric.Location = new System.Drawing.Point(103, 55);
			this.TasksNumeric.Minimum = new decimal(new int[] {
            1,
            0,
            0,
            0});
			this.TasksNumeric.Name = "TasksNumeric";
			this.TasksNumeric.Size = new System.Drawing.Size(190, 20);
			this.TasksNumeric.TabIndex = 4;
			this.TasksNumeric.Value = new decimal(new int[] {
            1,
            0,
            0,
            0});
			this.TasksNumeric.ValueChanged += new System.EventHandler(this.TasksNumeric_ValueChanged);
			// 
			// LabelTags
			// 
			this.LabelTags.AutoSize = true;
			this.LabelTags.Location = new System.Drawing.Point(3, 27);
			this.LabelTags.Name = "LabelTags";
			this.LabelTags.Padding = new System.Windows.Forms.Padding(0, 4, 0, 0);
			this.LabelTags.Size = new System.Drawing.Size(97, 17);
			this.LabelTags.TabIndex = 4;
			this.LabelTags.Text = "Import Name Tags:";
			// 
			// GH5ExpertPlusCheckbox
			// 
			this.GH5ExpertPlusCheckbox.AutoSize = true;
			this.GH5ExpertPlusCheckbox.Location = new System.Drawing.Point(106, 57);
			this.GH5ExpertPlusCheckbox.Name = "GH5ExpertPlusCheckbox";
			this.GH5ExpertPlusCheckbox.Size = new System.Drawing.Size(184, 17);
			this.GH5ExpertPlusCheckbox.TabIndex = 6;
			this.GH5ExpertPlusCheckbox.Text = "Import Guitar Hero Expert+ Drums";
			this.GH5ExpertPlusCheckbox.UseVisualStyleBackColor = true;
			this.GH5ExpertPlusCheckbox.CheckedChanged += new System.EventHandler(this.GH5ExpertPlusCheckbox_CheckedChanged);
			// 
			// NamePrefixCombo
			// 
			this.NamePrefixCombo.Dock = System.Windows.Forms.DockStyle.Top;
			this.NamePrefixCombo.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
			this.NamePrefixCombo.FormattingEnabled = true;
			this.NamePrefixCombo.Items.AddRange(new object[] {
            "None",
            "Prefix",
            "Postfix"});
			this.NamePrefixCombo.Location = new System.Drawing.Point(106, 30);
			this.NamePrefixCombo.Name = "NamePrefixCombo";
			this.NamePrefixCombo.Size = new System.Drawing.Size(187, 21);
			this.NamePrefixCombo.TabIndex = 7;
			this.NamePrefixCombo.SelectedIndexChanged += new System.EventHandler(this.NamePrefixCombo_SelectedIndexChanged);
			// 
			// ButtonLayout
			// 
			this.ButtonLayout.AutoSize = true;
			this.ButtonLayout.Controls.Add(this.CloseButton);
			this.ButtonLayout.Dock = System.Windows.Forms.DockStyle.Bottom;
			this.ButtonLayout.FlowDirection = System.Windows.Forms.FlowDirection.RightToLeft;
			this.ButtonLayout.Location = new System.Drawing.Point(0, 166);
			this.ButtonLayout.Name = "ButtonLayout";
			this.ButtonLayout.Size = new System.Drawing.Size(310, 29);
			this.ButtonLayout.TabIndex = 1;
			// 
			// CloseButton
			// 
			this.CloseButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
			this.CloseButton.Location = new System.Drawing.Point(232, 3);
			this.CloseButton.Name = "CloseButton";
			this.CloseButton.Size = new System.Drawing.Size(75, 23);
			this.CloseButton.TabIndex = 0;
			this.CloseButton.Text = "Close";
			this.CloseButton.UseVisualStyleBackColor = true;
			this.CloseButton.Click += new System.EventHandler(this.CloseButton_Click);
			// 
			// SettingsForm
			// 
			this.AcceptButton = this.CloseButton;
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.CancelButton = this.CloseButton;
			this.ClientSize = new System.Drawing.Size(310, 195);
			this.Controls.Add(this.Tab);
			this.Controls.Add(this.ButtonLayout);
			this.Name = "SettingsForm";
			this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
			this.Text = "Preferences";
			this.Load += new System.EventHandler(this.SettingsForm_Load);
			this.Tab.ResumeLayout(false);
			this.tabPage1.ResumeLayout(false);
			this.tabPage2.ResumeLayout(false);
			this.tableLayoutPanel1.ResumeLayout(false);
			this.tableLayoutPanel1.PerformLayout();
			this.tableLayoutPanel2.ResumeLayout(false);
			this.tableLayoutPanel2.PerformLayout();
			((System.ComponentModel.ISupportInitialize)(this.TasksNumeric)).EndInit();
			this.ButtonLayout.ResumeLayout(false);
			this.ResumeLayout(false);
			this.PerformLayout();

		}

		#endregion

		private System.Windows.Forms.TabControl Tab;
		private System.Windows.Forms.TabPage tabPage1;
		private System.Windows.Forms.TableLayoutPanel tableLayoutPanel1;
		private System.Windows.Forms.TabPage tabPage2;
		private System.Windows.Forms.Label LabelCustoms;
		private System.Windows.Forms.Label LabelTemporary;
		private System.Windows.Forms.Label LabelPerformance;
		private System.Windows.Forms.Label LabelDefaultAction;
		private System.Windows.Forms.TextBox CustomsText;
		private System.Windows.Forms.TextBox TemporaryText;
		private System.Windows.Forms.ComboBox PerformanceCombo;
		private System.Windows.Forms.ComboBox DefaultActionCombo;
		private System.Windows.Forms.TableLayoutPanel tableLayoutPanel2;
		private System.Windows.Forms.Label LabelTasks;
		private System.Windows.Forms.NumericUpDown TasksNumeric;
		private System.Windows.Forms.Label LabelStorage;
		private System.Windows.Forms.ComboBox LocalStorageCombo;
		private System.Windows.Forms.Label LabelTags;
		private System.Windows.Forms.CheckBox GH5ExpertPlusCheckbox;
		private System.Windows.Forms.ComboBox NamePrefixCombo;
		private System.Windows.Forms.FlowLayoutPanel ButtonLayout;
		private System.Windows.Forms.Button CloseButton;

	}
}