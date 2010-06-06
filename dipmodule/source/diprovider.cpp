#include "diprovider.h"
#include "dip.h"

#ifdef YARR

/* ReadDVD
Dec 01 09:32:54 <tueidj>         inbuffer[0] = 0xD0 << 24;
Dec 01 09:32:55 <tueidj>         inbuffer[1] = 0;
Dec 01 09:32:55 <tueidj>         inbuffer[2] = 0;
Dec 01 09:32:55 <tueidj>         inbuffer[3] = len;
Dec 01 09:32:55 <tueidj>         inbuffer[4] = lba;
Dec 01 09:32:55 <tueidj>         result = IOS_Ioctl(di_fd, 0xD0, inbuffer, 0x20, outbuf, len<<11);
Dec 01 09:33:30 <Aaron> Right, how does the LBA work?
Dec 01 09:40:12 <tueidj>    don't remember
Dec 01 09:40:12 <tueidj>    I think it's byte offset >> 11
Dec 01 09:40:12 <tueidj>    and len is the number of 2048 byte sectors to read
*/
/* Offset
u32 offset = (inbuf[1] << 30) | inbuf[2];
// Set drive offset
config.offset[1] = (offset & -0x8000);
*/

namespace ProxiIOS { namespace DIP {
	DiProvider::DiProvider(DIP* module)
	{
		ErrorCode = 0;
		Module = module;
	}

	int DiProvider::HandleIoctl(ipcmessage* message)
	{
		Message = message;
		u32* buffer = (u32*)Message->ioctl.buffer_in;
		
		switch (Message->ioctl.command) {
			case Ioctl::Inquiry:
				return Inquiry(Message->ioctl.buffer_io);
			case Ioctl::ReadDiscID:
				return ReadDiscID(Message->ioctl.buffer_io);
			case Ioctl::Read:
				return Read(Message->ioctl.buffer_io, buffer[1], buffer[2]);
			case Ioctl::WaitCoverClose:
				return WaitCoverClose();
			case Ioctl::CoverRegister:
				return CoverRegister();
			case Ioctl::ResetNotify:
				return ResetNotify();
			case Ioctl::CoverClear:
				return CoverClear();
			case Ioctl::GetCover:
				return GetCover();
			case Ioctl::Reset:
				return Reset(buffer[1]);
			case Ioctl::ClosePartition:
				return ClosePartition();
			case Ioctl::UnencryptedRead:
				return UnencryptedRead(Message->ioctl.buffer_io, buffer[1], buffer[2]);
			case Ioctl::EnableDVD:
				return EnableDVD(buffer[1]);
			case Ioctl::StatusRegister:
				return StatusRegister();
			case Ioctl::ReportKey:
				return ReportKey();
			case Ioctl::Seek:
				return Seek();
			case Ioctl::ReadDVD:
				return ReadDVD(buffer[1], buffer[2], buffer[3], buffer[4]);
			case Ioctl::StopLaser:
				return StopLaser();
			case Ioctl::Offset:
				return Offset(buffer[1], buffer[2]);
			case Ioctl::ReadBCA:
				return ReadBCA(Message->ioctl.buffer_io, Message->ioctl.length_io);
			case Ioctl::VerifyCover:
				return VerifyCover(Message->ioctl.buffer_io);
			case Ioctl::SetMaximumRotation:
				return SetMaximumRotation(buffer[1]);
			case Ioctl::RequestError:
				return RequestError(Message->ioctl.buffer_io);
			case Ioctl::StopMotor:
				return StopMotor(buffer[1], buffer[2]);
			case Ioctl::EnableAudio:
				return EnableAudio(buffer[0]);
		}
		
		return ForwardIoctl();
	}
	
	int DiProvider::HandleIoctlv(ipcmessage* message)
	{
		Message = message;
		
		switch (Message->ioctlv.command) {
			case Ioctl::OpenPartition:
				return OpenPartition(((u32*)Message->ioctlv.vector[0].data)[1], Message->ioctlv.vector[1].data, Message->ioctlv.vector[2].data, Message->ioctlv.vector[2].len, Message->ioctlv.vector[3].data, Message->ioctlv.vector[4].data);
		}
		
		return ForwardIoctlv();
	}

	int DiProvider::ForwardIoctl()
	{
		return Module->ForwardIoctl(Message, true);
	}
	
	int DiProvider::ForwardIoctlv()
	{
		return Module->ForwardIoctlv(Message, true);
	}
} }

#endif
