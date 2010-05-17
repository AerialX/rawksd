using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

namespace ConsoleHaxx.Common
{
	public class Mp3
	{
		public class Header
		{
			private MpegVersion Version;
			private MpegLayer Layer;
			private int _SampleRate;
			private int _BitRate;
			private bool IsProtected;
			private int Padding;
			private bool IsPrivate;
			private Mode ChannelMode;
			private int ModeExtension;
			private bool IsCopyright;
			private bool IsOriginal;
			private int Emphasis;

			public enum MpegVersion
			{
				Mpeg2_5 = 0,
				Reserved = 1,
				Mpeg2 = 2,
				Mpeg3 = 3
			}

			public enum MpegLayer
			{
				Reserved = 0,
				Layer3 = 1,
				Layer2 = 2,
				Layer1 = 3
			}

			public enum Mode
			{
				Stereo = 0,
				JointStereo = 1,
				DualChannel = 2,
				SingleChannel = 3
			}

			// Bitrate [h_version][h_layer][h_bitrate]
			public static readonly int[][][] BITRATE_INDEX = new int[][][] {
				new int[][] { // v2.5: Reserved, L3, L2, L1
					new int[] { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
					new int[] { 0, 8000, 16000, 24000, 32000, 40000, 48000, 56000, 64000, 80000, 96000, 112000, 128000, 144000, 160000, 0 },
					new int[] { 0, 8000, 16000, 24000, 32000, 40000, 48000, 56000, 64000, 80000, 96000, 112000, 128000, 144000, 160000, 0 },
					new int[] { 0, 32000, 48000, 56000, 64000, 80000, 96000, 112000, 128000, 144000, 160000, 176000, 192000, 224000, 256000, 0 }
				},
				// Reserved: Don't know bitrate, keep zero;
				new int[][] { // Reserved: Reserved, L3, L2, L1
					new int[] { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
					new int[] { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
					new int[] { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
					new int[] { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
				},
				new int[][] { // v2: Reserved, L3, L2, L1
					new int[] { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
					new int[] { 0, 8000, 16000, 24000, 32000, 40000, 48000, 56000, 64000, 80000, 96000, 112000, 128000, 144000, 160000, 0 },
					new int[] { 0, 8000, 16000, 24000, 32000, 40000, 48000, 56000, 64000, 80000, 96000, 112000, 128000, 144000, 160000, 0 },
					new int[] { 0, 32000, 48000, 56000, 64000, 80000, 96000, 112000, 128000, 144000, 160000, 176000, 192000, 224000, 256000, 0 }
				},
				new int[][] { // v1: Reserved, L3, L2, L1
					new int[] { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
					new int[] { 0, 32000, 40000, 48000, 56000, 64000, 80000, 96000, 112000, 128000, 160000, 192000, 224000, 256000, 320000, 0 },
					new int[] { 0, 32000, 48000, 56000, 64000, 80000, 96000, 112000, 128000, 160000, 192000, 224000, 256000, 320000, 384000, 0 },
					new int[] { 0, 32000, 64000, 96000, 128000, 160000, 192000, 224000, 256000, 288000, 320000, 352000, 384000, 416000, 448000, 0 }
				},

				// If bitrate is ever zero, we should call an error.
				// Sinc Free format is too much work to deal with.
				// TODO: Add VBR and Xing support.
			};

			// Samplerate [h_version][h_sampRate]
			public static readonly int[][] SAMPLING_FREQUENCY_INDEX = new int[][] {
				new int[] { 11025,	12000,	8000,	0 },	// v2.5
				new int[] { 0,		0,		0,		0 },	// vReserved
				new int[] { 22050,	24000,	16000,	0 },	// v2
				new int[] { 44100,	48000,	32000,	0 }		// v1
			};

			public Header(byte[] data)
			{
				if (!(((data[0] & 255) == 255) && (((data[1] >> 5) & 7) == 7)))
					throw new FormatException();
				
				Version = (MpegVersion)((data[1] >> 3) & 3);
				Layer = (MpegLayer)((data[1] >> 1) & 3);
				IsProtected = (data[1] & 1) == 1;
				_BitRate = (data[2] >> 4) & 15;
				_SampleRate = (data[2] >> 2) & 3;
				Padding = (data[2] >> 1) & 1;
				IsPrivate = (data[2] & 1) == 1;
				ChannelMode = (Mode)((data[3] >> 6) & 3);
				ModeExtension = (data[3] >> 4) & 3;
				IsCopyright = ((data[3] >> 3) & 1) == 1;
				IsOriginal = ((data[3] >> 2) & 1) == 1;
				Emphasis = (data[3] & 3);
			}

			public int FrameLength {
				get {
					switch (Layer) {
						case MpegLayer.Layer3:
						case MpegLayer.Layer2:
							return (144 * BitRate / SampleRate) + Padding;
						case MpegLayer.Layer1:
							return (12 * BitRate / SampleRate + Padding) * 4;
						default:
							return 0;
					}	
				}
			}

			public int SampleRate { get { return SAMPLING_FREQUENCY_INDEX[(int)Version][_SampleRate]; } }

			public int BitRate { get { return BITRATE_INDEX[(int)Version][(int)Layer][_BitRate]; } }
		}
	}
}
