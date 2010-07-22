using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using ConsoleHaxx.Common;
using System.Threading;

namespace ConsoleHaxx.RawkSD
{
	public static class Exceptions
	{
		public static void Warning(string message)
		{
			Warning(null, message);
		}

		public static void Warning(Exception exception, string message = null)
		{
			WriteException(0, exception is ProgressException ? null : exception, message);
		}

		private static void WriteException(int indent, Exception exception, string message)
		{
			if (message.HasValue())
				Write(indent, message);
			if (exception != null) {
				Write(indent, exception.GetType().FullName + ": " + exception.Message);
				Write(indent, exception.StackTrace);
				if (exception.InnerException != null)
					WriteException(indent + 1, exception.InnerException, null);
			}
		}

		private static void Write(int indent, string message)
		{
			foreach (string line in message.Split('\n')) {
				try {
					Indent(indent);
					Console.WriteLine(line);
				} catch { }
			}
		}

		private static void Indent(int indent)
		{
			for (int i = 0; i < indent; i++)
				Console.Write('\t');
			Console.Write("[" + DateTime.Now.ToString() + "] ");
		}

		public static void Error(string message)
		{
			Error(null, message);
		}

		public static void Error(Exception exception, string message = null)
		{
			Warning(exception, message);
			if (exception is ProgressException)
				throw exception;
			throw new ProgressException(message, exception);
		}
	}
}
