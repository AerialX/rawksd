#!/usr/bin/python
# Server program

import os, sys
from PyQt4 import QtCore, QtGui, QtNetwork

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

		self.listeningSocket = Listener(self)
		self.connect(self.listeningSocket, QtCore.SIGNAL("socketDebug"), self.update_debug)

	def update_debug(self, txt):
		self.txtSocket.appendPlainText("\n" + txt)

class Listener(QtCore.QObject):

	def __init__(self, parent):
		QtCore.QObject.__init__(self, parent)

		self.socket = QtNetwork.QUdpSocket(self)
		self.socket.bind(1137)
		self.connect(self.socket, QtCore.SIGNAL("readyRead()"), self.process_datagrams)
		self.count = 0
		parent.txtSocket.setPlainText("RiiFS Python Server is now ready for connections on 1137")

	def process_datagrams(self):
		while self.socket.hasPendingDatagrams():
			datagram, host, port = self.socket.readDatagram(self.socket.pendingDatagramSize())
			print "Recieving>>", self.count
			debug_string  = "%s\nraw=%s\n" %(self.count, datagram)

			properties = datagram.strip().split("\t")
			for prop in properties:
				debug_string += "%s\n" % prop
			self.emit(QtCore.SIGNAL("socketDebug"), debug_string)
			self.count += 1

def main():
	# Set global stuff
	root = os.get_path()

	# Set the socket parameters
	host = "localhost"
	port = 1137
	buf = 1024
	addr = (host,port)

	# Create socket and bind to address
	UDPSock = socket(AF_INET,SOCK_DGRAM)
	UDPSock.bind(addr)

	print "RiiFS Python Server is now readon for connections on %d" % port

	# Receive messages
	while 1:
		data,addr = UDPSock.recvfrom(buf)
		if not data:
			print "Client has exited!"
			break
		else:
			print "\nReceived message '", data,"'"

	# Close socket
	UDPSock.close()

if __name__ == "__main__":
	app = QtGui.QApplication(sys.argv)
	widget = SocketTest()
	widget.show()
	sys.exit(app.exec_())
