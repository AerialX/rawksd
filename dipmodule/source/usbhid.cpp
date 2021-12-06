#include "usbhid.h"
#include "logging.h"

// all values are big endian
typedef struct {
	u8 padding[16]; // anything you want can go here
	s32 device_no;
	union {
		struct {
			u8 bmRequestType;
			u8 bmRequest;
			u16 wValue;
			u16 wIndex;
			u16 wLength;
		} control;
		struct {
			u32 endpoint;
			u32 wLength;
		} interrupt;
		struct {
			u8 bIndex; // always uses US language ID (0x0409)
			// 7 unused bytes
		} string;
	};
	s32 data; // virtual, convert to physical before using
} usbhid_transfer; // 32 bytes

#define DRUM_ID 0x08

/* USB HID Ioctls:
0: GetDeviceChange, 0x600 out containing device descriptors, returns 0 or -1 on shutdown
1: SetSuspend, 8 bytes in (returns 0, unimplemented in IOS so just fallthrough)
2: Control message, 32 bytes in
3: Read Interrupt message, 32 bytes in
4: Write Interrupt message, 32 bytes in
5: Get US Language String, 32 bytes in, returns string length
6: Get version, 32 bytes out (unused) return 0x40001
7: Shutdown (force return from GetDeviceChange), returns 0 (crashes if no hook is set)
8: Cancel interrupt
*/

namespace ProxiIOS { namespace USB {
	HID::HID(u8 *stack, const int stacksize) : ProxyModule("/dev/usb/hi0", "/dev/usb/hid")
	{
		device_desc = (u8*)Memalign(32, 0x600);
		memset(device_desc+4, 0, 0x600-4);
		memset(device_desc, 0xFF, 4);
		device_changed = 0;
		ppc_device_notify = NULL;
		stack += stacksize;
		loop_thread = os_thread_create(hid_thread, this, stack, stacksize, 0x48, 0);
	}

	int HID::Start()
	{
		return os_thread_continue(loop_thread);
	}

	u32 HID::hid_thread(void* _p)
	{
		ProxiIOS::USB::HID *p = (ProxiIOS::USB::HID*)_p;

		// Hook Drum insertion
		ipcmessage *insert_msg = (ipcmessage*)Memalign(32, sizeof(ipcmessage));
		if (insert_msg) {
			memset(insert_msg, 0, sizeof(ipcmessage));
			insert_msg->ioctl.command = Drums_Insert;
			USB_DeviceInsertNotifyAsync("/dev/usb/oh0", 0x1BAD, 0x0003, p->queuehandle, insert_msg);
		}

		return (u32)p->Loop();
	}

	int HID::HandleOpen(ipcmessage *msg)
	{
		s32 ret = ProxyModule::HandleOpen(msg);

		LogPrintf("USB_HID OPENED\n");
		// Hook USB HID device change
		if (ret>=0) {
			ipcmessage *dev_change = (ipcmessage*)Memalign(32, sizeof(ipcmessage));
			if (dev_change) {
				memset(dev_change, 0, sizeof(ipcmessage));
				dev_change->ioctl.command = HID_Change;
				os_ioctl_async(ProxyHandle, 0, NULL, 0, device_desc, 0x600, queuehandle, dev_change);
			}
		}

		return ret;
	}

	s32 HID::PokeMessage(ipcmessage *msg)
	{
		s32 ret;
		ipcmessage *cb_msg = (ipcmessage*)Memalign(32, sizeof(ipcmessage));
		if (cb_msg==NULL)
			return IPC_ENOMEM;

		memset(cb_msg, 0, sizeof(ipcmessage));
		cb_msg->ioctl.command = Proxy_Async;
		cb_msg->ioctl.buffer_in = msg;
		cb_msg->ioctl.length_in = sizeof(ipcmessage);
		ret = os_ioctl_async(ProxyHandle, msg->ioctl.command, msg->ioctl.buffer_in, msg->ioctl.length_in, msg->ioctl.buffer_io, msg->ioctl.length_io, queuehandle, cb_msg);
		if (ret<0)
			Dealloc(cb_msg);
		return ret;
	}

