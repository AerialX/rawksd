RiiFS C++ Server
October 7th, 2010
riivolution@japaneatahand.com

Usage: riifs <path-to-root> <port>
 - path is optional, defaults to current directory
 - port is optional, defaults to 1137

The root given to the server is the root of the filesystem, and it's treated no differently than an SD card.
Think about what that means:
 - You can copy the contents of a Riivolution-ready SD card into a folder and point the server there, and it will work.
 - Path names like external="/" will actually point to the root path the server is started with.

Riivolution will automatically connect to a RiiFS server on a local LAN at startup.
However, to get Riivolution to connect to a specific or external server it needs to have an XML file tell it to do so. Like so:

<wiidisc version="1">
	<!-- id is optional, omit it in order to make Riivolution connect to the server regardless of the game inserted in the drive. -->
	<id game="SMN" /> 

	<!-- address may be an IP address or a hostname/domain. -->
	<network protocol="riifs" address="192.168.2.2" port="1137" />
</wiidisc>

Changelog

v1.03b
October 7th, 2010
 - Fix seeking after reaching end-of-file

v1.03
June 22, 2010
 - Initial release
