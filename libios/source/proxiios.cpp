#include "proxiios.h"

#include <string.h>

namespace ProxiIOS {
	Module::Module(const char* device)
	{
		strncpy(Device, device, 0x20);
		Device[0x20 - 1] = '\0';
		Fd = -1;

		os_thread_set_priority(0, 10);

		queuehandle = os_message_queue_create(queue, 8);

		os_device_register(Device, queuehandle);
	}

	int Module::Loop()
	{
		while (true) {
			ipcmessage* message;
			int result = 1;
			bool acknowledge = true;

			os_message_queue_receive(queuehandle, (u32*)&message, 0);

			if (!HandleOther((u32)message, result, acknowledge)) {
				switch (message->command) {
					case Ios::Open:
						result = HandleOpen(message);
						break;
					case Ios::Close:
						result = HandleClose(message);
						break;
					case Ios::Read:
						result = HandleRead(message);
						break;
					case Ios::Write:
						result = HandleWrite(message);
						break;
					case Ios::Seek:
						result = HandleSeek(message);
						break;
					case Ios::Ioctl:
						result = HandleIoctl(message);
						break;
					case Ios::Ioctlv:
						result = HandleIoctlv(message);
						break;
					case Ios::Callback:
						result = HandleCallback(message);
						break;
					default:
						result = -1; // wtf kind of IPC message is this?
						break;
				}
			}

			// Respond
			if (acknowledge)
				os_message_queue_ack(message, result);
		}

		return 0;
	}

	int Module::HandleOpen(ipcmessage* message)
	{
		if (strcmp(message->open.device, Device))
			return Errors::OpenFailure;
		Fd = 1;
		return Fd;
	}

	int ProxyModule::HandleOpen(ipcmessage* message)
	{
		int ret = Module::HandleOpen(message);
		if (ret == Errors::OpenFailure)
			return ret;

		ProxyHandle = os_open((char*)ProxyDevice, 0);
		if (ProxyHandle < 0)
			return Errors::OpenProxyFailure;
		else
			return ret;
	}

	int ProxyModule::HandleClose(ipcmessage* message)
	{
		os_close(ProxyHandle);
		return Module::HandleClose(message);
	}

	int ProxyModule::ForwardIoctl(ipcmessage* message)
	{
		return os_ioctl(ProxyHandle, message->ioctl.command, message->ioctl.buffer_in,
						message->ioctl.length_in, message->ioctl.buffer_io, message->ioctl.length_io);
	}

	int ProxyModule::ForwardIoctlv(ipcmessage* message)
	{
		return os_ioctlv(ProxyHandle, message->ioctlv.command, message->ioctlv.num_in,
						message->ioctlv.num_io, message->ioctlv.vector);
	}

	int ProxyModule::ForwardRead(ipcmessage* message)
	{
		return os_read(ProxyHandle, message->read.data, message->read.length);
	}

	int ProxyModule::ForwardWrite(ipcmessage* message)
	{
		return os_write(ProxyHandle, message->write.data, message->write.length);
	}

	int ProxyModule::ForwardSeek(ipcmessage* message)
	{
		return os_seek(ProxyHandle, message->seek.offset, message->seek.origin);
	}

	int ProxyModule::ForwardOpen(ipcmessage* message)
	{
		return os_open(message->open.device, message->open.mode);
	}

}
