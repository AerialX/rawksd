#include "dip.h"
#include "patch.h"

#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <print.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <gpio.h>

#include <files.h>

#define OPEN_MODE_BYPASS 0x80

struct TemporaryPatch
{
	s64 Offset;
	s64 Length;
	s64 FileOffset;
};

namespace ProxiIOS { namespace DIP {
	DIP::DIP() : ProxyModule("/dev/do", "/dev/di")
	{
		File_Init();
		
		for (int i = 0; i < MAX_OPEN_FILES; i++)
			OpenFiles[i] = -1;
		
		memset(Patches, 0, sizeof(Patches));
		memset(PatchCount, 0, sizeof(PatchCount));
		memset(AllocatedPatches, 0, sizeof(AllocatedPatches));
		Clusters = false;
		
		Provider = NULL;
	}
	
	int DIP::HandleOpen(ipcmessage* message)
	{
		if (message->open.mode == OPEN_MODE_BYPASS)
			return OPEN_MODE_BYPASS;
		
		int ret = ProxyModule::HandleOpen(message);
		if (ret < 0)
			return Errors::OpenProxyFailure;
		
		return ret;
	}
	
	int DIP::AddPatch(int index, void* data)
	{
		if (PatchCount[index] == AllocatedPatches[index])
			if (!Reallocate(index, 1))
				return -1;
		
		int size = GetPatchSize(index);
		memcpy((u8*)Patches[index] + PatchCount[index] * size, data, size);
		
		return PatchCount[index]++;
	}
	
