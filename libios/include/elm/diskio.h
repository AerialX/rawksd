/*-----------------------------------------------------------------------
/  Low level disk interface modlue include file  R0.07   (C)ChaN, 2009
/-----------------------------------------------------------------------*/

#ifndef __DISKIO_H__
#define __DISKIO_H__

#include <elm/integer.h>
#include <elm.h>

#define _READONLY	0	/* 1: Read-only mode */
#define _USE_IOCTL	1

/* Status of Disk Functions */
typedef BYTE	DSTATUS;

/* Results of Disk Functions */
typedef enum {
	RES_OK = 0,		/* 0: Successful */
	RES_ERROR,		/* 1: R/W Error */
	RES_WRPRT,		/* 2: Write Protected */
	RES_NOTRDY,		/* 3: Not Ready */
	RES_PARERR,		/* 4: Invalid Parameter */
	RES_NOENABLE		/* 5: Function not allowed for Device */
} DRESULT;

/*---------------------------------------*/
/* Prototypes for disk control functions */

DSTATUS disk_initialize(BYTE);
DSTATUS disk_deinitialize(BYTE);
DSTATUS disk_status(BYTE);
DRESULT disk_read(BYTE, BYTE*, DWORD, BYTE);
#if	_READONLY == 0
DRESULT disk_write(BYTE, const BYTE*, DWORD, BYTE);
#endif
DRESULT disk_ioctl(BYTE, BYTE, void*);

/* Disk Status Bits (DSTATUS) */

#define STA_OK			0x00	/* 0: Successful */
#define STA_NOINIT		0x01	/* Drive not initialized */
#define STA_NODISK		0x02	/* No medium in the drive */
#define STA_PROTECT		0x04	/* Write protected */

#endif /* __DISKIO_H__ */
