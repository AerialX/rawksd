using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Collections;

namespace ConsoleHaxx.Common
{
	public class Node
	{
		public string Name { get; set; }
		public DirectoryNode Parent { get; internal set; }

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

		public Node(string name)
		{
			Name = name;
		}

		public override string ToString()
		{
			return Name;
		}
	}

	public class DirectoryNode : Node, IEnumerable<Node>
	{
		protected List<Node> Children;
		public int ChildCount { get { return Children.Count; } }
		public IEnumerable<FileNode> Files { get { return Children.OfType<FileNode>(); } }
		public IEnumerable<DirectoryNode> Directories { get { return Children.OfType<DirectoryNode>(); } }
		public Node this[int index] { get { return Children[index]; } }
		public Node this[string name] { get { return Find(name); } }

		public DirectoryNode() : base()
		{
			Children = new List<Node>();
		}

		public DirectoryNode(string name) : base(name)
		{
			Children = new List<Node>();
		}

		public void AddChild(Node node)
		{
			Children.Add(node);
			node.Parent = this;
		}

		public void AddChildren(IEnumerable<Node> nodes)
		{
			Children.AddRange(nodes);
		}

		public void AddChildren(IEnumerable<FileNode> nodes)
		{
			AddChildren(nodes.Cast<Node>());
		}

		public void AddChildren(IEnumerable<DirectoryNode> nodes)
		{
			AddChildren(nodes.Cast<Node>());
		}

		public IEnumerator<Node> GetEnumerator()
		{
			return Children.GetEnumerator();
		}

		IEnumerator IEnumerable.GetEnumerator()
		{
			return Children.GetEnumerator();
		}

		public Node Find(string name, bool ignorecase = true)
		{
			return Find(name, SearchOption.TopDirectoryOnly, ignorecase);
		}

		public Node Find(string name, SearchOption search)
		{
			return Find(name, search, true);
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

		public Node Navigate(string path, bool create = false, bool ignorecase = true)
		{
			string[] nodes = path.Split(new char[] { '/', '\\' }, StringSplitOptions.RemoveEmptyEntries);
			Node current = this;
			foreach (string name in nodes) {
				if (!(current is DirectoryNode))
					return null; // Only the last node in the path can be a file

				if (name == ".")
					continue;
				else if (name == "..") {
					current = current.Parent;
					continue;
				}

				Node node = (current as DirectoryNode).Find(name, ignorecase);

				if (node == null && create) {
					DirectoryNode child = new DirectoryNode(name);
					(current as DirectoryNode).AddChild(child);
					current = child;
				} else
					current = node;
			}

			return current;
		}

		public static DirectoryNode FromPath(string path, DelayedStreamCache cache, FileAccess access = FileAccess.Read, FileShare share = FileShare.Read)
		{
			DirectoryInfo dir = new DirectoryInfo(path);
			DirectoryNode node = new DirectoryNode(dir.Name);
			FromPath(node, path, cache, access, share);
			return node;
		}

		protected static void FromPath(DirectoryNode parent, string path, DelayedStreamCache cache, FileAccess access, FileShare share)
		{
			string[] paths = Directory.GetFiles(path);
			foreach (string fpath in paths) {
				FileInfo finfo = new FileInfo(fpath);
				FileNode file = new FileNode(finfo.Name, (ulong)finfo.Length, new DelayedStream(cache.GenerateFileStream(finfo.FullName, FileMode.Open, access, share)));
				parent.AddChild(file);
			}
			paths = Directory.GetDirectories(path);
			foreach (string dpath in paths) {
				DirectoryInfo dinfo = new DirectoryInfo(dpath);
				DirectoryNode dir = new DirectoryNode(dinfo.Name);
				parent.AddChild(dir);
				FromPath(dir, dpath, cache, access, share);
			}
		}
	}

	public class FileNode : Node
	{
		public ulong Size { get; set; }
		public Stream Data { get; set; }

		public FileNode() : base() { }

		public FileNode(string name, ulong size, Stream data) : base(name)
		{
			Size = size;
			Data = data;
		}

		public FileNode(string name, Stream data) : base(name)
		{
			Data = data;
			Size = (ulong)data.Length;
		}
	}
}
