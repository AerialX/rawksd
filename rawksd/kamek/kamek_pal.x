OUTPUT_FORMAT ("binary")

SECTIONS {
	__OSReport	= 0x805C65F8;
	
	memcpy		= 0x80004000;
	memset		= 0x80004104;
	memcmp		= 0x80535FFC;
	memmove		= 0x80535ED8;
	
	strcasecmp	= 0x8053CF08;
	strcat		= 0x8053A1DC;
	strchr		= 0x8053A3B0;
	strcmp		= 0x8053A254;
	strcpy		= 0x8053A0D8;
	strlen		= 0x80541490;
	strncat		= 0x8053A208;
	strncmp		= 0x8053A370;
	strncpy		= 0x8053A198;
	strrchr		= 0x8053A3E0;
	strstr		= 0x8053A54C;
	strtok		= 0x8053A428;
	
	wcscmp		= 0x8053CC28;
	wcscpy		= 0x8053CBC8;
	wcslen		= 0x8053CBAC;
	wcsncpy		= 0x8053CBE4;
	
	sprintf		= 0x80538854;
	
	IOS_Close			= 0x805B48C8;
	IOS_CloseAsync		= 0x805B4808;
	IOS_Ioctl			= 0x805B5080;
	IOS_IoctlAsync		= 0x805B4F48;
	IOS_Ioctlv			= 0x805B53D0;
	IOS_IoctlvAsync		= 0x805B52EC;
	IOS_IoctlvReboot	= 0x805B54AC;
	IOS_Open			= 0x805B46E8;
	IOS_OpenAsync		= 0x805B45D0;
	IOS_Read			= 0x805B4A70;
	IOS_ReadAsync		= 0x805B4970;
	IOS_Seek			= 0x805B4E60;
	IOS_SeekAsync		= 0x805B4D80;
	IOS_Write			= 0x805B4C78;
	IOS_WriteAsync		= 0x805B4B78;

	ES_GetDeviceID		= 0x80588118;

	net_bind			= 0x8061AEC8;
	net_cleanup			= 0x8061A2C4;
	net_close			= 0x8061AE24;
	net_connect			= 0x8061AFB0;
	net_fcntl			= 0x8061B130;
	net_finish			= 0x80619D94;
	net_freeaddrinfo	= 0x8061C094;
	net_getaddrinfo		= 0x8061BDB0;
	net_gethostbyname	= 0x8061BC70;
	net_gethostid		= 0x8061BBF8;
	net_getinterfaceopt	= 0x8061C1FC;
	net_getlasterror	= 0x8061A474;
	net_htonl			= 0x8061B708;
	net_htons			= 0x8061B70C;
	inet_ntop			= 0x8061B5B0;
	inet_pton			= 0x8061B470;
	net_init			= 0x80619BCC;
	net_ntohl			= 0x8061B6FC;
	net_ntohs			= 0x8061B700;
	net_poll			= 0x8061B314;
	net_recv			= 0x8061B0C0;
	net_recvfrom		= 0x8061B098;
	net_send			= 0x8061B10C;
	net_sendto			= 0x8061B0E4;
	net_setsockopt		= 0x8061C0F8;
	net_shutdown		= 0x8061B260;
	net_socket			= 0x8061AD34;
	net_startup			= 0x80619E90;
	net_startupEx		= 0x80619E9C;

	/* RB2 Stuff */
	_Z8MemAllocii			= 0x80506A38;
	_Z7MemFreePv			= 0x80506C1C;
	_Z9PoolAllocii			= 0x8050D338;
	_ZN8DataNode8DestructEv	= 0x8000D440;
	_Z8PoolFreeiPv			= 0x8050D3BC;

	/* String */
	_ZN6String9ConstructEPKc	= 0x8050DA40;
	_ZN6String8DestructEv		= 0x8050DBF8;
	_ZN6String5c_strEv			= 0x8000D3DC;

	_ZN6SymbolC1EPKc			= 0x8050ECEC;

	/* App */
	_ZN3App3RunEv	= 0x8000CFB0;

	/* Splash */
	_ZN9SplashWii14EndShowLoadingEv	= 0x80352150;
	_ZN9SplashWii8DestructEv		= 0x80351CC0;
	_ZN6Splash8DestructEv			= 0x80348DFC;

	/* BinStream */
	_ZN9MemStreamC1Eb					= 0x80507F18;
	_ZN9MemStreamD1Ev					= 0x80066EA0;
	_ZN9BinStream17DisableEncryptionEv	= 0x804FAC40;
	_ZN9BinStream5WriteEPKvi			= 0x804FAD3C;
	_ZN9BinStream4ReadEPvi				= 0x804FAC78;
	_ZN9BinStream4SeekEiNS_8SeekTypeE	= 0x804FAE68;

	_ZN9DataArrayC1Ei					= 0x804D59F0;
	_ZN9DataArrayD1Ev					= 0x804D5B04;
	_ZN9DataArray4LoadEP9BinStream		= 0x804D6138;
	_ZN9DataArray6InsertEiR8DataNode	= 0x804D4CE8;
	_ZN9DataArray5CloneEbb				= 0x804D5894;
	_ZN9DataArray11InsertNodesEiPS_		= 0x804D4E24;
	_ZN9DataArray6RemoveEi				= 0x804D50D0;
	_ZN9DataArray9FindArrayE6SymbolS0_	= 0x804D52A8;
	_ZN9DataArray9FindArrayE6Symbolb	= 0x804D529C;

	_ZN8DataNodeC1EPKc					= 0x804DFD54;	
	_ZN8DataNodeC1Ei					= 0x8000D42C;
	_ZN8DataNode5ArrayEP9DataArray		= 0x804DFCF0;
	_ZN8DataNodeC1EP9DataArrayi			= 0x804DFEF0;
	_ZN8DataNode3StrEP9DataArray		= 0x804DFB34;
	_ZN8DataNode12LiteralArrayEP9DataArray	= 0x804DFD14;
	_ZN8DataNode3IntEP9DataArray		= 0x804DFA94;

	_Z4__rsP9BinStreamRP9DataArray		= 0x804D6A34;
	_Z4__lsP10TextStreamPK9DataArray	= 0x804D69DC;

	_Z12SystemConfig6SymbolS_			= 0x804D1A84;
	_ZN8DataNode9ConstructEPKc			= 0x804DFD54;
	
	/* RockCentralGateway */
	_ZN18RockCentralGateway17SubmitPlayerScoreEP6SymboliiiiiPN3Hmx6ObjectE		= 0x801D12D0;
	_ZN18RockCentralGateway15SubmitBandScoreEP6SymboliiiP6HxGuidPN3Hmx6ObjectE	= 0x801D1AB0;
	
	/* PlatformMgr */
	ThePlatformMgr	= 0x8082E800;
	_ZN11PlatformMgr15GetUsernameFullEv = 0x804CD2C8;

	/* SongMgr */
	gSongMgrWii									= 0x808234C0;
	gSongMgr									= 0x8082329C;
	_ZN7SongMgr10SongArtistEP6Symbol			= 0x80078EF0;
	_ZN7SongMgr8SongNameEP6Symbol				= 0x80078E5C;
	_ZN7SongMgr11AddSongDataEP9DataArrayPKci	= 0x8007BDF4;
	_ZN10WiiSongMgr6HandleEP9DataArrayb			= 0x8008A4F8;
	_ZN10WiiSongMgr4LoadEP9BinStream			= 0x804B6164;
	_ZN7SongMgr4DataE6Symbol					= 0x8007CBA8;

	_ZN17SongOfferProvider10DataSymbolEi			= 0x80137CC4;

	/* PassiveMessenger */
	_Z10GetPMPanelv								= 0x80108924;
	_ZN20PassiveMessagesPanel12QueueMessageEPKc	= 0x80108104;

	/* HttpWii */
	TheHttpWii							= 0x80831828;
	_ZN7HttpWii12GetFileAsyncEPKcPvi	= 0x805012D8;
	_ZN7HttpWii13CompleteAsyncEii		= 0x80501428;

	/* Net */
	TheNet								= 0x8082A038;

	/* Server */
	_ZN6Server11IsConnectedEv			= 0x80455E7C;

	/* UsbWii */
	_ZN6UsbWii7GetTypeEP9HIDDevice	= 0x804ACB18;
	
	.text : {
		FILL (0)
		
		__text_start = . ;
		*(.init)
		*(.text)
		*(.ctors)
		*(.dtors)
		*(.rodata)
		*(.fini)
		__text_end  = . ;
	}
}
