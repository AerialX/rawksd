#region Using Shortcuts
using s8 = System.SByte;
using u8 = System.Byte;
using s16 = System.Int16;
using u16 = System.UInt16;
using s32 = System.Int32;
using u32 = System.UInt32;
using s64 = System.Int64;
using u64 = System.UInt64;
#endregion

using System;
using System.Collections.Generic;
using System.Net;
using System.IO;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.Wii
{
	public class NUS
	{
		private static WebClient Client = new WebClient();

		public static void Download(u64 titleid, string filename, Stream outstream)
		{
			Download(Constants.NusCached, titleid, filename, outstream);
		}

		public static void Download(Uri nusbase, u64 titleid, string filename, Stream outstream)
		{
			Stream download = Download(nusbase, titleid, filename);

			Util.StreamCopy(outstream, download);

			download.Close();
		}

		public static Stream Download(u64 titleid, string filename)
		{
			return Download(Constants.NusCached, titleid, filename);
		}

		public static Stream Download(Uri nusbase, u64 titleid, string filename)
		{
			Uri url = new Uri(new Uri(nusbase, Util.ToString(titleid) + "/"), filename);

			return new ForwardStream(Client.OpenRead(url), 0, s64.Parse(Client.ResponseHeaders[HttpResponseHeader.ContentLength]));
		}

		public static Ticket DownloadTicket(u64 titleid)
		{
			return DownloadTicket(Constants.NusCached, titleid);
		}

		public static Ticket DownloadTicket(Uri nusbase, u64 titleid)
		{
			Stream download = Download(nusbase, titleid, "cetk");

			Ticket ticket = Ticket.Create(download);

			download.Close();

			return ticket;
		}

		public static TMD DownloadTMD(ulong titleid, u16 version)
		{
			return DownloadTMD(Constants.NusCached, titleid, version);
		}

		public static TMD DownloadTMD(Uri nusbase, ulong titleid, u16 version)
		{
			Stream download = Download(nusbase, titleid, "tmd." + version.ToString());

			TMD tmd = TMD.Create(download);

			download.Close();

			return tmd;
		}

		public static TMD DownloadTMD(ulong titleid)
		{
			return DownloadTMD(Constants.NusCached, titleid);
		}

		public static TMD DownloadTMD(Uri nusbase, ulong titleid)
		{
			Stream download = Download(nusbase, titleid, "tmd");

			TMD tmd = TMD.Create(download);

			download.Close();

			return tmd;
		}
	}
}
