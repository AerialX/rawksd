namespace ConsoleHaxx.RawkSD.SWF
{
	partial class PlatformForm
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
			this.tableLayoutPanel1 = new System.Windows.Forms.TableLayoutPanel();
			this.PlatformLabel = new System.Windows.Forms.Label();
			this.PlatformList = new System.Windows.Forms.ComboBox();
			this.flowLayoutPanel1 = new System.Windows.Forms.FlowLayoutPanel();
			this.CancelButton = new System.Windows.Forms.Button();
			this.OpenFileButton = new System.Windows.Forms.Button();
			this.OpenFolderButton = new System.Windows.Forms.Button();
			this.tableLayoutPanel1.SuspendLayout();
			this.flowLayoutPanel1.SuspendLayout();
			this.SuspendLayout();
			// 
			// tableLayoutPanel1
			// 
			this.tableLayoutPanel1.ColumnCount = 2;
			this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
			this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
			this.tableLayoutPanel1.Controls.Add(this.PlatformLabel, 0, 0);
			this.tableLayoutPanel1.Controls.Add(this.PlatformList, 1, 0);
			this.tableLayoutPanel1.Controls.Add(this.flowLayoutPanel1, 0, 1);
			this.tableLayoutPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
			this.tableLayoutPanel1.Location = new System.Drawing.Point(0, 0);
			this.tableLayoutPanel1.Name = "tableLayoutPanel1";
			this.tableLayoutPanel1.RowCount = 2;
			this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 100F));
			this.tableLayoutPanel1.Size = new System.Drawing.Size(293, 61);
			this.tableLayoutPanel1.TabIndex = 0;
			// 
			// PlatformLabel
			// 
			this.PlatformLabel.AutoSize = true;
			this.PlatformLabel.Location = new System.Drawing.Point(3, 0);
			this.PlatformLabel.Name = "PlatformLabel";
			this.PlatformLabel.Padding = new System.Windows.Forms.Padding(0, 4, 0, 0);
			this.PlatformLabel.Size = new System.Drawing.Size(48, 17);
			this.PlatformLabel.TabIndex = 0;
			this.PlatformLabel.Text = "Platform:";
			// 
			// PlatformList
			// 
			this.PlatformList.Dock = System.Windows.Forms.DockStyle.Top;
			this.PlatformList.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
			this.PlatformList.FormattingEnabled = true;
			this.PlatformList.Location = new System.Drawing.Point(57, 3);
			this.PlatformList.Name = "PlatformList";
			this.PlatformList.Size = new System.Drawing.Size(233, 21);
			this.PlatformList.TabIndex = 1;
			// 
			// flowLayoutPanel1
			// 
			this.tableLayoutPanel1.SetColumnSpan(this.flowLayoutPanel1, 2);
			this.flowLayoutPanel1.Controls.Add(this.CancelButton);
			this.flowLayoutPanel1.Controls.Add(this.OpenFileButton);
			this.flowLayoutPanel1.Controls.Add(this.OpenFolderButton);
			this.flowLayoutPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
			this.flowLayoutPanel1.FlowDirection = System.Windows.Forms.FlowDirection.RightToLeft;
			this.flowLayoutPanel1.Location = new System.Drawing.Point(3, 30);
			this.flowLayoutPanel1.Name = "flowLayoutPanel1";
			this.flowLayoutPanel1.Size = new System.Drawing.Size(287, 28);
			this.flowLayoutPanel1.TabIndex = 2;
			// 
			// CancelButton
			// 
			this.CancelButton.Location = new System.Drawing.Point(209, 3);
			this.CancelButton.Name = "CancelButton";
			this.CancelButton.Size = new System.Drawing.Size(75, 23);
			this.CancelButton.TabIndex = 0;
			this.CancelButton.Text = "&Cancel";
			this.CancelButton.UseVisualStyleBackColor = true;
			this.CancelButton.Click += new System.EventHandler(this.CancelButton_Click);
			// 
			// OpenFileButton
			// 
			this.OpenFileButton.Location = new System.Drawing.Point(128, 3);
			this.OpenFileButton.Name = "OpenFileButton";
			this.OpenFileButton.Size = new System.Drawing.Size(75, 23);
			this.OpenFileButton.TabIndex = 1;
			this.OpenFileButton.Text = "Open &File";
			this.OpenFileButton.UseVisualStyleBackColor = true;
			this.OpenFileButton.Click += new System.EventHandler(this.OpenFileButton_Click);
			// 
			// OpenFolderButton
			// 
			this.OpenFolderButton.Location = new System.Drawing.Point(47, 3);
			this.OpenFolderButton.Name = "OpenFolderButton";
			this.OpenFolderButton.Size = new System.Drawing.Size(75, 23);
			this.OpenFolderButton.TabIndex = 2;
			this.OpenFolderButton.Text = "Open F&older";
			this.OpenFolderButton.UseVisualStyleBackColor = true;
			this.OpenFolderButton.Click += new System.EventHandler(this.OpenFolderButton_Click);
			// 
			// PlatformForm
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.ClientSize = new System.Drawing.Size(293, 61);
			this.Controls.Add(this.tableLayoutPanel1);
			this.Name = "PlatformForm";
			this.Text = "Select Platform";
			this.Load += new System.EventHandler(this.PlatformForm_Load);
			this.tableLayoutPanel1.ResumeLayout(false);
			this.tableLayoutPanel1.PerformLayout();
			this.flowLayoutPanel1.ResumeLayout(false);
			this.ResumeLayout(false);

		}

		#endregion

		private System.Windows.Forms.TableLayoutPanel tableLayoutPanel1;
		private System.Windows.Forms.Label PlatformLabel;
		private System.Windows.Forms.ComboBox PlatformList;
		private System.Windows.Forms.FlowLayoutPanel flowLayoutPanel1;
		private System.Windows.Forms.Button CancelButton;
		private System.Windows.Forms.Button OpenFileButton;
		private System.Windows.Forms.Button OpenFolderButton;
	}
}