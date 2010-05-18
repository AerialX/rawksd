OUTPUT_FORMAT ("binary")

SECTIONS {
	OSReport	= 0x805C4BC8;
	
	memcpy		= 0x804902AC;
	memset		= 0x80004104;
	memcmp		= 0x805345D0;
	memcpy		= 0x805EF6E4; /* MD5_memcpy */
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
	MemAlloc__Fii		= 0x8050500C;
	MemFree_FPv			= 0x805051F0;
	
	/* RockCentralGateway */
	_ZN18RockCentralGateway17SubmitPlayerScoreEP6SymboliiiiiPN3Hmx6ObjectE		= 0x801D0468;
	_ZN18RockCentralGateway15SubmitBandScoreEP6SymboliiiP6HxGuidPN3Hmx6ObjectE	= 0x801D0BF0;
	
	/* PlatformMgr */
	ThePlatformMgr	= 0x8082BD00;
	_ZN11PlatformMgr15GetUsernameFullEv = 0x804CBABC;

	/* PassiveMessenger */
	_Z10GetPMPanelv								= 0x80108704;
	_ZN20PassiveMessagesPanel12QueueMessageEPKc	= 0x80107EE4;

	/* HttpWii */
	TheHttpWii							= 0x8082ED28;
	_ZN7HttpWii12GetFileAsyncEPKcPvi	= 0x804FF8AC;
	_ZN7HttpWii13CompleteAsyncEii		= 0x804FF9FC;

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
