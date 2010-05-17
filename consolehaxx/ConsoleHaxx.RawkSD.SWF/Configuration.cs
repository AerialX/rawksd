using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using ConsoleHaxx.Harmonix;
using System.IO;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.RawkSD.SWF
{
	public static class Configuration
	{
		public static int MaxConcurrentTasks { get; set; }
		public static bool LocalTranscode { get; set; }
		public static bool LocalTransfer { get; set; }
		public static bool MemorySongData { get { return FormatData.LocalSongCache; } set { FormatData.LocalSongCache = value; } }
		public static DefaultActionType DefaultAction { get; set; }

		public static ImportMap.NamePrefix NamePrefix { get { return ImportMap.ApplyNamePrefix; } set { ImportMap.ApplyNamePrefix = value; } }

		static Configuration()
		{
			MaxConcurrentTasks = Environment.ProcessorCount;
			LocalTranscode = false;
			LocalTransfer = true;
			MemorySongData = false;
			NamePrefix = ImportMap.NamePrefix.None;
			DefaultAction = DefaultActionType.InstallSD;

			Load();
		}

		public static void Load()
		{
			Stream stream = GetConfigurationStream(FileMode.Open, FileAccess.Read, FileShare.Read);
			if (stream == null)
				return;
			DTB.NodeTree tree = DTB.Create(new EndianReader(stream, Endianness.LittleEndian));
			stream.Close();
			DataArray data = new DataArray(tree);
			MaxConcurrentTasks = data.GetValue<int>("MaxConcurrentTasks");
			LocalTranscode = data.GetValue<bool>("LocalTranscode");
			LocalTransfer = data.GetValue<bool>("LocalTransfer");
			MemorySongData = data.GetValue<bool>("MemorySongData");
			DefaultAction = (DefaultActionType)data.GetValue<int>("DefaultAction");
			NamePrefix = (ImportMap.NamePrefix)data.GetValue<int>("NamePrefix");
		}

		public static void Save()
		{
			Stream stream = GetConfigurationStream(FileMode.Create, FileAccess.Write, FileShare.None);
			if (stream == null)
				return;
			DataArray data = new DataArray();
			data.SetValue("MaxConcurrentTasks", MaxConcurrentTasks);
			data.SetValue("LocalTranscode", LocalTranscode);
			data.SetValue("LocalTransfer", LocalTransfer);
			data.SetValue("MemorySongData", MemorySongData);
			data.SetValue("DefaultAction", (int)DefaultAction);
			data.SetValue("NamePrefix", (int)NamePrefix);
			data.Save(stream);
			stream.Close();
		}

		private static Stream GetConfigurationStream(FileMode fileMode, FileAccess fileAccess, FileShare fileShare)
		{
			try {
				return new FileStream(Path.Combine(Program.Form.RootPath, "rawkswf.conf"), fileMode, fileAccess, fileShare);
			} catch { }

			return null;
		}

		public enum DefaultActionType
		{
			Unknown =		0x00,
			InstallLocal =	0x01,
			InstallSD =		0x02,
			ExportRawk =	0x10,
			ExportRBA =		0x11,
			Export360 =		0x12,
			ExportFoF =		0x13
		}
	}
}
