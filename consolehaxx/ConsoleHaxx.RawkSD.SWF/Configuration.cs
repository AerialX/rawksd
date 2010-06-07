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
		private static MainForm Form;

		public static string InstallTitle { get { return PlatformRB2WiiCustomDLC.UnusedTitle; } set { PlatformRB2WiiCustomDLC.UnusedTitle = value; } }
		public static string LocalPath { get; set; }
		public static string TemporaryPath { get; set; }
		public static int MaxConcurrentTasks { get; set; }
		public static bool LocalTranscode { get; set; }
		public static bool MemorySongData { get { return FormatData.LocalSongCache; } set { FormatData.LocalSongCache = value; } }
		public static DefaultActionType DefaultAction { get; set; }

		public static bool ExpertPlusGH5 { get { return PlatformGH5WiiDisc.ImportExpertPlus; } set { PlatformGH5WiiDisc.ImportExpertPlus = value; } }

		public static ImportMap.NamePrefix NamePrefix { get { return ImportMap.ApplyNamePrefix; } set { ImportMap.ApplyNamePrefix = value; } }

		static Configuration()
		{
			MaxConcurrentTasks = Environment.ProcessorCount;
			LocalTranscode = false;
			MemorySongData = false;
			NamePrefix = ImportMap.NamePrefix.None;
			DefaultAction = DefaultActionType.InstallSD;
			LocalPath = "customs";
			TemporaryPath = "temp";
			InstallTitle = "cRBA";
			ExpertPlusGH5 = false;
		}

		public static void Load(MainForm form)
		{
			Form = form;

			Stream stream = GetConfigurationStream(FileMode.Open, FileAccess.Read, FileShare.Read);
			if (stream == null)
				return;
			DTB.NodeTree tree = DTB.Create(new EndianReader(stream, Endianness.LittleEndian));
			stream.Close();
			DataArray data = new DataArray(tree);
			MaxConcurrentTasks = data.GetValue<int>("MaxConcurrentTasks");
			LocalTranscode = data.GetValue<bool>("LocalTranscode");
			MemorySongData = data.GetValue<bool>("MemorySongData");
			DefaultAction = (DefaultActionType)data.GetValue<int>("DefaultAction");
			NamePrefix = (ImportMap.NamePrefix)data.GetValue<int>("NamePrefix");
			LocalPath = data.GetValue<string>("LocalPath");
			TemporaryPath = data.GetValue<string>("TemporaryPath");
			InstallTitle = data.GetValue<string>("InstallTitle");
			ExpertPlusGH5 = data.GetValue<bool>("ExpertPlusGH5");
		}

		public static void Save()
		{
			Stream stream = GetConfigurationStream(FileMode.Create, FileAccess.Write, FileShare.None);
			if (stream == null)
				return;
			DataArray data = new DataArray();
			data.SetValue("MaxConcurrentTasks", MaxConcurrentTasks);
			data.SetValue("LocalTranscode", LocalTranscode);
			data.SetValue("MemorySongData", MemorySongData);
			data.SetValue("DefaultAction", (int)DefaultAction);
			data.SetValue("NamePrefix", (int)NamePrefix);
			data.SetValue("LocalPath", LocalPath);
			data.SetValue("TemporaryPath", TemporaryPath);
			data.SetValue("InstallTitle", InstallTitle);
			data.SetValue("ExpertPlusGH5", ExpertPlusGH5);
			data.Save(stream);
			stream.Close();
		}

		private static Stream GetConfigurationStream(FileMode fileMode, FileAccess fileAccess, FileShare fileShare)
		{
			try {
				return new FileStream(Path.Combine(Form.RootPath, "rawkswf.conf"), fileMode, fileAccess, fileShare);
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
