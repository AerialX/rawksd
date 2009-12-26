/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2007        */
/*-----------------------------------------------------------------------*/
/* This is a stub disk I/O module that acts as front end of the existing */
/* disk I/O modules and attach it to FatFs module with common interface. */
/*-----------------------------------------------------------------------*/

/* Support headers */
#include <gctypes.h>
/* Generic libogc disc I/O stuff */
#include <disc_io.h>
/* Libogc's USB Storage stuff */
#include <usbstorage.h>
/* libsdcard's Front SD stuff. */
#include <wiisd_io.h>

#include <time.h>
#include <string.h>

/* Our ELM stuff. */
#include <elm/diskio.h>
#include <elm.h>

int SD_disk_deinitialize();
int USB_disk_deinitialize();

int SD_disk_initialize();
int USB_disk_initialize();

int SD_disk_status();
int USB_disk_status();

int SD_disk_read(BYTE *buff, DWORD sector, BYTE count);
int USB_disk_read(BYTE *buff, DWORD sector, BYTE count);

int SD_disk_write(BYTE *buff, DWORD sector, BYTE count);
int USB_disk_write(BYTE *buff, DWORD sector, BYTE count);

int SD_disk_ioctl(BYTE ctrl, BYTE *buff);
int USB_disk_ioctl(BYTE ctrl, BYTE *buff);

DRESULT get_resluts(int reslut)
{
	if(reslut == -1)
		return RES_PARERR;
	if(reslut == -2)
		return RES_ERROR;
	if(reslut == -3)
		return RES_WRPRT;
	if(reslut == -4)
		return RES_NOTRDY;
	if(reslut == -5)
		return RES_NOENABLE;
	return RES_OK;
}

DSTATUS get_satts(int reslut)
{
	if(reslut == -1)
		return STA_NOINIT;
	if(reslut == -2)
		return STA_NODISK;
	if(reslut == -3)
		return STA_PROTECT;
	return STA_OK;
}

/*-----------------------------------------------------------------------*/
/* Deinidialize a Drive                                                  */
/* drv						Drive number.		 */

DSTATUS disk_deinitialize(BYTE drv)
{
	DSTATUS satt;
	int reslut = -1;
	
	switch (drv) {
		case ELM_SD:
			reslut = SD_disk_deinitialize();
			break;
		case ELM_USB:
			reslut = USB_disk_deinitialize();
			break;
	}
	satt = get_satts(reslut);
	return satt;
}

int SD_disk_deinitialize()
{
	int reslut = __io_wiisd.shutdown();
	if(reslut == 0)
		return -1;
	return 0;
}

int USB_disk_deinitialize()
{
	int reslut = __io_usbstorage.shutdown();
	if(reslut == 1)
		return 0;
	return -2;
}

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/* drv						Drive number.		 */

DSTATUS disk_initialize(BYTE drv)
{
	DSTATUS satt;
	int reslut = -1;

	switch (drv) {
		case ELM_SD:
			reslut = SD_disk_initialize();
			break;
		case ELM_USB:
			reslut = USB_disk_initialize();
			break;
	}
	satt = get_satts(reslut);
	return satt;
}

int SD_disk_initialize()
{
	int reslut = __io_wiisd.startup();
	if(reslut == 0)
		return -1;
	if(!__io_wiisd.isInserted()) {
		__io_wiisd.shutdown();
		return -2;
	}
	return 0;
}

int USB_disk_initialize()
{
	int reslut = __io_usbstorage.startup();
	if(reslut == 1)
		return 0;
	return -2;
}

/*-----------------------------------------------------------------------*/
/* Return Disk Status                                                    */
/* drv						Drive number.		 */

DSTATUS disk_status(BYTE drv)
{
	DSTATUS satt;
	int reslut = -1;

	switch (drv) {
		case ELM_SD:
			reslut = SD_disk_status();
			break;
		case ELM_USB:
			reslut = USB_disk_status();
			break;
	}
	satt = get_satts(reslut);
	return satt;
}

int SD_disk_status()
{
	if(__io_wiisd.isInserted())
		return 0;
	return -2;
}

