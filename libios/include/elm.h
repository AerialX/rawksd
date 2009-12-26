/*---------------------------------------------------------------------------/
/  FatFs - FAT file system module include file  R0.07c       (C)ChaN, 2009
/----------------------------------------------------------------------------/
/ FatFs module is an open source software to implement FAT file system to
/ small embedded systems. This is a free software and is opened for education,
/ research and commercial developments under license policy of following trems.
/
/  Copyright (C) 2009, ChaN, all right reserved.
/
/ * The FatFs module is a free software and there is NO WARRANTY.
/ * No restriction on use. You can use, modify and redistribute it for
/   personal, non-profit or commercial product UNDER YOUR RESPONSIBILITY.
/ * Redistributions of source code must retain the above copyright notice.
/----------------------------------------------------------------------------*/

#ifndef __ELM_H__
#define __ELM_H__

#include <elm/integer.h>
#include <elm/ff.h>
#include <elm/diskio.h>

/*-----------------------------------------------------------------------*/
/* Correspondence between physical drive number and physical drive.      */

#define ELM_SD			0
#define ELM_USB			1

#if _MULTI_PARTITION			/* Multiple partition configuration */

typedef struct _PARTITION {
	BYTE pd;			/* Physical drive# */
	BYTE pt;			/* Partition # (0-3) */
} PARTITION;

#endif /* _MULTI_PARTITION */


/* Type of file name on FatFs API */

#if _LFN_UNICODE && _USE_LFN
typedef WCHAR XCHAR;			/* Unicode */
#else
typedef char XCHAR;			/* SBCS, DBCS */
#endif


/* File system object structure */

typedef struct _FATFS_ {
	BYTE	fs_type;		/* FAT sub type */
	BYTE	drive;			/* Physical drive number */
	BYTE	csize;			/* Number of sectors per cluster */
	BYTE	n_fats;			/* Number of FAT copies */
	BYTE	wflag;			/* win[] dirty flag (1:must be written back) */
	WORD	id;			/* File system mount ID */
	WORD	n_rootdir;		/* Number of root directory entries (0 on FAT32) */
#if _FS_REENTRANT
	_SYNC_t	sobj;			/* Identifier of sync object */
#endif
#if 512 != 512
	WORD	s_size;			/* Sector size */
#endif
#if !_FS_READONLY
	BYTE	fsi_flag;		/* fsinfo dirty flag (1:must be written back) */
	DWORD	last_clust;		/* Last allocated cluster */
	DWORD	free_clust;		/* Number of free clusters */
	DWORD	fsi_sector;		/* fsinfo sector */
#endif
#if _FS_RPATH
	DWORD	cdir;			/* Current directory (0:root)*/
#endif
	DWORD	sects_fat;		/* Sectors per fat */
	DWORD	max_clust;		/* Maximum cluster# + 1. Number of clusters is max_clust - 2 */
	DWORD	fatbase;		/* FAT start sector */
	DWORD	dirbase;		/* Root directory start sector (Cluster# on FAT32) */
	DWORD	database;		/* Data start sector */
	DWORD	winsect;		/* Current sector appearing in the win[] */
	BYTE	win[512];		/* Disk access window for Directory/FAT */
} FATFS;



/* Directory object structure */

typedef struct _DIR_ {
	FATFS*	fs;			/* Pointer to the owner file system object */
	WORD	id;			/* Owner file system mount ID */
	WORD	index;			/* Current read/write index number */
	DWORD	sclust;			/* Table start cluster (0:Static table) */
	DWORD	clust;			/* Current cluster */
	DWORD	sect;			/* Current sector */
	BYTE*	dir;			/* Pointer to the current SFN entry in the win[] */
	BYTE*	fn;			/* Pointer to the SFN (in/out) {file[8],ext[3],status[1]} */
#if _USE_LFN
	WCHAR*	lfn;			/* Pointer to the LFN working buffer */
	WORD	lfn_idx;		/* Last matched LFN index number (0xFFFF:No LFN) */
#endif
} DIR;



/* File object structure */

typedef struct _FIL_ {
	FATFS*	fs;			/* Pointer to the owner file system object */
	WORD	id;			/* Owner file system mount ID */
	BYTE	flag;			/* File status flags */
	BYTE	csect;			/* Sector address in the cluster */
	DWORD	fptr;			/* File R/W pointer */
	DWORD	fsize;			/* File size */
	DWORD	org_clust;		/* File start cluster */
	DWORD	curr_clust;		/* Current cluster */
	DWORD	dsect;			/* Current data sector */
#if !_FS_READONLY
	DWORD	dir_sect;		/* Sector containing the directory entry */
	BYTE*	dir_ptr;		/* Ponter to the directory entry in the window */
#endif
#if !_FS_TINY
	BYTE	buf[512];		/* File R/W buffer */
#endif
} FIL;



/* File status structure */

typedef struct _FILINFO_ {
	DWORD	fsize;			/* File size */
	WORD	fdate;			/* Last modified date */
	WORD	ftime;			/* Last modified time */
	BYTE	fattrib;		/* Attribute */
	char	fname[13];		/* Short file name (8.3 format) */
#if _USE_LFN
	XCHAR*	lfname;			/* Pointer to the LFN buffer */
	int 	lfsize;			/* Size of LFN buffer [chrs] */
#endif
} FILINFO;



/* File function return code (FRESULT) */

