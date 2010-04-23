using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.Graces
{
	public class Tex
	{
		public static Txm Create(Stream stream)
		{
			FPS4 fps = new FPS4(stream);
			if (fps.Type == "txmv") {
				if (fps.Root.Children.Count != 2)
					throw new FormatException();

				return new Txm((fps.Root.Children[0] as FileNode).Data, (fps.Root.Children[1] as FileNode).Data);
			} else if (fps.Type == "pktx") {
				return Create((fps.Root.Children[0] as FileNode).Data);
			}

			throw new FormatException();
		}
	}
}