	void HID::PPCDeviceChange()
	{
		int device_count=0;

		if (ppc_device_notify==NULL) {
			device_changed = 1;
			return;
		}

		memcpy(ppc_device_notify->ioctl.buffer_io, device_desc, ppc_device_notify->ioctl.length_io);
		u32 *end = (u32*)ppc_device_notify->ioctl.buffer_io;
		while (end[0] != 0xFFFFFFFF) {
			LogPrintf("Device %d: %08X\n", device_count++, end[4]);
			end += end[0]>>2;
		}
		Drummer.PutDescriptor(end);
		if (end[0] != 0xFFFFFFFF) {
			end[1] = DRUM_ID;
			device_count++;
		}

		LogPrintf("Acknowledging PPC device attach (%d devices)\n", device_count);

		os_sync_after_write(ppc_device_notify->ioctl.buffer_io, ppc_device_notify->ioctl.length_io);
		os_message_queue_ack(ppc_device_notify, 0);
		ppc_device_notify = NULL;
		device_changed = 0;
	}

	bool HID::HandleOther(u32 message, int &result, bool &ack)
	{
		ipcmessage *msg = (ipcmessage*)message;
		usbhid_transfer *xfr = (usbhid_transfer*)msg->ioctl.buffer_in;

		if (msg->command != Ios::Ioctl)
			return 0; // wtf is this, ditch it

		result = IPC_EINVAL;

		switch (msg->ioctl.command) {
			case 1: // SetSuspend. Use proxy function.
			case 4: // WriteInterrupt, not used/supported
			case 5: // GetString, not used/supported
			case 6: // GetVersion. Use proxy function.
			case 8: // CancelInterrupt, not used/supported
				LogPrintf("Unsupported IOCTL %d, passing through\n", msg->ioctl.command);
				return 0;
			case 0: // GetDeviceChange
				LogPrintf("PPC GetDevice Change\n");
				if (ppc_device_notify==NULL) {
					ppc_device_notify = msg;
					if (device_changed)
						PPCDeviceChange();
					else
						LogPrintf("Hook stored\n");
					ack = false;
				}
				break;
			case 2: // ControlMessage
				if (msg->ioctl.length_in == sizeof(usbhid_transfer)) {
					os_sync_before_read(xfr, sizeof(usbhid_transfer));
					LogPrintf("PPC Control message for device %d\n", xfr->device_no);
					LogPrintf("bmRequestType %02X, bmRequest %02X, wValue %04X, wIndex %04X, wLength %04X\n", xfr->control.bmRequestType, xfr->control.bmRequest, xfr->control.wValue, xfr->control.wIndex, xfr->control.wLength);
					if (xfr->control.wLength==1)
						LogPrintf("Data: %02X\n", *(u8*)(xfr->data&0x7FFFFFFF));
					else if (xfr->control.wLength==2)
						LogPrintf("Data: %04X\n", *(u16*)(xfr->data&0x7FFFFFFF));

					if (xfr->device_no != DRUM_ID) {
						result = PokeMessage(msg);
						if (result>=0)
							ack = false;
					} else {
						if (xfr->control.bmRequestType == USB_REQTYPE_SET) {
							if (xfr->control.bmRequest == USB_REQ_SETPROTOCOL) {
								result = Drummer.SetProtocol(xfr->control.wValue);
							} else if (xfr->control.bmRequest == USB_REQ_SETREPORT && xfr->control.wValue == (USB_REPTYPE_OUTPUT<<8)) {
								result = Drummer.SetOutput((u8*)(xfr->data&0x7FFFFFFF), xfr->control.wLength);
							}
						} else if (xfr->control.bmRequestType == USB_REQTYPE_GET && xfr->control.bmRequest == USB_REQ_GETREPORT && xfr->control.wValue==(USB_REPTYPE_FEATURE<<8)) {
							result = Drummer.GetFeature((u8*)(xfr->data&0x7FFFFFFF), xfr->control.wLength);
						}
					}
				}
				break;
			case 3: // ReadInterrupt
				if (msg->ioctl.length_in == sizeof(usbhid_transfer)) {
					os_sync_before_read(xfr, sizeof(usbhid_transfer));
					//LogPrintf("PPC Get Interrupt for device %d (size %u)\n", xfr->device_no, xfr->interrupt.wLength);
					if (xfr->device_no == DRUM_ID) {
						result = Drummer.GetReport((u8*)(xfr->data&0x7FFFFFFF), xfr->interrupt.wLength);
						LogPrintf("GetDrummer report returned %d\n", result);
					} else {
						result = PokeMessage(msg);
						if (result>=0)
							ack = false;
					}
				}
				break;
			case 7: // ReleaseDeviceChange
				LogPrintf("PPC Release Attach\n");
				if (ppc_device_notify) {
					memset(ppc_device_notify->ioctl.buffer_io, 0xFF, 0x4);
					os_sync_after_write(ppc_device_notify->ioctl.buffer_io, 4);
					os_message_queue_ack(ppc_device_notify, -1);
					ppc_device_notify = NULL;
					return 0;
				}
				break;
			default: // wtf is this
				LogPrintf("Unknown IOCTL %d\n", msg->ioctl.command);
		}

		return 1;
	}

