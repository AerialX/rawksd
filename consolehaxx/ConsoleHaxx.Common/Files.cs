using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

namespace ConsoleHaxx.Common
{
	public class Node
	{
		public string Name;
		public DirectoryNode Parent;

		public string Filename
		{
			get
			{
				string name = Name;
				for (DirectoryNode dir = Parent; dir.Parent != null; dir = dir.Parent) {
					name = dir.Name + "/" + name;
				}
				return name;
			}
		}

		public Node()
		{
			Name = string.Empty;
		}

		public Node(string name, DirectoryNode parent)
		{
			Name = name;
			Parent = parent;

			if (Parent != null)
				Parent.Children.Add(this);
		}

		public override string ToString()
		{
			return Name;
		}
	}

	public class DirectoryNode : Node
	{
		public DirectoryNode() : base()
		{
			Children = new List<Node>();
		}

		public DirectoryNode(string name, DirectoryNode parent) : base(name, parent)
		{
			Children = new List<Node>();
		}

		public List<Node> Children;

		public Node Find(string name)
		{
			return Find(name, false);
		}

		public Node Find(string name, bool ignorecase)
		{
			return Find(name, SearchOption.TopDirectoryOnly, ignorecase);
		}

		public Node Find(string name, SearchOption search)
		{
			return Find(name, search, false);
		}

		public Node Find(string name, SearchOption search, bool ignorecase)
		{
			foreach (Node node in Children) {
				if (String.Compare(node.Name, name, ignorecase) == 0)
					return node;

				if (search == SearchOption.AllDirectories && node is DirectoryNode) {
					Node found = (node as DirectoryNode).Find(name, search, ignorecase);
					if (found != null)
						return found;
				}
			}

			return null;
		}

		public Node Navigate(string path)
		{
			return Navigate(path, false);
		}

		public Node Navigate(string path, bool create)
		{
			return Navigate(path, create, false);
		}

		public Node Navigate(string path, bool create, bool ignorecase)
		{
			string[] nodes = path.Split(new char[] { '/' }, StringSplitOptions.RemoveEmptyEntries);
			Node current = this;
			foreach (string name in nodes) {
				if (!(current is DirectoryNode))
					return null; // Only the last node in the path can be a file

				Node node = (current as DirectoryNode).Find(name, ignorecase);

				if (node == null && create)
					current = new DirectoryNode(name, current as DirectoryNode);
				else
					current = node;
			}

			return current;
		}

		public static DirectoryNode FromPath(string path, DelayedStreamCache cache, FileAccess access = FileAccess.ReadWrite)
		{
			DirectoryInfo dir = new DirectoryInfo(path);
			DirectoryNode node = new DirectoryNode(dir.Name, null);
			FromPath(node, path, cache, access);
			return node;
		}

		protected static void FromPath(DirectoryNode parent, string path, DelayedStreamCache cache, FileAccess access)
		{
			string[] paths = Directory.GetFiles(path, "*", SearchOption.TopDirectoryOnly);
			foreach (string fpath in paths) {
				FileInfo finfo = new FileInfo(fpath);
				FileNode file = new FileNode(finfo.Name, parent, (ulong)finfo.Length, new DelayedStream(cache.GenerateFileStream(finfo.FullName, FileMode.Open, access)));
			}
			paths = Directory.GetDirectories(path, "*", SearchOption.TopDirectoryOnly);
			foreach (string dpath in paths) {
				DirectoryInfo dinfo = new DirectoryInfo(dpath);
				DirectoryNode dir = new DirectoryNode(dinfo.Name, parent);
				FromPath(dir, dpath, cache, access);
			}
		}
	}

	public class FileNode : Node
	{
		public ulong Size;
		public Stream Data;

		public FileNode() : base() { }

		public FileNode(string name, DirectoryNode parent, ulong size, Stream data) : base(name, parent)
		{
			Size = size;
			Data = data;
		}
	}
}
