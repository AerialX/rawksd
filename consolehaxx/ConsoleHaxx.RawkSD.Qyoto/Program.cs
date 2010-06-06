using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Qyoto;

namespace ConsoleHaxx.RawkSD.Qyoto
{
	class Program
	{
		public static MainForm Form;

		static void Main(string[] args)
		{
			Platform.Initialise();

			QApplication app = new QApplication(args);

			Form = new MainForm();

			QApplication.Exec();
		}
	}
}
