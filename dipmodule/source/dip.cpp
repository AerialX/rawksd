#include "dip.h"
#include "patch.h"
#include "emu.h"

#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <print.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <files.h>

#include "fileprovider.h"

#include "logging.h"

#define OPEN_MODE_BYPASS 0x80
#define DIPIDLE_MSG 0xF17E1D7E
#define DIPIDLE_TIMEOUT 37968750 // 20s in starlet timer units
#define DIPIDLE_TICK 2000000 // check for idle files every 2 seconds

struct TemporaryPatch
{
	s64 Offset;
	s64 Length;
	s64 FileOffset;
};

namespace ProxiIOS { namespace DIP {
	DIP::DIP() : ProxyModule("/dev/do", "/dev/di")
	{
		FreeFiles = DIPFiles;
		OpenFiles = NULL;
		for (int i = 0; i < MAX_OPEN_FILES-1; i++) {
			DIPFiles[i].fd = -1;
			DIPFiles[i].next = DIPFiles+i+1;
		}
		DIPFiles[MAX_OPEN_FILES-1].fd = -1;
		DIPFiles[MAX_OPEN_FILES-1].next = NULL;

		Idle_Timer = os_create_timer(DIPIDLE_TICK, 0, queuehandle, DIPIDLE_MSG);

		memset(Patches, 0, sizeof(Patches));
		memset(PatchCount, 0, sizeof(PatchCount));
		memset(AllocatedPatches, 0, sizeof(AllocatedPatches));
		Clusters = false;

#ifdef YARR
		Provider = NULL;
#endif

		ShiftBase = 0x200000000ULL;
		PatchPartition = 0;

		LogInit();
	}