	int HID::HandleCallback(ipcmessage* message)
	{
		switch (message->ioctl.command) {
			case Proxy_Async:
				{
					ipcmessage *msg = (ipcmessage*)message->ioctl.buffer_in;
					//LogPrintf("USB_HID proxy response for %p: %d\n", message->ioctl.buffer_in, message->result);
					if (msg->ioctl.command == 3) {
#if 0
						usbhid_transfer *xfer = (usbhid_transfer*)msg->ioctl.buffer_in;
						u32 *udata = (u32*)(xfer->data&0x7FFFFFFF);
						u8 *bdata = (u8*)udata;
						if (message->result==15)
							LogPrintf("%08X %08X %08X %02X%02X%02X\n", udata[0], udata[1], udata[2], bdata[12], bdata[13], bdata[14]);
						if (message->result==16)
							LogPrintf("%08X %08X %08X %08X\n", udata[0], udata[1], udata[2], udata[3]);
#endif
					}

					os_message_queue_ack(msg, message->result);
				}
				break;
			case Drums_Insert:
				LogPrintf("XBOX 360 Drums inserted\n");
				{
					usb_device* d = Drummer.Connect();
					ipcmessage *remove_msg = (ipcmessage*)Memalign(32, sizeof(ipcmessage));
					if (remove_msg) {
						memset(remove_msg, 0, sizeof(ipcmessage));
						remove_msg->ioctl.command = Drums_Remove;
						USB_DeviceRemovalNotifyAsync(d, queuehandle, remove_msg);
					}
				}
				PPCDeviceChange();
				break;
			case Drums_Remove:
				LogPrintf("XBOX 360 Drums removed\n");
				{
					Drummer.Disconnect();
					ipcmessage *insert_msg = (ipcmessage*)Memalign(32, sizeof(ipcmessage));
					if (insert_msg) {
						memset(insert_msg, 0, sizeof(ipcmessage));
						insert_msg->ioctl.command = Drums_Insert;
						USB_DeviceInsertNotifyAsync("/dev/usb/oh0", 0x1BAD, 0x0003, queuehandle, insert_msg);
					}
				}
				PPCDeviceChange();
				break;
			case HID_Change:
				LogPrintf("USB_HID device change returned %d\n", message->result);
				{
					// pretend keyboard is a guitar (needs key remapping)
					//if (((u32*)device_desc)[4] == 0x1BAD3330)
					//	((u32*)device_desc)[4] = 0x1BAD0004;
					PPCDeviceChange();
					ipcmessage *hook_msg = (ipcmessage*)Memalign(32, sizeof(ipcmessage));
					if (hook_msg) {
						memset(hook_msg, 0, sizeof(ipcmessage));
						hook_msg->ioctl.command = HID_Change;
						os_ioctl_async(ProxyHandle, 0, NULL, 0, device_desc, 0x600, queuehandle, hook_msg);
					}
				}
				break;
			default:
				LogPrintf("Bad callback notify: %d\n", message->ioctl.command);
		}

		Dealloc(message);
		return 0;
	}

