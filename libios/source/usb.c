/*-------------------------------------------------------------

usb.c -- USB lowlevel

Copyright (C) 2008
Michael Wiedenbauer (shagkur)
Dave Murphy (WinterMute)

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

#include <string.h>

#include "gcutil.h"
#include "ipc.h"
#include "syscalls.h"
#include "mem.h"
#include "gpio.h"

#include "usb.h"

#define debug_printf(fmt, args...)

#define USB_IOCTL_CTRLMSG				0
#define USB_IOCTL_BLKMSG				1
#define USB_IOCTL_INTRMSG				2
#define USB_IOCTL_SUSPENDDEV			5
#define USB_IOCTL_RESUMEDEV				6
#define USB_IOCTL_GETDEVLIST			12
#define USB_IOCTL_DEVREMOVALHOOK		26
#define USB_IOCTL_DEVINSERTHOOK			27

static s32 __usb_control_message(usb_device *dev,u8 bmRequestType,u8 bmRequest,u16 wValue,u16 wIndex,u16 wLength,void *rpData,int timeout)
{
	if((s32)rpData&0x1F)
	{
		gpio_set_on(GPIO_OSLOT);
		return IPC_EINVAL;
	}
	if(wLength && !rpData)	return IPC_EINVAL;
	if(!wLength && rpData)	return IPC_EINVAL;

	dev->pRqType[0] = bmRequestType;
	dev->pRq[0] = bmRequest;
	dev->pValue[0] = SWAP16(wValue);
	dev->pIndex[0] = SWAP16(wIndex);
	dev->pLength[0] = SWAP16(wLength);
	dev->pNull[0] = 0;

	dev->vec[0].data = dev->pRqType;
	dev->vec[0].len = sizeof(u8);
	dev->vec[1].data = dev->pRq;
	dev->vec[1].len = sizeof(u8);
	dev->vec[2].data = dev->pValue;
	dev->vec[2].len = sizeof(u16);
	dev->vec[3].data = dev->pIndex;
	dev->vec[3].len = sizeof(u16);
	dev->vec[4].data = dev->pLength;
	dev->vec[4].len = sizeof(u16);
	dev->vec[5].data = dev->pNull;
	dev->vec[5].len = sizeof(u8);
	dev->vec[6].data = rpData;
	dev->vec[6].len = wLength;
	// flush all parameters
	os_sync_after_write(dev->pRqType, 32*6);

	if (timeout>0)
	{
		u32 msg;
		s32 ret = os_ioctlv_async(dev->fd,USB_IOCTL_CTRLMSG,6,1,dev->vec,dev->timeout_queue,&dev->cbmessage);
		if (ret<0)
			return ret;

		ret = os_restart_timer(dev->timeout_alarm, timeout*1000000, 0);
		if (ret < 0)
			return ret;

		while ((ret = os_message_queue_receive(dev->timeout_queue, &msg, 0))==0)
		{
			if (msg==USB_ETIMEDOUT)
			{
				ret = USB_ETIMEDOUT;
				break;
			}
			if (&dev->cbmessage == (ipcmessage*)msg)
			{
				os_stop_timer(dev->timeout_alarm);
				ret = dev->cbmessage.result;
				os_message_queue_ack(&dev->cbmessage, 0);
				break;
			}
			// else... where did this message come from?
		}
		return ret;
	}

	return os_ioctlv(dev->fd,USB_IOCTL_CTRLMSG,6,1,dev->vec);
}

static s32 __usb_interrupt_bulk_message(usb_device *dev,u8 ioctl,u8 bEndpoint,u16 wLength,void *rpData,int timeout)
{
	if((s32)rpData&0x1F)
	{
		gpio_set_on(GPIO_OSLOT);
		return IPC_EINVAL;
	}
	if(wLength && !rpData)  return IPC_EINVAL;
	if(!wLength && rpData)  return IPC_EINVAL;

	dev->pEndp[0] = bEndpoint;
	dev->pLength[0] = wLength;

	dev->vec[0].data = dev->pEndp;
	dev->vec[0].len = sizeof(u8);
	dev->vec[1].data = dev->pLength;
	dev->vec[1].len = sizeof(u16);
	dev->vec[2].data = rpData;
	dev->vec[2].len = wLength;
	os_sync_after_write(dev->pEndp, 32*2);

	if (timeout>0)
	{
		u32 msg;
		s32 ret = os_ioctlv_async(dev->fd,ioctl,2,1,dev->vec,dev->timeout_queue,&dev->cbmessage);
		if (ret<0)
			return ret;

		ret = os_restart_timer(dev->timeout_alarm, timeout*1000000, 0);
		if (ret < 0)
			return ret;

		while ((ret = os_message_queue_receive(dev->timeout_queue, &msg, 0))==0)
		{
			if (msg==USB_ETIMEDOUT)
			{
				ret = USB_ETIMEDOUT;
				break;
			}
			if (&dev->cbmessage == (ipcmessage*)msg)
			{
				os_stop_timer(dev->timeout_alarm);
				ret = dev->cbmessage.result;
				os_message_queue_ack(&dev->cbmessage, 0);
				break;
			}
			// else... where did this message come from?
		}
		return ret;
	}

	return os_ioctlv(dev->fd,ioctl,2,1,dev->vec);
}

static inline s32 __usb_getdesc(usb_device *dev, u8 *buffer, u8 type, u8 index, u8 size)
{
	return __usb_control_message(dev, USB_ENDPOINT_IN ,USB_REQ_GETDESCRIPTOR, (type << 8) | index, 0, size, buffer, 0);
}

static u32 __find_next_endpoint(u8 *buffer,u32 size)
{
	u8 *ptr = buffer;

	while(size>0) {
		if(buffer[1]==USB_DT_ENDPOINT || buffer[1]==USB_DT_INTERFACE) break;

		size -= buffer[0];
		buffer += buffer[0];
	}

	return (buffer - ptr);
}

usb_device *USB_OpenDevice(const char *device,u16 vid,u16 pid)
{
	usb_device *dev = NULL;
	s32 fd;
	char *devicepath = NULL;
	int i;

	// I guess one day we might want to open something from oh1
	if (!device || (strcmp(device, "oh0") && strcmp(device, "oh1")))
		return NULL;

	devicepath = Alloc(USB_MAXPATH);
	if(devicepath==NULL) return NULL;

	// try to avoid linking with _sprintf
#if 0
	_sprintf(devicepath,"/dev/usb/%s/%x/%x",device,vid,pid);
#else
	strcpy(devicepath, "/dev/usb/");
	strcat(devicepath, device);

	for (i=0; i<2; i++)
	{
		int j;
		char *nextchar;
		u16 id = !i ? vid : pid;
		strcat(devicepath, "/");
		nextchar = devicepath + strlen(devicepath);
		// ignore leading zeroes
		for (j=3; j>0; j--)
		{
			u16 idshift = id >> (j*4);
			if (!idshift)
				continue;
			idshift &= 0x0F;
			*nextchar++ = (idshift>9) ? idshift+'a'-10 : idshift+'0';
		}
		id &= 0x0F;
		*nextchar++ = (id>9) ? id+'a'-10 : id+'0';
		*nextchar = '\0';
	}
#endif

	debug_printf("USB_OPEN: %s\n", devicepath);

	fd = os_open(devicepath,0);
	Dealloc(devicepath);
	if(fd<0)
		return NULL;

	if ((dev = Alloc(sizeof(usb_device))))
	{
		u8 *params = Memalign(32, 6*32);
		memset(dev, 0, sizeof(*dev));
		dev->fd = fd;
		if (params==NULL) goto open_error;
		dev->pRqType = params;
		dev->pRq = params+32;
		dev->pNull = dev->pEndp = params+64;
		dev->pLength = (u16*)(params+96);
		dev->pValue = (u16*)(params+128);
		dev->pIndex = (u16*)(params+160);

		dev->vec = Alloc(7*sizeof(ioctlv));
		if (dev->vec == NULL)
			goto open_error;

		dev->timeout_queue = os_message_queue_create(dev->messages, sizeof(dev->messages)/sizeof(u32));
		if (dev->timeout_queue<0)
			goto open_error;

		dev->timeout_alarm = os_create_timer(9999999, 0, dev->timeout_queue, USB_ETIMEDOUT);
		if (dev->timeout_alarm<0)
			goto open_error;
		os_stop_timer(dev->timeout_alarm);
	}

	return dev;
open_error:
	USB_CloseDevice(dev);
	return NULL;
}

s32 USB_CloseDevice(usb_device *dev)
{
	if (dev)
	{
		if (dev->fd >= 0)
			os_close(dev->fd);

		if (dev->timeout_alarm >=0)
			os_destroy_timer(dev->timeout_alarm);
		if (dev->timeout_queue >=0)
			os_message_queue_destroy(dev->timeout_queue);

		Dealloc(dev->vec);
		Dealloc(dev->pRqType);
		Dealloc(dev);
	}
	return IPC_OK;
}

s32 USB_GetDeviceDescription(usb_device *dev,usb_devdesc *devdesc)
{
	s32 ret;
	usb_devdesc *p;

	p = Memalign(32, USB_DT_DEVICE_SIZE);
	if(p==NULL) return IPC_ENOMEM;

	ret = __usb_control_message(dev,USB_ENDPOINT_IN,USB_REQ_GETDESCRIPTOR,(USB_DT_DEVICE<<8),0,USB_DT_DEVICE_SIZE,p,0);
	if(ret>=0) memcpy(devdesc,p,USB_DT_DEVICE_SIZE);
	devdesc->configurations = NULL;

	Dealloc(p);
	return ret;
}

s32 USB_GetDescriptors(usb_device *dev, usb_devdesc *udd)
{
	u8 *buffer = NULL;
	u8 *ptr = NULL;
	usb_configurationdesc *ucd = NULL;
	usb_interfacedesc *uid = NULL;
	usb_endpointdesc *ued = NULL;
	s32 retval = 0;
	u32 size,i;
	u32 iConf, iInterface, iEndpoint;

	buffer = Memalign(32, sizeof(*udd));
	if(buffer == NULL)
	{
		retval = IPC_ENOMEM;
		goto free_and_error;
	}

	retval = __usb_getdesc(dev, buffer, USB_DT_DEVICE, 0, USB_DT_DEVICE_SIZE);
	if(retval < 0)
		goto free_and_error;

	memcpy(udd, buffer, USB_DT_DEVICE_SIZE);
	Dealloc(buffer);

	udd->bcdUSB = SWAP16(udd->bcdUSB);
	udd->idVendor = SWAP16(udd->idVendor);
	udd->idProduct = SWAP16(udd->idProduct);
	udd->bcdDevice = SWAP16(udd->bcdDevice);

	udd->configurations = Memalign(32, udd->bNumConfigurations*sizeof(*udd->configurations));
	if(udd->configurations == NULL)
	{
		retval = IPC_ENOMEM;
		goto free_and_error;
	}
	memset(udd->configurations, 0, udd->bNumConfigurations*sizeof(*udd->configurations));

	for(iConf = 0; iConf < udd->bNumConfigurations; iConf++)
	{
		buffer = Memalign(32, USB_DT_CONFIG_SIZE);
		if(buffer == NULL)
		{
			retval = IPC_ENOHEAP;
			goto free_and_error;
		}

		retval = __usb_getdesc(dev, buffer, USB_DT_CONFIG, iConf, USB_DT_CONFIG_SIZE);
		ucd = &udd->configurations[iConf];
		memcpy(ucd, buffer, USB_DT_CONFIG_SIZE);
		Dealloc(buffer);

		ucd->wTotalLength = SWAP16(ucd->wTotalLength);
		size = ucd->wTotalLength;
		buffer = Memalign(32, size);
		if(buffer == NULL)
		{
			retval = IPC_ENOHEAP;
			goto free_and_error;
		}

		retval = __usb_getdesc(dev, buffer, USB_DT_CONFIG, iConf, ucd->wTotalLength);
		if(retval < 0)
			goto free_and_error;

		ptr = buffer;
		ptr += ucd->bLength;
		size -= ucd->bLength;

		retval = IPC_ENOMEM;
		ucd->interfaces = Memalign(32, ucd->bNumInterfaces*sizeof(*ucd->interfaces));
		if(ucd->interfaces == NULL)
			goto free_and_error;
		memset(ucd->interfaces, 0, ucd->bNumInterfaces*sizeof(*ucd->interfaces));

		for(iInterface = 0; iInterface < ucd->bNumInterfaces; iInterface++)
		{
			uid = &ucd->interfaces[iInterface];
			memcpy(uid, ptr, USB_DT_INTERFACE_SIZE);
			ptr += uid->bLength;
			size -= uid->bLength;

			uid->endpoints = Memalign(32, uid->bNumEndpoints*sizeof(*uid->endpoints));
			if(uid->endpoints == NULL)
				goto free_and_error;
			memset(uid->endpoints, 0, uid->bNumEndpoints*sizeof(*uid->endpoints));

			/* This skips vendor and class specific descriptors */
			i = __find_next_endpoint(ptr, size);
			uid->extra_size = i;
			if(i>0)
			{
				uid->extra = Alloc(i);
				if(uid->extra == NULL)
					goto free_and_error;

				memcpy(uid->extra, ptr, i);
				ptr += i;
				size -= i;
			}
			memset(uid->endpoints,0,uid->bNumEndpoints* sizeof(*uid->endpoints));
			for(iEndpoint = 0; iEndpoint < uid->bNumEndpoints; iEndpoint++)
			{
				ued = &uid->endpoints[iEndpoint];
				memcpy(ued, ptr, USB_DT_ENDPOINT_SIZE);
				ptr += ued->bLength;
				ued->wMaxPacketSize = SWAP16(ued->wMaxPacketSize);
			}
		}
		Dealloc(buffer);
		buffer = NULL;
	}
	retval = IPC_OK;

