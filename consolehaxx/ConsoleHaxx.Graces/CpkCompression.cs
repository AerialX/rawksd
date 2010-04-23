using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.Graces
{
	public static class CpkCompression
	{
		const int vle_levels = 4;

		public static ushort GetNextBits(EndianReader reader, ref long offset_p, ref byte bit_pool_p, ref int bits_left_p, int bit_count)
		{
			int out_bits = 0;
			int num_bits_produced = 0;
			while (num_bits_produced < bit_count) {
				if (0 == bits_left_p) {
					reader.Position = offset_p;
					bit_pool_p = reader.ReadByte();
					bits_left_p = 8;
					offset_p--;
				}

				int bits_this_round;
				if (bits_left_p > (bit_count - num_bits_produced))
					bits_this_round = bit_count - num_bits_produced;
				else
					bits_this_round = bits_left_p;

				out_bits <<= bits_this_round;
				out_bits |= (bit_pool_p >> (bits_left_p - bits_this_round)) & ((1 << bits_this_round) - 1);

				bits_left_p -= bits_this_round;
				num_bits_produced += bits_this_round;
			}

			return (ushort)out_bits;
		}

		public static void Decompress(Stream data, Stream dest)
		{
			EndianReader reader = new EndianReader(data, Endianness.LittleEndian);
			for (int i = 0; i < 2; i++)
				if (reader.ReadUInt32() != 0)
					throw new FormatException();

			long length = reader.ReadUInt32();

			long headeroffset = reader.ReadUInt32() + 0x10;

			byte[] output = new byte[length + 0x100];

			reader.Position = headeroffset;
			reader.Read(output, 0, 0x100);

			long input_end = data.Length - 0x100 - 1;
			long input_offset = input_end;
			long output_end = 0x100 + length - 1;
			byte bit_pool = 0;
			int bits_left = 0;
			long bytes_output = 0;

			while (bytes_output < length) {
				if (GetNextBits(reader, ref input_offset, ref bit_pool, ref bits_left, 1) != 0) {
					long backreference_offset = output_end - bytes_output + GetNextBits(reader, ref input_offset, ref bit_pool, ref bits_left, 13) + 3;
					long backreference_length = 3;

					// decode variable length coding for length
					int[] vle_lens = new int[] { 2, 3, 5, 8 };
					int vle_level;
					for (vle_level = 0; vle_level < vle_levels; vle_level++) {
						int this_level = GetNextBits(reader, ref input_offset, ref bit_pool, ref bits_left, vle_lens[vle_level]);
						backreference_length += this_level;
						if (this_level != ((1 << vle_lens[vle_level])-1))
							break;
					}
					if (vle_level == vle_levels) {
						int this_level;
						do {
							this_level = GetNextBits(reader, ref input_offset, ref bit_pool, ref bits_left, 8);
							backreference_length += this_level;
						} while (this_level == 255);
					}

					for (int i=0;i<backreference_length;i++) {
						output[output_end - bytes_output] = output[backreference_offset--];
						bytes_output++;
					}
				} else {
					output[output_end - bytes_output] = (byte)GetNextBits(reader, ref input_offset, ref bit_pool, ref bits_left, 8);
					bytes_output++;
				}
			}

			dest.Write(output, 0, 0x100 + (int)length);
		}
	}
}
