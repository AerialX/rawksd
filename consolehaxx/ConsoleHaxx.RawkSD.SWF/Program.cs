using System;
using System.Windows.Forms;

namespace ConsoleHaxx.RawkSD.SWF
{
	static class Program
	{
		public static MainForm Form { get; set; }

		[STAThread]
		static void Main()
		{
			Platform.Initialise();

			Application.EnableVisualStyles();
			Application.SetCompatibleTextRenderingDefault(false);
			Form = new MainForm();
			Application.Run(Form);
		}
	}
}
