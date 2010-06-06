using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Qyoto;

namespace ConsoleHaxx.RawkSD.Qyoto
{
	partial class MainForm : QMainWindow
	{
		public MainForm()
		{
			Initialise();

			Resize(300, 300);
			Move(300, 300);
			Show();
		}
	}
}
