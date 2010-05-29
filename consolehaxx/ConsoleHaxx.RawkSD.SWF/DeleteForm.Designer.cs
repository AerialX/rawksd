namespace ConsoleHaxx.RawkSD.SWF
{
	partial class DeleteForm
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
			this.ButtonLayout = new System.Windows.Forms.FlowLayoutPanel();
			this.CloseButton = new System.Windows.Forms.Button();
			this.ButtonCancel = new System.Windows.Forms.Button();
			this.LabelDescription = new System.Windows.Forms.Label();
			this.PlatformList = new System.Windows.Forms.ListBox();
			this.tableLayoutPanel1.SuspendLayout();
			this.ButtonLayout.SuspendLayout();
			this.SuspendLayout();
			// 
			// tableLayoutPanel1
			// 
			this.tableLayoutPanel1.ColumnCount = 1;
			this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
			this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 20F));
			this.tableLayoutPanel1.Controls.Add(this.ButtonLayout, 0, 2);
			this.tableLayoutPanel1.Controls.Add(this.LabelDescription, 0, 0);
			this.tableLayoutPanel1.Controls.Add(this.PlatformList, 0, 1);
			this.tableLayoutPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
			this.tableLayoutPanel1.Location = new System.Drawing.Point(0, 0);
			this.tableLayoutPanel1.Name = "tableLayoutPanel1";
			this.tableLayoutPanel1.RowCount = 3;
			this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 100F));
			this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle());
			this.tableLayoutPanel1.Size = new System.Drawing.Size(292, 148);
			this.tableLayoutPanel1.TabIndex = 0;
			// 
			// ButtonLayout
			// 
			this.ButtonLayout.AutoSize = true;
			this.ButtonLayout.Controls.Add(this.CloseButton);
			this.ButtonLayout.Controls.Add(this.ButtonCancel);
			this.ButtonLayout.Dock = System.Windows.Forms.DockStyle.Bottom;
			this.ButtonLayout.FlowDirection = System.Windows.Forms.FlowDirection.RightToLeft;
			this.ButtonLayout.Location = new System.Drawing.Point(3, 116);
			this.ButtonLayout.Name = "ButtonLayout";
			this.ButtonLayout.Size = new System.Drawing.Size(286, 29);
			this.ButtonLayout.TabIndex = 2;
			// 
			// CloseButton
			// 
			this.CloseButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
			this.CloseButton.Location = new System.Drawing.Point(208, 3);
			this.CloseButton.Name = "CloseButton";
			this.CloseButton.Size = new System.Drawing.Size(75, 23);
			this.CloseButton.TabIndex = 0;
			this.CloseButton.Text = "&OK";
			this.CloseButton.UseVisualStyleBackColor = true;
			this.CloseButton.Click += new System.EventHandler(this.CloseButton_Click);
			// 
			// ButtonCancel
			// 
			this.ButtonCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
			this.ButtonCancel.Location = new System.Drawing.Point(127, 3);
			this.ButtonCancel.Name = "ButtonCancel";
			this.ButtonCancel.Size = new System.Drawing.Size(75, 23);
			this.ButtonCancel.TabIndex = 1;
			this.ButtonCancel.Text = "&Cancel";
			this.ButtonCancel.UseVisualStyleBackColor = true;
			this.ButtonCancel.Click += new System.EventHandler(this.ButtonCancel_Click);
			// 
			// LabelDescription
			// 
			this.LabelDescription.AutoSize = true;
			this.LabelDescription.Location = new System.Drawing.Point(3, 0);
			this.LabelDescription.Name = "LabelDescription";
			this.LabelDescription.Size = new System.Drawing.Size(219, 13);
			this.LabelDescription.TabIndex = 0;
			this.LabelDescription.Text = "Select the platforms to delete the songs from:";
			// 
			// PlatformList
			// 
			this.PlatformList.Dock = System.Windows.Forms.DockStyle.Fill;
			this.PlatformList.FormattingEnabled = true;
			this.PlatformList.Location = new System.Drawing.Point(3, 16);
			this.PlatformList.Name = "PlatformList";
			this.PlatformList.SelectionMode = System.Windows.Forms.SelectionMode.MultiExtended;
			this.PlatformList.Size = new System.Drawing.Size(286, 94);
			this.PlatformList.TabIndex = 1;
			this.PlatformList.DoubleClick += new System.EventHandler(this.PlatformList_DoubleClick);
			// 
			// DeleteForm
			// 
			this.AcceptButton = this.CloseButton;
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.CancelButton = this.ButtonCancel;
			this.ClientSize = new System.Drawing.Size(292, 148);
			this.Controls.Add(this.tableLayoutPanel1);
			this.Name = "DeleteForm";
			this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
			this.Text = "Delete Songs";
			this.tableLayoutPanel1.ResumeLayout(false);
			this.tableLayoutPanel1.PerformLayout();
			this.ButtonLayout.ResumeLayout(false);
			this.ResumeLayout(false);

		}

		#endregion

		private System.Windows.Forms.TableLayoutPanel tableLayoutPanel1;
		private System.Windows.Forms.Label LabelDescription;
		private System.Windows.Forms.ListBox PlatformList;
		private System.Windows.Forms.FlowLayoutPanel ButtonLayout;
		private System.Windows.Forms.Button CloseButton;
		private System.Windows.Forms.Button ButtonCancel;
	}
}