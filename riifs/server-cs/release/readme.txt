RiiFS C# Server
March 28, 2010
riivolution@japaneatahand.com

Requires Microsoft .NET 2.0 or Mono to run.

Usage: riifs.exe <path-to-root> <port>
 - path is optional, defaults to current directory
 - port is optional, defaults to 1137

The root given to the server is the root of the filesystem, and it's treated no differently than an SD card.
Think about what that means:
 - You can copy the contents of a Riivolution-ready SD card into a folder and point the server there, and it will work.
 - Path names like external="/" will actually point to the root path the server is started with.

To get Riivolution to connect to a server, it needs to have an XML file to tell it to do so. Like so:

<wiidisc version="1">
	<!-- id is optional, omit it in order to make Riivolution connect to the server regardless of the game inserted in the drive. -->
	<id game="SMN" /> 

	<!-- address may be an IP address or a hostname/domain. -->
	<network protocol="riifs" address="192.168.2.2" port="1137" />
</wiidisc>

Changelog

v1.02
March 28, 2010
 - Support for Riivolution v1.02
 - Cleanup and file read fixes
 - Better logging format

v1.01
March 23, 2010
 - Initial release