free_and_error:
	Dealloc(buffer);
	if(retval < 0)
		USB_FreeDescriptors(udd);
	return retval;
}

s32 USB_GetHIDDescriptor(usb_device *dev,usb_hiddesc *uhd)
{
	u8 *buffer = NULL;
	s32 retval = IPC_OK;

	buffer = Memalign(32, sizeof(usb_hiddesc));
	if(buffer==NULL) {
		retval = IPC_ENOMEM;
		goto free_and_error;
	}

	retval = __usb_getdesc(dev,buffer,USB_DT_HID,0,USB_DT_HID_SIZE);
	if(retval<0) goto free_and_error;

	memcpy(uhd,buffer,USB_DT_HID_SIZE);
	uhd->bcdHID = SWAP16(uhd->bcdHID);
	uhd->descr[0].wDescriptorLength = SWAP16(uhd->descr[0].wDescriptorLength);

	retval = IPC_OK;

free_and_error:
	Dealloc(buffer);
	return retval;
}

void USB_FreeDescriptors(usb_devdesc *udd)
{
	int iConf, iInterface;
	usb_configurationdesc *ucd;
	usb_interfacedesc *uid;
	if(udd->configurations != NULL)
	{
		for(iConf = 0; iConf < udd->bNumConfigurations; iConf++)
		{
			ucd = &udd->configurations[iConf];
			if(ucd->interfaces != NULL)
			{
				for(iInterface = 0; iInterface < ucd->bNumInterfaces; iInterface++)
				{
					uid = &ucd->interfaces[iInterface];
					Dealloc(uid->endpoints);
					Dealloc(uid->extra);
				}
				Dealloc(ucd->interfaces);
			}
		}
		Dealloc(udd->configurations);
	}
}

