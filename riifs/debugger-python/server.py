#!/usr/bin/python

# Server program

from PyQt4 import QtCore, QtGui, QtNetwork

from sys import argv, exit
import os
#from time import sleep
import struct
from Struct import *
import binascii
from collections import deque

#commands	(val|0x80000000) = ARM side
#==============
#0x00=none
#0x01=poke byte				//IMPLEMENTED IN MODULE
#0x02=poke short			//IMPLEMENTED IN MODULE
#0x03=poke int
#0x04=peek address
#0x06=freeze
#0x07=unfreeze
#0x08=resume game			//NOT IMPLEMENTED YET
#0x09=dump
#0x99=version number		//IMPLEMENTED IN MODULE

def endian_swap(string_in):
	return struct.unpack( '>i' , string_in )[0]

class CommandStruct(Struct):
	__endian__ = Struct.BE
	def __format__(self):
		self.command = Struct.uint32
		self.data1 = Struct.uint32
		self.data2 = Struct.uint32
		self.data3 = Struct.uint32

class ServerThread(QtCore.QThread):
	Options = ["" for x in xrange(17)]
	Handshake = 0x00
	File = 0x01
	Path = 0x02
	Mode = 0x03
	Length = 0x04
	Data = 0x05
	SeekWhere = 0x06
	SeekWhence = 0x07
	RenameSource = 0x08
	RenameDestination = 0x09
	Ping = 0x10

	def __init__(self, socketDescriptor, rBuffer, clientID, parent):
		QtCore.QThread.__init__(self, parent)

		self.socketDescriptor = socketDescriptor
		self.rBuffer = rBuffer
		self.clientID = clientID
		self.ServerVersion = 0x04
		self.Files = {}
		self.opened = 0

	def run(self):
		self.tcpSocket = QtNetwork.QTcpSocket()
		print "Connection Established"

		if not self.tcpSocket.setSocketDescriptor(self.socketDescriptor):
			self.emit(QtCore.SIGNAL("error(int)"), tcpSocket.error())
			print "Error on setSocketDescriptor"
			return

		#print "Initial Socket state:", self.tcpSocket.state()
		while self.tcpSocket.state() == 3:
			if self.WaitForAction() == 0:
				break
		print "Out of while loop"

	def WaitForAction(self):
		if self.tcpSocket.bytesAvailable() == 0:
			if self.tcpSocket.waitForReadyRead(0x6000) == True:
				#print "Ready to Read"
				pass
			else:
				#print "Not Ready to Read"
				pass
		if self.tcpSocket.bytesAvailable() == 0:
			return
		#print "ToRead: %08x" % self.tcpSocket.bytesAvailable()
		unparsed = self.tcpSocket.read(4)
		if unparsed:
			action = endian_swap(unparsed)
		else:
			action = 0
		if action:
			#print "Action:", action
			pass
		if action == 1:			# SEND #
			option = struct.unpack( '>i', self.tcpSocket.read(4) )[0]
			#print "Option: ", option
			length = struct.unpack( '>i', self.tcpSocket.read(4) )[0]
			#print "Length: ", length
			if length > 0:
				data = self.GetData(length)
				#print "Data:", data
			else:
				data = ""

			self.Options[option] = data

			if option == self.Ping:
				#print "Ping()"
				pass
		elif action == 2:		# RECEIVE #
			command = struct.unpack( '>i', self.tcpSocket.read(4) )[0]
			#print "Command: ", command
			if command == 0:		# HANDSHAKE #
				clientversion = self.Options[self.Handshake]
				print "Handshake: Client Version:", clientversion
				if clientversion != "1.03":
					print "Client Version hex: %s" % binascii.hexlify(clientversion)
					self.Respond(-1)
				else:
					#print "Responded %08x" % self.ServerVersion
					self.Respond(self.ServerVersion)
					#self.Respond(1)
			elif command == 1:		# GOODBYE #
				print "Goodbye"
				self.Respond(1)
				return False
			elif command == 2:		# LOG #
				sys.stdout.write("Log: %s" % self.Options[self.Data])
				self.Respond(1)
			elif command == 3:		# POLL #
				leng = self.GetLength()
				#print "Poll"
				ret_strct = CommandStruct()
				if len(command_list) != 0:
					cmd = command_list.popleft()
					#print cmd
					ret_strct.command = cmd[0]
					ret_strct.data1 = cmd[1]
					ret_strct.data2 = cmd[2]
					ret_strct.data3 = cmd[3]
				else:
					cmd = (0,0,0,0)
					ret_strct.command = 0
					ret_strct.data1 = 0
					ret_strct.data2 = 0
					ret_strct.data3 = 0
				if leng < len(ret_strct):
					self.Pad(leng)
					self.Respond(0)
				else:
					self.Write(ret_strct.pack(),len(ret_strct))
					if leng > len(ret_strct):
						self.Pad(leng - len(ret_strct))
					self.Respond(1)
			elif command == 4:		# DUMP #
				print "Dump"
				'''
				leng = len(self.Options[self.Data])
				sys.stdout.write("Dump....")
				fp = open('dump.bin', 'wb')
				fp.write(self.Options[self.Data])
				fp.close()
				sys.stdout.write("Done\n")
				'''
				self.Respond(1)
			elif command == 5:		# WRITE #
				file = self.GetFD()
				value = self.Options[self.Data]
				self.Files[file].write(value)
				self.Respond(1)
				if ((self.written % 0x1000) == 0):
					sys.stdout.write(".")
				self.written += 1
			elif command == 6:		# OPEN #
				sys.stdout.write("Open(")
				fd = open("dump.bin.%d" % self.opened, 'wb')
				self.Files[self.opened] = fd
				self.Respond(self.opened)
				print "%d);\n" % self.opened
				self.opened += 1
				self.written = 0
			elif command == 7:		# CLOSE #
				sys.stdout.write("Close(")
				fd = self.Files[self.GetFD()]
				fd.close()
				print "%d);" % self.GetFD()
				self.Respond(1)
				self.written = 0
			else:
				print "Command : %08x" % command
				pass
		else:
			pass
		#print "post-WaitForAction Socket state:", self.tcpSocket.state()

	def Pad(self, length):
		string = '\0' * length
		written = self.tcpSocket.writeData( string[:length] )
		#print "ToWrite: %08x" % self.tcpSocket.bytesToWrite()
		ret = self.tcpSocket.waitForBytesWritten(100000)
		#print "\tRespond: (%d of %d bytes: %d) : %s" % (written, length, ret, binascii.hexlify(string))

	def Write(self, string, length):
		assert len(string) >= length
		written = self.tcpSocket.writeData( string[:length] )
		#print "ToWrite: %08x" % self.tcpSocket.bytesToWrite()
		ret = self.tcpSocket.waitForBytesWritten(100000)
		#print "\tRespond: (%d of %d bytes: %d) : %s" % (written, length, ret, binascii.hexlify(string))

	def Respond(self, value):
		written = self.tcpSocket.writeData( struct.pack('>i', value) )
		#print "ToWrite: %08x" % self.tcpSocket.bytesToWrite()
		ret = self.tcpSocket.waitForBytesWritten(100000)
		#print "\tRespond: (%d bytes: %d) : %d" % (written, ret, value)

	def GetData(self, length):
		if self.tcpSocket.bytesAvailable() == 0:
			if self.tcpSocket.waitForReadyRead(0x6000) == True:
				#print "Ready to Read"
				pass
			else:
				#print "Not Ready to Read"
				pass
		read = ""
		read_len = 0
		out = ""
		while read_len < length:
			read = self.tcpSocket.read(length - read_len)
			if read <= 0:
				break
			read_len += len(read)
			out += read
		#print "Length: %d Should be: %d" % (len(out), length)
		return out

	def GetLength(self):
		return endian_swap( self.Options[self.Length] )

	def GetString(self, string_in):
		return string_in

	def GetPath(self):
		path = self.GetString( self.Options[self.Path] )
		return path
	
	def GetFD(self):
		return endian_swap( self.Options[self.File] )