	bool DIP::HandleOther(u32 message, int &result, bool &ack)
	{
		if (message == DIPIDLE_MSG) {
			os_stop_timer(Idle_Timer);
			u32 time_now = os_time_now();
			struct DIPFile *PrevFile = NULL;
			struct DIPFile *ThisFile = OpenFiles;

			// look for "expired" open files
			// they're sorted, so all files after the first expired one will be closed
			while (ThisFile && (time_now - ThisFile->lastaccess) < DIPIDLE_TIMEOUT) {
				PrevFile = ThisFile;
				ThisFile = ThisFile->next;
			}

			if (PrevFile) // close the tail of the OpenFiles list
				PrevFile->next = NULL;
			else // else all files will be closed
				OpenFiles = NULL;

			while (ThisFile) {
				PrevFile = ThisFile;
				File_Close(ThisFile->fd);
				ThisFile->fd = -1;
				ThisFile = ThisFile->next;
				PrevFile->next = FreeFiles;
				FreeFiles = PrevFile;
			}

			os_restart_timer(Idle_Timer, DIPIDLE_TICK, 0);
			ack = false;
			return true;
		}

		return false;
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
		u32 *buffer_in = (u32*)message->ioctl.buffer_in;
		os_sync_before_read(buffer_in, (message->ioctl.length_in+31)&~31);
		switch (message->ioctl.command) {
			case Ioctl::Allocate: {
				LogPrintf("IOCTL: Allocate(0x%08x);\n", buffer_in[0]);
				if (Reallocate(buffer_in[0], buffer_in[1]))
					return 1;
				return -1;
			}
			case Ioctl::AddShift: {
				u32 len = buffer_in[0];
				u64 originaloffset = ((u64)buffer_in[1] << 32) | buffer_in[2];
				u64 newoffset = ((u64)buffer_in[3] << 32) | buffer_in[4];
				LogPrintf("IOCTL: AddShift(0x%08x, 0x%08x%08x, 0x%08x%08x);\n", len, (u32)(originaloffset >> 32), (u32)originaloffset, (u32)(newoffset >> 32), (u32)newoffset);

				Shift shift;
				shift.Length = len;
				shift.OriginalOffset = originaloffset >> 2;
				shift.Offset = newoffset >> 2;

				return AddPatch(PatchType::Shift, &shift);
			}
			case Ioctl::AddPatch: {
				s32 id = buffer_in[0];
				//u32 fileoffset = buffer_in[1];
				u64 offset = ((u64)buffer_in[2] << 32) | buffer_in[3];
				u32 length = buffer_in[4];
				PatchPartition = CurrentPartition;

				LogPrintf("IOCTL: AddPatch(0x%08x, 0x%08x%08x, 0x%08x);\n", id, (u32)(offset >> 32), (u32)offset, length);

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
				char* filename = (char*)message->ioctl.buffer_in;
				int len = message->ioctl.length_in;
				LogPrintf("IOCTL: AddFile(\"%s\");\n", filename);

				FileDesc file;

				if (Clusters) {
					if (message->ioctl.length_io > 0) {
						os_sync_before_read(message->ioctl.buffer_io, message->ioctl.length_io);
						file.Cluster = *(u64*)message->ioctl.buffer_io; // TODO: Casting u64 to u32... Saving memory but could be bad.
						LogPrintf("\t Cluster: 0x%08x%08x;\n", *(u32*)message->ioctl.buffer_io, (u32)file.Cluster);
						if (*(u32*)message->ioctl.buffer_io)
							LogPrintf("\tWARNING! Cluster too large for cluster hack to work (u64).\n");
					} else {
						Stats st;
						if (File_Stat(filename, &st) || st.Mode & S_IFDIR)
							return -1; // File doesn't exist
						file.Cluster = st.Identifier;
					}
				} else {
					file.Filename = new char[len];
					memcpy(file.Filename, filename, len);
				}

				return AddPatch(PatchType::File, &file);
			}
#ifdef YARR
			case Ioctl::SetFileProvider:
				LogPrintf("IOCTL: SetFileProvider(\"%s\");\n", (const char*)message->ioctl.buffer_in);
				Provider = new FileProvider(this, (const char*)message->ioctl.buffer_in);
				if (!Provider)
					return -1;
				return 1;
#endif
			case Ioctl::SetShiftBase:
				ShiftBase = ((u64)buffer_in[0] << 32) | buffer_in[1];
				LogPrintf("IOCTL: SetShiftBase(0x%08x%08x);\n", (u32)(ShiftBase >> 32), (u32)ShiftBase);
			case Ioctl::SetClusters:
				Clusters = buffer_in[0];
				LogPrintf("IOCTL: SetClusters(%s);\n", Clusters ? "true" : "false");
				return 1;
			case Ioctl::UnencryptedRead: {
				LogPrintf("IOCTL: UnencryptedRead(0x%08x%08x, 0x%08x, *0x%08x);\n", buffer_in[2]>>30, buffer_in[2]<<2, buffer_in[1], (u32)message->ioctl.buffer_io);
				int ret = ForwardIoctl(message);
				LogPrintf("\tForward %d\n", ret);
				return ret;
			}
			case Ioctl::Read: {
				u32 len = buffer_in[1];
				s64 pos = (s64)buffer_in[2] << 2;
				//LogPrintf("IOCTL: Read(0x%08x%08x, 0x%08x, *0x%08x);\n", (u32)(pos >> 32), (u32)pos, len, (u32)message->ioctl.buffer_io);

				if (CurrentPartition != PatchPartition) {
					int ret = ForwardIoctl(message);
					//LogPrintf("\tForward %d\n", ret);
					return ret;
				}

				Shift* shifts[MAX_FOUND];
				int foundshifts = FindPatch(PatchType::Shift, pos, len, (void**)shifts, MAX_FOUND);
				STACK_ALIGN(ipcmessage, tempmessage, 1, 0x20);
				STACK_ALIGN(u8, tempmessagebufferin, 0x20, 0x20);
				if (foundshifts) {
					memcpy(tempmessage, message, sizeof(ipcmessage));
					tempmessage->ioctl.buffer_in = (u32*)tempmessagebufferin;
					tempmessage->ioctl.length_in = MIN(message->ioctl.length_in, 0x20);
					memcpy(tempmessagebufferin, message->ioctl.buffer_in, tempmessage->ioctl.length_in);
					message = tempmessage;
					os_sync_after_write(message, sizeof(ipcmessage));

					for (int i = 0; i < foundshifts; i++) {
						Shift* shift = shifts[i];
						s64 offset = pos - ((s64)shift->Offset << 2);
						u32* bufferoffset = (u32*)message->ioctl.buffer_in + 2;
						*bufferoffset = (offset + ((u64)shift->OriginalOffset << 2)) >> 2;
						LogPrintf("\tShifting: 0x%08x%08x\n", *bufferoffset >> 30, *bufferoffset << 2);
					}

					os_sync_after_write(message->ioctl.buffer_in, message->ioctl.length_in);
				}

				Patch* found[MAX_FOUND];
				int foundpatches = FindPatch(PatchType::Patch, pos, len, (void**)found, MAX_FOUND);
				if (foundpatches == 0) {
					int ret = ForwardIoctl(message);
					//LogPrintf("\tForward %d\n", ret);
					return ret;
				}

				LogPrintf("\tFound 0x%08x patches\n", foundpatches);

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
					patches[i].Length = MIN(patches[i].Length, len - patches[i].Offset);

					// Fuck it, too lazy to map every patch
					if (patches[i].Offset == 0 && patches[i].Length == len)
						filecover = true;
				}
				if (!filecover) {
					if ((u64)pos < ShiftBase || foundshifts)
						ForwardIoctl(message);
					os_sync_before_read(message->ioctl.buffer_io, message->ioctl.length_io);
				}
				for (int i = 0; i < foundpatches; i++) {
					LogPrintf("\tBuffer Offset: 0x%08x%08x\n", (u32)(patches[i].Offset >> 32), (u32)patches[i].Offset);
					if (!ReadFile(found[i]->File, patches[i].FileOffset, (u8*)message->ioctl.buffer_io + patches[i].Offset, patches[i].Length))
						return 2; // File error
				}

				return 1;
			}
			case Ioctl::ClosePartition:
				CurrentPartition = 0;
				return ForwardIoctl(message);
			case Ioctl::BanTitle: {
				s32 emu_fd = os_open(EMU_MODULE_NAME, 0);
				if (emu_fd<0)
					return emu_fd;
				int ret = os_ioctl(emu_fd, EMU::Ioctl::BanTicket, buffer_in, message->ioctl.length_in, NULL, 0);
				os_close(emu_fd);
				return ret;
			}
			case Ioctl::DLCDir: {
				s32 emu_fd = os_open(EMU_MODULE_NAME, 0);
				if (emu_fd<0)
					return emu_fd;
				int ret = os_ioctl(emu_fd, EMU::Ioctl::DLCDir, buffer_in, message->ioctl.length_in, NULL, 0);
				os_close(emu_fd);
				return ret;
			}
			case Ioctl::Seek: {
				u64 offset = (u64)buffer_in[1] << 2;
				LogPrintf("IOCTL: Seek(0x%08x%08x);\n", (u32)(offset >> 32), (u32)offset);
				if (offset >= ShiftBase) {
					LogPrintf("\tReturn 1\n");
					return 1;
				}
				int ret = ForwardIoctl(message);
				LogPrintf("\tForward %d\n", ret);
				return ret;
			}
			default: {
				//LogPrintf("IOCTL: Unknown(0x%02X)\n", message->ioctl.command);
				int ret = ForwardIoctl(message);
				//LogPrintf("\tForward %d\n", ret);
				return ret;
			}
		}
	}

