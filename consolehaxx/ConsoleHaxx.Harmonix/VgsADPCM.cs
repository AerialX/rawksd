using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.Harmonix
{
	public static class VgsADPCM
	{
		static int[][] xa_table = new int[][] 
		{
			new int[] {0, 0},
			new int[] {60, 0},
			new int[] {115, -52},
			new int[] {98, -55},
			new int[] {122, -60}
		};

		public static bool XA_Decode(int[] state, byte[] indata, ref long inoffset, short[] outdata, long outoffset)
		{
			int j;
			int predict1, predict2, shift;
			int s1, s2;

			if (indata[inoffset + 1] == 0x01) // End of stream
				return false;

			int pred = Math.Min(indata[inoffset] >> 4, 4);
			predict1 = xa_table[pred][0];
			predict2 = xa_table[pred][1];
			shift = indata[inoffset] & 0x0F;
			inoffset += 2;
			s1 = state[0];
			s2 = state[1];
			for (j = 0; j < 28; j += 2, inoffset++) {
				int low = ((short)(indata[inoffset] << 12)) >> shift;
				int high = ((short)((indata[inoffset] & 0xF0) << 8)) >> shift;
				s2 = low + ((s1 * predict1 + s2 * predict2 + 32) >> 6);
				outdata[j + outoffset] = (short)s2;
				s1 = high + ((s2 * predict1 + s1 * predict2 + 32) >> 6);
				outdata[j + 1 + outoffset] = (short)s1;
			}
			state[0] = s1;
			state[1] = s2;

			return true;
		}

		public static long Decompress(int[] state, byte[] data, short[] samples)
		{
			long written = 0;
			long inoffset = 0;
			while (written <= samples.Length - 28) {
				if (!XA_Decode(state, data, ref inoffset, samples, written))
					break;
				written += 28;
			}

			return written;
		}

		public static long BytesToSamples(long bytes)
		{
			return bytes * 7 / 4;
		}
	}
}
