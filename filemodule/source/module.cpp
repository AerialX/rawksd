#include "filemodule.h"

#include "rtc.h"
#include "gpio.h"

#include "file_fat.h"
#include "file_riifs.h"
#include "file_isfs.h"

#include "print.h"

#define FILE_IOCTL_FD 0x123
#define FSIDLE_MSG 0x500
#define FSRTC_MSG  0x501
#define SHORT_NAME "/......"

namespace ProxiIOS { namespace Filesystem {
	struct FilePathDesc {
		FilePathDesc(Filesystem* module, const char* path);

		const char* Path;
		FilesystemHandler* System;
	};

	Filesystem::Filesystem() : Module(FILE_MODULE_NAME)
	{
		memset(Mounted, null, sizeof(Mounted));
		memset(Disk, null, sizeof(Disk));

		Timer_Init();

		Mounted[0] = new IsfsHandler(this);
		Mounted[0]->Mount(null, 0);

		Idle_Timer = os_create_timer(FSIDLE_TICK, 0, queuehandle, FSIDLE_MSG);

		Default = 0;
		Long_Path = NULL;
		LogFS = -1;
	}

	int Filesystem::HandleIoctl(ipcmessage* message)
	{
		u32 *buffer_in = (u32*)message->ioctl.buffer_in;
		if (message->ioctl.length_in)
			os_sync_before_read(buffer_in, message->ioctl.length_in);

		switch (message->ioctl.command) {
			case Ioctl::Epoch: {
				time_t epoch;
				memcpy(&epoch, message->ioctl.buffer_in, sizeof(time_t));
				RTC_Init(epoch);
				// update (approximately) every 10 minutes
				os_create_timer(0, 600000000, queuehandle, FSRTC_MSG);
				return Errors::Success; }
			case Ioctl::SetSlotLED: {
				if (buffer_in[0])
					gpio_enable_toggle(GPIO_OSLOT);
				else
					gpio_disable_toggle(GPIO_OSLOT);
				return Errors::Success;
				}
			case Ioctl::InitDisc: {
				int index = 0;
				for (index = 0; Disk[index] != null && index < FILE_MAX_DISKS; index++)
					;

				if (index == FILE_MAX_DISKS)
					return Errors::OutOfMemory;

				int diskid = buffer_in[0];
				switch (diskid) {
					case Disks::SD:
						Disk[index] = const_cast<DISC_INTERFACE*>(&__io_wiisd);
						break;
					case Disks::USB:
						Disk[index] = const_cast<DISC_INTERFACE*>(&__io_usbstorage);
						break;
					default:
						return Errors::Unrecognized;
				}

				if (!Disk[index]->startup()) {
					Disk[index] = null;
					return Errors::DiskNotStarted;
				}
				if (!Disk[index]->isInserted()) {
					Disk[index] = null;
					return Errors::DiskNotInserted;
				}

				return index; }
			case Ioctl::Mount: {
				int index = 0;
				for (index = 0; Mounted[index] != null && index < FILE_MAX_MOUNTED; index++)
					;
				if (index == FILE_MAX_MOUNTED)
					return Errors::OutOfMemory;

				FilesystemHandler* system = null;
				Filesystems::Enum fs = (Filesystems::Enum)buffer_in[0];
				switch (fs) {
					case Filesystems::FAT:
						system = new FatHandler(this);
						break;
					case Filesystems::RiiFS:
						system = new RiiHandler(this);
						break;
					default:
						break;
				}

				if (!system)
					return Errors::Unrecognized;

				os_sync_before_read(message->ioctl.buffer_io, message->ioctl.length_io);
				int ret = system->Mount(message->ioctl.buffer_io, message->ioctl.length_io);
				if (ret < 0) {
					delete system;
					return ret;
				}

				Mounted[index] = system;

				return index; }
			case Ioctl::Unmount: {
				int index = buffer_in[0];
				FilesystemHandler* system = Mounted[index];
				if (!system)
					return Errors::NotMounted;

				int ret = system->Unmount();

				if (ret >= 0) {
					delete system;
					Mounted[index] = null;
					if (index == LogFS)
						LogFS = -1;
					if (index == Default)
						Default = 0;
				}
				return ret; }
			case Ioctl::CheckPhysical: {
				int index = buffer_in[0];
				FilesystemHandler* system = Mounted[index];
				if (!system)
					return Errors::NotMounted;

				return system->CheckPhysical(); }
			case Ioctl::GetMountPoint: {
				int index = buffer_in[0];
				FilesystemHandler* system = Mounted[index];
				if (!system)
					return Errors::NotMounted;

				strncpy((char*)message->ioctl.buffer_io, system->MountPoint, message->ioctl.length_io);
				os_sync_after_write(message->ioctl.buffer_io, message->ioctl.length_io);

				return Errors::Success; }
			case Ioctl::SetDefault: {
				FilesystemHandler* system;
				if (message->ioctl.length_io) {
					// this isn't how an output buffer should be used
					os_sync_before_read(message->ioctl.buffer_io, message->ioctl.length_io);
					FilePathDesc desc(this, (const char*)message->ioctl.buffer_io);
					system = desc.System;
				} else
					system = Mounted[buffer_in[0]];

				if (!system)
					return Errors::NotMounted;

				u32 index;
				for (index = 0; Mounted[index] != system; index++)
					;
				Default = index;

				return Errors::Success; }
			case Ioctl::SetLogFS: {
				int index = buffer_in[0];
				if (index>0 && !Mounted[index])
					return Errors::NotMounted;

				LogFS = index;
				return Errors::Success; }
			case Ioctl::GetLogFS:
				return LogFS;
			case Ioctl::Stat: {
				FilePathDesc descriptor(this, (const char*)message->ioctl.buffer_in);
				if (!descriptor.System)
					return Errors::NotMounted;
				int ret = descriptor.System->Stat(descriptor.Path, (Stats*)message->ioctl.buffer_io);
				if (ret >= 0)
					os_sync_after_write(message->ioctl.buffer_io, message->ioctl.length_io);
				return ret; }
			case Ioctl::CreateFile: {
				FilePathDesc descriptor(this, (const char*)message->ioctl.buffer_in);
				if (!descriptor.System)
					return Errors::NotMounted;
				return descriptor.System->CreateFile(descriptor.Path); }
			case Ioctl::Delete: {
				FilePathDesc descriptor(this, (const char*)message->ioctl.buffer_in);
				if (!descriptor.System)
					return Errors::NotMounted;
				return descriptor.System->Delete(descriptor.Path); }
			case Ioctl::Rename: {
				os_sync_before_read((void*)buffer_in[0], IPC_MAXPATH_LEN);
				os_sync_before_read((void*)buffer_in[1], IPC_MAXPATH_LEN);
				FilePathDesc source(this, (const char*)buffer_in[0]);
				FilePathDesc dest(this, (const char*)buffer_in[1]);
				if (!source.System)
					return Errors::NotMounted;
				if (source.System != dest.System)
					return Errors::Unrecognized; // Cross-filesystem renaming not yet possible; TODO: Copy file then delete
				return source.System->Rename(source.Path, dest.Path); }
			case Ioctl::CreateDir: {
				FilePathDesc descriptor(this, (const char*)message->ioctl.buffer_in);
				if (!descriptor.System)
					return Errors::NotMounted;
				return descriptor.System->CreateDir(descriptor.Path); }
			case Ioctl::OpenDir: {
				FilePathDesc descriptor(this, (const char*)message->ioctl.buffer_in);
				if (!descriptor.System)
					return Errors::NotMounted;
				int ret = (int)descriptor.System->OpenDir(descriptor.Path);
				if (!ret)
					return Errors::NotOpened;
				return ret; }
			case Ioctl::CloseDir: {
				FileInfo* dir = (FileInfo*)buffer_in[0];
				return dir->System->CloseDir(dir); }
			case Ioctl::Shorten: {
				if (memcmp(message->ioctl.buffer_in, FILE_MODULE_NAME, FILE_MODULE_NAME_LENGTH))
					return Errors::Unrecognized;
				Dealloc(Long_Path);
				Long_Path = Alloc(message->ioctl.length_in);
				if (Long_Path) {
					memcpy(Long_Path, (u8*)(message->ioctl.buffer_in)+FILE_MODULE_NAME_LENGTH, message->ioctl.length_in-FILE_MODULE_NAME_LENGTH);
					memcpy(message->ioctl.buffer_io, FILE_MODULE_NAME SHORT_NAME, FILE_MODULE_NAME_LENGTH+strlen(SHORT_NAME)+1);
					os_sync_after_write(message->ioctl.buffer_io, FILE_MODULE_NAME_LENGTH+strlen(SHORT_NAME)+1);
					return Errors::Success;
				}
				return Errors::OutOfMemory; }
			case Ioctl::Log: {
				if (LogFS >= 0 && Mounted[LogFS] && message->ioctl.length_in)
					return Mounted[LogFS]->Log(message->ioctl.buffer_in, message->ioctl.length_in);
				return Errors::Success; }
			case Ioctl::GetFreeSpace:
				if (message->ioctl.length_in==sizeof(u32) && message->ioctl.length_io==sizeof(u64)) {
					FilesystemHandler *system = Mounted[buffer_in[0]];
					if (system) {
						u64 space;
						int ret = system->GetFreeSpace(&space);
						memcpy(message->ioctl.buffer_io, &space, sizeof(u64));
						os_sync_after_write(message->ioctl.buffer_io, sizeof(u64));
						return ret;
					}
					return Errors::NotMounted;
				}
				return Errors::Unrecognized;
#if 0 // testing only
			case Ioctl::Context: {
				static const char *exception_name[15] = {
						"System Reset", "Machine Check", "DSI", "ISI",
						"Interrupt", "Alignment", "Program", "Floating Point",
						"Decrementer", "System Call", "Trace", "Performance",
						"IABR", "Reserved", "Thermal"};
				int i;
				if (buffer_in[36]<15)
					PrintLog("%s Exception!\n", exception_name[buffer_in[36]]);
				else
					PrintLog("Unrecoverable Exception!\n");
				PrintLog("-------------- Exception 0x%08x Context 0x%08x ---------------\n", buffer_in[36], (u32)buffer_in|0x80000000);
				for (i=0; i < 16; i++)
					PrintLog("r%2d  = 0x%08x (%14d)  r%2d  = 0x%08x (%14d)\n", i, buffer_in[i], buffer_in[i], i+16, buffer_in[i+16], buffer_in[i+16]);
				PrintLog("LR   = 0x%08x                   CR   = 0x%08x\n", buffer_in[33], buffer_in[32]);
				PrintLog("SRR0 = 0x%08x                   SRR1 = 0x%08x\n", buffer_in[102], buffer_in[103]);
				PrintLog("DSISR= 0x%08x                   DAR  = 0x%08x\n", buffer_in[34], buffer_in[35]);
				PrintLog("Address:      Back Chain    LR Save\n");
				u32* stack = (u32*)(buffer_in[1]&0x3FFFFFFF); // r1 = sp
				for (i=0; i<16 && stack && ((s32)((u32)stack|0x80000000)+0x10000)!=-1; i++, stack = (u32*)(stack[0]&0x3FFFFFFF)) {
					os_sync_before_read(stack, 8);
					PrintLog("0x%08x:   0x%08x    0x%08x\n", (u32)stack|0x80000000, stack[0], stack[1]);
				}
				return Errors::Success; }
#endif
			default:
				return -1;
		}
	}