typedef enum {
	FR_OK = 0,			/* 0 */
	FR_DISK_ERR,			/* 1 */
	FR_INT_ERR,			/* 2 */
	FR_NOT_READY,			/* 3 */
	FR_NO_FILE,			/* 4 */
	FR_NO_PATH,			/* 5 */
	FR_INVALID_NAME,		/* 6 */
	FR_DENIED,			/* 7 */
	FR_EXIST,			/* 8 */
	FR_INVALID_OBJECT,		/* 9 */
	FR_WRITE_PROTECTED,		/* 10 */
	FR_INVALID_DRIVE,		/* 11 */
	FR_NOT_ENABLED,			/* 12 */
	FR_NO_FILESYSTEM,		/* 13 */
	FR_MKFS_ABORTED,		/* 14 */
	FR_TIMEOUT			/* 15 */
} FRESULT;



/*--------------------------------------------------------------*/
/* FatFs module application interface                           */

FRESULT f_mount   (BYTE, FATFS*);					/* Mount/Unmount a logical drive */
FRESULT f_open    (FIL*, const XCHAR*, BYTE);				/* Open or create a file */
FRESULT f_read    (FIL*, void*, UINT, UINT*);				/* Read data from a file */
FRESULT f_write   (FIL*, const void*, UINT, UINT*);			/* Write data to a file */
FRESULT f_lseek   (FIL*, DWORD);					/* Move file pointer of a file object */
FRESULT f_close   (FIL*);						/* Close an open file object */
FRESULT f_opendir (DIR*, const XCHAR*);					/* Open an existing directory */
FRESULT f_readdir (DIR*, FILINFO*);					/* Read a directory item */
FRESULT f_stat    (const XCHAR*, FILINFO*);				/* Get file status */
FRESULT f_getfree (const XCHAR*, DWORD*, FATFS**);			/* Get number of free clusters on the drive */
FRESULT f_truncate(FIL*);						/* Truncate file */
FRESULT f_sync    (FIL*);						/* Flush cached data of a writing file */
FRESULT f_unlink  (const XCHAR*);					/* Delete an existing file or directory */
FRESULT	f_mkdir   (const XCHAR*);					/* Create a new directory */
FRESULT f_chmod   (const XCHAR*, BYTE, BYTE);				/* Change attriburte of the file/dir */
FRESULT f_utime   (const XCHAR*, const FILINFO*);			/* Change timestamp of the file/dir */
FRESULT f_rename  (const XCHAR*, const XCHAR*);				/* Rename/Move a file or directory */
FRESULT f_forward (FIL*, UINT(*)(const BYTE*,UINT), UINT, UINT*);	/* Forward data to the stream */
FRESULT f_mkfs    (BYTE, BYTE, WORD);					/* Create a file system on the drive */
FRESULT f_chdir   (const XCHAR*);					/* Change current directory */
FRESULT f_chdrive (BYTE);						/* Change current drive */

#if _USE_STRFUNC
int f_putc (int, FIL*);							/* Put a character to the file */
int f_puts (const char*, FIL*);						/* Put a string to the file */
int f_printf (FIL*, const char*, ...);					/* Put a formatted string to the file */
char* f_gets (char*, int, FIL*);					/* Get a string from the file */
#define f_eof(fp) (((fp)->fptr == (fp)->fsize) ? 1 : 0)
#define f_error(fp) (((fp)->flag & FA__ERROR) ? 1 : 0)
#ifndef EOF
#define EOF -1
#endif
#endif



/*--------------------------------------------------------------*/
/* User defined functions                                       */

/* Real time clock */
#if !_FS_READONLY
DWORD get_fattime(void);	/* 31-25: Year(0-127 org.1980), 24-21: Month(1-12), 20-16: Day(1-31) */
				/* 15-11: Hour(0-23), 10-5: Minute(0-59), 4-0: Second(0-29 *2) */
#endif

/* Unicode - OEM code conversion */
#if _USE_LFN
WCHAR ff_convert (WCHAR, UINT);
WCHAR ff_wtoupper (WCHAR);
#endif

/* Sync functions */
#if _FS_REENTRANT
BOOL ff_cre_syncobj(BYTE, _SYNC_t*);
BOOL ff_del_syncobj(_SYNC_t);
BOOL ff_req_grant(_SYNC_t);
void ff_rel_grant(_SYNC_t);
#endif



/*--------------------------------------------------------------*/
/* Flags and offset address                                     */


/* File access control and file status flags (FIL.flag) */

#define	FA_READ				0x01
#define	FA_OPEN_EXISTING		0x00
#if _FS_READONLY == 0
#define	FA_WRITE			0x02
#define	FA_CREATE_NEW			0x04
#define	FA_CREATE_ALWAYS		0x08
#define	FA_OPEN_ALWAYS			0x10
#define FA__WRITTEN			0x20
#define FA__DIRTY			0x40
#endif
#define FA__ERROR			0x80


/* FAT sub type (FATFS.fs_type) */

#define FS_FAT12	1
#define FS_FAT16	2
#define FS_FAT32	3


/* File attribute bits for directory entry */

#define	AM_RDO		0x01		/* Read only */
#define	AM_HID		0x02		/* Hidden */
#define	AM_SYS		0x04		/* System */
#define	AM_VOL		0x08		/* Volume label */
#define AM_LFN		0x0F		/* LFN entry */
#define AM_DIR		0x10		/* Directory */
#define AM_ARC		0x20		/* Archive */
#define AM_MASK		0x3F		/* Mask of defined bits */

#endif /* __ELM_H__ */