	XBOX360_Drums::XBOX360_Drums()
	{
		usb = NULL;
	}

	usb_device* XBOX360_Drums::Connect()
	{
		static const u8 InitOutput[8] = {0x01, 0x08, 0xFF};
		usb = USB_OpenDevice("oh0", 0x1BAD, 0x0003);
		SetOutput(InitOutput, sizeof(InitOutput));
		return usb;
	}

	void XBOX360_Drums::Disconnect()
	{
		if (usb) {
			USB_CloseDevice(usb);
			usb = NULL;
		}
	}

	void XBOX360_Drums::PutDescriptor(u32 *buffer)
	{
		if (usb==NULL) {
			buffer[0] = 0xFFFFFFFF;
			return;
		}

		LogPrintf("Faking USB device descriptor for Wii drums\n");

		// this is basically a copy of the RB3 keyboard descriptor
		buffer[0x00] = 0x44; // size of the entire descriptor
		// buffer[0x01] = device number, assigned by parent
		// DEVICE DESCRIPTOR
		buffer[0x02] = 0x12010110; // bLength=18, bDescriptorType=01, bcdUSB=1.10
		buffer[0x03] = 0x00000008; // bDeviceClass=0, bDeviceSubClass=0, bDeviceProtocol=0, bMaxPacketSize=8
		buffer[0x04] = 0x1BAD0005; // VID=0x1BAD (Harmonix), PID=0x0005 (Wii RB1 Drums)
		buffer[0x05] = 0x00050000; // bcdDevice=5, iManufacturer=0, iProduct=0
		buffer[0x06] = 0x00010000; // iSerialNumber=0, bNumConfigurations=1
		// CONFIGURATION DESCRIPTOR
		buffer[0x07] = 0x09020020; // bLength=9, bDescriptorType=2, wTotalLength=32 (no HID descriptor)
		buffer[0x08] = 0x01010080; // bNumInterfaces=1, bConfigurationValue=1, iConfiguration=0, bmAttributes=0x80
		buffer[0x09] = 0x32000000; // bMaxPower=100ma (?)
		// INTERFACE DESCRIPTOR
		buffer[0x0A] = 0x09040000; // bLength=9, bDescriptorType=4, bInterfaceNumber=0, bAlternateSetting=0
		buffer[0x0B] = 0x02030000; // bNumEndpoints=2, bInterfaceClass=3(HID), bInterfaceSubClass=0, bInterfaceProtocol=0
		buffer[0x0C] = 0x00000000; // iInterface
		// ENDPOINT DESCRIPTOR (OUT)
		buffer[0x0D] = 0x07050203; // bLength=7, bDescriptorType=5, bEndpointAddress=0x02, bmAttributes=0x03(Interrupt)
		buffer[0x0E] = 0x00400100; // wMaxPacketSize=64, bInterval=1
		// ENDPOINT DESCRIPTOR (IN)
		buffer[0x0F] = 0x07058103; // bLength=7, bDescriptorType=5, bEndpointAddress=0x81, bmAttributes=0x03(Interrupt)
		buffer[0x10] = 0x00400A00; // wMaxPacketSize=64, bInterval=10
		// total size = 0x11*sizeof(u32) = 0x44
		buffer[0x11] = 0xFFFFFFFF;
	}

