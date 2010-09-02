#pragma once

#include "gctypes.h"
#include "disc_io.h"
#include "usb.h"

#ifdef __cplusplus
extern "C" {
#endif

#define	USBSTORAGE_OK				0
#define	USBSTORAGE_ENOINTERFACE		-10000
#define	USBSTORAGE_ESENSE			-10001
#define	USBSTORAGE_ESHORTWRITE		-10002
#define	USBSTORAGE_ESHORTREAD		-10003
#define	USBSTORAGE_ESIGNATURE		-10004
#define	USBSTORAGE_ETAG				-10005
#define	USBSTORAGE_ESTATUS			-10006
#define	USBSTORAGE_EDATARESIDUE		-10007
#define	USBSTORAGE_ETIMEDOUT		-10008
#define	USBSTORAGE_EINIT			-10009
#define USBSTORAGE_PROCESSING		-10010

typedef struct
{
	u8 configuration;
	u32 interface;
	u32 altInterface;

	u8 ep_in;
	u8 ep_out;

	u8 max_lun;
	u32 *sector_size;

	usb_device *usbdev;

	u32 tag;
	u8 suspended;

	u8 *buffer;
} usbstorage_handle;

s32 USBStorage_Open(usbstorage_handle *dev, const char *bus, u16 vid, u16 pid);
s32 USBStorage_Close(usbstorage_handle *dev);
s32 USBStorage_Reset(usbstorage_handle *dev);

s32 USBStorage_GetMaxLUN(usbstorage_handle *dev);
s32 USBStorage_MountLUN(usbstorage_handle *dev, u8 lun);
s32 USBStorage_Suspend(usbstorage_handle *dev);

s32 USBStorage_ReadCapacity(usbstorage_handle *dev, u8 lun, u32 *sector_size, u32 *n_sectors);
s32 USBStorage_Read(usbstorage_handle *dev, u8 lun, u32 sector, u16 n_sectors, u8 *buffer);
s32 USBStorage_Write(usbstorage_handle *dev, u8 lun, u32 sector, u16 n_sectors, const u8 *buffer);
s32 USBStorage_StartStop(usbstorage_handle *dev, u8 lun, u8 lo_ej, u8 start, u8 imm);

extern DISC_INTERFACE __io_usbstorage;

#ifdef __cplusplus
   }
#endif
