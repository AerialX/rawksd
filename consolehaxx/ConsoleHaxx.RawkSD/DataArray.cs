using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using ConsoleHaxx.Harmonix;
using System.IO;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.RawkSD
{
	public class DataArray
	{
		public static Dictionary<Type, StructDescriptor> RegisteredStructs;

		static DataArray()
		{
			RegisteredStructs = new Dictionary<Type, StructDescriptor>();
		}

		public static void RegisterStruct(Type type, StructDescriptor desc)
		{
			RegisteredStructs.Add(type, desc);
		}

		public class Struct
		{
			public Struct(StructDescriptor desc)
			{
				Descriptor = desc;
				Values = new object[desc.Types.Length];
			}

			public object[] Values;
			public StructDescriptor Descriptor;
		}

		public class StructDescriptor
		{
			public Type[] Types;

			public StructDescriptor(Type[] types)
			{
				Types = types;
			}
		}

		public event Action<DataArray> PropertyChanged;
		public DTB.NodeTree Options;

		public DataArray(DTB.NodeTree tree)
		{
			Options = tree;
		}

		public DataArray()
		{
			Options = new DTB.NodeTree();
		}

		public static DataArray Create(Stream stream)
		{
			try {
				return new DataArray(DTB.Create(new EndianReader(stream, Endianness.LittleEndian)));
			} catch (FormatException) {
				return null;
			}
		}

		public void Save(Stream stream)
		{
			Options.Save(new EndianReader(stream, Endianness.LittleEndian));
		}

		public void RaisePropertyChangedEvent()
		{
			if (PropertyChanged != null)
				PropertyChanged(this);
		}


		public T GetValue<T>(string name)
		{
			return GetValue<T>(name, 0);
		}

		public T GetValue<T>(string name, int index)
		{
			DataArray tree = GetSubtree(name, false);
			return tree == null ? default(T) : tree.GetValue<T>(index);
		}

		public T GetValue<T>(int index)
		{
			return (T)InternalGetValue(typeof(T), index);
		}

		protected object InternalGetValue(Type type, int index)
		{
			try {
				if (type == typeof(bool))
					return Options.GetValue<DTB.NodeInt32>(index + 1).Number != 0;
				else if (type == typeof(int))
					return Options.GetValue<DTB.NodeInt32>(index + 1).Number;
				else if (type == typeof(uint))
					return (uint)Options.GetValue<DTB.NodeInt32>(index + 1).Number;
				else if (type == typeof(float))
					return Options.GetValue<DTB.NodeFloat32>(index + 1).Number;
				else if (type == typeof(string))
					return Options.GetValue<DTB.NodeString>(index + 1).Text;
				else if (type == typeof(byte[]))
					return Options.GetValue<DTB.NodeData>(index + 1).Contents;
				else if (type.IsSubclassOf(typeof(Struct))) {
					Struct st = (Struct)type.GetConstructor(Type.EmptyTypes).Invoke(new object[0]);
					StructDescriptor desc = RegisteredStructs[type];
					for (int i = 0; i < desc.Types.Length; i++)
						st.Values[i] = InternalGetValue(desc.Types[i], index * desc.Types.Length + i);
					return st;
				}
			} catch { }

			return null;
		}

		public DataArray GetSubtree(string name)
		{
			return GetSubtree(name, false);
		}

		public DataArray GetSubtree(string name, bool create)
		{
			DTB.NodeTree tree = Options.FindByKeyword(name);
			if (tree == null) {
				if (create) {
					tree = new DTB.NodeTree() { Type = 0x10 };
					tree.Nodes.Add(new DTB.NodeKeyword() { Type = 0x05, Text = name });
					Options.Nodes.Add(tree);
				} else
					return null;
			}

			DataArray subtree = new DataArray(tree);
			subtree.PropertyChanged = this.PropertyChanged;

			return subtree;
		}

		public void SetSubtree(string name, DTB.NodeTree dtb)
		{
			DataArray tree = GetSubtree(name, true);
			tree.Options.Nodes.AddOrReplace(1, dtb);

			RaisePropertyChangedEvent();
		}

		public DTB.NodeTree GetSubtree()
		{
			return Options.Nodes[1] as DTB.NodeTree;
		}

		public void SetValue(string name, object value)
		{
			SetValue(name, value, 0);
		}

		public void SetValue(string name, object value, int index)
		{
			DataArray tree = GetSubtree(name, true);

			tree.InternalSetValue(value, index);
		}

		protected void InternalSetValue(object value, int index)
		{
			if (value == null) {
				Options.Nodes.AddOrReplace(index + 1, new DTB.NodeInt32() { Number = 0 });
				return;
			}

			Type type = value.GetType();
			if (value is bool)
				Options.Nodes.AddOrReplace(index + 1, new DTB.NodeInt32() { Number = (bool)value ? 1 : 0 });
			else if (value is int)
				Options.Nodes.AddOrReplace(index + 1, new DTB.NodeInt32() { Number = (int)value });
			else if (value is uint)
				Options.Nodes.AddOrReplace(index + 1, new DTB.NodeInt32() { Number = (int)(uint)value });
			else if (value is float)
				Options.Nodes.AddOrReplace(index + 1, new DTB.NodeFloat32() { Type = 0x01, Number = (float)value });
			else if (value is string)
				Options.Nodes.AddOrReplace(index + 1, new DTB.NodeString() { Type = 0x12, Text = (string)value });
			else if (value is byte[])
				Options.Nodes.AddOrReplace(index + 1, new DTB.NodeData() { Type = 0xF8, Contents = (byte[])value });
			else if (value is Struct) {
				Struct st = (Struct)value;
				for (int i = 0; i < st.Descriptor.Types.Length; i++)
					InternalSetValue(st.Values[i], index * st.Descriptor.Types.Length + i);
			} else
				throw new NotSupportedException();

			RaisePropertyChangedEvent();
		}

		public IList<T> GetArray<T>(string name)
		{
			DataArray array = GetSubtree(name);
			if (array == null)
				return null;

			int count = array.GetValue<int>("count");

			DataArray items = array.GetSubtree("data");
			if (items == null)
				return null;

			T[] ret = new T[count];
			for (int i = 0; i < count; i++)
				ret[i] = items.GetValue<T>(i);

			return ret;
		}

		public void SetArray<T>(string name, IList<T> value)
		{
			DataArray subtree = GetSubtree(name, true);

			subtree.SetValue("count", value.Count);
			DataArray data = subtree.GetSubtree("data", true);

			for (int i = 0; i < value.Count; i++)
				data.InternalSetValue(value[i], i);

			RaisePropertyChangedEvent();
		}
	}
}
