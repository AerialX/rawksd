#pragma once

#include "gctypes.h"
#include "ipc.h"

#define USB_MAXPATH						IPC_MAXPATH_LEN

#define USB_OK							0
#define USB_FAILED						1

#define USB_CLASS_HID					0x03
#define USB_SUBCLASS_BOOT				0x01
#define USB_PROTOCOL_KEYBOARD			0x01
#define USB_PROTOCOL_MOUSE				0x02
#define USB_REPTYPE_INPUT				0x01
#define USB_REPTYPE_OUTPUT				0x02
#define USB_REPTYPE_FEATURE				0x03
#define USB_REQTYPE_GET					0xA1
#define USB_REQTYPE_SET					0x21

/* Descriptor types */
#define USB_DT_DEVICE					0x01
#define USB_DT_CONFIG					0x02
#define USB_DT_STRING					0x03
#define USB_DT_INTERFACE				0x04
#define USB_DT_ENDPOINT					0x05
#define USB_DT_HID						0x21
#define USB_DT_REPORT					0x22

/* Standard requests */
#define USB_REQ_GETSTATUS				0x00
#define USB_REQ_CLEARFEATURE			0x01
#define USB_REQ_SETFEATURE				0x03
#define USB_REQ_SETADDRESS				0x05
#define USB_REQ_GETDESCRIPTOR			0x06
#define USB_REQ_SETDESCRIPTOR			0x07
#define USB_REQ_GETCONFIG				0x08
#define USB_REQ_SETCONFIG				0x09
#define USB_REQ_GETINTERFACE			0x0a
#define USB_REQ_SETINTERFACE			0x0b
#define USB_REQ_SYNCFRAME				0x0c

#define USB_REQ_GETPROTOCOL				0x03
#define USB_REQ_SETPROTOCOL				0x0B
#define USB_REQ_GETREPORT				0x01
#define USB_REQ_SETREPORT				0x09

/* Descriptor sizes per descriptor type */
#define USB_DT_DEVICE_SIZE				18
#define USB_DT_CONFIG_SIZE				9
#define USB_DT_INTERFACE_SIZE			9
#define USB_DT_ENDPOINT_SIZE			7
#define USB_DT_ENDPOINT_AUDIO_SIZE		9	/* Audio extension */
#define USB_DT_HID_SIZE					9
#define USB_DT_HUB_NONVAR_SIZE			7

/* control message request type bitmask */
#define USB_CTRLTYPE_DIR_HOST2DEVICE	(0<<7)
#define USB_CTRLTYPE_DIR_DEVICE2HOST	(1<<7)
#define USB_CTRLTYPE_TYPE_STANDARD		(0<<5)
#define USB_CTRLTYPE_TYPE_CLASS			(1<<5)
#define USB_CTRLTYPE_TYPE_VENDOR		(2<<5)
#define USB_CTRLTYPE_TYPE_RESERVED		(3<<5)
#define USB_CTRLTYPE_REC_DEVICE			0
#define USB_CTRLTYPE_REC_INTERFACE		1
#define USB_CTRLTYPE_REC_ENDPOINT		2
#define USB_CTRLTYPE_REC_OTHER			3

#define USB_FEATURE_ENDPOINT_HALT		0

#define USB_ENDPOINT_INTERRUPT			0x03
#define USB_ENDPOINT_IN					0x80
#define USB_ENDPOINT_OUT				0x00

#define	USB_ETIMEDOUT					-10008