	int DIP::HandleIoctlv(ipcmessage* message)
	{
		int ret;
		for (u32 i=0; i < message->ioctlv.num_in; i++) {
			if (message->ioctlv.vector[i].len)
				os_sync_before_read(message->ioctlv.vector[i].data, message->ioctlv.vector[i].len);
		}
		switch (message->ioctlv.command) {
			case Ioctl::OpenPartition:
				CurrentPartition = (u64)((u32*)message->ioctlv.vector[0].data)[1] << 2;
				ret = ForwardIoctlv(message);
				if (ret < 0 || ret == 2)
					CurrentPartition = 0;
				return ret;
			case Ioctl::AddEmu:
				LogPrintf("IOCTL: AddEmu();\n");
				if (message->ioctlv.num_in < 3)
					return -1;
				return DoEmu((const char*)message->ioctlv.vector[0].data, (const char*)message->ioctlv.vector[1].data, (const int*)message->ioctlv.vector[2].data);
		}
		return ForwardIoctlv(message);
	}

	int DIP::CopyDir(const char *in_dir, const char *out_dir)
	{
		char nandfile[16]; // NAND filenames are 12 chars max
		char *in_string=NULL, *out_string=NULL;
		Stats st;
		int in, ret=1;
		in = File_OpenDir(in_dir);
		if (in<0)
			return in;

		if (File_CreateDir(out_dir)==ERROR_NOTMOUNTED)
			return ERROR_NOTMOUNTED;

		// 17=1(null char) + sizeof(nandfile) (there's a / in there somewhere too)
		in_string = (char*)Alloc(strlen(in_dir)+17);
		out_string = (char*)Alloc(strlen(out_dir)+17);
		if (in_string==NULL || out_string==NULL) {
			File_CloseDir(in);
			Dealloc(in_string);
			Dealloc(out_string);
			return ERROR_OUTOFMEMORY;
		}
		strcpy(in_string, in_dir);
		strcpy(out_string, out_dir);

		while (File_NextDir(in, nandfile, &st)>=0) {
			strcpy(in_string+strlen(in_dir), "/");
			strcat(in_string, nandfile);
			strcpy(out_string+strlen(out_dir), "/");
			strcat(out_string, nandfile);

			if (st.Mode & S_IFDIR) {
				if (nandfile[0]=='.')
					continue;
				ret = CopyDir(in_string, out_string);
			} else if (st.Mode & S_IFREG) { // S_IFREG test is just a precaution
				int in_file = -1;
				int out_file;

				// check if it exists
				out_file = File_Open(out_string, O_RDONLY);
				if (out_file<0) {
					in_file = File_Open(in_string, O_RDONLY);
					if (in_file<0) {
						ret = in_file;
						break;
					}
					out_file = File_Open(out_string, O_CREAT|O_WRONLY);
					if (out_file<0) {
						File_Close(in_file);
						ret = out_file;
						break;
					}

					void *copy_buf = Alloc(0x8000);
					if (copy_buf == NULL)
						ret = ERROR_OUTOFMEMORY;
					else {
						while (st.Size) {
							int readed = File_Read(in_file, copy_buf, MIN(0x8000, st.Size));
							if (readed<=0)
								ret = -1;
							else if (File_Write(out_file, copy_buf, readed)!=readed)
								ret = -1;
							else
								st.Size -= readed;
						}
					}
					Dealloc(copy_buf);
				}

				File_Close(out_file);
				if (in_file>=0)
					File_Close(in_file);
			}
			if (ret<0)
				break;
		}

		File_CloseDir(in);

		Dealloc(in_string);
		Dealloc(out_string);
		return ret;
	}