s32 USB_GetAsciiString(usb_device *dev,u16 wIndex,u16 wLangID,u16 wLength,void *rpData)
{
	s32 ret;
	u8 bo, ro;
	u8 *buf;
	u8 *rp = (u8 *)rpData;

	if(wLength > 255)
		wLength = 255;

	buf = Memalign(32, 255); /* 255 is the highest possible length of a descriptor */
	if(buf == NULL)
		return IPC_ENOMEM;

	ret = __usb_control_message(dev, USB_ENDPOINT_IN, USB_REQ_GETDESCRIPTOR, ((USB_DT_STRING << 8) + wIndex), wLangID, 255, buf, 0);

	/* index 0 gets a list of supported languages */
	if(wIndex == 0)
	{
		if(ret > 0)
			memcpy(rpData, buf, wLength);
		Dealloc(buf);
		return ret;
	}

	if(ret > 0)
	{
		bo = 2;
		ro = 0;
		// convert from unicode
		while(ro < (wLength - 1) && bo < buf[0])
		{
			if(buf[bo + 1])
				rp[ro++] = '?';
			else
				rp[ro++] = buf[bo];
			bo += 2;
		}
		rp[ro] = 0;
		ret = ro - 1;
	}

	Dealloc(buf);
 	return ret;
}