#ifdef __cplusplus
   extern "C" {
#endif /* __cplusplus */

typedef struct _usbendpointdesc
{
	u8 bLength;
	u8 bDescriptorType;
	u8 bEndpointAddress;
	u8 bmAttributes;
	u16 wMaxPacketSize;
	u8 bInterval;
} ATTRIBUTE_PACKED usb_endpointdesc;

typedef struct _usbinterfacedesc
{
	u8 bLength;
	u8 bDescriptorType;
	u8 bInterfaceNumber;
	u8 bAlternateSetting;
	u8 bNumEndpoints;
	u8 bInterfaceClass;
	u8 bInterfaceSubClass;
	u8 bInterfaceProtocol;
	u8 iInterface;
	u8 *extra;
	u8 extra_size;
	struct _usbendpointdesc *endpoints;
} ATTRIBUTE_PACKED usb_interfacedesc;

typedef struct _usbconfdesc
{
	u8 bLength;
	u8 bDescriptorType;
	u16 wTotalLength;
	u8 bNumInterfaces;
	u8 bConfigurationValue;
	u8 iConfiguration;
	u8 bmAttributes;
	u8 bMaxPower;
	struct _usbinterfacedesc *interfaces;
} ATTRIBUTE_PACKED usb_configurationdesc;

typedef struct _usbdevdesc
{
	u8  bLength;
	u8  bDescriptorType;
	u16 bcdUSB;
	u8  bDeviceClass;
	u8  bDeviceSubClass;
	u8  bDeviceProtocol;
	u8  bMaxPacketSize0;
	u16 idVendor;
	u16 idProduct;
	u16 bcdDevice;
	u8  iManufacturer;
	u8  iProduct;
	u8  iSerialNumber;
	u8  bNumConfigurations;
	struct _usbconfdesc *configurations;
} ATTRIBUTE_PACKED usb_devdesc;

typedef struct _usbhiddesc
{
	u8 bLength;
	u8 bDescriptorType;
	u16 bcdHID;
	u8 bCountryCode;
	u8 bNumDescriptors;
	struct {
		u8 bDescriptorType;
		u16 wDescriptorLength;
	} descr[1];
} ATTRIBUTE_PACKED usb_hiddesc;

// typedef struct _usbdevice usbdevice;

typedef struct _usbdevice
{
	s32 fd;
	u32 messages[4];
	osqueue_t timeout_queue;
	ostimer_t timeout_alarm;
	ipcmessage cbmessage;
	ioctlv *vec;
	// control
	u8 *pRqType;
	u8 *pRq;
	u8 *pNull;
	u16 *pLength;
	u16 *pValue;
	u16 *pIndex;
	// bulk
	u8 *pEndp;
} usb_device;

usb_device* USB_OpenDevice(const char *device,u16 vid,u16 pid);
s32 USB_CloseDevice(usb_device* dev);

s32 USB_GetDescriptors(usb_device *dev, usb_devdesc *udd);
void USB_FreeDescriptors(usb_devdesc *udd);

s32 USB_GetHIDDescriptor(usb_device *dev,usb_hiddesc *uhd);

s32 USB_GetDeviceDescription(usb_device *dev,usb_devdesc *devdesc);

void USB_SuspendDevice(usb_device *dev);
void USB_ResumeDevice(usb_device *dev);

s32 USB_ReadIntrMsg(usb_device *dev,u8 bEndpoint,u16 wLength,void *rpData,int timeout);
s32 USB_WriteIntrMsg(usb_device *dev,u8 bEndpoint,u16 wLength,void *rpData,int timeout);

s32 USB_ReadBlkMsg(usb_device *dev,u8 bEndpoint,u16 wLength,void *rpData,int timeout);
s32 USB_WriteBlkMsg(usb_device *dev,u8 bEndpoint,u16 wLength,void *rpData,int timeout);

s32 USB_ReadCtrlMsg(usb_device *dev,u8 bmRequestType,u8 bmRequest,u16 wValue,u16 wIndex,u16 wLength,void *rpData,int timeout);
s32 USB_WriteCtrlMsg(usb_device *dev,u8 bmRequestType,u8 bmRequest,u16 wValue,u16 wIndex,u16 wLength,void *rpData,int timeout);

s32 USB_GetConfiguration(usb_device *dev, u8 *configuration);
s32 USB_SetConfiguration(usb_device *dev, u8 configuration);
s32 USB_SetAlternativeInterface(usb_device *dev, u8 interface, u8 alternateSetting);
s32 USB_ClearHalt(usb_device *dev, u8 endpointAddress);
s32 USB_GetDeviceList(const char *devpath,void *descr_buffer,u8 num_descr,u8 b0,u8 *cnt_descr);

s32 USB_DeviceRemovalNotifyAsync(usb_device *dev, osqueue_t cb_queue, ipcmessage *cb_msg);
s32 USB_DeviceInsertNotifyAsync(const char *devpath,u16 vid,u16 pid,osqueue_t cb_queue, ipcmessage *cb_msg);

#ifdef __cplusplus
   }
#endif
