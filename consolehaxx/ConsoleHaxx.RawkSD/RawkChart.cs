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
		public NoteChart Chart;

		public ChartFormat(NoteChart chart)
		{
			Chart = chart;
		}

		public static ChartFormat Create(Midi midi)
		{
			ChartFormat format = new ChartFormat(NoteChart.Create(midi));
			return format;
		}

		public static ChartFormat Create(Stream stream)
		{
			return Create(Midi.Create(Mid.Create(stream)));
		}

		public void Save(Stream stream)
		{
			Chart.ToMidi().ToMid().Save(stream);
		}
	}
}