s32 USB_ReadIntrMsg(usb_device *dev,u8 bEndpoint,u16 wLength,void *rpData, int timeout)
{
	s32 ret = __usb_interrupt_bulk_message(dev,USB_IOCTL_INTRMSG,bEndpoint,wLength,rpData,timeout);
	if (ret >=0 && wLength>0 && rpData)
		os_sync_before_read(rpData, wLength);
	return ret;
}

s32 USB_WriteIntrMsg(usb_device *dev,u8 bEndpoint,u16 wLength,void *rpData,int timeout)
{
	if (rpData && wLength>0)
		os_sync_after_write(rpData, wLength);
	return __usb_interrupt_bulk_message(dev,USB_IOCTL_INTRMSG,bEndpoint,wLength,rpData,timeout);
}

s32 USB_ReadBlkMsg(usb_device *dev,u8 bEndpoint,u16 wLength,void *rpData,int timeout)
{
	s32 ret = __usb_interrupt_bulk_message(dev,USB_IOCTL_BLKMSG,bEndpoint,wLength,rpData,timeout);
	if (ret>=0 && rpData && wLength>0)
		os_sync_before_read(rpData, wLength);
	return ret;
}

s32 USB_WriteBlkMsg(usb_device *dev,u8 bEndpoint,u16 wLength,void *rpData,int timeout)
{
	s32 ret;
	if (rpData && wLength>0 && !(bEndpoint&0x80))
		os_sync_after_write(rpData, wLength);
	ret = __usb_interrupt_bulk_message(dev,USB_IOCTL_BLKMSG,bEndpoint,wLength,rpData,timeout);
	if (rpData && wLength>0 && (bEndpoint&0x80))
		os_sync_before_read(rpData, wLength);
	return ret;
}

