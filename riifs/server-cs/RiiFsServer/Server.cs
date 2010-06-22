using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Net.Sockets;
using System.Net;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.RiiFS
{
	public class Server
	{
		public int BroadcastPort { get; protected set; }
		public int Port { get; protected set; }
		public string Root { get; protected set; }
		public bool ReadOnly { get; set; }

		protected Thread ServerThread;
		protected Thread TimeoutThread;
		protected Thread BroadcastThread;
		protected TcpListener Listener;
		protected Mutex ConnectionMutex;
		protected UdpClient UDP;

		protected List<Connection> Connections;

		protected volatile bool Running;

		public Server(string path, int port = 1137)
		{
			Root = path;
			Port = port;
			BroadcastPort = 1137;

			ReadOnly = true;
			Running = true;

			Connections = new List<Connection>();
			ConnectionMutex = new Mutex();

			TimeoutThread = new Thread(TimeoutThreadMethod);
			TimeoutThread.IsBackground = true;
			TimeoutThread.Priority = ThreadPriority.BelowNormal;
			TimeoutThread.Start();

			Listener = new TcpListener(IPAddress.Any, Port);
			Listener.Start();
			if (Port == 0)
				Port = (Listener.LocalEndpoint as IPEndPoint).Port;
		}

		public void Stop()
		{
			if (!Running)
				return;

			Running = false;
			TimeoutThread.Interrupt();
			Listener.Stop();
			if (UDP != null)
				UDP.Close();

			ConnectionMutex.WaitOne();
			foreach (Connection connection in Connections)
				connection.Close();
			ConnectionMutex.ReleaseMutex();

			if (BroadcastThread != null)
				BroadcastThread.Abort();
			if (TimeoutThread != null)
				TimeoutThread.Abort();
			if (ServerThread != null)
				ServerThread.Abort();
		}

		public void StartAsync()
		{
			ServerThread = new Thread(Start);
			ServerThread.Start();
		}

		public void Start()
		{
			Console.WriteLine("RiiFS C# Server is now ready for connections on " + Listener.LocalEndpoint.ToString());
			while (Running) {
				try {
					TcpClient client = Listener.AcceptTcpClient();
					Connection connection = new Connection(this, client);
					connection.DebugPrint("Connection Established");
					connection.StartAsync();
				} catch (SocketException) { }
			}
		}

		protected void TimeoutThreadMethod()
		{
			while (Running) {
				ConnectionMutex.WaitOne();
				List<Connection> pingout = new List<Connection>();
				foreach (Connection connection in Connections) {
					TimeSpan diff = DateTime.Now - connection.LastPing;
					if (diff > TimeSpan.FromSeconds(120)) {
						connection.DebugPrint("Ping Timeout (" + diff.TotalSeconds.ToString() + " seconds)");
						pingout.Add(connection);
					}
				}
				ConnectionMutex.ReleaseMutex();

				foreach (Connection connection in pingout)
					connection.Close();

				try {
					Thread.Sleep(TimeSpan.FromSeconds(15));
				} catch (ThreadInterruptedException) { }
			}
		}

		public void StartBroadcastAsync(int port)
		{
			BroadcastPort = port;
			StartBroadcastAsync();
		}

		public void StartBroadcastAsync()
		{
			BroadcastThread = new Thread(StartBroadcast);
			BroadcastThread.IsBackground = true;
			BroadcastThread.Priority = ThreadPriority.BelowNormal;
			BroadcastThread.Start();
		}

		public void StartBroadcast()
		{
			UDP = new UdpClient(BroadcastPort, AddressFamily.InterNetwork);
			UDP.EnableBroadcast = true;
			UDP.Client.ReceiveTimeout = 1000;

			while (Running) {
				IPEndPoint endpoint = new IPEndPoint(IPAddress.Any, 0);
				try {
					byte[] data = UDP.Receive(ref endpoint);
					if (data.Length == 4) {
						Option command = (Option)BigEndianConverter.ToInt32(data);
						if (command == Option.Ping) {
							Console.WriteLine("Broadcast ping from " + endpoint.ToString() + ", replying with port " + Port.ToString());
							UDP.Send(BigEndianConverter.GetBytes(Port), 4, endpoint);
						}
					}
				} catch (SocketException) { }
			}
		}

		public void RemoveConnection(Connection connection)
		{
			ConnectionMutex.WaitOne();

			Connections.Remove(connection);

			ConnectionMutex.ReleaseMutex();
		}

		public void AddConnection(Connection connection)
		{
			ConnectionMutex.WaitOne();

			Connections.Add(connection);

			ConnectionMutex.ReleaseMutex();
		}
	}
}
