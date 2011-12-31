#pragma once

#include <proxiios.h>
#include <usb.h>

namespace ProxiIOS { namespace USB {

	class XBOX360_Drums
	{
	private:
		usb_device *usb;

	public:
		XBOX360_Drums();

		void PutDescriptor(u32 *buffer);

		usb_device* Connect();
		void Disconnect();

		s32 GetReport(u8 *buffer, u32 size);
		s32 GetFeature(u8 *buffer, u32 size);
		s32 SetOutput(const u8 *buffer, u32 size);
		s32 SetProtocol(u16 protocol);
	};

	class HID : public ProxiIOS::ProxyModule
	{
	private:
		int loop_thread;
		ipcmessage *ppc_device_notify;
		u8 *device_desc;
		int device_changed;

		enum CB_Ioctl {
			Proxy_Async = 0x20,
			Drums_Insert,
			Drums_Remove,
			HID_Change,
		};

		class XBOX360_Drums Drummer;

		s32 PokeMessage(ipcmessage *msg);
		void PPCDeviceChange();

	public:
		HID(u8 *stack, const int stacksize);

		int Start();

		bool HandleOther(u32 message, int &result, bool &ack);
		int HandleCallback(ipcmessage* message);
		int HandleOpen(ipcmessage* message);

		static u32 hid_thread(void*);
	};

} }