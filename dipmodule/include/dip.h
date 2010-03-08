#pragma once

#include <proxiios.h>

#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <ipc.h>

#include <files.h>

#include "patch.h"

#include "stream.h"

#define MAX_OPEN_FILES 8
#define MAX_FOUND MAX_OPEN_FILES

//#define YARR

namespace ProxiIOS { namespace DIP {
	namespace Ioctl {
		enum Enum {
			Inquiry			= 0x12,
			ReadDiskID		= 0x70,
			Read			= 0x71,
			WaitCoverClose	= 0x79,
			ResetNotify		= 0x7E,
			GetCover		= 0x88,
			Reset			= 0x8A,
			ClosePartition	= 0x8C,
			UnencryptedRead	= 0x8D,
			EnableDVD		= 0x8E,
			Seek			= 0xAB,
			ReadDVD			= 0xD0,
			StopLaser		= 0xD2,
			Offset			= 0xD9,
			VerifyCover		= 0xDB,
			RequestError	= 0xE0,
			StopMotor		= 0xE3,
			AudioStreaming	= 0xE4,

			AddFile			= 0xC1,
			AddPatch		= 0xC2,
			AddShift		= 0xC3,
			SetClusters		= 0xC4,
			Allocate		= 0xC5,
			AddFsPatch		= 0xC6,
			HandleFs		= 0xC7,
			GetFsHook		= 0xC8,
			Yarr_Enable		= 0xCA,
			Yarr_AddIso		= 0xCB,
			InitLog			= 0xCF,

			// cIOS
			cIOS_EnableDVD		= 0xF0,
			cIOS_SetOffsetBase	= 0xF1,
			cIOS_GetOffsetBase	= 0xF2,
			cIOS_CustomCommand	= 0xFF,
			
			// Ioctlv
			OpenPartition	= 0x8B
		};
	}
	
	namespace YarrMode {
		enum Enum {
			File			= 0x01,
			DVD				= 0x02
		};
	}
	
	class DIP : public ProxiIOS::ProxyModule
	{
	public:
		s32 MyFd;
		
		s32 CurrentPartition;

		u32 AllocatedFiles;
		u32 AllocatedPatches;
		u32 AllocatedFsPatches;
		u32 AllocatedShifts;

		u32 FileCount;
		u32 PatchCount;
		u32 FsPatchCount;
		u32 ShiftCount;
		FileDesc* Files;
		Patch* Patches;
		Shift* Shifts;
		FsPatch* FsPatches;
		s32 OpenFiles[MAX_OPEN_FILES];
		s32 OpenFds[MAX_OPEN_FILES];
		
		s32 FsFds[MAX_OPEN_FILES];
		
		bool Clusters; // Whether we're using the space-saving FAT cluster hack or not
		
		u32 OffsetBase;
		
		Shift* FoundShifts[MAX_FOUND];
		Patch* FoundPatches[MAX_FOUND];
		
		
		DIP();
		
		int HandleOpen(ipcmessage* message);
		int HandleIoctl(ipcmessage* message);
		int HandleIoctlv(ipcmessage* message);

		int FindShift(u64 pos, u32 len);
		int FindPatch(u64 pos, u32 len);
		FsPatch* FindFsPatch(char* filename);
		bool ReadFile(s16 fileid, u32 offset, void* data, u32 length);
		bool Reallocate(u32 type, int toadd);
		int IsFileOpen(s16 fileid);
		int HandleFsMessage(ipcmessage* message, int* ret);

		// yarr
		bool Yarr;
		int Yarr_Mode;
		Stream* YarrStream;
		PartitionStream* YarrPartitionStream;
		
		int yarr_HandleIoctl(ipcmessage* message);
		int yarr_HandleIoctlv(ipcmessage* message);
		void yarr_Enable(bool enable, int mode);
		int yarr_AddIso(char* path);
		int yarr_UnencryptedRead(u64 pos, void* data, u32 len);
		int yarr_Read(u64 pos, void* data, u32 len);
		int yarr_OpenPartition(u64 offset, void* ticket, u32 ticketlen, void* cert, u32 certlen, void* out, u32 outlen, void* errorbuffer, u32 errorlen);
		int yarr_ClosePartition();

		int ForwardIoctl(ipcmessage* message);
		int ForwardIoctlv(ipcmessage* message);
	};
} }
