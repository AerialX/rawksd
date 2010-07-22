using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.RawkSD.SWF
{
	public partial class MainForm
	{
		public List<TaskScheduler> ProgressList;
		public Dictionary<TaskScheduler, ProgressItem> ProgressControls;
		public TaskScheduler Progress { get { return ProgressList.OrderBy(p => p.Tasks.Count).FirstOrDefault(); } }

		private TaskScheduler AsyncProgress = null;

		public TaskScheduler GetAsyncProgress()
		{
			if (AsyncProgress != null)
				return AsyncProgress;
			AsyncProgress = AddTaskScheduler();
			RemoveTaskScheduler(AsyncProgress);
			AsyncProgress.OnUpdate += new Action<TaskScheduler, string, string, double>(AsyncProgress_OnUpdate);
			return AsyncProgress;
		}

		void AsyncProgress_OnUpdate(TaskScheduler task, string rootstatus, string status, double percent)
		{
			if (rootstatus == null && status == null && percent == 0)
				AsyncProgress = null;
		}

		public TaskScheduler AddTaskScheduler()
		{
			TaskScheduler task = new TaskScheduler(this);
			ProgressList.Add(task);

			ProgressItem item = new ProgressItem();
			item.Minimum = 0;
			item.Maximum = 100000;
			item.Visible = false;
			ProgressControls.Add(task, item);

			task.OnUpdate += new Action<TaskScheduler, string, string, double>(Progress_OnUpdate);
			task.OnError += new Action<TaskScheduler, ProgressException>(Progress_OnError);

			return task;
		}

		void Progress_OnError(TaskScheduler task, ProgressException exception)
		{
			Invoke((Action<TaskScheduler, ProgressException>)Progress_OnError_Base, task, exception);
		}

		void Progress_OnError_Base(TaskScheduler task, ProgressException exception)
		{
			MessageBox.Show(this, (exception.Message.HasValue() ? exception.Message : "An unspecified error occurred.") +
				"\nFor more information about this error, see the rawksd.log file (in the same folder as RawkSD is)." +
				"\nIf the problem persists or you need help with RawkSD, please send this log file along with any related files and an explanation of what you were doing to rawksd@japaneatahand.com"
				, "RawkSD Error!", MessageBoxButtons.OK, MessageBoxIcon.Error);
		}

		void Progress_OnUpdate(TaskScheduler task, string rootstatus, string status, double percent)
		{
			Invoke((Action<TaskScheduler, string, string, double>)Progress_OnUpdate_Base, task, rootstatus, status, percent);
		}

		void Progress_OnUpdate_Base(TaskScheduler task, string rootstatus, string status, double percent)
		{
			ProgressItem item = ProgressControls[task];
			if (rootstatus == null && status == null && percent == 0) {
				item.Visible = false;
				if (item.Parent != null)
					item.Parent.Controls.Remove(item);
				AutoManageTasks();
				return;
			} else if (item.Visible == false) {
				item.Visible = true;
				ProgressLayout.Controls.Add(item);
				item.Dock = DockStyle.Fill;
				AutoManageTasks();
			}

			long progress;
			long max;
			task.Progress.GetTask(out progress, out max);
			string taskprogress = string.Empty;
			if (max > 1)
				taskprogress = "[" + (progress + 1).ToString() + " / " + max.ToString() + "] ";
			item.Text = taskprogress + rootstatus;
			item.ToolTipText = status;
			item.Value = (int)(percent * (double)item.Maximum);

			AutoResizeTasks();
		}

		private void AutoManageTasks()
		{
			if (ProgressLayout.Controls.Count == 0) {
				ProgressLayout.Width = 0;
				ProgressLayout.Height = 0;
				return;
			}

			ProgressLayout.RowCount = Math.Min(4, ProgressLayout.Controls.Count);
			ProgressLayout.ColumnCount = (int)Util.RoundUp(ProgressLayout.Controls.Count, ProgressLayout.RowCount) / ProgressLayout.RowCount;
			
			ProgressLayout.ColumnStyles.Clear();
			for (int x = 0; x < ProgressLayout.ColumnCount; x++)
				ProgressLayout.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute));
			ProgressLayout.RowStyles.Clear();
			for (int y = 0; y < ProgressLayout.RowCount; y++)
				ProgressLayout.RowStyles.Add(new RowStyle(SizeType.AutoSize));

			int index = 0;
			for (int x = ProgressLayout.ColumnCount - 1; x >= 0; x--) {
				for (int y = 0; y < ProgressLayout.RowCount; y++) {
					if (index < ProgressLayout.Controls.Count) {
						Control control = ProgressLayout.Controls[index++];
						ProgressLayout.SetColumn(control, x);
						ProgressLayout.SetRow(control, y);
					}
				}
			}
		}

		private void AutoResizeTasks()
		{
			if (ProgressLayout.Controls.Count == 0) {
				ProgressLayout.Width = 0;
				ProgressLayout.Height = 0;
				return;
			}
			int cumulativewidth = 0;
			for (int x = 0; x < ProgressLayout.ColumnCount; x++) {
				int width = 0;
				for (int y = 0; y < ProgressLayout.RowCount; y++) {
					ProgressItem item = ProgressLayout.GetControlFromPosition(x, y) as ProgressItem;
					if (item == null)
						continue;
					if (item.PreferredWidth > width)
						width = item.PreferredWidth;
				}
				ProgressLayout.ColumnStyles[x].Width = width;
				cumulativewidth += width + 3;
			}
			cumulativewidth += 3;

			if (ProgressLayout.Width != cumulativewidth)
				ProgressLayout.Width = cumulativewidth;
		}

		public void RemoveTaskScheduler(TaskScheduler task)
		{
			ProgressList.Remove(task);
			task.OnUpdate += new Action<TaskScheduler, string, string, double>(RemoveTask_OnUpdate);
		}

		void RemoveTask_OnUpdate(TaskScheduler task, string rootstatus, string status, double percent)
		{
			if (rootstatus == null && status == null && percent == 0)
				task.Exit(false);
		}
	}
}