	void Filesystem::PrintLog(const char* fmt, ...)
	{
		static char buf[200] ATTRIBUTE_ALIGN(32);
		if (LogFS<0 || !Mounted[LogFS])
			return;

		va_list arg;
		va_start(arg, fmt);
		int len = _vsprintf(buf, fmt, arg);
		va_end(arg);

		Mounted[LogFS]->Log(buf, len);
	}

	int Filesystem::HandleIoctlv(ipcmessage* message)
	{
		for (u32 i = 0; i < message->ioctlv.num_in; i++)
			os_sync_before_read(message->ioctlv.vector[i].data, message->ioctlv.vector[i].len);
		switch (message->ioctlv.command) {
			case Ioctl::NextDir: {
				if (message->ioctlv.num_in != 1 || message->ioctlv.num_io != 2)
					return -1;
				FileInfo* dir = (FileInfo*)((u32*)message->ioctlv.vector[0].data)[0];
				int ret = dir->System->NextDir(dir, (char*)message->ioctlv.vector[1].data, (Stats*)message->ioctlv.vector[2].data);
				//os_sync_after_write(message->ioctlv.vector[1].data, message->ioctlv.vector[1].len);
				os_sync_after_write(message->ioctlv.vector[1].data, MAXPATHLEN);
				os_sync_after_write(message->ioctlv.vector[2].data, message->ioctlv.vector[2].len);
				return ret; }
			default:
				return -1;
		}
	}