s32 USB_ReadCtrlMsg(usb_device *dev,u8 bmRequestType,u8 bmRequest,u16 wValue,u16 wIndex,u16 wLength,void *rpData,int timeout)
{
	s32 ret = __usb_control_message(dev,bmRequestType,bmRequest,wValue,wIndex,wLength,rpData,timeout);
	if (ret>=0 && rpData && wLength>0)
		os_sync_before_read(rpData, wLength);
	return ret;
}

s32 USB_WriteCtrlMsg(usb_device *dev,u8 bmRequestType,u8 bmRequest,u16 wValue,u16 wIndex,u16 wLength,void *rpData,int timeout)
{
	if (rpData && wLength>0)
		os_sync_after_write(rpData, wLength);
	return __usb_control_message(dev,bmRequestType,bmRequest,wValue,wIndex,wLength,rpData,timeout);
}

s32 USB_DeviceRemovalNotifyAsync(usb_device *dev, osqueue_t cb_queue, ipcmessage *cb_msg)
{
	return os_ioctl_async(dev->fd,USB_IOCTL_DEVREMOVALHOOK,NULL,0,NULL,0,cb_queue,cb_msg);
}

s32 USB_DeviceInsertNotifyAsync(const char *devpath,u16 vid,u16 pid,osqueue_t cb_queue, ipcmessage *cb_msg)
{
	s32 ret;
	ioctlv vec[2];
	s32 fd = os_open(devpath, 0);
	if (fd<0)
		return IPC_ENOENT;

	vec[0].data = &vid;
	vec[1].data = &pid;
	vec[0].len = sizeof(u16);
	vec[1].len = sizeof(u16);

	ret = os_ioctlv_async(fd,USB_IOCTL_DEVINSERTHOOK,2,0,vec,cb_queue,cb_msg);

	os_close(fd);
	return ret;
}

