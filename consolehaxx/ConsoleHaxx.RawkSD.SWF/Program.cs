using System;
using System.Windows.Forms;
using System.Threading;

namespace ConsoleHaxx.RawkSD.SWF
{
	static class Program
	{
		public static MainForm Form { get; set; }

		private static Mutex SingleMutex;

		[STAThread]
		static void Main()
		{
			try {
				SingleMutex = Mutex.OpenExisting("RawkSDMutexProcess");
				return;
			} catch {
				SingleMutex = new Mutex(false, "RawkSDMutexProcess");
			}
			Application.ApplicationExit += new EventHandler(Application_ApplicationExit);

			Platform.Initialise();

			Application.EnableVisualStyles();
			Application.SetCompatibleTextRenderingDefault(false);
			Form = new MainForm();
			Application.Run(Form);
		}

		static void Application_ApplicationExit(object sender, EventArgs e)
		{
			try {
				SingleMutex.ReleaseMutex();
			} catch { }
		}
	}
}
