#!/usr/bin/python
# Server program

import os, sys
from PyQt4 import QtCore, QtGui, QtNetwork

class ServerThread(QtCore.QThread):
	def __init__(self, connection):
		self.connection = connection
	def run(self):
		self.connection.Run()

class Connection(object):
	Handshake = 0
	Goodbye = 1
	FileOpen = 0x10
	FileRead = 0x11
	FileWrite = 0x12
	FileSeek = 0x13
	FileTell = 0x14
	FileSync = 0x15
	FileClose = 0x16
	FileStat = 0x17
	FileCreate = 0x18
	FileDelete = 0x19
	FileRename = 0x1A
	FileCreateDir = 0x20
	FileOpenDir = 0x21
	FileCloseDir = 0x22
	FileNextDirPath = 0x23
	FileNextDirStat = 0x24
	FileNextDirCache = 0x25

	def __init__(self, root, client=None):
		self.OpenFileFD = 1;
		self.OpenDirs = {}					#FIXME
		self.OpenFiles = {}					#FIXME
		self.Options = {}					#FIXME

		self.Root = root					#FIXME

		self.Client = client
		#self.Stream = client.GetStream()	#FIXME
		#self.Writer = None					#FIXME
		#FIXME
		#added
		self.Client.readyRead.connect(self.get_message)

	def get_message(self):
		message = self.Client.readLine()
		print message.data()
		print type(message)
		main_window.txtSocket.appendPlainText( "\n" + message.data() )

	def Run(self):
		while self.Client.state() == QtNetwork.QAbstractSocket.ConnectedState:
			#try:
			if not WaitForAction( self.Client ):
				break
			#catch:							#FIXME
			#	pass						#FIXME
		self.Close()

	def Close(self):
		print "Disconnected."
		Program.Connections.remove(self)

		#FIXME
	
	def GetData( size ):
		data = ""
		read = 0
		print "Bytes available", self.Client.bytesAvailable()
		while read < size:
			#read += Stream.Read(data, read, size - read)
			data += self.Client.read(size)
			read = size
		return data

	def Return( value ):
		self.Client.writeData( str(value) )
		print "Return", value

	def WaitForAction( client ):
		action = GetData(4)

		if action == 1:
			option = GetData(4)
			length = GetData(4)
			data = []
			if length > 0:
				data = GetData(length)

			if data == 3:
				print "Ping()"
		elif action == 2:
			command = GetData(4)

			if command == self.Handshake:
				#FIXME
				clientversion = GetString()
				print "Handshake: Client Version %s" % clientversion
				if clientversion != "1.02":
					Return(-1)
				else:
					Return(ServerVersion)
			elif command == self.Goodbye:
				print "Goodbye"
				Return(1)
				return False
			elif command == self.FileOpen:
				path = ""
			else:
				pass
		

class SocketTest(QtGui.QWidget):
	def __init__(self, parent=None):
		QtGui.QWidget.__init__(self,parent)
		self.setWindowTitle("RiiFS Python Server")
		self.setMinimumWidth(500)

		layout = QtGui.QVBoxLayout(self)

		self.txtSocket = QtGui.QPlainTextEdit()
		layout.addWidget(self.txtSocket)

		self.quitButton = QtGui.QPushButton(self.tr("&Quit"))
		self.connect(self.quitButton, QtCore.SIGNAL("clicked()"), self, QtCore.SLOT("close()"))
		layout.addWidget(self.quitButton)

		self.listeningServer = Listener(self)
		self.connect(self.listeningServer, QtCore.SIGNAL("serverDebug"), self.update_debug)

	def update_debug(self, txt):
		self.txtSocket.appendPlainText("\n" + txt)

class Listener(QtCore.QObject):

	def __init__(self, parent):
		QtCore.QObject.__init__(self, parent)

		self.connections = []
		self.threads = []
		self.count = 0
		self.server = QtNetwork.QTcpServer(self)
		#FIXME
		#tcp_address = QtNetwork.QHostAddress.Any
		tcp_address = QtNetwork.QHostAddress("192.168.0.146")
		tcp_port = 1137
		self.connect(self.server, QtCore.SIGNAL("newConnection()"), self.socket_connected)
		self.server.listen(tcp_address, tcp_port)
		parent.txtSocket.setPlainText("RiiFS Python Server is now ready for connections on 1137")

	def socket_connected(self):
		socket = self.server.nextPendingConnection()
		Root = os.getcwd()
		connection = Connection( Root , socket )
		#FIXME
		self.connections.append(connection)
		thread = ServerThread(self.connections[len(self.connections)-1])
		thread.start()
		self.threads.append(thread)
		self.emit(QtCore.SIGNAL("serverDebug"), "Socket Connected on %d" % self.server.serverPort())

	def process_messages(self):
		'''
		while self.socket.hasPendingDatagrams():
			datagram, host, port = self.socket.readDatagram(self.socket.pendingDatagramSize())
			print "Recieving>>", self.count
			debug_string  = "%s\nraw=%s\n" %(self.count, datagram)

			properties = datagram.strip().split("\t")
			for prop in properties:
				debug_string += "%s\n" % prop
			self.emit(QtCore.SIGNAL("serverDebug"), debug_string)
			self.count += 1
		'''
		'''
		debug_string = self.socket.readLine()
		self.emit(QtCore.SIGNAL("serverDebug"), debug_string)
		'''
		pass

if __name__ == "__main__":
	global main_window
	app = QtGui.QApplication(sys.argv)
	main_window = SocketTest()
	main_window.show()
	sys.exit(app.exec_())
