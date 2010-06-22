using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

namespace ConsoleHaxx.RiiFS
{
	class Program
	{
		static void Main(string[] args)
		{
			string root = Environment.CurrentDirectory;
			int port = 1137;
			bool read = false;

			List<string> param = new List<string>();
			foreach (string arg in args) {
				if (arg.StartsWith("-r")) {
					read = true;
				} else
					param.Add(arg);
			}

			if (param.Count > 0)
				root = param[0];

			if (!Path.IsPathRooted(root))
				root = Path.Combine(Environment.CurrentDirectory, root);

			if (param.Count > 1)
				port = int.Parse(param[1]);

			Server server = new Server(root, port);
			server.ReadOnly = read;
			server.StartBroadcastAsync();
			server.Start();
		}
	}
}
