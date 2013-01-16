OUTPUT_FORMAT ("binary")

SECTIONS {
	__OSReport	= 0x805C4BC8;
	
	memcpy		= 0x80004000;
	memset		= 0x80004104;
	memcmp		= 0x805345D0;
	memmove		= 0x805344AC;
	
	strcasecmp	= 0x8053B4DC;
	strcat		= 0x805387B0;
	strchr		= 0x80538984;
	strcmp		= 0x80538828;
	strcpy		= 0x805386AC;
	strlen		= 0x8053FA64;
	strncat		= 0x805387DC;
	strncmp		= 0x80538944;
	strncpy		= 0x8053876C;
	strrchr		= 0x805389B4;
	strstr		= 0x80538B20;
	strtok		= 0x805389FC;
	
	wcscmp		= 0x8053B1FC;
	wcscpy		= 0x8053B19C;
	wcslen		= 0x8053B180;
	wcsncpy		= 0x8053B1B8;
	
	sprintf		= 0x80536E28;
	
	IOS_Close			= 0x805B2E98;
	IOS_CloseAsync		= 0x805B2DD8;
	IOS_Ioctl			= 0x805B3650;
	IOS_IoctlAsync		= 0x805B3518;
	IOS_Ioctlv			= 0x805B39A0;
	IOS_IoctlvAsync		= 0x805B38BC;
	IOS_IoctlvReboot	= 0x805B3A7C;
	IOS_Open			= 0x805B2CB8;
	IOS_OpenAsync		= 0x805B2BA0;
	IOS_Read			= 0x805B3040;
	IOS_ReadAsync		= 0x805B2F40;
	IOS_Seek			= 0x805B3430;
	IOS_SeekAsync		= 0x805B3350;
	IOS_Write			= 0x805B3248;
	IOS_WriteAsync		= 0x805B3148;

	ES_GetDeviceID		= 0x805866E8;

	net_bind			= 0x80619498;
	net_cleanup			= 0x80618894;
	net_close			= 0x806193F4;
	net_connect			= 0x80619580;
	net_fcntl			= 0x80619700;
	net_finish			= 0x80618364;
	net_freeaddrinfo	= 0x8061A664;
	net_getaddrinfo		= 0x8061A380;
	net_gethostbyname	= 0x8061A240;
	net_gethostid		= 0x8061A1C8;
	net_getinterfaceopt	= 0x8061A7CC;
	net_getlasterror	= 0x80618A44;
	net_htonl			= 0x80619CD8;
	net_htons			= 0x80619CDC;
	inet_ntop			= 0x80619B80;
	inet_pton			= 0x80619A40;
	net_init			= 0x8061819C;
	net_ntohl			= 0x80619CCC;
	net_ntohs			= 0x80619CD0;
	net_poll			= 0x806198E4;
	net_recv			= 0x80619690;
	net_recvfrom		= 0x80619668;
	net_send			= 0x806196DC;
	net_sendto			= 0x806196B4;
	net_setsockopt		= 0x8061A6C8;
	net_shutdown		= 0x80619830;
	net_socket			= 0x80619304;
	net_startup			= 0x80618460;
	net_startupEx		= 0x8061846C;

	/* RB2 Stuff */
	_Z8MemAllocii			= 0x8050500C;
	_Z7MemFreePv			= 0x805051F0;
	_Z9PoolAllocii			= 0x8050B90C;
	_Z8PoolFreeiPv			= 0x8050B990;

	/* String */
	_ZN6String9ConstructEPKc	= 0x8050C014;
	_ZN6String8DestructEv		= 0x8050C1CC;
	_ZN6String5c_strEv			= 0x8000D414;

	_ZN6SymbolC1EPKc			= 0x8050D2C0;

	/* App */
	_ZN3App3RunEv	= 0x8000CFE8;

	/* Splash */
	_ZN9SplashWii14EndShowLoadingEv	= 0x803511C4;
	_ZN9SplashWii8DestructEv		= 0x80350D34;
	_ZN6Splash8DestructEv			= 0x80347E70;

	/* BinStream */
	_ZN9MemStreamC1Eb					= 0x805064EC;
	_ZN9MemStreamD1Ev					= 0x80066D34;
	_ZN9BinStream17DisableEncryptionEv	= 0x804F9214;
	_ZN9BinStream5WriteEPKvi			= 0x804F9310;
	_ZN9BinStream4ReadEPvi				= 0x804F924C;
	_ZN9BinStream4SeekEiNS_8SeekTypeE	= 0x804F943C;

	_ZN9DataArrayC1Ei					= 0x804D3FC4;
	_ZN9DataArray4LoadEP9BinStream		= 0x804D470C;
	_ZN9DataArray6InsertEiR8DataNode	= 0x804D32BC;
	_ZN9DataArray5CloneEbb				= 0x804D3E68;
	_ZN9DataArrayD1Ev					= 0x804D40D8;
	_ZN9DataArray11InsertNodesEiPS_		= 0x804D33F8;
	_ZN9DataArray6RemoveEi				= 0x804D36A4;
	_ZN9DataArray9FindArrayE6SymbolS0_	= 0x804D387C;
	_ZN9DataArray9FindArrayE6Symbolb	= 0x804D3870;	

	_ZN8DataNodeC1EPKc					= 0x804DE328;	
	_ZN8DataNodeC1Ei					= 0x8000D464;
	_ZN8DataNode5ArrayEP9DataArray		= 0x804DE2C4;
	_ZN8DataNodeC1EP9DataArrayi			= 0x804DE4C4;
	_ZN8DataNode8DestructEv				= 0x8000D478;
	_ZN8DataNode3StrEP9DataArray		= 0x804DE108;
	_ZN8DataNode12LiteralArrayEP9DataArray	= 0x804DE2E8;
	_ZN8DataNode3IntEP9DataArray		= 0x804DE068;

	_Z4__rsP9BinStreamRP9DataArray		= 0x804D5008;
	_Z4__lsP10TextStreamPK9DataArray	= 0x804D4FB0;

	_Z12SystemConfig6SymbolS_			= 0x804D0094;
	_ZN8DataNode9ConstructEPKc			= 0x804DE328;
	
	/* RockCentralGateway */
	_ZN18RockCentralGateway17SubmitPlayerScoreEP6SymboliiiiiPN3Hmx6ObjectE		= 0x801D0468;
	_ZN18RockCentralGateway15SubmitBandScoreEP6SymboliiiP6HxGuidPN3Hmx6ObjectE	= 0x801D0BF0;
	
	/* PlatformMgr */
	ThePlatformMgr	= 0x8082BD00;
	_ZN11PlatformMgr15GetUsernameFullEv = 0x804CBABC;

	/* SongMgr */
	gSongMgrWii									= 0x808209C0;
	gSongMgr									= 0x8082079C;
	_ZN7SongMgr10SongArtistEP6Symbol			= 0x80078D84;
	_ZN7SongMgr8SongNameEP6Symbol				= 0x80078CF0;
	_ZN7SongMgr11AddSongDataEP9DataArrayPKci	= 0x8007BC88;
	_ZN10WiiSongMgr6HandleEP9DataArrayb			= 0x8008A38C;
	_ZN10WiiSongMgr4LoadEP9BinStream			= 0x804B4F98;
	_ZN7SongMgr8SongPackE6Symbol				= 0x80078F40;
	_ZN7SongMgr10IsDownloadE6SymbolP9DataArray	= 0x800785F4;
	_ZN7SongMgr4DataE6Symbol					= 0x8007CA3C;

	_ZN17SongOfferProvider10DataSymbolEi			= 0x801375D0;
	_ZN17SongOfferProvider12GetSongOfferE6Symbol	= 0x801317A4;

	/* PassiveMessenger */
	_Z10GetPMPanelv								= 0x80108704;
	_ZN20PassiveMessagesPanel12QueueMessageEPKc	= 0x80107EE4;

	/* HttpWii */
	TheHttpWii							= 0x8082ED28;
	_ZN7HttpWii12GetFileAsyncEPKcPvi	= 0x804FF8AC;
	_ZN7HttpWii13CompleteAsyncEii		= 0x804FF9FC;

	/* Net */
	TheNet								= 0x80827538;

	/* Server */
	_ZN6Server11IsConnectedEv			= 0x80454C2C;

	/* UsbWii */
	_ZN6UsbWii7GetTypeEP9HIDDevice	= 0x804AB95C;

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
