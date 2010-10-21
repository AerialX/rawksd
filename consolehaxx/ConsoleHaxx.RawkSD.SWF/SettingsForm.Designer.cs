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
			this.OptionsLayout = new System.Windows.Forms.TableLayoutPanel();
			this.LabelCustoms = new System.Windows.Forms.Label();
			this.LabelTemporary = new System.Windows.Forms.Label();
			this.LabelDefaultAction = new System.Windows.Forms.Label();
			this.CustomsText = new System.Windows.Forms.TextBox();
			this.TemporaryText = new System.Windows.Forms.TextBox();
			this.DefaultActionCombo = new System.Windows.Forms.ComboBox();
			this.LabelTasks = new System.Windows.Forms.Label();
			this.TasksNumeric = new System.Windows.Forms.NumericUpDown();
			this.LabelPriority = new System.Windows.Forms.Label();
			this.PriorityCombo = new System.Windows.Forms.ComboBox();
			this.tabPage2 = new System.Windows.Forms.TabPage();
			this.ImportsLayout = new System.Windows.Forms.TableLayoutPanel();
			this.LabelStorage = new System.Windows.Forms.Label();
			this.LocalStorageCombo = new System.Windows.Forms.ComboBox();
			this.LabelTags = new System.Windows.Forms.Label();
			this.GH5ExpertPlusCheckbox = new System.Windows.Forms.CheckBox();
			this.NamePrefixCombo = new System.Windows.Forms.ComboBox();
			this.ButtonLayout = new System.Windows.Forms.FlowLayoutPanel();
			this.CloseButton = new System.Windows.Forms.Button();
			this.Tab.SuspendLayout();
			this.tabPage1.SuspendLayout();
			this.OptionsLayout.SuspendLayout();
			((System.ComponentModel.ISupportInitialize)(this.TasksNumeric)).BeginInit();
			this.tabPage2.SuspendLayout();
			this.ImportsLayout.SuspendLayout();
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
			this.Tab.Size = new System.Drawing.Size(310, 164);
			this.Tab.TabIndex = 0;
			// 
			// tabPage1
			// 
			this.tabPage1.Controls.Add(this.OptionsLayout);
			this.tabPage1.Location = new System.Drawing.Point(4, 22);
			this.tabPage1.Name = "tabPage1";
			this.tabPage1.Padding = new System.Windows.Forms.Padding(3);
			this.tabPage1.Size = new System.Drawing.Size(302, 138);
			this.tabPage1.TabIndex = 0;
			this.tabPage1.Text = "Options";
			this.tabPage1.UseVisualStyleBackColor = true;
			// 
			// OptionsLayout
			// 
			this.OptionsLayout.AutoSize = true;
			this.OptionsLayout.ColumnCount = 2;
			this.OptionsLayout.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
			this.OptionsLayout.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
			this.OptionsLayout.Controls.Add(this.LabelCustoms, 0, 0);
			this.OptionsLayout.Controls.Add(this.LabelTemporary, 0, 1);
			this.OptionsLayout.Controls.Add(this.LabelDefaultAction, 0, 4);
			this.OptionsLayout.Controls.Add(this.CustomsText, 1, 0);
			this.OptionsLayout.Controls.Add(this.TemporaryText, 1, 1);
			this.OptionsLayout.Controls.Add(this.DefaultActionCombo, 1, 4);
			this.OptionsLayout.Controls.Add(this.LabelTasks, 0, 2);
			this.OptionsLayout.Controls.Add(this.TasksNumeric, 1, 2);
			this.OptionsLayout.Controls.Add(this.LabelPriority, 0, 3);
			this.OptionsLayout.Controls.Add(this.PriorityCombo, 1, 3);
			this.OptionsLayout.Dock = System.Windows.Forms.DockStyle.Fill;
			this.OptionsLayout.Location = new System.Drawing.Point(3, 3);
			this.OptionsLayout.Name = "OptionsLayout";
			this.OptionsLayout.RowCount = 5;
			this.OptionsLayout.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.OptionsLayout.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.OptionsLayout.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.OptionsLayout.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.OptionsLayout.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.OptionsLayout.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 20F));
			this.OptionsLayout.Size = new System.Drawing.Size(296, 132);
			this.OptionsLayout.TabIndex = 0;
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
			// LabelPriority
			// 
			this.LabelPriority.AutoSize = true;
			this.LabelPriority.Location = new System.Drawing.Point(3, 78);
			this.LabelPriority.Name = "LabelPriority";
			this.LabelPriority.Padding = new System.Windows.Forms.Padding(0, 4, 0, 0);
			this.LabelPriority.Size = new System.Drawing.Size(68, 17);
			this.LabelPriority.TabIndex = 1;
			this.LabelPriority.Text = "Task Priority:";
			// 
			// PriorityCombo
			// 
			this.PriorityCombo.Dock = System.Windows.Forms.DockStyle.Top;
			this.PriorityCombo.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
			this.PriorityCombo.FormattingEnabled = true;
			this.PriorityCombo.Items.AddRange(new object[] {
            "Lowest",
            "Below Normal",
            "Normal",
            "Above Normal",
            "Highest"});
			this.PriorityCombo.Location = new System.Drawing.Point(103, 81);
			this.PriorityCombo.Name = "PriorityCombo";
			this.PriorityCombo.Size = new System.Drawing.Size(190, 21);
			this.PriorityCombo.TabIndex = 5;
			this.PriorityCombo.SelectedIndexChanged += new System.EventHandler(this.PriorityCombo_SelectedIndexChanged);
			// 
			// tabPage2
			// 
			this.tabPage2.Controls.Add(this.ImportsLayout);
			this.tabPage2.Location = new System.Drawing.Point(4, 22);
			this.tabPage2.Name = "tabPage2";
			this.tabPage2.Padding = new System.Windows.Forms.Padding(3);
			this.tabPage2.Size = new System.Drawing.Size(302, 138);
			this.tabPage2.TabIndex = 1;
			this.tabPage2.Text = "Imports";
			this.tabPage2.UseVisualStyleBackColor = true;
			// 
			// ImportsLayout
			// 
			this.ImportsLayout.ColumnCount = 2;
			this.ImportsLayout.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
			this.ImportsLayout.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
			this.ImportsLayout.Controls.Add(this.LabelStorage, 0, 0);
			this.ImportsLayout.Controls.Add(this.LocalStorageCombo, 1, 0);
			this.ImportsLayout.Controls.Add(this.LabelTags, 0, 1);
			this.ImportsLayout.Controls.Add(this.GH5ExpertPlusCheckbox, 1, 2);
			this.ImportsLayout.Controls.Add(this.NamePrefixCombo, 1, 1);
			this.ImportsLayout.Dock = System.Windows.Forms.DockStyle.Fill;
			this.ImportsLayout.Location = new System.Drawing.Point(3, 3);
			this.ImportsLayout.Name = "ImportsLayout";
			this.ImportsLayout.RowCount = 3;
			this.ImportsLayout.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.ImportsLayout.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.ImportsLayout.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.ImportsLayout.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 20F));
			this.ImportsLayout.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 20F));
			this.ImportsLayout.Size = new System.Drawing.Size(296, 132);
			this.ImportsLayout.TabIndex = 0;
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
            "Store Original Format Locally",
            "Store Rock Band 2 Format Locally"});
			this.LocalStorageCombo.Location = new System.Drawing.Point(106, 3);
			this.LocalStorageCombo.Name = "LocalStorageCombo";
			this.LocalStorageCombo.Size = new System.Drawing.Size(187, 21);
			this.LocalStorageCombo.TabIndex = 5;
			this.LocalStorageCombo.SelectedIndexChanged += new System.EventHandler(this.LocalStorageCombo_SelectedIndexChanged);
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
			this.ButtonLayout.Location = new System.Drawing.Point(0, 164);
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
			this.ClientSize = new System.Drawing.Size(310, 193);
			this.Controls.Add(this.Tab);
			this.Controls.Add(this.ButtonLayout);
			this.MaximizeBox = false;
			this.MinimumSize = new System.Drawing.Size(318, 220);
			this.Name = "SettingsForm";
			this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
			this.Text = "Preferences";
			this.Load += new System.EventHandler(this.SettingsForm_Load);
			this.Tab.ResumeLayout(false);
			this.tabPage1.ResumeLayout(false);
			this.tabPage1.PerformLayout();
			this.OptionsLayout.ResumeLayout(false);
			this.OptionsLayout.PerformLayout();
			((System.ComponentModel.ISupportInitialize)(this.TasksNumeric)).EndInit();
			this.tabPage2.ResumeLayout(false);
			this.ImportsLayout.ResumeLayout(false);
			this.ImportsLayout.PerformLayout();
			this.ButtonLayout.ResumeLayout(false);
			this.ResumeLayout(false);
			this.PerformLayout();

		}

		#endregion

		private System.Windows.Forms.TabControl Tab;
		private System.Windows.Forms.TabPage tabPage1;
		private System.Windows.Forms.TableLayoutPanel OptionsLayout;
		private System.Windows.Forms.TabPage tabPage2;
		private System.Windows.Forms.Label LabelCustoms;
		private System.Windows.Forms.Label LabelTemporary;
		private System.Windows.Forms.Label LabelDefaultAction;
		private System.Windows.Forms.TextBox CustomsText;
		private System.Windows.Forms.TextBox TemporaryText;
		private System.Windows.Forms.ComboBox DefaultActionCombo;
		private System.Windows.Forms.TableLayoutPanel ImportsLayout;
		private System.Windows.Forms.Label LabelTasks;
		private System.Windows.Forms.NumericUpDown TasksNumeric;
		private System.Windows.Forms.Label LabelStorage;
		private System.Windows.Forms.ComboBox LocalStorageCombo;
		private System.Windows.Forms.Label LabelTags;
		private System.Windows.Forms.CheckBox GH5ExpertPlusCheckbox;
		private System.Windows.Forms.ComboBox NamePrefixCombo;
		private System.Windows.Forms.FlowLayoutPanel ButtonLayout;
		private System.Windows.Forms.Button CloseButton;
		private System.Windows.Forms.Label LabelPriority;
		private System.Windows.Forms.ComboBox PriorityCombo;

	}
}