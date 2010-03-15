#pragma once

#include <proxiios.h>

#include <diprovider.h>

#define MAX_OPEN_FILES 8
#define MAX_FOUND MAX_OPEN_FILES
#define MAX_PATCH_TYPES 0x03

namespace ProxiIOS { namespace DIP {
	namespace Ioctl {
		enum Enum {
			Inquiry			= 0x12,
			ReadDiscID		= 0x70,
			Read			= 0x71,
			WaitCoverClose	= 0x79,
			CoverRegister	= 0x7A,
			ResetNotify		= 0x7E,
			CoverClear		= 0x86,
			GetCover		= 0x88,
			Reset			= 0x8A,
			ClosePartition	= 0x8C,
			UnencryptedRead	= 0x8D,
			EnableDVD		= 0x8E,
			StatusRegister	= 0x95,
			ReportKey		= 0xA4,
			Seek			= 0xAB,
			ReadDVD			= 0xD0,
			StopLaser		= 0xD2,
			Offset			= 0xD9,
			ReadBCA			= 0xDA,
			VerifyCover		= 0xDB,
			SetMaximumRotation	= 0xDD,
			RequestError	= 0xE0,
			StopMotor		= 0xE3,
			EnableAudio		= 0xE4,
			
			AddFile			= 0xC1,
			AddPatch		= 0xC2,
			AddShift		= 0xC3,
			SetClusters		= 0xC4,
			Allocate		= 0xC5,
			AddEmu			= 0xC6,
			SetFileProvider = 0xC7,
			SetShiftBase	= 0xC8,
			
			// Ioctlv
			OpenPartition	= 0x8B
		};
	}
	
	namespace DiErrors {
		enum Enum {
			OK					= 0x000000,
			MotorStopped		= 0x020400,
			DiscIdNotRead		= 0x020401,
			NoDisc				= 0x023A00,
			SeekError			= 0x030200,
			ReadError			= 0x031100,
			ProtocolError		= 0x040800,
			InvalidMode			= 0x052000,
			NoAudioBuffer		= 0x052001,
			BlockOutOfRange		= 0x052100,
			InvalidField		= 0x052400,
			InvalidAudioCommand	= 0x052401,
			InvalidConfiguration= 0x052402,
			WrongDisc			= 0x053100,
			EndOfPartition		= 0x056300,
			DiscChanged			= 0x062800,
			RemoveDisc			= 0x0B5A01
		};
	}
	
	namespace PatchType {
		enum Enum {
			Patch = 0,
			Shift,
			File
		};
	}
	
	class DIP : public ProxiIOS::ProxyModule
	{
		public:
			u64 CurrentPartition;
			u64 ShiftBase;
			u64 PatchPartition;
			
			u32 AllocatedPatches[MAX_PATCH_TYPES];
			u32 PatchCount[MAX_PATCH_TYPES];
			void* Patches[MAX_PATCH_TYPES];
			
			s32 OpenFiles[MAX_OPEN_FILES];
			s32 OpenFds[MAX_OPEN_FILES];
			
			bool Clusters;
#ifdef YARR
			DiProvider* Provider;
#endif
			DIP();
			
			int HandleOpen(ipcmessage* message);
			int HandleIoctl(ipcmessage* message);
			int HandleIoctlv(ipcmessage* message);
			
			int ForwardIoctl(ipcmessage* message);
			int ForwardIoctlv(ipcmessage* message);
			int ForwardIoctl(ipcmessage* message, bool bypass);
			int ForwardIoctlv(ipcmessage* message, bool bypass);
			
			int GetPatchSize(int index);
			int FindPatch(int index, s64 pos, u32 len, void** found, int limit);
			bool Reallocate(int index, int toadd);
			int AddPatch(int index, void* data);
			
			bool ReadFile(s16 fileid, u32 offset, void* data, u32 length);
			int IsFileOpen(s16 fileid);
	};
} }
