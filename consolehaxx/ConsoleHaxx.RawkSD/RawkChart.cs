using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using ConsoleHaxx.Harmonix;
using System.IO;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.RawkSD
{
	// TODO: This is much too RB2-specific, needs generalizing
	public class ChartFormat
	{
		NoteChart Chart;

		public ChartFormat(NoteChart chart)
		{
			Chart = chart;
		}

		public static ChartFormat Create(Stream stream)
		{
			ChartFormat format = new ChartFormat(NoteChart.Create(Midi.Create(Mid.Create(stream))));
			return format;
		}

		public void Save(Stream stream)
		{
			Chart.ToMidi().ToMid().Save(stream);
		}
	}
}
