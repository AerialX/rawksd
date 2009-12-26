/*-------------------------------------------------------------

usbstorage_starlet.c -- USB mass storage support, inside starlet
Copyright (C) 2009 Kwiirk

If this driver is linked before libogc, this will replace the original
usbstorage driver by svpe from libogc
This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1.	The origin of this software must not be misrepresented; you
must not claim that you wrote the original software. If you use
this software in a product, an acknowledgment in the product
documentation would be appreciated but is not required.

2.	Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3.	This notice may not be removed or altered from any source
distribution.

-------------------------------------------------------------*/

#include "syscalls.h"
#include "gctypes.h"
#include "gcutil.h"
#include "usbstorage.h"
#include "ipc.h"

#define VISUALIZE

#ifdef VISUALIZE
#include "gpio.h"
#endif

#include <stdio.h>
#include <string.h>

#define DEVICE_TYPE_WII_USB		(('W'<<24)|('U'<<16)|('S'<<8)|'B')

/* IOCTL commands */
#define UMS_BASE			(('U'<<24)|('M'<<16)|('S'<<8))
#define USB_IOCTL_UMS_INIT	        (UMS_BASE+0x1)
#define USB_IOCTL_UMS_GET_CAPACITY      (UMS_BASE+0x2)
#define USB_IOCTL_UMS_READ_SECTORS      (UMS_BASE+0x3)
#define USB_IOCTL_UMS_WRITE_SECTORS	(UMS_BASE+0x4)
#define USB_IOCTL_UMS_READ_STRESS	(UMS_BASE+0x5)
#define USB_IOCTL_UMS_SET_VERBOSE	(UMS_BASE+0x6)

/* Constants */
#define USB_MAX_SECTORS			64

/* Variables */
static char fs[] = "/dev/usb/ehc";
static s32  fd = -1;

static u32  sectorSz = 0;

s32 __usbstorage_GetCapacity(u32 *_sectorSz)
{
	STACK_ALIGN(u32, sector, 1, 32);
	ioctlv vector;

	if (fd >= 0) {
		s32 ret;

		/* Setup vector */
		vector.data = sector;
		vector.len  = sizeof(u32);

		os_sync_after_write(&vector, sizeof(ioctlv));

		/* Get capacity */
		ret = os_ioctlv(fd, USB_IOCTL_UMS_GET_CAPACITY, 0, 1, &vector);

		os_sync_before_read(sector, sizeof(u32));

		if (ret>0 && _sectorSz)
			*_sectorSz = *sector;

		return ret;
	}

	return IPC_ENOENT;
}


bool usbstorage_Init(void)
{
	s32 ret;

	/* Already open */
	if (fd >= 0)
		return true;

	/* Open USB device */
	fd = os_open(fs, 0);
	if (fd < 0)
		return false;

	/* Initialize USB storage */
	os_ioctlv(fd, USB_IOCTL_UMS_INIT, 0, 0, NULL);

	/* Get device capacity */
	ret = __usbstorage_GetCapacity(&sectorSz);
	if (ret <= 0)
		goto err;

	return true;

err:
	/* Close USB device */
	usbstorage_Shutdown();

	return false;
}

bool usbstorage_Shutdown(void)
{
	if (fd >= 0) {
		/* Close USB device */
		os_close(fd);

		/* Remove descriptor */
		fd = -1;
	}

	return true;
}

bool usbstorage_IsInserted(void)
{
	s32 ret;

	/* Get device capacity */
	ret = __usbstorage_GetCapacity(NULL);

	return (ret > 0);
}

bool usbstorage_Transfer(u32 cmd, u32 sector, u32 numSectors, void *buffer)
{
	u32 cnt, in=2, out=1, _sector, _numSectors=1;
	s32 ret;
	ioctlv vector[3];

	/* Device not opened */
	if (fd < 0 || (u32)buffer & 0x1F)
		return false;

	if (!numSectors)
		return true;

#ifdef VISUALIZE
	gpio_set_on(GPIO_OSLOT);
#endif

	/* Setup vector */
	vector[0].data = &_sector;
	vector[0].len = sizeof(_sector);
	vector[1].data = &_numSectors;
	vector[1].len = sizeof(_numSectors);

	if (cmd==USB_IOCTL_UMS_WRITE_SECTORS)
	{
		in = 3;
		out = 0;
	}

	for (cnt=0; cnt < numSectors; cnt += _numSectors)
	{
		u32 i;
		void *ptr = (char *)buffer + (sectorSz * cnt);

		_sector     = sector + cnt;
		_numSectors = numSectors - cnt;

		/* Limit sector count */
		/* FIXME: assumes 512 byte sectors */
		if (_numSectors > USB_MAX_SECTORS)
			_numSectors = USB_MAX_SECTORS;

		/* update vector */
		vector[2].data = ptr;
		vector[2].len = _numSectors * sectorSz;

		for (i=0; i < in; i++)
			os_sync_after_write(vector[i].data, vector[i].len);
		os_sync_after_write(vector, sizeof(ioctlv)*3);

		/* Read/Write sectors */
		ret = os_ioctlv(fd, cmd, in, out, vector);
		if (ret<0)
			break;
	}

	if (ret>=0 && out)
		os_sync_before_read(buffer, numSectors*sectorSz);

#ifdef VISUALIZE
	gpio_set_off(GPIO_OSLOT);
#endif

	return ret>=0;
}

bool usbstorage_ReadSectors(u32 sector, u32 numSectors, void *buffer)
{
	return usbstorage_Transfer(USB_IOCTL_UMS_READ_SECTORS, sector, numSectors, buffer);
}

bool usbstorage_WriteSectors(u32 sector, u32 numSectors, void *buffer)
{
	return usbstorage_Transfer(USB_IOCTL_UMS_WRITE_SECTORS, sector, numSectors, buffer);
}

bool usbstorage_ClearStatus(void)
{
	return true;
}


const DISC_INTERFACE __io_usbstorage = {
	DEVICE_TYPE_WII_USB,
	FEATURE_MEDIUM_CANREAD | FEATURE_MEDIUM_CANWRITE | FEATURE_WII_USB,
	(FN_MEDIUM_STARTUP)&usbstorage_Init,
	(FN_MEDIUM_ISINSERTED)&usbstorage_IsInserted,
	(FN_MEDIUM_READSECTORS)&usbstorage_ReadSectors,
	(FN_MEDIUM_WRITESECTORS)&usbstorage_WriteSectors,
	(FN_MEDIUM_CLEARSTATUS)&usbstorage_ClearStatus,
	(FN_MEDIUM_SHUTDOWN)&usbstorage_Shutdown
};
