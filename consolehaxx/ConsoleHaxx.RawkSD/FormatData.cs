using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using ConsoleHaxx.Common;
using ConsoleHaxx.Harmonix;

namespace ConsoleHaxx.RawkSD
{
	public class FormatData
	{
		private SongData songdata;
		public virtual SongData Song {
			get {
				if (songdata != null)
					return songdata;
				Stream stream = GetStream("songdata");
				songdata = SongData.Create(stream);
				CloseStream(stream);
				songdata.PropertyChanged += new Action<SongData>(Song_PropertyChanged);
				return songdata;
			} set {
				Stream stream = AddStream("songdata");
				value.Save(stream);
				CloseStream(stream);
				songdata = value;
				songdata.PropertyChanged += new Action<SongData>(Song_PropertyChanged);
			}
		}

		public virtual IList<IFormat> Formats
		{
			get
			{
				IList<int> formats = Song.Data.GetArray<int>("FormatIDs");
				if (formats == null)
					return new List<IFormat>();
				return formats.Select(f => Platform.GetFormat(f)).ToArray();
			}
		}

		public IList<IFormat> GetFormats(FormatType type)
		{
			return Formats.Where(f => f.Type == type).ToList();
		}

		public void AddFormat(IFormat format)
		{
			List<IFormat> formats = new List<IFormat>(Formats);

			if (formats.Contains(format))
				return;

			formats.Add(format);

			Song.Data.SetArray<int>("FormatIDs", formats.Select(f => f.ID).ToArray());
			Song_PropertyChanged(Song);
		}

		public void RemoveFormat(IFormat format)
		{
			List<IFormat> formats = new List<IFormat>(Formats);

			if (!formats.Contains(format))
				return;

			formats.Remove(format);

			string[] streams = GetStreams();
			foreach (string name in streams) {
				string[] parts = name.Split('.');
				int id;
				if (int.TryParse(parts[0], out id)) {
					if (id == format.ID)
						DeleteStream(name);
				}
			}

			Song.Data.SetArray<int>("FormatIDs", formats.Select(f => f.ID).ToArray());
			Song_PropertyChanged(Song);
		}

		private List<string> Data;
		private Dictionary<string, Stream> Streams;

		public FormatData(SongData song) : this()
		{
			Song = song;
		}

		void Song_PropertyChanged(SongData song)
		{
			Song = song;
		}

		public FormatData()
		{
			Data = new List<string>();
			Streams = new Dictionary<string, Stream>();
			StreamOwnership = true;
		}

		~FormatData()
		{
			var streams = Streams.ToArray();
			foreach (var s in streams)
				CloseStream(s.Value);
			Streams.Clear();
			Data.Clear();
		}

		public bool StreamOwnership { get; set; }

		public virtual void DeleteStream(string name)
		{
			CloseStream(name);

			Data.Remove(name);
		}

		public virtual Stream AddStream(string name)
		{
			if (HasStream(name))
				DeleteStream(name);
			Stream ret = AddStreamBase(name);
			Streams[name] = ret;
			if (ret != null)
				Data.Add(name);
			return ret;
		}

		public virtual bool HasStream(string name)
		{
			return Data.Contains(name);
		}

		public virtual Stream GetStream(string name)
		{
			if (!Streams.ContainsKey(name))
				Streams[name] = OpenStream(name);

			Stream stream = Streams[name];
			stream.Position = 0;

			return stream;
		}

		public virtual void CloseStream(string name)
		{
			if (StreamOwnership && Streams.ContainsKey(name)) {
				Streams[name].Close();
				Streams.Remove(name);
			}
		}

		public virtual void CloseStream(Stream stream)
		{
			foreach (var s in Streams) {
				if (s.Value == stream) {
					CloseStream(s.Key);
					return;
				}
			}
		}

		protected virtual void AddStreamEntry(string name)
		{
			Data.Add(name);
		}

		public virtual void SetStream(string name, Stream stream)
		{
			if (stream == null)
				return;

			if (HasStream(name))
				CloseStream(name);
			else
				Data.Add(name);

			Streams[name] = stream;
		}

		public string[] GetStreams()
		{
			return Data.ToArray();
		}

