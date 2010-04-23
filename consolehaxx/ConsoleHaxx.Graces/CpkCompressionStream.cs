using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.Graces
{
	public class CpkCompressionStream : Stream
	{
		public override bool CanRead { get { return true; } }

		public override bool CanSeek { get { return true; } }

		public override bool CanWrite { get { return false; } }

		public override void Flush()
		{
			throw new NotImplementedException();
		}

		public override long Length { get { throw new NotImplementedException(); } }

		public override long Position
		{
			get
			{
				throw new NotImplementedException();
			}
			set
			{
				throw new NotImplementedException();
			}
		}

		public override long Seek(long offset, SeekOrigin origin)
		{
			switch (origin) {
				case SeekOrigin.Current:
					offset = Position + offset;
					break;
				case SeekOrigin.End:
					offset += Length;
					break;
			}

			Position = offset;

			return offset;
		}

		public override void SetLength(long value)
		{
			throw new NotImplementedException();
		}

		public override void Write(byte[] buffer, int offset, int count)
		{
			throw new NotImplementedException();
		}

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

		public override int Read(byte[] buffer, int offset, int count)
		{
			throw new NotImplementedException();
		}
	}
}
