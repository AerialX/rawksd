using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace ConsoleHaxx.RawkSD
{
	public class ChartFormatChart : IChartFormat
	{
		public static readonly ChartFormatChart Instance;
		static ChartFormatChart()
		{
			Instance = new ChartFormatChart();
		}

		public override int ID {
			get { return 0x07; }
		}

		public override string Name {
			get { return "FeedBack .chart"; }
		}

		public override bool Writable {
			get { return false; }
		}

		public override bool Readable {
			get { return false; }
		}

		public override ChartFormat DecodeChart(FormatData data)
		{
			throw new NotImplementedException();
		}

		public override void EncodeChart(ChartFormat data, FormatData destination, ProgressIndicator progress)
		{
			throw new NotImplementedException();
		}
	}
}
