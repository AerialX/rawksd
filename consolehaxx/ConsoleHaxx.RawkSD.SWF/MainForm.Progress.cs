using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace ConsoleHaxx.RawkSD.SWF
{
	public partial class MainForm
	{
		public List<TaskScheduler> ProgressList;
		public Dictionary<TaskScheduler, VisualTask> ProgressControls;
		public TaskScheduler Progress { get { return ProgressList.OrderBy(p => p.Tasks.Count).FirstOrDefault(); } }

		public TaskScheduler GetAsyncProgress()
		{
			if (Progress.Tasks.Count == 0)
				return Progress;
			TaskScheduler progress = AddTaskScheduler();
			progress.OnUpdate += (a, b, c, d) => {
				if (progress.Tasks.Count == 0) {
					progress.Exit();
					RemoveTaskScheduler(a);
				}
			};
			return progress;
		}

		public class VisualTask
		{
			public VisualTask(ToolStrip status, ToolStripLabel label, ToolStripProgressBar progress)
			{
				Status = status;
				Label = label;
				Progress = progress;
			}

			public ToolStrip Status;
			public ToolStripLabel Label;
			public ToolStripProgressBar Progress;
		}

		public TaskScheduler AddTaskScheduler()
		{
			TaskScheduler task = new TaskScheduler(this);
			ProgressList.Add(task);

			ToolStrip status = new ToolStrip();
			status.GripStyle = ToolStripGripStyle.Visible;
			ToolStripProgressBar progress = new ToolStripProgressBar();
			progress.Style = ProgressBarStyle.Continuous;
			progress.Minimum = 0;
			progress.Maximum = 100000;
			ToolStripLabel label = new ToolStripLabel();
			label.AutoSize = true;
			status.Items.Add(progress);
			status.Items.Add(label);
			status.Visible = false;
			status.AutoSize = true;
			status.CanOverflow = false;
			ProgressControls.Add(task, new VisualTask(status, label, progress));
			task.OnUpdate += new Action<TaskScheduler, string, string, double>(Progress_OnUpdate);
			task.OnError += new Action<TaskScheduler, ProgressException>(Progress_OnError);

			return task;
		}

		void Progress_OnError(TaskScheduler task, ProgressException exception)
		{
			throw new NotImplementedException();
		}

		void Progress_OnUpdate(TaskScheduler task, string rootstatus, string status, double percent)
		{
			VisualTask controls = ProgressControls[task];
			if (rootstatus == null && status == null && percent == 0) {
				controls.Status.Visible = false;
				Control parent = controls.Status.Parent;
				if (parent != null)
					parent.Controls.Remove(controls.Status);
				return;
			} else if (controls.Status.Visible == false) {
				controls.Status.Visible = true;
				ToolDock.BottomToolStripPanel.Controls.Add(controls.Status);
			}

			long progress;
			long max;
			task.Progress.GetTask(out progress, out max);
			string taskprogress = string.Empty;
			if (max > 1)
				taskprogress = "[" + (progress + 1).ToString() + " / " + max.ToString() + "] ";
			controls.Label.Text = taskprogress + rootstatus.Replace("&", "&&");
			controls.Label.ToolTipText = status;
			controls.Progress.ToolTipText = status;
			controls.Progress.Value = (int)(percent * (double)controls.Progress.Maximum);
		}

		public void RemoveTaskScheduler(TaskScheduler task)
		{
			ProgressList.Remove(task);
		}
	}
}