class Server(QtNetwork.QTcpServer):

	threadList=[]
	bufferList=[]

	def __init__(self, parent=None):
		QtNetwork.QTcpServer.__init__(self, parent)
		self.parent=parent

	def incomingConnection(self, socketDescriptor):
		wBuffer = QtCore.QBuffer()
		self.threadID=str(len(self.threadList)+1)
		thread = ServerThread(socketDescriptor, wBuffer, self.threadID, self)
		self.bufferList.append(wBuffer)
		self.threadList.append(thread)
		if not self.parent.clientList.isEnabled():
			self.parent.clientList.setEnabled(True)
		self.parent.clientList.insertItem(int(self.threadID), QtCore.QString('Client '+self.threadID))
		self.parent.clientList.setItemData(int(self.threadID), QtCore.QVariant(self.threadID), 0)
		thread.start()

	def sendMsg(self, msgt, args, client=None,):
		print "Sending..."
		clientID=self.parent.clientList.currentIndex()
		cBuffer=client or self.bufferList[clientID]
		if cBuffer.open(QtCore.QBuffer.WriteOnly):
			cBuffer.writeData('server,'+msgt+','+args)
			cBuffer.waitForBytesWritten(1000)
			cBuffer.close()
		else:
			print "Failed to Send"

class ServerGUI(QtGui.QWidget):
	_running=False

	def __init__(self, parent=None):
		QtGui.QWidget.__init__(self, parent)

		self.setGeometry(50,50,350,350)
		self.setWindowTitle('MegaIOS Python Server')

		self.vLayout=QtGui.QVBoxLayout()
		self.setLayout(self.vLayout)

		self.ipLayout=QtGui.QHBoxLayout()
		self.labelIP=QtGui.QLabel("IP Address",self)
		self.ipLayout.addWidget(self.labelIP)
		self.specAddr=QtGui.QLineEdit('192.168.0.146')
		self.ipLayout.addWidget(self.specAddr)
		self.vLayout.addLayout(self.ipLayout)

		self.portLayout=QtGui.QHBoxLayout()
		self.labelPort=QtGui.QLabel("Port",self)
		self.portLayout.addWidget(self.labelPort)
		self.specPort=QtGui.QLineEdit('20002')
		self.portLayout.addWidget(self.specPort)
		self.vLayout.addLayout(self.portLayout)

		self.serverCtrl=QtGui.QPushButton('Start Server')
		self.serverCtrl.clicked.connect(self.ctrlServer)
		self.vLayout.addWidget(self.serverCtrl)

		self.clientList=QtGui.QComboBox()
		self.clientList.setEnabled(False)
		self.vLayout.addWidget(self.clientList)

		'''
		self.serverTrace=QtGui.QTextEdit()
		self.vLayout.addWidget(self.serverTrace)
		'''

		self.peekLayout=QtGui.QHBoxLayout()
		self.addressPeek=QtGui.QLineEdit('0x8000191C')
		self.peekLayout.addWidget(self.addressPeek)
		self.peekCtrl=QtGui.QPushButton('Peek')
		self.peekCtrl.clicked.connect(self.peek)
		self.peekLayout.addWidget(self.peekCtrl)
		self.vLayout.addLayout(self.peekLayout)

		self.pokeLayout=QtGui.QHBoxLayout()
		self.pokeValLayout=QtGui.QVBoxLayout()
		self.addressPoke=QtGui.QLineEdit('0x8000191C')
		self.pokeValLayout.addWidget(self.addressPoke)
		self.valuePoke=QtGui.QLineEdit('0x00000000')
		self.pokeValLayout.addWidget(self.valuePoke)
		self.pokeLayout.addLayout(self.pokeValLayout)
		self.pokeCtrl=QtGui.QPushButton('Poke')
		self.pokeCtrl.clicked.connect(self.poke)
		self.pokeLayout.addWidget(self.pokeCtrl)
		self.vLayout.addLayout(self.pokeLayout)

		self.frozenLayout=QtGui.QHBoxLayout()
		self.freezeCtrl=QtGui.QPushButton('Freeze')
		self.freezeCtrl.clicked.connect(self.freeze)
		self.frozenLayout.addWidget(self.freezeCtrl)
		self.unfreezeCtrl=QtGui.QPushButton('UnFreeze')
		self.unfreezeCtrl.clicked.connect(self.unfreeze)
		self.frozenLayout.addWidget(self.unfreezeCtrl)
		self.vLayout.addLayout(self.frozenLayout)

		self.dumpLayout=QtGui.QHBoxLayout()
		self.dumpValLayout=QtGui.QVBoxLayout()
		self.addressDump=QtGui.QLineEdit('0x90000000')
		self.dumpValLayout.addWidget(self.addressDump)
		self.rangeDump=QtGui.QLineEdit('0x02000000')
		self.dumpValLayout.addWidget(self.rangeDump)
		self.dumpLayout.addLayout(self.dumpValLayout)
		self.dumpCtrl=QtGui.QPushButton('Dump')
		self.dumpCtrl.clicked.connect(self.dump)
		self.dumpLayout.addWidget(self.dumpCtrl)
		self.vLayout.addLayout(self.dumpLayout)

	def freeze(self):
		command_list.append((0x80000006, 0, 0, 0))

	def unfreeze(self):
		command_list.append((0x80000007, 0, 0, 0))

	def peek(self):
		#print int(str(self.addressPeek.text()), 16)
		command_list.append((0x80000004, int(str(self.addressPeek.text()),16), 0, 0))

	def poke(self):
		command_list.append((0x80000003, int(str(self.addressPoke.text()),16), int(str(self.valuePoke.text()),16), 0))

	def dump(self):
		command_list.append((0x80000009, int(str(self.addressDump.text()),16), int(str(self.rangeDump.text()),16), 0))

	def ctrlServer(self):
		if not self._running:
			self._server = Server(self)

			#FIXME
			#if not self._server.listen(QtNetwork.QHostAddress.Any, self.specPort.text().toInt()[0]):
			if not self._server.listen(QtNetwork.QHostAddress(self.specAddr.text()), self.specPort.text().toInt()[0]):
				print "Listen ERROR!"
				self.close()
				return
			else:
				print "Server Running"
				self.serverCtrl.setText('Stop Server')
				self._running=True
		else:
			for thread in self._server.threadList:
				thread.exit()
				self._server.close()
			self.close()

if __name__ == "__main__":
	global command_list
	command_list = deque()
	app = QtGui.QApplication(argv)
	server = ServerGUI()
	server.show()
	exit(app.exec_())

