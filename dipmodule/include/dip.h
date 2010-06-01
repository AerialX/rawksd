#pragma once

#include <proxiios.h>

#include <diprovider.h>

#define MAX_OPEN_FILES 8
#define MAX_FOUND MAX_OPEN_FILES

namespace ProxiIOS { namespace DIP {
	namespace Ioctl {
		enum Enum {
			Inquiry	                     = 0x12,
			ReadDiscID                   = 0x70,
			Read                         = 0x71,
			WaitCoverClose               = 0x79,
			CoverRegister                = 0x7A,
			ResetNotify	                 = 0x7E,
			ReadPhysical                 = 0x80,
			ReadCopyright                = 0x81,
			ReadDiscKey                  = 0x82,
			ImmediateData                = 0x84,
			DisableCVR                   = 0x85,
			CoverClear	                 = 0x86,
			GetCover                     = 0x88,
			EnableCVR                    = 0x89,
			Reset                        = 0x8A,
			OpenPartition                = 0x8B, // Ioctlv
			ClosePartition               = 0x8C,
			UnencryptedRead	             = 0x8D,
			EnableDVD                    = 0x8E,
			GetNoDiscOpenPartitionParams = 0x90, // Ioctlv
			GetNoDiscBufferSizes         = 0x92, // Ioctlv
			OpenPartitionwithTMDandTicket= 0x94, // Ioctlv
			StatusRegister               = 0x95,
			ControlRegister              = 0x96,
			ReportKey                    = 0xA4,
			ReadCommand                  = 0xA8,
			Seek                         = 0xAB,
			ReadDVD                      = 0xD0,
			ReadConfig                   = 0xD1,
			StopLaser                    = 0xD2,
			Offset                       = 0xD9,
			ReadBCA                      = 0xDA,
			VerifyCover                  = 0xDB,
			RequestRetryNumber           = 0xDC,
			SetMaximumRotation           = 0xDD,
			SerMeasControl               = 0xDF,
			RequestError                 = 0xE0,
			PlayAudio                    = 0xE1,
			AudioStatus                  = 0xE2,
			StopMotor                    = 0xE3,
			EnableAudio                  = 0xE4,

			AddFile                      = 0xC1,
			AddPatch                     = 0xC2,
			AddShift                     = 0xC3,
			SetClusters                  = 0xC4,
			Allocate                     = 0xC5,
			AddEmu                       = 0xC6, // Ioctlv
			SetFileProvider              = 0xC7,
			SetShiftBase                 = 0xC8,
			BanTitle                     = 0xC9,
			DLCDir                       = 0xCA
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
			File,
			Max
		};
	}

	class DIP : public ProxiIOS::ProxyModule
	{
		private:
			ostimer_t Idle_Timer;

			struct DIPFile {
				s16 fileid;
				s32 fd;
				u32 lastaccess;
				struct DIPFile *next;
			} DIPFiles[MAX_OPEN_FILES];

			struct DIPFile *OpenFiles;
			struct DIPFile *FreeFiles;
			struct DIPFile* GetFile(s16 fileid);

			int CopyDir(const char *in_dir, const char *out_dir);
			int DoEmu(const char* nand_dir, const char* ext_dir, const int* clone);
		public:
			u64 CurrentPartition;
			u64 ShiftBase;
			u64 PatchPartition;

			u32 AllocatedPatches[PatchType::Max];
			u32 PatchCount[PatchType::Max];
			void* Patches[PatchType::Max];

			bool Clusters;
#ifdef YARR
			DiProvider* Provider;
#endif
			DIP();

			int HandleOpen(ipcmessage* message);
			int HandleIoctl(ipcmessage* message);
			int HandleIoctlv(ipcmessage* message);
			bool HandleOther(u32 message, int &result, bool &ack);

			int ForwardIoctl(ipcmessage* message);
			int ForwardIoctlv(ipcmessage* message);
			int ForwardIoctl(ipcmessage* message, bool bypass);
			int ForwardIoctlv(ipcmessage* message, bool bypass);

			int GetPatchSize(int index);
			int FindPatch(int index, s64 pos, u32 len, void** found, int limit);
			bool Reallocate(int index, int toadd);
			int AddPatch(int index, void* data);

			bool ReadFile(s16 fileid, u32 offset, void* data, u32 length);
	};
} }
