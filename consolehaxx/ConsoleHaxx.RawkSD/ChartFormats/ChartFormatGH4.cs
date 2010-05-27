using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using Nanook.QueenBee.Parser;

namespace ConsoleHaxx.RawkSD
{
	public class ChartFormatGH4 : IChartFormat
	{
		public const string ChartName = "chart";

		public static ChartFormatGH4 Instance;
		public static void Initialise()
		{
			Instance = new ChartFormatGH4();
			Platform.AddFormat(Instance);
		}

		public override int ID {
			get { return 0x45; }
		}

		public override string Name {
			get { return "Guitar Hero 4 Chart"; }
		}

		public void Create(FormatData data, Stream[] streams, bool expertplus)
		{
			for (int i = 0; i < streams.Length; i++)
				data.SetStream(this, ChartName + (i == 0 ? string.Empty : ("." + i.ToString())), streams[i]); ;

			data.Song.Data.SetValue("GH5ChartExpertPlus", expertplus);
		}

		public override bool Writable {
			get { return false; }
		}

		public override bool Readable {
			get { return true; }
		}

		public Stream[] GetChartStreams(FormatData data)
		{
			List<Stream> streams = new List<Stream>();

			int i = 0;
			string name = ChartName;
			do {
				streams.Add(data.GetStream(this, name));
				i++;
				name = ChartName + "." + i.ToString();
			} while (data.HasStream(this, name));

			return streams.ToArray();
		}

		public override ChartFormat DecodeChart(FormatData data, ProgressIndicator progress)
		{
			if (!data.HasStream(this, ChartName))
				throw new FormatException();

			Stream[] streams = GetChartStreams(data);

			ChartFormat format = ChartFormatGH5.Instance.DecodeChart(data, progress, streams);

			foreach (Stream stream in streams)
				data.CloseStream(stream);

			return format;
		}

		public override void EncodeChart(ChartFormat data, FormatData destination, ProgressIndicator progress)
		{
			throw new NotImplementedException();
		}
	}
}