		public virtual void Save(FormatData data)
		{
			foreach (string name in Data) {
				Util.StreamCopy(data.AddStream(name), GetStream(name));
				data.CloseStream(name);
				CloseStream(name);
			}
		}

		protected virtual Stream OpenStream(string name) { throw new NotImplementedException(); }
		protected virtual Stream AddStreamBase(string name) { throw new NotImplementedException(); }

		public void DeleteStream(IFormat format, string name) { DeleteStream(MixName(format, name)); }
		public Stream AddStream(IFormat format, string name) { Stream ret = AddStream(MixName(format, name)); AddFormat(format); return ret; }
		public bool HasStream(IFormat format, string name) { return HasStream(MixName(format, name)); }
		public Stream GetStream(IFormat format, string name) { return GetStream(MixName(format, name)); }
		public void CloseStream(IFormat format, string name) { CloseStream(MixName(format, name)); }
		public void SetStream(IFormat format, string name, Stream stream) { SetStream(MixName(format, name), stream); AddFormat(format); }
		public string MixName(IFormat format, string name) { return format.ID.ToString() + (name == null ? string.Empty : ("." + name)); }
	}

	public class TemporaryFormatData : FormatData
	{
		public TemporaryFormatData() : base() { }
		public TemporaryFormatData(SongData song) : base(song) {  }

		protected List<Stream> MyStreams = new List<Stream>();

		protected override Stream AddStreamBase(string name)
		{
			Stream stream = new TemporaryStream();
			MyStreams.Add(stream);
			return stream;
		}

		protected override Stream OpenStream(string name)
		{
			throw new NotSupportedException();
		}

		public override void CloseStream(Stream stream)
		{
			return;
		}

		~TemporaryFormatData()
		{
			foreach (Stream stream in MyStreams)
				stream.Close();
		}
	}

	public class MemoryFormatData : FormatData
	{
		public MemoryFormatData() : base() { }
		public MemoryFormatData(SongData song) : base(song) { }

		protected List<Stream> MyStreams = new List<Stream>();

		protected override Stream AddStreamBase(string name)
		{
			Stream stream = new MemoryStream();
			MyStreams.Add(stream);
			return stream;
		}

		protected override Stream OpenStream(string name)
		{
			throw new NotSupportedException();
		}

		public override void CloseStream(Stream stream)
		{
			return;
		}

		~MemoryFormatData()
		{
			foreach (Stream stream in MyStreams)
				stream.Close();
		}
	}

	public class FolderFormatData : FormatData
	{
		protected string Folder;

		public FolderFormatData(string path) : base()
		{
			Folder = path;

			Refresh();
		}

		public FolderFormatData(SongData song, string path) : base()
		{
			Folder = path;

			Refresh();

			Song = song;
		}

		private void Refresh()
		{
			string[] files = Directory.GetFiles(Folder, "*", SearchOption.TopDirectoryOnly);
			foreach (string file in files)
				AddStreamEntry(Path.GetFileName(file));
		}

		private string GetPath(string name)
		{
			return Path.Combine(Folder, name);
		}

		protected override Stream AddStreamBase(string name)
		{
			return new FileStream(GetPath(name), FileMode.Create, FileAccess.ReadWrite);
		}

		public override void DeleteStream(string name)
		{
			base.DeleteStream(name);

			File.Delete(GetPath(name));
		}

		protected override Stream OpenStream(string name)
		{
			return new FileStream(GetPath(name), FileMode.Open, FileAccess.ReadWrite);
		}
	}

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
				StructDescriptor desc = RegisteredStructs[type];
				Struct st = (Struct)type.GetConstructor(new Type[] { typeof(StructDescriptor) }).Invoke(new object[] { desc });
				for (int i = 0; i < desc.Types.Length; i++)
					st.Values[i] = InternalGetValue(desc.Types[i], index * desc.Types.Length + i);
				return st;
			}

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

			return new DataArray(tree);
		}

		public void SetSubtree(string name, DTB.NodeTree dtb)
		{
			DataArray tree = GetSubtree(name, true);
			tree.Options.Nodes.AddOrReplace(1, dtb);
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
		}
	}
}
