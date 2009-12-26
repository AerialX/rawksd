#include "dip.h"

#include <string.h>
#include <print.h>

#include "logging.h"

static const char HEX_CHARS[] = "0123456789abcdef";
static void IntToHex(char* dest, u32 num, int length)
{
	for (int i = length - 1; i >= 0; i--)
		*(dest++) = HEX_CHARS[(num >> (i * 4)) & 0x0F];
}

struct TemporaryPatch
{
	s64 Offset;
	s64 Length;
	s64 FileOffset;
};

namespace ProxiIOS { namespace DIP {
	DIP::DIP() : ProxyModule("/dev/do", "/dev/di")
	{
		OffsetBase = 0;
		
		File_Init();
		
		for (int i = 0; i < MAX_OPEN_FILES; i++)
			OpenFiles[i] = -1;
		FileCount = 0;
		PatchCount = 0;
		ShiftCount = 0;
		
		AllocatedFiles = 0;
		AllocatedPatches = 0;
		AllocatedShifts = 0;
		
		Files = NULL;
		Patches = NULL;
		Shifts = NULL;
		
		Clusters = false;
		
		Yarr = false;
		YarrPartitionStream = NULL;
		YarrStream = NULL;
	}
	
	int DIP::HandleOpen(ipcmessage* message)
	{
		int ret = ProxyModule::HandleOpen(message);
		if (ret < 0)
			return Errors::OpenProxyFailure;
		
		return ret;
	}

	int DIP::FindShift(u64 pos, u32 len)
	{
		int shift = 0;
		
		for (u32 i = 0; i < ShiftCount; i++) {
			s64 offset = pos - ((u64)Shifts[i].Offset << 2);
			if ((offset == 0) || (offset < 0 && offset + len > 0) || (offset > 0 && offset < Shifts[i].Size)) {
				FoundShifts[shift++] = Shifts + i;
				if (shift == MAX_FOUND)
					break;
			}
		}

		return shift;
	}

	int DIP::FindPatch(u64 pos, u32 len)
	{
		int patch = 0;
		for (u32 i = 0; i < PatchCount; i++) {
			s64 offset = pos - ((u64)Patches[i].Offset << 2);
			if ((offset == 0) || (offset < 0 && offset + len > 0) || (offset > 0 && offset < Patches[i].Length)) {
				FoundPatches[patch++] = Patches + i;
				if (patch == MAX_FOUND)
					break;
			}
		}

		return patch;
	}

	int DIP::IsFileOpen(s16 fileid)
	{
		for (int i = 0; i < MAX_OPEN_FILES; i++)
			if (OpenFiles[i] == fileid)
				return i;
		return -1;
	}

	bool DIP::ReadFile(s16 fileid, u32 offset, void* data, u32 length)
	{
		FileDesc* file = Files + fileid;
		int openindex = IsFileOpen(fileid);
		if (openindex < 0) { // Re-open it
			for (int k = 0; k < MAX_OPEN_FILES; k++) {
				if (OpenFiles[k] == -1) {
					OpenFiles[k] = fileid;
					openindex = k;
					break;
				}
			}
			if (openindex < 0) {
				File_Close(OpenFds[0]);
				for (int i = 0; i < MAX_OPEN_FILES - 1; i++) {
					OpenFiles[i] = OpenFiles[i + 1];
					OpenFds[i] = OpenFds[i + 1];
				}
				OpenFiles[MAX_OPEN_FILES - 1] = fileid;
				openindex = MAX_OPEN_FILES - 1;
			}
			
			if (Clusters) {
				// 1 + 8 + strlen("id\\") = 3 = 0x0C
				char clusterpath[0x0C];
				strcpy(clusterpath, "id\\");
				IntToHex(clusterpath + 0x03, file->Cluster, 0x08);
				clusterpath[0x0B] = '\0';
				
				OpenFds[openindex] = File_Open(clusterpath, O_RDONLY);
			} else
				OpenFds[openindex] = File_Open(file->Filename, O_RDONLY);
			
			if (OpenFds[openindex] < 0) {
				OpenFiles[openindex] = -1;
				return false;
			}
		}
		
		File_Seek(OpenFds[openindex], offset, SEEK_SET);
		
		int ret = File_Read(OpenFds[openindex], (u8*)data, length);
		
		if (ret > 0)
			os_sync_after_write(data, ret);

		return ret >= 0;
	}

