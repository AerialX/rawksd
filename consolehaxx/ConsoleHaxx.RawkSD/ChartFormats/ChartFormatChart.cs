using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using ConsoleHaxx.Harmonix;

namespace ConsoleHaxx.RawkSD
{
	public class ChartFormatChart : IChartFormat
	{
		public const string ChartName = "chart";

		public static ChartFormatChart Instance;
		public static void Initialise()
		{
			Instance = new ChartFormatChart();
			Platform.AddFormat(Instance);
		}

		public override int ID {
			get { return 0x40; }
		}

		public override string Name {
			get { return "FeedBack .chart"; }
		}

		public override bool Writable {
			get { return false; }
		}

		public override bool Readable {
			get { return true; }
		}

		public override ChartFormat DecodeChart(FormatData data, ProgressIndicator progress)
		{
			Stream stream = data.GetStream(this, ChartName);
			Chart chart = Chart.Create(stream);
			data.CloseStream(stream);
			return new ChartFormat(chart.GetChart());
		}

		public override void EncodeChart(ChartFormat data, FormatData destination, ProgressIndicator progress)
		{
			throw new NotImplementedException();
		}

		public void Create(FormatData data, Stream stream, bool coop)
		{
			data.SetStream(this, ChartName, stream);

			data.Song.Data.SetValue("ChartChartCoop", coop);
		}
	}
}
