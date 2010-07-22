using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Windows.Forms;

namespace ConsoleHaxx.RawkSD.SWF
{
	public class TaskScheduler
	{
		public static ThreadPriority DefaultThreadPriority = ThreadPriority.BelowNormal;
		public ProgressIndicator Progress;
		public Queue<Action<ProgressIndicator>> Tasks;
		public Thread Thread;
		public Mutex FlagMutex;
		public Control Owner;

		public event Action<TaskScheduler, string, string, double> OnUpdate;
		public event Action<TaskScheduler, ProgressException> OnError;

		protected int exitflag;
		protected int taskflag;
		protected int waitflag;

		protected string lastrootstatus;
		protected string laststatus;
		protected double lastpercent;

		public TaskScheduler(Control owner)
		{
			Progress = new ProgressIndicator();
			Tasks = new Queue<Action<ProgressIndicator>>();
			FlagMutex = new Mutex();

			Owner = owner;

			Progress.OnDone += new Action<ProgressIndicator>(Progress_OnDone);
			Progress.OnUpdate += new Action<ProgressIndicator>(Progress_OnUpdate);
			Progress.OnError += new Action<ProgressIndicator, ProgressException>(Progress_OnError);

			SetFlag(ref exitflag, false);
			SetFlag(ref taskflag, false);
			SetFlag(ref waitflag, false);
			Thread = new Thread(ExecutionThread);
			Thread.Priority = DefaultThreadPriority;
			Thread.IsBackground = true;
			Thread.Start();
		}

		public void ExecutionThread()
		{
			while (!ReadFlag(ref exitflag)) {
				if (ReadFlag(ref taskflag)) {
					try {
						Tasks.Peek()(Progress);
					} catch (ThreadAbortException) {
					} catch (Exception exception) {
						if (!(exception is ProgressException)) {
							Exceptions.Warning(exception);
							exception = new ProgressException(exception.Message, exception);
						}
						Progress.ErrorHandler(exception as ProgressException);
						Progress.Unwind();
					}
				} else {
					SetFlag(ref waitflag, true);
					try {
						Thread.Sleep(Timeout.Infinite);
					} catch (ThreadInterruptedException) { }
					SetFlag(ref waitflag, false);
				}
			}
		}

		public void Exit(bool force = false)
		{
			SetFlag(ref exitflag, true);
			Interrupt();
			if (force)
				Thread.Abort();
		}

		public void Interrupt()
		{
			if (ReadFlag(ref waitflag))
				Thread.Interrupt();
		}

		protected bool ReadFlag(ref int flag)
		{
			FlagMutex.WaitOne();
			bool ret = Thread.VolatileRead(ref flag) != 0;
			FlagMutex.ReleaseMutex();
			return ret;
		}

		protected void SetFlag(ref int flag, bool value)
		{
			FlagMutex.WaitOne();
			Thread.VolatileWrite(ref flag, value ? 1 : 0);
			FlagMutex.ReleaseMutex();
		}

		void Progress_OnError(ProgressIndicator progress, ProgressException exception)
		{
			if (OnError != null)
				OnError(this, exception);
		}

		void Progress_OnUpdate(ProgressIndicator progress)
		{
			if (OnUpdate != null) {
				if (Tasks.Count == 0) {
					lastrootstatus = null;
					laststatus = null;
					lastpercent = 0;
					OnUpdate(this, null, null, 0);
					return;
				}
				string rootstatus = Progress.RootStatus;
				string status = Progress.Status;
				double percent = Progress.Percent;
				if (lastrootstatus == rootstatus && laststatus == status && (Math.Abs(lastpercent - percent) < 0.0001))
					return;

				lastrootstatus = rootstatus;
				laststatus = status;
				lastpercent = percent;

				OnUpdate(this, rootstatus, status, percent);
			}
		}

		void Progress_OnDone(ProgressIndicator progress)
		{
			Tasks.Dequeue();
			Progress.DequeueTask();

			if (Tasks.Count == 0) {
				Progress.ResetTasks();
				SetFlag(ref taskflag, false);
			}
		}

		public void QueueTask(Action<ProgressIndicator> task)
		{
			Tasks.Enqueue(task);
			Progress.QueueTask();
			if (!ReadFlag(ref taskflag)) {
				SetFlag(ref taskflag, true);
				Interrupt();
			}
		}
	}
}
