#include "dip.h"

namespace ProxiIOS { namespace DIP {
	int DIP::ForwardIoctl(ipcmessage* message)
	{
		if (Yarr)
			return yarr_HandleIoctl(message);
		return ProxyModule::ForwardIoctl(message);
	}

	int DIP::ForwardIoctlv(ipcmessage* message)
	{
		if (Yarr)
			return yarr_HandleIoctlv(message);
		return ProxyModule::ForwardIoctlv(message);
	}

	int DIP::yarr_HandleIoctl(ipcmessage* message)
	{
#ifdef YARR
		switch (message->ioctl.command) {
			case Ioctl::Inquiry:
				break;
			case Ioctl::ReadDiskID:
				return yarr_UnencryptedRead(0, message->ioctl.buffer_io, 0x20);
			case Ioctl::Read:
				os_sync_before_read(message->ioctl.buffer_in, message->ioctl.length_in);
				return yarr_Read((u64)message->ioctl.buffer_in[2] << 2, message->ioctl.buffer_io, message->ioctl.buffer_in[1]);
			case Ioctl::WaitCoverClose:
				switch (Yarr_Mode) {
					case YarrMode::File:
						return 1;
					case YarrMode::DVD:
						return ProxyModule::ForwardIoctl(message);
				}
			case Ioctl::ResetNotify:
				break;
			case Ioctl::GetCover:
				switch (Yarr_Mode) {
					case YarrMode::File:
						return 1;
					case YarrMode::DVD:
						return ProxyModule::ForwardIoctl(message);
				}
				break;
			case Ioctl::Reset:
				switch (Yarr_Mode) {
					case YarrMode::File:
						return 1;
					case YarrMode::DVD:
						return ProxyModule::ForwardIoctl(message);
				}
				break;
			case Ioctl::ClosePartition:
				if (YarrPartitionStream != NULL)
					delete YarrPartitionStream;
				YarrPartitionStream = NULL;
				break;
			case Ioctl::UnencryptedRead:
				os_sync_before_read(message->ioctl.buffer_in, message->ioctl.length_in);
				return yarr_UnencryptedRead((u64)message->ioctl.buffer_in[2] << 2, message->ioctl.buffer_io, message->ioctl.buffer_in[1]);
			case Ioctl::Seek:
				break;
			case Ioctl::EnableDVD:
				return ProxyModule::ForwardIoctl(message);
				break;
			case Ioctl::ReadDVD:
				return ProxyModule::ForwardIoctl(message);
				break;
			case Ioctl::StopLaser:
				break;
			case Ioctl::Offset:
				break;
			case Ioctl::VerifyCover:
				switch (Yarr_Mode) {
					case YarrMode::File:
						message->ioctl.buffer_io[0] = false;
						os_sync_after_write(message->ioctl.buffer_io, message->ioctl.length_io);
						return 1;
					case YarrMode::DVD:
						return ProxyModule::ForwardIoctl(message);
				}
			case Ioctl::RequestError:
				break;
			case Ioctl::StopMotor:
				return 1;
			case Ioctl::AudioStreaming:
				break;
		}
#endif
		return -1;
	}

	int DIP::yarr_HandleIoctlv(ipcmessage* message)
	{
#ifdef YARR
		switch (message->ioctlv.command) {
			case Ioctl::OpenPartition:
				os_sync_before_read(message->ioctlv.vector, message->ioctlv.num_in * sizeof(ioctlv));
				return yarr_OpenPartition((u64)((u32*)message->ioctlv.vector[0].data)[1] << 2, // Partition offset
					message->ioctlv.vector[1].data, message->ioctlv.vector[1].len, // Ticket
					message->ioctlv.vector[2].data, message->ioctlv.vector[2].len, // Cert
					message->ioctlv.vector[3].data, message->ioctlv.vector[3].len, // Out
					message->ioctlv.vector[4].data, message->ioctlv.vector[4].len // Error buffer
					);
				break;
		}
#endif
		return -1;
	}

	void DIP::yarr_Enable(bool enable, int mode)
	{
		Yarr = enable;
		Yarr_Mode = mode;
#ifdef YARR
		if (YarrStream != NULL)
			delete YarrStream;
		
		switch (Yarr_Mode) {
			case YarrMode::File:
				YarrStream = new MultifileStream();
				break;
			case YarrMode::DVD:
				YarrStream = new DvdStream(ProxyHandle);
				break;
		}
#endif
	}
	
	int DIP::yarr_AddIso(char* path)
	{
#ifdef YARR
		if (Yarr_Mode != YarrMode::File)
			return -1;
		
		Stats st;
		if (File_Stat(path, &st) != 0)
			return -1;
		s32 fid = File_Open(path, O_RDONLY);
		((MultifileStream*)YarrStream)->AddStream(new FileStream(fid), st.Size);
#endif
		return 1;
	}

	int DIP::yarr_UnencryptedRead(u64 pos, void* data, u32 len)
	{
#ifdef YARR
		YarrStream->Seek(pos);
		YarrStream->Read((u8*)data, len);
		os_sync_after_write(data, len);
#endif
		return 1;
	}
	int DIP::yarr_Read(u64 pos, void* data, u32 len)
	{
#ifdef YARR
		if (CurrentPartition > 0) {
			YarrPartitionStream->Seek(pos);
			YarrPartitionStream->Read((u8*)data, len);
			os_sync_after_write(data, len);
			return 1;
		}
#endif
		return -1;
	}
	int DIP::yarr_OpenPartition(u64 offset, void* ticket, u32 ticketlen, void* cert, u32 certlen, void* out, u32 outlen, void* errorbuffer, u32 errorlen)
	{
#ifdef YARR
		YarrPartitionStream = new PartitionStream(YarrStream, offset);
		
		if (out) {
			YarrStream->Seek(offset + 0x2C0);
			YarrStream->Read((u8*)out, outlen);
			os_sync_after_write(out, outlen);
		}
		
		if (errorbuffer) {
			memset(errorbuffer, 0, errorlen);
			os_sync_after_write(errorbuffer, errorlen);
		}
#endif
		return 1;
	}
} }
