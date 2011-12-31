#include <string.h>

#include "gctypes.h"
#include "ipc.h"
#include "syscalls.h"
#include "mem.h"
#include "usb.h"
#include "ch341.h"
#include "gpio.h"

typedef struct ch341_private_t {
	usb_device *dev;
	u32 baud_rate;
	u8 *inbuffer;
	u8 *outbuffer[2];
	u8 out_flip;
	u8 dtr;
	u8 rts;
} ch341_private;

static ch341_private *ch341_device = NULL;

static int ch341_control_out(usb_device *dev, u8 request, u16 value, u16 index)
{
	return USB_WriteCtrlMsg(dev, USB_CTRLTYPE_TYPE_VENDOR|USB_CTRLTYPE_REC_DEVICE|USB_CTRLTYPE_DIR_HOST2DEVICE, request, value, index, 0, NULL,0);
}

static int ch341_control_in(usb_device *dev, u8 request, u16 value, u16 index, void *buf, u32 size)
{
	return USB_ReadCtrlMsg(dev, USB_CTRLTYPE_TYPE_VENDOR|USB_CTRLTYPE_REC_DEVICE|USB_CTRLTYPE_DIR_DEVICE2HOST, request, value, index, size, buf,0);
}

static int ch341_set_baudrate(ch341_private *priv)
{
	u16 a, b;
	int r;

	switch (priv->baud_rate)
	{
	case 2400:
		a = 0xd901;
		b = 0x0038;
		break;
	case 4800:
		a = 0x6402;
		b = 0x001f;
		break;
	case 9600:
		a = 0xb202;
		b = 0x0013;
		break;
	case 19200:
		a = 0xd902;
		b = 0x000d;
		break;
	case 38400:
		a = 0x6403;
		b = 0x000a;
		break;
	case 115200:
		a = 0xcc03;
		b = 0x0008;
		break;
	default:
		return IPC_EINVAL;
	}

	r = ch341_control_out(priv->dev, 0x9a, 0x1312, a);
	if (!r)
		r = ch341_control_out(priv->dev, 0x9a, 0x0f2c, b);

	return r;
}

static int ch341_set_handshake(ch341_private *priv)
{
	return ch341_control_out(priv->dev, 0x0a4, ~((priv->dtr ? (1<<5) : 0) | (priv->rts ? (1<<6) : 0)), 0);
}

static int ch341_get_status(ch341_private *priv)
{
	u8 *buffer;
	int r;
	const unsigned int size = 8;

	buffer = Memalign(32, size);
	if (!buffer)
		return IPC_ENOMEM;

	r = ch341_control_in(priv->dev, 0x95, 0x0706, 0, buffer, size);
	if (r >= 0)
		r = 0;

	Dealloc(buffer);
	return r;
}

static int ch341_configure(ch341_private *priv)
{
	u8 *buffer;
	int r;
	const unsigned int size = 8;

	buffer = Memalign(32, size);
	if (!buffer)
		return IPC_ENOMEM;

	r = ch341_control_in(priv->dev, 0x5f, 0, 0, buffer, size);
	if (r < 0)
		goto out;

	r = ch341_control_out(priv->dev, 0xa1, 0, 0);
	if (r < 0)
		goto out;

	r = ch341_set_baudrate(priv);
	if (r < 0)
		goto out;

	r = ch341_control_in(priv->dev, 0x95, 0x2518, 0, buffer, size);
	if (r < 0)
		goto out;

	r = ch341_control_out(priv->dev, 0x9a, 0x2518, 0x0050);
	if (r < 0)
		goto out;

	r = ch341_get_status(priv);
	if (r < 0)
		goto out;

	r = ch341_control_out(priv->dev, 0xa1, 0x501f, 0xd90a);
	if (r < 0)
		goto out;

	r = ch341_set_baudrate(priv);
	if (r < 0)
		goto out;

	r = ch341_set_handshake(priv);
	if (r < 0)
		goto out;

	r = ch341_get_status(priv);

out:
	Dealloc(buffer);
	return r;
}

int ch341_send(const void *_data, u32 size)
{
	ch341_private *priv = ch341_device;
	const u8* data = (const u8*)_data;
	unsigned int i=0;

	if (!priv || !data || !priv->dev || !priv->outbuffer[0])
		return IPC_EINVAL;

	while (size>i)
	{
		int j;
		u32 bytes_to_send = MIN(32, size-i);
		s32 bytes_sent=0;
		// change \n to \r, since most terminals treat \n as linefeed only
		for (j=0; j < bytes_to_send; j++)
		{
			//if (data[j]=='\n')
			//	*(priv->outbuffer[priv->out_flip]+j) = '\r';
			//else
				*(priv->outbuffer[priv->out_flip]+j) = data[j];
		}
		if (bytes_to_send)
		{
			os_sync_after_write(priv->outbuffer[priv->out_flip], bytes_to_send);
			bytes_sent = USB_WriteBlkMsg(priv->dev, 0x02, bytes_to_send, priv->outbuffer[priv->out_flip], 0);
		}
		else
			break;
		if (bytes_sent<0)
		{
			gpio_set_on(GPIO_OSLOT);
			USB_ClearHalt(priv->dev, 0x02);
			gpio_set_off(GPIO_OSLOT);
			return bytes_sent;
		}
		i += bytes_to_send;
		data += bytes_to_send;
		priv->out_flip ^= 1;
	}

	return i;
}

int ch341_open()
{
	static const char open_string[] = "CH341 device opened\n";

	if (ch341_device != NULL)
		return 0;

	ch341_device = Alloc(sizeof(*ch341_device));

	if (!ch341_device)
		return IPC_ENOMEM;
	memset(ch341_device, 0, sizeof(*ch341_device));

	ch341_device->baud_rate = 115200;
	ch341_device->dtr = 1;
	ch341_device->rts = 1;
	ch341_device->outbuffer[0] = Memalign(32, 64);
	ch341_device->outbuffer[1] = ch341_device->outbuffer[0]+32;

	if (!ch341_device->outbuffer[0])
		goto end;

	if ((ch341_device->dev = USB_OpenDevice("oh0", 0x4348, 0x5523))==NULL)
		goto end;

	if (ch341_configure(ch341_device)<0)
		goto end;

	if (ch341_set_handshake(ch341_device)<0)
		goto end;

	if (ch341_set_baudrate(ch341_device)<0)
		goto end;

	ch341_send((void*)open_string, strlen(open_string));

	return 0;

end:
	ch341_close();
	return IPC_ENOMEM;
}

void ch341_close()
{
	const char close_string[] = "CH341 device closed\n";
	ch341_send(close_string, strlen(close_string));

	if (!ch341_device)
		return;

	if (ch341_device->dev)
		USB_CloseDevice(ch341_device->dev);

	Dealloc(ch341_device->inbuffer);
	Dealloc(ch341_device->outbuffer);
	Dealloc(ch341_device);
	ch341_device = NULL;
}