	int Filesystem::HandleOpen(ipcmessage* message)
	{
		if (!strcmp(message->open.device, FILE_MODULE_NAME))
			return FILE_IOCTL_FD;

		FilePathDesc descriptor(this, message->open.device + FILE_MODULE_NAME_LENGTH);

		if (!descriptor.System)
			return Errors::NotMounted;

		int ret = (int)descriptor.System->Open(descriptor.Path, message->open.mode);

		Dealloc(Long_Path);
		Long_Path = NULL;

		if (!ret)
			return Errors::NotOpened;

		return ret;
	}

	int Filesystem::HandleClose(ipcmessage* message)
	{
		if (message->fd == FILE_IOCTL_FD)
			return 1;

		FileInfo* file = (FileInfo*)message->fd;
		if (!file)
			return Errors::NotOpened;
		return file->System->Close(file);
	}

	int Filesystem::HandleSeek(ipcmessage* message)
	{
		FileInfo* file = (FileInfo*)message->fd;
		if (!file)
			return Errors::NotOpened;
		switch (message->seek.origin) {
			case Ioctl::Tell:
				return file->System->Tell(file);
			case Ioctl::Sync:
				return file->System->Sync(file);
			default:
				return file->System->Seek(file, message->seek.offset, message->seek.origin);
		}
	}