	bool DIP::Reallocate(u32 type, s32 toadd)
	{
		switch (type) {
			case 0: { // Files
				FileDesc* oldfiles = Files;
				Files = new FileDesc[AllocatedFiles + toadd];
				if (Files == NULL) {
					Files = oldfiles;
					return false;
				} else if (oldfiles != NULL) {
					memcpy(Files, oldfiles, sizeof(FileDesc) * FileCount);
					delete[] oldfiles;
				}
				AllocatedFiles += toadd;
				return true; }
			case 1: { // Patches
				Patch* oldpatches = Patches;
				Patches = new Patch[AllocatedPatches + toadd];
				if (Patches == NULL) {
					Patches = oldpatches;
					return false;
				} else if (oldpatches != NULL) {
					memcpy(Patches, oldpatches, sizeof(Patch) * PatchCount);
					delete[] oldpatches;
				}
				AllocatedPatches += toadd;
				return true; }
			case 2: { // Shifts
				Shift* oldshifts = Shifts;
				Shifts = new Shift[AllocatedShifts + toadd];
				if (Shifts == NULL) {
					Shifts = oldshifts;
					return false;
				} else if (oldshifts != NULL) {
					memcpy(Shifts, oldshifts, sizeof(Shift) * ShiftCount);
					delete[] oldshifts;
				}
				AllocatedShifts += toadd;
				return true; }
		}

		return false;
	}

