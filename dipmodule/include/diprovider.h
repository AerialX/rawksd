#pragma once

#include <proxiios.h>

namespace ProxiIOS { namespace DIP {
	class DIP;
	
	class DiProvider
	{
	protected:
		DIP* Module;
		ipcmessage* Message;
		u32 ErrorCode;
	public:
		DiProvider(DIP* module);
		
		virtual int HandleIoctl(ipcmessage* message);
		virtual int HandleIoctlv(ipcmessage* message);
		
		virtual int ForwardIoctl();
		virtual int ForwardIoctlv();
		
		virtual int Inquiry(void* driveid) { return ForwardIoctl(); }
		virtual int ReadDiscID(void* discid) { return ForwardIoctl(); }
		virtual int Read(void* buffer, u32 size, u32 offset) { return ForwardIoctl(); }
		virtual int WaitCoverClose() { return ForwardIoctl(); }
		virtual int ResetNotify() { return ForwardIoctl(); }
		virtual int GetCover() { return ForwardIoctl(); }
		virtual int Reset(int param) { return ForwardIoctl(); }
		virtual int ClosePartition() { return ForwardIoctl(); }
		virtual int UnencryptedRead(void* buffer, int size, int offset) { return ForwardIoctl(); }
		virtual int EnableDVD(bool enable) { return ForwardIoctl(); }
		virtual int ReportKey() { return ForwardIoctl(); }
		virtual int Seek() { return ForwardIoctl(); }
		virtual int ReadDVD(int zero1, int zero2, int sectors, int sector) { return ForwardIoctl(); }
		virtual int StopLaser() { return ForwardIoctl(); }
		virtual int Offset(int param1, int param2) { return ForwardIoctl(); }
		virtual int ReadBCA(void* buffer, int length) { return ForwardIoctl(); }
		virtual int VerifyCover(u32* output) { return ForwardIoctl(); }
		virtual int RequestError(u32* errorcode) { return ForwardIoctl(); }
		virtual int StopMotor(bool eject, bool kill) { return ForwardIoctl(); }
		virtual int EnableAudio(bool enable) { return ForwardIoctl(); }
		virtual int CoverRegister() { return ForwardIoctl(); }
		virtual int CoverClear() { return ForwardIoctl(); }
		virtual int StatusRegister() { return ForwardIoctl(); }
		virtual int SetMaximumRotation(int value) { return ForwardIoctl(); }
		virtual int OpenPartition(u32 offset, void* ticket, void* certificate, u32 certificateLength, void* tmd, void* errors) { return ForwardIoctlv(); }
	};
} }