int USB_disk_status()
{
	if(__io_usbstorage.isInserted())
		return 0;
	return -2;
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/* drv						Drive number.		 */
/* buff						Data buffer.		 */
/* sector					Sector Address. (LBA)	 */
/* count					Sector Count. (1 - 255)	 */

DRESULT disk_read(BYTE drv, BYTE *buff, DWORD sector, BYTE count)
{
	DRESULT res;
	int reslut = -1;

	switch (drv) {
		case ELM_SD:
			reslut = SD_disk_read(buff, sector, count);
			break;
		case ELM_USB:
			reslut = USB_disk_read(buff, sector, count);
			break;
	}
	res = get_resluts(reslut);
	return res;
}

int SD_disk_read(BYTE *buff, DWORD sector, BYTE count)
{
	int reslut = __io_wiisd.readSectors(sector, count, buff);
	if(reslut == 0)
		return -2;
	return 0;
}

int USB_disk_read(BYTE *buff, DWORD sector, BYTE count)
{
	int reslut = __io_usbstorage.readSectors(sector, count, buff);
	if(reslut == 0)
		return -2;
	return 0;
}


/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/* drv						Drive number.		 */
/* buff						Data buffer.		 */
/* sector					Sector Address. (LBA)	 */
/* count					Sector Count. (1 - 255)	 */

#if _READONLY == 0
DRESULT disk_write(BYTE drv, const BYTE *buff, DWORD sector, BYTE count)
{
	DRESULT res;
	int reslut = -1;

	switch (drv) {
		case ELM_SD:
			reslut = SD_disk_write(buff, sector, count);
			break;
		case ELM_USB:
			reslut = USB_disk_write(buff, sector, count);
			break;
	}
	res = get_resluts(reslut);
	return res;
}

int SD_disk_write(BYTE *buff, DWORD sector, BYTE count)
{
	//BYTE *obuff = memalign(32, count * 512);
	//memcpy(obuff, buff, count * 512);
	//int reslut = __io_wiisd.writeSectors(sector, count, obuff);
	int reslut = __io_wiisd.writeSectors(sector, count, buff); // Let's assume alignment!
	//BYTE *obuff = Alloc(count * 512);
	//memcpy(obuff, buff, count * 512);
	//int reslut = __io_wiisd.writeSectors(sector, count, obuff);
	if(reslut == 0) {
		return -2;
	}
	return 0;
}

int USB_disk_write(BYTE *buff, DWORD sector, BYTE count)
{
	int reslut = __io_usbstorage.writeSectors(sector, count, buff);
	if(reslut == 0)
		return -2;
	return 0;
}

#endif /* _READONLY */

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/* drv						Drive number.		 */
/* ctrl						Control Code.		 */
/* buff						Data buffer.		 */

DRESULT disk_ioctl(BYTE drv, BYTE ctrl, void *buff)
{
	DRESULT res;
	int reslut = -1;

	switch (drv) {
		case ELM_SD:
			reslut = SD_disk_ioctl(ctrl, buff);
			break;
		case ELM_USB:
			reslut = USB_disk_ioctl(ctrl, buff);
			break;
	}
	res = get_resluts(reslut);
	return res;
}

int SD_disk_ioctl(BYTE ctrl, BYTE *buff)
{
	return 0;
}

int USB_disk_ioctl(BYTE ctrl, BYTE *buff)
{
	return 0;
}

DWORD get_fattime()
{
	return 0xB3294924; // 09/09/09... Fucking Starlet
/*
	time_t ti = time(NULL);
	struct tm* tyme = localtime(&ti);
	int reslut  = (tyme->tm_sec / 2) & 0x1F;
	    reslut |= ((tyme->tm_min)  & 0x3F) << 5;
	    reslut |= ((tyme->tm_hour) & 0x1F) << 11;
	    reslut |= ((tyme->tm_mday) & 0x1F) << 16;
	    reslut |= ((tyme->tm_mon)  & 0xF)  << 21;
	    reslut |= ((tyme->tm_year) & 0x7F) << 25;
	return reslut;
*/
}