	s32 XBOX360_Drums::GetReport(u8 *buffer, u32 size)
	{
		STACK_ALIGN(u8, report, 32, 32);
		s32 ret;

		if (usb==NULL)
			return IPC_EINVAL;

		if (size<20)
			return -7008; // buffer not large enough

		// GetReport id 0
		ret = USB_ReadCtrlMsg(usb, USB_REQTYPE_GET, USB_REQ_GETREPORT, (USB_REPTYPE_INPUT<<8)|0, 0, 20, report, 0);
		if (ret >= 8) {
			ret -= 8;
			if (ret == 20 && report[0] == 0 && report[1] == 20) { // translate report 0 only
				// translate values
				report[0] =  ((report[3]&0x10)>>3);// Green 0x02
				report[0] |= ((report[3]&0x20)>>3);// Red 0x04
				report[0] |= ((report[3]&0x80)>>4);// Yellow 0x08
				report[0] |= ((report[3]&0x40)>>6);// Blue 0x01
				report[0] |= ((report[3]&0x01)<<4);// Orange 0x10
				report[1] =  ((report[2]&0x10)>>3);// start/plus 0x02
				report[1] |= ((report[2]&0x20)>>5);// back/minus 0x01
				report[1] |= ((report[3]&0x04)<<2);// home 0x10
				switch (report[2]&0x0F) {
					case 1: // up
						report[2] = 0;
						break;
					case 9: // up+right
						report[2] = 1;
						break;
					case 8: // right
						report[2] = 2;
						break;
					case 10: // right+down
						report[2] = 3;
						break;
					case 2: // down
						report[2] = 4;
						break;
					case 6: // down+left
						report[2] = 5;
						break;
					case 4: // left
						report[2] = 6;
						break;
					case 5: // left+up
						report[2] = 7;
						break;
					default: // invalid combo
						report[2] = 8;
				}
			} else {
				report[0] = report[1] = 0;
				report[2] = 8;
			}

			// what are buffer[5] and buffer[6] for drums?
			report[3] = 0x80;
			report[4] = 0x80;
			memset(report+5, 0, 32-5);
			memcpy(buffer, report, ret);
			os_sync_after_write(buffer, ret);
			return ret;
		}

		return IPC_EINVAL;
	}

	s32 XBOX360_Drums::GetFeature(u8 *buffer, u32 size)
	{
		if (usb==NULL)
			return -4;

		LogPrintf("Drums->GetFeature\n");

		if (size>=8) {
			// values copied from keyboard. What does this mean?
			buffer[0] = 0x21;
			buffer[1] = 0x26;
			buffer[2] = 0x01;
			buffer[3] = 0x06;
			buffer[4] = buffer[5] = buffer[6] = buffer[7] = 0;
			os_sync_after_write(buffer, 8);
			return 16;
		}

		// not enough room
		return -7008;
	}

	s32 XBOX360_Drums::SetOutput(const u8 *buffer, u32 size)
	{
		STACK_ALIGN(u8, report, 3, 32);
		s32 ret = -1;

		if (usb==NULL || size != 8 || buffer[0] != 1)
			return -4;

		LogPrintf("Drums->SetOutput\n");

		os_sync_before_read(buffer, size);

		report[0] = 1; // Output report 1;
		report[1] = 3; // length 3
		switch (buffer[2]) {
			case 0:
				report[2] = 0;
				break;
			case 1:
				report[2] = 2;
				break;
			case 2:
				report[2] = 3;
				break;
			case 4:
				report[2] = 4;
				break;
			case 8:
				report[2] = 5;
				break;
			default:
				report[2] = 13;
		}
		ret = USB_WriteCtrlMsg(usb, USB_REQTYPE_SET, USB_REQ_SETREPORT, USB_REPTYPE_OUTPUT<<8, 0, 3, report, 0);
		if (ret>0)
			ret = 16;

		return ret;
	}

	s32 XBOX360_Drums::SetProtocol(u16 protocol)
	{
		LogPrintf("Drums->SetProtocol\n");
		// screw protocol, we're not a boot device
		if (usb)
			return 8;
		return -4;
	}

} }