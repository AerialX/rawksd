using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace ConsoleHaxx.Common
{
	public class Pair<TKey, TValue>
	{
		public TKey Key;
		public TValue Value;

		public Pair(TKey key, TValue value)
		{
			Key = key;
			Value = value;
		}
	}
}