	int Filesystem::HandleRead(ipcmessage* message)
	{
		FileInfo* file = (FileInfo*)message->fd;
		if (!file)
			return Errors::NotOpened;
		int ret = file->System->Read(file, (u8*)message->read.data, message->read.length);
		os_sync_after_write(message->read.data, message->read.length);
		return ret;
	}

	int Filesystem::HandleWrite(ipcmessage* message)
	{
		os_sync_before_read(message->write.data, message->write.length);
		FileInfo* file = (FileInfo*)message->fd;
		if (!file)
			return Errors::NotOpened;
		return file->System->Write(file, (const u8*)message->write.data, message->write.length);
	}

	bool Filesystem::HandleOther(u32 message, int &result, bool &ack)
	{
		if (message == FSIDLE_MSG)
		{
			os_stop_timer(Idle_Timer);
			for (int index = 0; index < FILE_MAX_MOUNTED; index++)
			{
				if (Mounted[index])
					Mounted[index]->IdleTick();
			}
			os_restart_timer(Idle_Timer, FSIDLE_TICK, 0);
			ack = false;
			return true;
		} else if (message == FSRTC_MSG) {
			RTC_Update();
			ack = false;
			return true;
		}

		return false;
	}

	FilePathDesc::FilePathDesc(Filesystem* module, const char* path)
	{
		Path = path;
		System = module->Mounted[module->Default];

		if (!strcmp(path, SHORT_NAME) && module->Long_Path)
			Path = (char*)(module->Long_Path);

		for (int i = 0; i < FILE_MAX_MOUNTED; i++) {
			FilesystemHandler* system = module->Mounted[i];
			if (!system)
				continue;
			int mountlength = strlen(system->MountPoint);
			if (!strncmp(Path, system->MountPoint, mountlength)) {
				System = system;
				Path += mountlength;
				return;
			}
		}
	}
} }