void USB_SuspendDevice(usb_device *dev)
{
	os_ioctl(dev->fd,USB_IOCTL_SUSPENDDEV,NULL,0,NULL,0);
}

void USB_ResumeDevice(usb_device *dev)
{
	os_ioctl(dev->fd,USB_IOCTL_RESUMEDEV,NULL,0,NULL,0);
}

s32 USB_GetDeviceList(const char *devpath,void *descr_buffer,u8 num_descr,u8 b0,u8 *cnt_descr)
{
	s32 fd, ret=IPC_ENOMEM;
	ioctlv *vec=NULL;
	u8 cntdevs = 0;

	if(devpath==NULL || *devpath=='\0' || descr_buffer==NULL) return IPC_EINVAL;

	fd = os_open(devpath,IPC_OPEN_NONE);
	if(fd<0)
		return fd;

	vec = Memalign(32, sizeof(ioctlv)*4);
	if (vec==NULL) goto done;

	vec[0].data = &num_descr;
	vec[0].len = sizeof(u8);
	vec[1].data = &b0;
	vec[1].len = sizeof(u8);
	vec[2].data = &cntdevs;
	vec[2].len = sizeof(u8);

	vec[3].data = descr_buffer;
	vec[3].len = (num_descr<<3);

	ret = os_ioctlv(fd, USB_IOCTL_GETDEVLIST, 2, 2, vec);
	if (cnt_descr)
		*cnt_descr = cntdevs;

done:
	Dealloc(vec);

	if (fd>=0)
		os_close(fd);

	return ret;
}

s32 USB_SetConfiguration(usb_device *dev, u8 configuration)
{
	return __usb_control_message(dev, (USB_CTRLTYPE_DIR_HOST2DEVICE | USB_CTRLTYPE_TYPE_STANDARD | USB_CTRLTYPE_REC_DEVICE), USB_REQ_SETCONFIG, configuration, 0, 0, NULL, 0);
}

s32 USB_GetConfiguration(usb_device *dev, u8 *configuration)
{
	u8 *_configuration;
	s32 retval;

	_configuration = Memalign(32, 1);
	if(_configuration == NULL)
		return IPC_ENOMEM;

	retval = __usb_control_message(dev, (USB_CTRLTYPE_DIR_DEVICE2HOST | USB_CTRLTYPE_TYPE_STANDARD | USB_CTRLTYPE_REC_DEVICE), USB_REQ_GETCONFIG, 0, 0, 1, _configuration, 0);
	if(retval >= 0)
		*configuration = *_configuration;
	Dealloc(_configuration);

	return retval;
}

s32 USB_SetAlternativeInterface(usb_device *dev, u8 interface, u8 alternateSetting)
{
	if(alternateSetting == 0)
		return IPC_EINVAL;
	return __usb_control_message(dev, (USB_CTRLTYPE_DIR_HOST2DEVICE | USB_CTRLTYPE_TYPE_STANDARD | USB_CTRLTYPE_REC_INTERFACE), USB_REQ_SETINTERFACE, alternateSetting, interface, 0, NULL, 0);
}

s32 USB_ClearHalt(usb_device *dev, u8 endpoint)
{
	return __usb_control_message(dev, (USB_CTRLTYPE_DIR_HOST2DEVICE | USB_CTRLTYPE_TYPE_STANDARD | USB_CTRLTYPE_REC_ENDPOINT), USB_REQ_CLEARFEATURE, USB_FEATURE_ENDPOINT_HALT, endpoint, 0, NULL, 0);
}

