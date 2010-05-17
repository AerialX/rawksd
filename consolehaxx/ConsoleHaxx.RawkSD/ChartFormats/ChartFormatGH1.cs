using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

namespace ConsoleHaxx.RawkSD
{
	public class ChartFormatGH1 : IChartFormat
	{
		public const string ChartFile = "chart";

		public static readonly ChartFormatGH1 Instance;
		static ChartFormatGH1()
		{
			Instance = new ChartFormatGH1();
			Platform.AddFormat(Instance);
		}

		public override int ID {
			get { return 0x01; }
		}

		public override string Name {
			get { return "Guitar Hero 1 MIDI Chart"; }
		}

		public void Create(FormatData data, Stream chart)
		{
			data.SetStream(this, ChartFile, chart);
		}

		public override bool Writable {
			get { return false; }
		}

		public override bool Readable {
			get { return true; }
		}

		public override ChartFormat DecodeChart(FormatData data, ProgressIndicator progress)
		{
			if (!data.HasStream(this, ChartFile))
				throw new FormatException();

			Stream stream = data.GetStream(this, ChartFile);
			ChartFormat chart = ChartFormat.Create(stream);
			data.CloseStream(stream);

			ChartFormatGH2.DecodeOverdrive(chart.Chart);

			return chart;
		}

		public override void EncodeChart(ChartFormat data, FormatData destination, ProgressIndicator progress)
		{
			throw new NotImplementedException();
		}
	}
}
