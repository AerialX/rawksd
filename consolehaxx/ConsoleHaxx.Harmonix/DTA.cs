using System;
using System.Collections.Generic;
using System.IO;
using System.Globalization;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.Harmonix
{
	public static class DTA
	{
		public static DTB.NodeTree Create(Stream stream)
		{
			return Create(new StreamReader(stream, Util.Encoding));
		}

		public static DTB.NodeTree Create(StreamReader reader)
		{
			int linenumber = 1;

			DTB.NodeTree tree = new DTB.NodeTree();
			tree.Type = 0x01;
			tree.ID = 0;

			DTB.NodeTree parent = tree;

			string token = string.Empty;
			int quoted = -1;

			while (!reader.EndOfStream) {
				int readchar = reader.Read();
				if (readchar < 0)
					break;
				char c = (char)readchar;

				if (quoted == -1) {
					switch (c) {
						case '(':
							HandleToken(ref token, tree, quoted);

							tree = AddNode(tree, new DTB.NodeTree() { Type = 0x10, ID = (uint)linenumber }) as DTB.NodeTree;
							break;
						case '{':
							HandleToken(ref token, tree, quoted);

							tree = AddNode(tree, new DTB.NodeTree() { Type = 0x11, ID = (uint)linenumber }) as DTB.NodeTree;
							break;
						case ')':
						case '}':
							HandleToken(ref token, tree, quoted);

							tree = tree.Parent;
							break;
						case '"':
						case '\'':
							HandleToken(ref token, tree, quoted);
							quoted = (c == '\'') ? 0 : 1;
							break;
						case '[':
						case ']':
							throw new Exception("wut?");
						case '#':
						case '%':
						case '&':
						case '\\':
						case '|':
							throw new Exception("wut?");
						case ';':
							reader.ReadLine();
							linenumber++;
							break;
						default:
							if (char.IsWhiteSpace(c) && quoted == -1)
								HandleToken(ref token, tree, quoted);
							else
								token += c;
							if (c == '\n')
								linenumber++;
							break;
					}
				} else {
					switch (c) {
						case '\'':
							if (quoted == 0) {
								HandleToken(ref token, tree, quoted);
								quoted = -1;
								continue;
							}
							break;
						case '"':
							if (quoted == 1) {
								HandleToken(ref token, tree, quoted);
								quoted = -1;
								continue;
							}
							break;
					}

					token += c;
				}
			}

			tree = parent.Nodes[0] as DTB.NodeTree;
			tree.Parent = null;

			return tree;
		}

		private static void HandleToken(ref string token, DTB.NodeTree tree, int tokentype)
		{
			if (token.Length == 0)
				return;

			if (tokentype == 0) {
				AddNode(tree, new DTB.NodeKeyword() { Text = token, Type = 0x05 });
				token = string.Empty;
				return;
			} else if (tokentype == 1) {
				AddNode(tree, new DTB.NodeString() { Text = token, Type = 0x12 });
				token = string.Empty;
				return;
			} else {
				if (token[0] == '$') {
					AddNode(tree, new DTB.NodeFunction() { Text = token.Substring(1), Type = 0x02 });
					token = string.Empty;
					return;
				} else {
					switch (token.ToLower()) {
						case "kDataUnhandled":
							AddNode(tree, new DTB.NodeInt32() { Number = 0, Type = 0x06 });
							token = string.Empty;
							return;
						// More kslowspeed stuff
					}
				}

				int num = 0;
				float num2 = 0;
				if (token.ToLower().StartsWith("0x") && int.TryParse(token.Substring(2), NumberStyles.HexNumber, CultureInfo.InvariantCulture, out num)) {
					AddNode(tree, new DTB.NodeInt32() { Number = num, Type = 0x00 });
				} else if (int.TryParse(token, out num)) {
					AddNode(tree, new DTB.NodeInt32() { Number = num, Type = 0x00 });
				} else if (float.TryParse(token, NumberStyles.Float, CultureInfo.InvariantCulture, out num2)) {
					AddNode(tree, new DTB.NodeFloat32() { Number = num2, Type = 0x01 });
				} else
					AddNode(tree, new DTB.NodeKeyword() { Text = token, Type = 0x05 });
			}

			token = string.Empty;
		}

		private static DTB.Node AddNode(DTB.NodeTree tree, DTB.Node node)
		{
			tree.Nodes.Add(node);
			node.Parent = tree;

			return node;
		}

		public static void SaveDTA(this DTB.NodeTree dtb, Stream stream)
		{
			dtb.SaveDTA(new StreamWriter(stream, Util.Encoding));
		}

		public static void SaveDTA(this DTB.NodeTree dtb, StreamWriter writer)
		{
			writer.Write("(\r\n");
			foreach (DTB.Node node in dtb.Nodes) {
				if (node is DTB.NodeTree) {
					SaveDTA(node as DTB.NodeTree, writer);
				}
				if (node is DTB.NodeFloat32) {
					writer.Write((node as DTB.NodeFloat32).Number.ToString("0.00", CultureInfo.InvariantCulture));
					writer.Write(" ");
				}
				if (node is DTB.NodeKeyword) {
					writer.Write("'"); // RBN likes to do this?
					writer.Write((node as DTB.NodeKeyword).Text);
					writer.Write("' ");
				} else if (node is DTB.NodeString) {
					writer.Write("\"");
					writer.Write((node as DTB.NodeString).Text);
					writer.Write("\" ");
				} else if (node is DTB.NodeInt32) {
					writer.Write((node as DTB.NodeInt32).Number.ToString(CultureInfo.InvariantCulture));
					writer.Write(" ");
				}
			}
			writer.Write(")\r\n");
			writer.Flush();
		}
	}
}