	int DIP::HandleIoctl(ipcmessage* message)
	{
		switch (message->ioctl.command) {
			case Ioctl::InitLog:
				LogInit();
				return 1;
			case Ioctl::Allocate: {
				os_sync_before_read(message->ioctl.buffer_in, message->ioctl.length_in);
				if (Reallocate(message->ioctl.buffer_in[0], (s32)message->ioctl.buffer_in[1]))
					return 1;
				return -1;
			}
			case Ioctl::AddShift: {
				os_sync_before_read(message->ioctl.buffer_in, message->ioctl.length_in);
				u32 size = message->ioctl.buffer_in[0];
				u64 originaloffset = *(u64*)&message->ioctl.buffer_in[1];
				u64 newoffset = *(u64*)&message->ioctl.buffer_in[3];

				Shift shift;
				shift.Size = size;
				shift.OriginalOffset = originaloffset >> 2;
				shift.Offset = newoffset >> 2;

				if (ShiftCount == AllocatedShifts) {
					if (!Reallocate(2, 1))
						return -1;
				}

				Shifts[ShiftCount] = shift;
				ShiftCount++;
				
				return 1;
			}
			case Ioctl::AddFile: {
				os_sync_before_read(message->ioctl.buffer_in, message->ioctl.length_in);
				
				FileDesc file;
				
				char* filename = (char*)message->ioctl.buffer_in;
				int len = message->ioctl.length_in;
				
				if (Clusters) {
					if (message->ioctl.length_io > 0) {
						os_sync_before_read(message->ioctl.buffer_io, message->ioctl.length_io);
						file.Cluster = message->ioctl.buffer_io[0];
					} else {
						Stats st;
						if (File_Stat(filename, &st) != 0)
							return -1; // File doesn't exist
						file.Cluster = st.Identifier;
					}
				} else {
					file.Filename = new char[len];
					memcpy(file.Filename, filename, len);
				}
				
				if (FileCount == AllocatedFiles)
					if (!Reallocate(0, 1))
						return -1;
				
				Files[FileCount] = file;
				FileCount++;
				
				return FileCount - 1;
			}
			case Ioctl::AddPatch: {
				os_sync_before_read(message->ioctl.buffer_in, message->ioctl.length_in);
				s32 id = message->ioctl.buffer_in[0];
				u32 fileoffset = message->ioctl.buffer_in[1];
				u64 offset = *(u64*)&message->ioctl.buffer_in[2];
				u32 length = message->ioctl.buffer_in[4];
				
				if (id < 0)
					return -1;
				
				Patch patch;
				patch.File = id;
				//patch.FileOffset = fileoffset;
				patch.Offset = offset >> 2;
				patch.Length = length;
				
				if (PatchCount == AllocatedPatches) {
					if (!Reallocate(1, 1))
						return -1;
				}
				
				Patches[PatchCount] = patch;
				PatchCount++;
				
				return 1;
			}
			case Ioctl::SetClusters: {
				os_sync_before_read(message->ioctl.buffer_in, message->ioctl.length_in);
				Clusters = message->ioctl.buffer_in[0];
				
				return 1;
			}
			case Ioctl::Yarr_Enable:
				os_sync_before_read(message->ioctl.buffer_in, message->ioctl.length_in);
				yarr_Enable((bool)message->ioctl.buffer_in[0], message->ioctl.buffer_in[1]);
				return 1;
			case Ioctl::Yarr_AddIso:
				os_sync_before_read(message->ioctl.buffer_in, message->ioctl.length_in);
				return yarr_AddIso((char*)message->ioctl.buffer_in);
			case Ioctl::Read: {
				os_sync_before_read(message->ioctl.buffer_in, message->ioctl.length_in);
				u32 len = message->ioctl.buffer_in[1];
				u64 pos = message->ioctl.buffer_in[2];
				pos <<= 2;

				int foundshifts = FindShift(pos, len);
				for (int i = 0; i < foundshifts; i++) {
					Shift* shift = FoundShifts[i];
					s64 offset = pos - ((u64)shift->Offset << 2);
					message->ioctl.buffer_in[2] = (offset + ((u64)shift->OriginalOffset << 2)) >> 2;
					os_sync_after_write(message->ioctl.buffer_in, message->ioctl.length_in);
				}
				
				int foundpatches = FindPatch(pos, len);
				
				if (foundpatches == 0)
					return ForwardIoctl(message);
				
				int ret = 1;
				TemporaryPatch patches[MAX_FOUND];
				bool filecover = false;
				for (int i = 0; i < foundpatches; i++) {
					Patch* patch = FoundPatches[i];
					patches[i].Length = patch->Length;
					patches[i].Offset = ((u64)patch->Offset << 2) - pos;
					patches[i].FileOffset = 0;
					if (patches[i].Offset < 0) { // Patch starts before the read
						patches[i].Length += patches[i].Offset; // Reduce the length
						patches[i].FileOffset -= patches[i].Offset; // Increase the file offset
						patches[i].Offset = 0;
					}
					patches[i].Length = MIN(patches[i].Length - patches[i].Offset, len - patches[i].Offset);
					
					// Fuck it, too lazy to map every patch
					if (patches[i].Offset == 0 && patches[i].Length == len)
						filecover = true;
				}
				if (!filecover) {
					ForwardIoctl(message);
					os_sync_before_read(message->ioctl.buffer_io, message->ioctl.length_io);
				}
				for (int i = 0; i < foundpatches; i++) {
					if (!ReadFile(FoundPatches[i]->File, patches[i].FileOffset, (u8*)message->ioctl.buffer_io + patches[i].Offset, patches[i].Length))
						return -1; // File error
				}
				
				return 1;
			}
			case Ioctl::ClosePartition:
				CurrentPartition = 0;
				return ForwardIoctl(message);
			default:
				return ForwardIoctl(message);
		}
	}

	int DIP::HandleIoctlv(ipcmessage* message)
	{
		switch (message->ioctlv.command) {
			case Ioctl::OpenPartition:
				CurrentPartition = *((u32*)message->ioctlv.vector[0].data);
				return ForwardIoctlv(message);
			default:
				return ForwardIoctlv(message);
				break;
		}
	}

} }
