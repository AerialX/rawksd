using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using ConsoleHaxx.Common;
using ConsoleHaxx.Wii;
using ConsoleHaxx.Harmonix;
using ConsoleHaxx.Xbox360;

using PlatformList = System.Collections.Generic.List<ConsoleHaxx.Common.Pair<ConsoleHaxx.RawkSD.Engine, ConsoleHaxx.RawkSD.Game>>;

namespace ConsoleHaxx.RawkSD
{
	public static class PlatformDetection
	{
		public static List<Engine> Selections;

		public static event Action<string, PlatformList> DetectDirectory;
		public static event Action<string, DirectoryNode, PlatformList> DetectDirectoryNode;

		public static event Action<string, Stream, PlatformList> DetectFile;
		public static event Action<string, Disc, PlatformList> DetectWiiDisc;
		public static event Action<string, Iso9660, PlatformList> DetectIso9660;
		public static event Action<string, StfsArchive, PlatformList> DetectXbox360Dlc;
		public static event Action<string, DlcBin, PlatformList> DetectWiiDlcBin;
		public static event Action<string, Ark, PlatformList> DetectHarmonixArk;
		public static event Action<string, U8, PlatformList> DetectWiiU8;

		static PlatformDetection()
		{
			Selections = new List<Engine>();

			DetectDirectory += new Action<string, PlatformList>(PlatformDetection_DetectDirectory);
			DetectFile += new Action<string, Stream, PlatformList>(PlatformDetection_DetectFile);
			DetectWiiDisc += new Action<string, Disc, PlatformList>(PlatformDetection_DetectWiiDisc);
			DetectIso9660 += new Action<string, Iso9660, PlatformList>(PlatformDetection_DetectIso9660);
			DetectXbox360Dlc += new Action<string, StfsArchive, PlatformList>(PlatformDetection_DetectXbox360Dlc);
			DetectWiiDlcBin += new Action<string, DlcBin, PlatformList>(PlatformDetection_DetectWiiDlcBin);
			DetectWiiU8 += new Action<string, U8, PlatformList>(PlatformDetection_DetectWiiU8);

			DetectDirectoryNode += new Action<string, DirectoryNode, PlatformList>(PlatformDetection_DetectDirectoryNode);
		}

		static void PlatformDetection_DetectDirectoryNode(string path, DirectoryNode dir, PlatformList platforms)
		{
			if (DetectHarmonixArk != null) {
				try {
					Ark ark = HarmonixMetadata.GetHarmonixArk(dir);
					DetectHarmonixArk(path, ark, platforms);
				} catch (FormatException) { } catch (Exception exception) {
					Exceptions.Warning(exception, "Trying to open as Ark: " + path);
				}
			}
		}

		static void PlatformDetection_DetectWiiU8(string path, U8 u8, PlatformList platforms)
		{
			if (DetectDirectoryNode != null)
				DetectDirectoryNode(path, u8.Root, platforms);
		}

		static void PlatformDetection_DetectWiiDlcBin(string path, DlcBin dlc, PlatformList platforms)
		{
			if (DetectDirectoryNode != null) {
				try {
					U8 u8 = new U8(dlc.Data);
					DetectDirectoryNode(path, u8.Root, platforms);
				} catch (FormatException) { } catch (Exception exception) {
					Exceptions.Warning(exception, "Trying to open as DLC U8: " + path);
				}
			}
		}

		static void PlatformDetection_DetectXbox360Dlc(string path, StfsArchive stfs, PlatformList platforms)
		{
			if (DetectDirectoryNode != null)
				DetectDirectoryNode(path, stfs.Root, platforms);
		}

		static void PlatformDetection_DetectIso9660(string path, Iso9660 disc, PlatformList platforms)
		{
			if (DetectDirectoryNode != null)
				DetectDirectoryNode(path, disc.Root, platforms);
		}

		static void PlatformDetection_DetectWiiDisc(string path, Disc disc, PlatformList platforms)
		{
			if (DetectDirectoryNode != null)
				DetectDirectoryNode(path, disc.DataPartition.Root.Root, platforms);
		}

		static void PlatformDetection_DetectFile(string path, Stream stream, PlatformList platforms)
		{
			if (DetectWiiDisc != null) {
				try {
					stream.Position = 0;
					Disc disc = new Disc(stream);
					DetectWiiDisc(path, disc, platforms);
				} catch (FormatException) { } catch (Exception exception) {
					Exceptions.Warning(exception, "Trying to open as Wiidisc: " + path);
				}
			}

			if (DetectIso9660 != null) {
				try {
					stream.Position = 0;
					Iso9660 disc = new Iso9660(stream);
					DetectIso9660(path, disc, platforms);
				} catch (FormatException) { } catch (Exception exception) {
					Exceptions.Warning(exception, "Trying to open as ISO: " + path);
				}
			}

			if (DetectXbox360Dlc != null) {
				try {
					stream.Position = 0;
					StfsArchive stfs = new StfsArchive(stream);
					DetectXbox360Dlc(path, stfs, platforms);
				} catch (FormatException) { } catch (Exception exception) {
					Exceptions.Warning(exception, "Trying to open as 360 DLC: " + path);
				}
			}

			if (DetectWiiU8 != null) {
				try {
					stream.Position = 0;
					U8 u8 = new U8(stream);
					DetectWiiU8(path, u8, platforms);
				} catch (FormatException) { } catch (Exception exception) {
					Exceptions.Warning(exception, "Trying to open as U8: " + path);
				}
			}

			if (DetectWiiDlcBin != null) {
				try {
					stream.Position = 0;
					DlcBin dlc = new DlcBin(stream);
					DetectWiiDlcBin(path, dlc, platforms);
				} catch (FormatException) { } catch (Exception exception) {
					Exceptions.Warning(exception, "Trying to open as Wii DLC: " + path);
				}
			}

			if (DetectHarmonixArk != null) {
				string extension = Path.GetExtension(path).ToLower();
				if (extension.EndsWith(".ark") || extension.EndsWith(".hdr"))
					PlatformDetection_DetectDirectory(Path.GetDirectoryName(path), platforms);
			}
		}

		static void PlatformDetection_DetectDirectory(string path, PlatformList platforms)
		{
			if (DetectDirectoryNode != null) {
				DelayedStreamCache cache = new DelayedStreamCache();
				DirectoryNode root = DirectoryNode.FromPath(path, cache, FileAccess.Read, FileShare.Read);
				DetectDirectoryNode(path, root, platforms);
				cache.Dispose();
			}
		}

		public static void AddSelection(Engine platform)
		{
			Selections.Add(platform);
		}

		public static PlatformList DetectPlaform(string path)
		{
			PlatformList platforms = new PlatformList();
			if (Directory.Exists(path)) {
				if (DetectDirectory != null)
					DetectDirectory(path, platforms);
			} else if (File.Exists(path)) {
				FileStream stream = new FileStream(path, FileMode.Open, FileAccess.Read);

				if (DetectFile != null)
					DetectFile(path, stream, platforms);

				stream.Close();
			}

			// Get rid of all duplicates and favour entries with a higher Game value (and thus filter out Game.Unknown if possible)
			return platforms.GroupBy(p => p.Key).Select(p => p.OrderByDescending(g => (int)g.Value).First()).ToList();
		}
	}
}
