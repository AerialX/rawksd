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

		public static readonly ChartFormatGH4 Instance;
		static ChartFormatGH4()
		{
			Instance = new ChartFormatGH4();
		}

		public override int ID {
			get { return 0x04; }
		}

		public override string Name {
			get { return "Guitar Hero World Tour / Metallica / Smash Hits / Van Halen"; }
		}

		public void Create(FormatData data, Stream stream, PakFormatType paktype)
		{
			data.SetStream(this, ChartName, stream);
		}

		public override bool Writable {
			get { return false; }
		}

		public override bool Readable {
			get { return true; }
		}

		public override ChartFormat DecodeChart(FormatData data)
		{
			if (!data.HasStream(this, ChartName))
				throw new FormatException();

			Stream chartstream = data.GetStream(this, ChartName);

			ChartFormat format = ChartFormatGH5.Instance.DecodeChart(data, chartstream);

			data.CloseStream(this, ChartName);

			return format;
		}

		public override void EncodeChart(ChartFormat data, FormatData destination, ProgressIndicator progress)
		{
			throw new NotImplementedException();
		}
	}
}