	int DIP::HandleIoctl(ipcmessage* message)
	{
		switch (message->ioctl.command) {
			case Ioctl::Allocate: {
				os_sync_before_read(message->ioctl.buffer_in, message->ioctl.length_in);
				if (Reallocate(message->ioctl.buffer_in[0], message->ioctl.buffer_in[1]))
					return 1;
				return -1;
			}
			case Ioctl::AddShift: {
				os_sync_before_read(message->ioctl.buffer_in, message->ioctl.length_in);
				u32 len = message->ioctl.buffer_in[0];
				u64 originaloffset = *(u64*)&message->ioctl.buffer_in[1];
				u64 newoffset = *(u64*)&message->ioctl.buffer_in[3];
				
				Shift shift;
				shift.Length = len;
				shift.OriginalOffset = originaloffset >> 2;
				shift.Offset = newoffset >> 2;
				
				return AddPatch(PatchType::Shift, &shift);
			}
			case Ioctl::AddPatch: {
				os_sync_before_read(message->ioctl.buffer_in, message->ioctl.length_in);
				s32 id = message->ioctl.buffer_in[0];
				//u32 fileoffset = message->ioctl.buffer_in[1];
				u64 offset = *(u64*)&message->ioctl.buffer_in[2];
				u32 length = message->ioctl.buffer_in[4];
				
				if (id < 0)
					return -1;

				Patch patch;
				patch.File = id;
				//patch.FileOffset = fileoffset;
				patch.Offset = offset >> 2;
				patch.Length = length;
				
				return AddPatch(PatchType::Patch, &patch);
			}
			case Ioctl::AddFile: {
				os_sync_before_read(message->ioctl.buffer_in, message->ioctl.length_in);
				char* filename = (char*)message->ioctl.buffer_in;
				int len = message->ioctl.length_in;
				
				FileDesc file;
				
				if (Clusters) {
					if (message->ioctl.length_io > 0) {
						os_sync_before_read(message->ioctl.buffer_io, message->ioctl.length_io);
						file.Cluster = *(u64*)message->ioctl.buffer_io;
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
				
				return AddPatch(PatchType::File, &file);
			}
			case Ioctl::SetClusters:
				os_sync_before_read(message->ioctl.buffer_in, message->ioctl.length_in);
				Clusters = message->ioctl.buffer_in[0];
				return 1;
			case Ioctl::Read: {
				os_sync_before_read(message->ioctl.buffer_in, message->ioctl.length_in);
				u32 len = message->ioctl.buffer_in[1];
				s64 pos = (s64)message->ioctl.buffer_in[2] << 2;

				Shift* shifts[MAX_FOUND];
				int foundshifts = FindPatch(PatchType::Shift, pos, len, (void**)shifts, MAX_FOUND);
				ipcmessage tempmessage; // Allows us to adjust the original message without actually writing to it
				u8 tempmessagebufferin[0x20];
				if (foundshifts) {
					memcpy(&tempmessage, message, sizeof(ipcmessage));
					tempmessage.ioctl.buffer_in = (u32*)tempmessagebufferin;
					tempmessage.ioctl.length_in = MIN(message->ioctl.length_in, 0x20);
					memcpy(tempmessagebufferin, message->ioctl.buffer_in, tempmessage.ioctl.length_in);
					message = &tempmessage;
					os_sync_after_write(message, sizeof(ipcmessage));
					
					for (int i = 0; i < foundshifts; i++) {
						Shift* shift = shifts[i];
						s64 offset = pos - ((s64)shift->Offset << 2);
						message->ioctl.buffer_in[2] = (offset + ((u64)shift->OriginalOffset << 2)) >> 2;
					}
					
					os_sync_after_write(message->ioctl.buffer_in, message->ioctl.length_in);
				}
				
				Patch* found[MAX_FOUND];
				int foundpatches = FindPatch(PatchType::Patch, pos, len, (void**)found, MAX_FOUND);
				if (foundpatches == 0)
					return ForwardIoctl(message);

				TemporaryPatch patches[MAX_FOUND];
				bool filecover = false;
				for (int i = 0; i < foundpatches; i++) {
					Patch* patch = found[i];
					patches[i].Length = patch->Length;
					patches[i].Offset = ((s64)patch->Offset << 2) - pos;
					patches[i].FileOffset = 0;
					if (patches[i].Offset < 0) { // Patch starts before the read
						patches[i].Length += patches[i].Offset; // Reduce the length
						patches[i].FileOffset -= patches[i].Offset; // Increase the file offset
						patches[i].Offset = 0;
					}
					patches[i].Length = MIN(patches[i].Length, len) - patches[i].Offset;
					
					// Fuck it, too lazy to map every patch
					if (patches[i].Offset == 0 && patches[i].Length == len)
						filecover = true;
				}
				if (!filecover) {
					if (((u32)(pos >> 2)) < 0xA0000000)
						ForwardIoctl(message);
					os_sync_before_read(message->ioctl.buffer_io, message->ioctl.length_io);
				}
				for (int i = 0; i < foundpatches; i++) {
					if (!ReadFile(found[i]->File, patches[i].FileOffset, (u8*)message->ioctl.buffer_io + patches[i].Offset, patches[i].Length))
						return 2; // File error
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
				CurrentPartition = ((u32*)message->ioctlv.vector[0].data)[1];
				return ForwardIoctlv(message);
		}
		
		return ForwardIoctlv(message);
	}
	
	int DIP::GetPatchSize(int index)
	{
		switch (index) {
			case PatchType::Patch:
				return sizeof(Patch);
			case PatchType::Shift:
				return sizeof(Shift);
			case PatchType::File:
				return sizeof(FileDesc);
		}
		
		return 0;
	}
	
	bool DIP::Reallocate(int index, int toadd)
	{
		int size = GetPatchSize(index);
		int allocated = AllocatedPatches[index] + toadd;
		void* old = Patches[index];
		Patches[index] = Alloc(allocated * size);
		if (!Patches[index]) {
			Patches[index] = old;
			return false;
		} else if (old) {
			memcpy(Patches[index], old, size * MIN(PatchCount[index], (u32)allocated));
			Dealloc(old);
		}
		
		AllocatedPatches[index] = allocated;
		
		return true;
	}
	
	int DIP::FindPatch(int index, s64 pos, u32 len, void** found, int limit)
	{
		int size = GetPatchSize(index);
		
		int count = 0;
		for (u32 i = 0; i < PatchCount[index]; i++) {
			void* pointer = (u8*)Patches[index] + i * size;
			OffsetPatch* patch = (OffsetPatch*)pointer;
			s64 offset = pos - ((s64)patch->Offset << 2);
			if ((offset == 0) || (offset < 0 && offset + len > 0) || (offset > 0 && offset < patch->Length)) {
				found[count++] = pointer;
				if (count == limit)
					break;
			}
		}
		
		return count;
	}
	
	bool DIP::ReadFile(s16 fileid, u32 offset, void* data, u32 length)
	{
		FileDesc* file = (FileDesc*)Patches[PatchType::File] + fileid;
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
			
			if (Clusters)
				OpenFds[openindex] = File_Open_ID(file->Cluster, O_RDONLY);
			else
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
	
	int DIP::IsFileOpen(s16 fileid)
	{
		for (int i = 0; i < MAX_OPEN_FILES; i++)
			if (OpenFiles[i] == fileid)
				return i;
		return -1;
	}
	
	int DIP::ForwardIoctl(ipcmessage* message)
	{
		return ForwardIoctl(message, false);
	}
	int DIP::ForwardIoctlv(ipcmessage* message)
	{
		return ForwardIoctlv(message, false);
	}
	int DIP::ForwardIoctl(ipcmessage* message, bool bypass)
	{
		if (!Provider || bypass)
			return ProxyModule::ForwardIoctl(message);
		else
			return Provider->HandleIoctl(message);
	}
	int DIP::ForwardIoctlv(ipcmessage* message, bool bypass)
	{
		if (!Provider || bypass)
			return ProxyModule::ForwardIoctlv(message);
		else
			return Provider->HandleIoctl(message);
	}
} }
