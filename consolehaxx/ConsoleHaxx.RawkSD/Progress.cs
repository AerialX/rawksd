using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.RawkSD
{
	[Serializable]
	public class ProgressException : Exception
	{
		public ProgressException() { }
		public ProgressException(string message) : base(message) { }
		public ProgressException(string message, Exception inner) : base(message, inner) { }
	}

	public class ProgressIndicator
	{
		protected List<Task> Tasks;
		protected long NextWeight;

		public event Action<ProgressIndicator> OnDone;
		public event Action<ProgressIndicator> OnUpdate;
		public event Action<ProgressIndicator, ProgressException> OnError;

		public ProgressIndicator()
		{
			NextWeight = 1;
			Tasks = new List<Task>();
			Reset();
		}

		public void QueueTask(long weight = 1)
		{
			Tasks[0].Max += weight;
		}

		public void DequeueTask(long weight = 1)
		{
			Tasks[0].Progress += weight;
		}

		public void ResetTasks()
		{
			Tasks[0].Max = 0;
			Tasks[0].Progress = 0;
		}

		public void Reset()
		{
			Tasks.Clear();
			NewTask(0);
		}

		public void SetNextWeight(long weight)
		{
			NextWeight = weight;
		}

		public void NewTask(long max = 1, long weight = 0) { NewTask("", max, weight); }
		public void NewTask(string name = "", long max = 1, long weight = 0)
		{
			if (weight == 0)
				weight = NextWeight;
			NextWeight = 1;
			Tasks.Add(new Task(name, max, weight));
			ProgressChanged();
		}

		public void SetTask(long max = 1, long weight = 1) { SetTask("", max, weight); }
		public void SetTask(string name = "", long max = 1, long weight = 1)
		{
			Task task = Tasks.Last();
			task.Name = name;
			task.Max = max;
			task.Weight = weight;
			ProgressChanged();
		}

		public void GetTask(out long progress, out long max)
		{
			Task task = Tasks[0];
			progress = task.Progress;
			max = task.Max;
		}

		public void EndTask()
		{
			Tasks.RemoveAt(Tasks.Count - 1);

			if (Tasks.Count == 1) {
				if (OnDone != null)
					OnDone(this);
			}

			ProgressChanged();
		}

		public void SetProgress(long progress)
		{
			NextWeight = 1;
			Tasks.Last().Progress = progress;
			ProgressChanged();
		}

		public void Progress(long increment = 1)
		{
			NextWeight = 1;
			Tasks.Last().Progress += increment;
			ProgressChanged();
		}

		protected void ProgressChanged()
		{
			if (OnUpdate != null)
				OnUpdate(this);
		}

		public void Error(string message, Exception exception = null)
		{
			throw new ProgressException(message, exception);
		}

		public void ErrorHandler(ProgressException exception)
		{
			if (OnError != null)
				OnError(this, exception);
		}

		public string RootStatus
		{
			get
			{
				for (int i = 1; i < Tasks.Count; i++)
					if (Tasks[i].Name.HasValue())
						return Tasks[i].Name;

				return string.Empty;
			}
		}

		public string Status
		{
			get
			{
				for (int i = Tasks.Count - 1; i > 1; i--)
					if (Tasks[i].Name.HasValue())
						return Tasks[i].Name;

				return string.Empty;
			}
		}

		public double Percent
		{
			get
			{
				double percent = 0;
				double unit = 1.0;
				foreach (Task task in Tasks) {
					if (task.Max == 0)
						return 0;

					percent += unit * task.Percent * (double)task.Weight;
					unit *= 1.0 / (double)task.Max * (double)task.Weight;
				}

				return Math.Min(percent, 1.0);
			}
		}

		protected class Task
		{
			public Task(string name, long max, long weight)
			{
				Name = name;
				Max = max;
				Weight = weight;
				Progress = 0;
			}

			public string Name;
			public long Max;
			public long Progress;
			public long Weight;

			public double Percent
			{
				get
				{
					return (double)Progress / (double)Max;
				}
			}
		}
	}
}
