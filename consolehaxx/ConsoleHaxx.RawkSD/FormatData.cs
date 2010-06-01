using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using ConsoleHaxx.Common;
using ConsoleHaxx.Harmonix;

namespace ConsoleHaxx.RawkSD
{
	#region FormatData
	public class FormatData : IDisposable
	{
		public static bool LocalSongCache = true;

		public PlatformData PlatformData { get; protected set; }
		public virtual bool StreamOwnership { get; set; }

		protected List<string> Data;
		protected Dictionary<string, Stream> Streams;

		public virtual SongData Song {
			get {
				return SongDataCache.Load(this);
			} set {
				SongDataCache.Save(this, value);
			}
		}

		internal void Song_PropertyChanged(SongData song)
		{
			Song = song;
		}

		public virtual IList<IFormat> Formats
		{
			get
			{
				List<int> formats = new List<int>();
				string[] streams = GetStreamNames();
				foreach (string name in streams) {
					string[] parts = name.Split('.');
					int id;
					if (int.TryParse(parts[0], out id) && !formats.Contains(id))
						formats.Add(id);
				}
				/*
				IList<int> formats = Song.Data.GetArray<int>("FormatIDs");
				if (formats == null)
					return new List<IFormat>();
				*/
				return formats.Select(f => Platform.GetFormat(f)).ToArray();
			}
		}

		public IFormat GetFormat(FormatType type)
		{
			return Formats.FirstOrDefault(f => f.Type == type && f.Readable);
		}

		public IFormat GetFormatAny(FormatType type)
		{
			return Formats.FirstOrDefault(f => f.Type == type);
		}

		public IList<IFormat> GetFormats(FormatType type)
		{
			return Formats.Where(f => f.Type == type).ToList();
		}

		public void AddFormat(IFormat format)
		{
			/*
			List<IFormat> formats = new List<IFormat>(Formats);

			if (formats.Contains(format))
				return;

			formats.Add(format);

			Song.Data.SetArray<int>("FormatIDs", formats.Select(f => f.ID).ToArray());
			*/
		}

		public void RemoveFormat(IFormat format)
		{
			List<IFormat> formats = new List<IFormat>(Formats);

			if (!formats.Contains(format))
				return;

			formats.Remove(format);

			string[] streams = GetStreamNames();
			foreach (string name in streams) {
				string[] parts = name.Split('.');
				int id;
				if (int.TryParse(parts[0], out id)) {
					if (id == format.ID)
						DeleteStream(name);
				}
			}

			// Song.Data.SetArray<int>("FormatIDs", formats.Select(f => f.ID).ToArray());
		}

		public FormatData(SongData song) : this(song, null) { }
		public FormatData(SongData song, PlatformData data) : this()
		{
			PlatformData = data;
			if (song != null) {
				Song = song;
				song.PropertyChanged += new Action<SongData>(Song_PropertyChanged);
			}
		}

		public FormatData()
		{
			Data = new List<string>();
			Streams = new Dictionary<string, Stream>();
			StreamOwnership = true;
		}

		~FormatData()
		{
			Dispose();
		}

		protected bool Disposed;
		public virtual void Dispose()
		{
			if (Disposed)
				return;

			var streams = Streams.ToArray();
			foreach (var s in streams)
				CloseStream(s.Value);
			Streams.Clear();
			Data.Clear();

			Disposed = true;
		}

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
			if (ret != null) {
				Streams[name] = ret;

				Data.Add(name);
			}
			return ret;
		}

		public virtual bool HasStream(string name)
		{
			return Data.Contains(name);
		}

		public virtual Stream GetStream(string name)
		{
			if (!Streams.ContainsKey(name)) {
				if (!HasStream(name))
					return null;

				Streams[name] = OpenStream(name);
			}

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

		public string[] GetStreamNames()
		{
			return Data.ToArray();
		}

		public IList<string> GetStreamNames(IFormat format)
		{
			List<string> streams = new List<string>();
			foreach (string name in Data) {
				string[] parts = name.Split('.');
				int id;
				if (int.TryParse(parts[0], out id)) {
					if (id == format.ID)
						streams.Add(name);
				}
			}
			return streams.ToArray();
		}

		public IList<Stream> GetStreams(IFormat format)
		{
			return GetStreamNames(format).Select(n => GetStream(n)).ToArray();
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

		public virtual void SaveTo(FormatData data, FormatType type = FormatType.Unknown)
		{
			foreach (string name in Data) {
				if (type != FormatType.Unknown) {
					try {
						int format = int.Parse(name.Split('.')[0]);
						if (Platform.GetFormat(format).Type != type)
							continue;
					} catch { }
				}
				Util.StreamCopy(data.AddStream(name), GetStream(name));
				data.CloseStream(name);
				CloseStream(name);
			}
		}
	}
	#endregion

	#region Implementations
	public abstract class FormatDataPersistance : FormatData
	{
		protected Dictionary<string, Stream> MyStreams = new Dictionary<string, Stream>();

		public override bool StreamOwnership { get { return false; } }

		public FormatDataPersistance() : base() { }
		public FormatDataPersistance(SongData song) : base(song) { }
		public FormatDataPersistance(SongData song, PlatformData data) : base(song, data) { }

		protected abstract Stream AddStreamBase();
		protected virtual void RemoveStreamBase(Stream stream) { stream.Close(); }

		protected override Stream AddStreamBase(string name)
		{
			Stream stream = AddStreamBase();
			MyStreams[name] = stream;
			return stream;
		}

		protected override Stream OpenStream(string name)
		{
			if (MyStreams.ContainsKey(name))
				return MyStreams[name];

			return null;
		}

		public override void DeleteStream(string name)
		{
			if (MyStreams.ContainsKey(name)) {
				RemoveStreamBase(MyStreams[name]);
				MyStreams.Remove(name);
			}

			base.DeleteStream(name);
		}

		~FormatDataPersistance()
		{
			Dispose();
		}

		public override void Dispose()
		{
			foreach (var stream in MyStreams)
				RemoveStreamBase(stream.Value);
			base.Dispose();
		}
	}

	public class TemporaryFormatData : FormatDataPersistance
	{
		public TemporaryFormatData() : base() { }
		public TemporaryFormatData(SongData song) : base(song) {  }
		public TemporaryFormatData(SongData song, PlatformData data) : base(song, data) { }

		protected override Stream AddStreamBase()
		{
			return new TemporaryStream();
		}
	}

	public class MemoryFormatData : FormatDataPersistance
	{
		public MemoryFormatData() : base() { }
		public MemoryFormatData(SongData song) : base(song) { }
		public MemoryFormatData(SongData song, PlatformData data) : base(song, data) { }

		protected override Stream AddStreamBase()
		{
			return new MemoryStream();
		}
	}

	public class FolderFormatData : FormatData
	{
		protected string Folder;

		public FolderFormatData(string path) : this(null, null, path) { }

		public FolderFormatData(SongData song, string path) : this(song, null, path) { }
		public FolderFormatData(PlatformData data, string path) : this(null, data, path) { }

		public FolderFormatData(SongData song, PlatformData data, string path) : base()
		{
			Folder = path;

			if (!Directory.Exists(Folder))
				Directory.CreateDirectory(Folder);

			Refresh();

			if (data != null)
				PlatformData = data;
			if (song != null)
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
	#endregion
}
