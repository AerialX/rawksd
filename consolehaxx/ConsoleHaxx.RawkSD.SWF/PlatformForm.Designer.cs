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
			this.MainLayout = new System.Windows.Forms.TableLayoutPanel();
			this.PlatformLabel = new System.Windows.Forms.Label();
			this.PlatformList = new System.Windows.Forms.ComboBox();
			this.ButtonLayout = new System.Windows.Forms.FlowLayoutPanel();
			this.ButtonCancel = new System.Windows.Forms.Button();
			this.OpenFileButton = new System.Windows.Forms.Button();
			this.MessageLabel = new System.Windows.Forms.Label();
			this.MainLayout.SuspendLayout();
			this.ButtonLayout.SuspendLayout();
			this.SuspendLayout();
			// 
			// MainLayout
			// 
			this.MainLayout.ColumnCount = 2;
			this.MainLayout.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
			this.MainLayout.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
			this.MainLayout.Controls.Add(this.PlatformLabel, 0, 1);
			this.MainLayout.Controls.Add(this.PlatformList, 1, 1);
			this.MainLayout.Controls.Add(this.ButtonLayout, 0, 2);
			this.MainLayout.Controls.Add(this.MessageLabel, 0, 0);
			this.MainLayout.Dock = System.Windows.Forms.DockStyle.Fill;
			this.MainLayout.Location = new System.Drawing.Point(0, 0);
			this.MainLayout.Name = "MainLayout";
			this.MainLayout.RowCount = 3;
			this.MainLayout.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 20F));
			this.MainLayout.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.MainLayout.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 100F));
			this.MainLayout.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 20F));
			this.MainLayout.Size = new System.Drawing.Size(387, 81);
			this.MainLayout.TabIndex = 0;
			// 
			// PlatformLabel
			// 
			this.PlatformLabel.AutoSize = true;
			this.PlatformLabel.Location = new System.Drawing.Point(3, 20);
			this.PlatformLabel.Name = "PlatformLabel";
			this.PlatformLabel.Padding = new System.Windows.Forms.Padding(0, 4, 0, 0);
			this.PlatformLabel.Size = new System.Drawing.Size(48, 17);
			this.PlatformLabel.TabIndex = 0;
			this.PlatformLabel.Text = "&Platform:";
			// 
			// PlatformList
			// 
			this.PlatformList.Dock = System.Windows.Forms.DockStyle.Top;
			this.PlatformList.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
			this.PlatformList.FormattingEnabled = true;
			this.PlatformList.Location = new System.Drawing.Point(57, 23);
			this.PlatformList.Name = "PlatformList";
			this.PlatformList.Size = new System.Drawing.Size(327, 21);
			this.PlatformList.TabIndex = 1;
			// 
			// ButtonLayout
			// 
			this.MainLayout.SetColumnSpan(this.ButtonLayout, 2);
			this.ButtonLayout.Controls.Add(this.ButtonCancel);
			this.ButtonLayout.Controls.Add(this.OpenFileButton);
			this.ButtonLayout.Dock = System.Windows.Forms.DockStyle.Fill;
			this.ButtonLayout.FlowDirection = System.Windows.Forms.FlowDirection.RightToLeft;
			this.ButtonLayout.Location = new System.Drawing.Point(3, 50);
			this.ButtonLayout.Name = "ButtonLayout";
			this.ButtonLayout.Size = new System.Drawing.Size(381, 28);
			this.ButtonLayout.TabIndex = 2;
			// 
			// ButtonCancel
			// 
			this.ButtonCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
			this.ButtonCancel.Location = new System.Drawing.Point(303, 3);
			this.ButtonCancel.Name = "ButtonCancel";
			this.ButtonCancel.Size = new System.Drawing.Size(75, 23);
			this.ButtonCancel.TabIndex = 0;
			this.ButtonCancel.Text = "&Cancel";
			this.ButtonCancel.UseVisualStyleBackColor = true;
			this.ButtonCancel.Click += new System.EventHandler(this.CancelButton_Click);
			// 
			// OpenFileButton
			// 
			this.OpenFileButton.Location = new System.Drawing.Point(222, 3);
			this.OpenFileButton.Name = "OpenFileButton";
			this.OpenFileButton.Size = new System.Drawing.Size(75, 23);
			this.OpenFileButton.TabIndex = 1;
			this.OpenFileButton.Text = "&Open";
			this.OpenFileButton.UseVisualStyleBackColor = true;
			this.OpenFileButton.Click += new System.EventHandler(this.OpenFileButton_Click);
			// 
			// MessageLabel
			// 
			this.MessageLabel.AutoSize = true;
			this.MainLayout.SetColumnSpan(this.MessageLabel, 2);
			this.MessageLabel.Location = new System.Drawing.Point(3, 0);
			this.MessageLabel.Name = "MessageLabel";
			this.MessageLabel.Size = new System.Drawing.Size(0, 13);
			this.MessageLabel.TabIndex = 3;
			// 
			// PlatformForm
			// 
			this.AcceptButton = this.OpenFileButton;
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.AutoSize = true;
			this.CancelButton = this.ButtonCancel;
			this.ClientSize = new System.Drawing.Size(387, 81);
			this.Controls.Add(this.MainLayout);
			this.Name = "PlatformForm";
			this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
			this.Text = "Select Platform";
			this.Load += new System.EventHandler(this.PlatformForm_Load);
			this.MainLayout.ResumeLayout(false);
			this.MainLayout.PerformLayout();
			this.ButtonLayout.ResumeLayout(false);
			this.ResumeLayout(false);

		}

		#endregion

		private System.Windows.Forms.TableLayoutPanel MainLayout;
		private System.Windows.Forms.Label PlatformLabel;
		private System.Windows.Forms.ComboBox PlatformList;
		private System.Windows.Forms.FlowLayoutPanel ButtonLayout;
		private System.Windows.Forms.Button ButtonCancel;
		private System.Windows.Forms.Button OpenFileButton;
		private System.Windows.Forms.Label MessageLabel;
	}
}