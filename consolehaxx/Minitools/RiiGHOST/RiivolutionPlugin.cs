using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Nanook.TheGhost;
using ConsoleHaxx.Wii;
using System.IO;
using ConsoleHaxx.Common;
using System.Windows.Forms;

namespace RiiGHOST
{
	public class RiivolutionPlugin : IPluginFileCopy
	{
		Disc ISO;
		string BasePath;
		string FileBasePath;

		public RiivolutionPlugin()
		{
			BasePath = null;
		}

		public void ExportFile(string diskFilename, string gameFilename, FileCopyProgress callback)
		{
			string outpath = Path.Combine(FileBasePath, gameFilename.StartsWith("/") ? gameFilename.Substring(1) : gameFilename);

			Directory.CreateDirectory(Path.GetDirectoryName(outpath));

			FileStream fin = new FileStream(diskFilename, FileMode.Open, FileAccess.Read);
			FileStream fout = new FileStream(outpath, FileMode.Create, FileAccess.Write);
			Util.StreamCopy(fout, fin);
			fin.Close();
			fout.Close();

			string flistpath = Path.Combine(BasePath, "riivolution");
			Directory.CreateDirectory(flistpath);
			flistpath = Path.Combine(flistpath, "theghost.xml");
			FileStream flist = new FileStream(flistpath, FileMode.Create);
			StreamWriter writer = new StreamWriter(flist, Util.Encoding);
			writer.WriteLine("<wiidisc version=\"1\">");
				writer.WriteLine("<id game=\"RGH\" />"); // TODO: Aerosmith?
				writer.WriteLine("<options>");
					writer.WriteLine("<section name=\"TheGHOST\">");
						writer.WriteLine("<option name=\"TheGHOST\">");
							writer.WriteLine("<choice name=\"Enabled\">");
								writer.WriteLine("<patch id=\"theghostfolder\" />");
							writer.WriteLine("</choice>");
						writer.WriteLine("</option>");
					writer.WriteLine("</section>");
				writer.WriteLine("</options>");
				writer.WriteLine("<patch id=\"theghostfolder\">");
					writer.WriteLine("<folder disc=\"/\" external=\"/theghost\" create=\"true\" />");
				writer.WriteLine("</patch>");
			writer.WriteLine("</wiidisc>");
			writer.Close();
		}

		public void ExportFiles(string[] diskFilenames, string[] gameFilenames, FileCopyProgress callback)
		{
			for (int i = 0; i < diskFilenames.Length; i++)
				ExportFile(diskFilenames[i], gameFilenames[i], callback);
		}

		public void ImportFile(string gameFilename, string diskFilename, FileCopyProgress callback)
		{
			Node node = ISO.DataPartition.Root.Root;
			FileNode file = (node as DirectoryNode).Navigate(gameFilename) as FileNode;
			if (file == null)
				return;

			FileStream f = new FileStream(diskFilename, FileMode.Create, FileAccess.Write);
			file.Data.Position = 0;
			Util.StreamCopy(f, file.Data, file.Size);
			f.Close();
		}

		public void ImportFiles(string[] gameFilenames, string[] diskFilenames, FileCopyProgress callback)
		{
			for (int i = 0; i < diskFilenames.Length; i++)
				ImportFile(gameFilenames[i], diskFilenames[i], callback);
		}

		public void SetSourcePath(string path)
		{
			ISO = new Disc(new FileStream(path, FileMode.Open, FileAccess.Read));

			if (BasePath == null) {
				FolderBrowserDialog folder = new FolderBrowserDialog();
				folder.Description = "Select your SD/SDHC/USB root that you will use for Riivolution";
				if (folder.ShowDialog() == DialogResult.Cancel)
					return;
				BasePath = folder.SelectedPath;

				FileBasePath = Path.Combine(BasePath, "theghost");
			}
		}

		public string Description()
		{
			return "Pwnage";
		}

		public string Title()
		{
			return "Riivolution Export Plugin";
		}

		public float Version()
		{
			return 1.0f;
		}
	}
}