	int DIP::DoEmu(const char* nand_dir, const char* ext_dir, const int* clone)
	{
		int i=0;
		char *mnt_dir;
		char *end_path;

		if (strlen(ext_dir)<=0 || strlen(nand_dir)<=0)
			return -1;

		mnt_dir = (char*)Memalign(32, strlen(ext_dir)+1);
		if (mnt_dir==NULL)
			return ERROR_OUTOFMEMORY;

		strcpy(mnt_dir, ext_dir);
		if (!strncmp(mnt_dir, "/mnt/net/", 9))
			end_path = strchr(mnt_dir+9, '/')+1;
		else if (!strncmp(mnt_dir, "/mnt/", 5))
			end_path = strchr(mnt_dir+5, '/')+1;
		else
			end_path = mnt_dir+1;
		while ((end_path = strchr(end_path, '/'))) {
			*end_path = 0;
			File_CreateDir(mnt_dir);
			*end_path++ = '/';
		}
		if (mnt_dir[strlen(mnt_dir)-1]!='/')
			File_CreateDir(mnt_dir);

		Dealloc(mnt_dir);

		if (*clone) {
			mnt_dir = (char*)Memalign(32, strlen(nand_dir)+11);
			if (mnt_dir) {
				strcpy(mnt_dir, "/mnt/isfs");
				strcat(mnt_dir, nand_dir);
				i = CopyDir(mnt_dir, ext_dir);
				Dealloc(mnt_dir);
			} else
				i = ERROR_OUTOFMEMORY;
		}

		if (i>=0) {
			int emu_fd = os_open(EMU_MODULE_NAME, 0);
			if (emu_fd<0)
				i = emu_fd;
			else {
				LogPrintf("DIP->EMU ioctl\n");
				ioctlv vec[2];
				vec[0].data = (void*)nand_dir;
				vec[0].len = strlen(nand_dir)+1;
				vec[1].data = (void*)ext_dir;
				vec[1].len = strlen(ext_dir)+1;
				os_sync_after_write(vec, sizeof(ioctlv)*2);
				i = os_ioctlv(emu_fd, EMU::Ioctl::RedirectDir, 2, 0, vec);
				os_close(emu_fd);
			}
		}

		return i;
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

	bool DIP::ReadFile(s16 fileid, u32 offset, void* buffer, u32 length)
	{
		void* data = buffer;

		LogPrintf("\tReadFile(0x%04x, 0x%08x, 0x%08x) : ", (u32)fileid, offset, length);

		FileDesc* file = (FileDesc*)Patches[PatchType::File] + fileid;
		struct DIPFile *ThisFile = GetFile(fileid);
		if (ThisFile==NULL) {
			LogPrintf("\t\tGetFile failed! (PANIC)\n");
			return false;
		}
		if (ThisFile->fd < 0) {
			if (Clusters)
				ThisFile->fd = File_Open_ID(file->Cluster, O_RDONLY);
			else
				ThisFile->fd = File_Open(file->Filename, O_RDONLY);

			if (ThisFile->fd < 0) { // move it back to the free list
				LogPrintf("0x%08x\n\t\tFile_Open failed!\n", ThisFile->fd);
				OpenFiles = ThisFile->next;
				ThisFile->next = FreeFiles;
				FreeFiles = ThisFile;
				return false;
			}

			ThisFile->fileid = fileid;
		}

		if ((int)buffer & 0x1F) { // Just in case...
			data = Memalign(0x20, ROUND_UP(length, 0x20));
			if (!data)
				data = buffer;
		}

		File_Seek(ThisFile->fd, offset, SEEK_SET);

		int ret = File_Read(ThisFile->fd, (u8*)data, length);

		ThisFile->lastaccess = os_time_now();

		LogPrintf("0x%08x\n", ret);

		if (ret <= 0)
			LogPrintf("\t\tFile_Read error!\n");

		if (data != buffer) {
			if (ret>0)
				memcpy(buffer, data, ret);
			Dealloc(data);
		}
		os_sync_after_write(buffer, length);

		return ret >= 0;
	}

	struct DIP::DIPFile* DIP::GetFile(s16 fileid)
	{
		struct DIPFile *ThisFile = NULL;
		if (OpenFiles==NULL) { // take the head of the free list
			OpenFiles = ThisFile = FreeFiles;
			FreeFiles = FreeFiles->next;
			ThisFile->next = NULL;
		} else if (OpenFiles->fileid == fileid)
			ThisFile = OpenFiles;
		else {
			struct DIPFile* PrevFile = OpenFiles;
			for (ThisFile=OpenFiles->next; ThisFile->next; PrevFile=ThisFile, ThisFile=ThisFile->next) {
				if (ThisFile->fileid == fileid)
					break;
			}
			if (ThisFile->fileid != fileid) { // need to open it
				if (FreeFiles==NULL) { // close oldest open file and move it to the free list
					PrevFile->next = ThisFile->next;
					File_Close(ThisFile->fd);
					ThisFile->fd = -1;
					ThisFile->next = FreeFiles;
					FreeFiles = ThisFile;
				}

				ThisFile = FreeFiles;
				FreeFiles = FreeFiles->next;
			} else
				PrevFile->next = ThisFile->next;
			ThisFile->next = OpenFiles;
			OpenFiles = ThisFile;
		}
		return ThisFile;
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
#ifdef YARR
		if (Provider && !bypass)
			return Provider->HandleIoctl(message);
		else
#endif
			return ProxyModule::ForwardIoctl(message);

	}
	int DIP::ForwardIoctlv(ipcmessage* message, bool bypass)
	{
#ifdef YARR
		if (Provider && !bypass)
			return Provider->HandleIoctlv(message);
		else
#endif
			return ProxyModule::ForwardIoctlv(message);
	}
} }
