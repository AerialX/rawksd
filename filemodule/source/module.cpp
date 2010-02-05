#include "module.h"

#include "file_fat.h"
#include "file_isfs.h"

static int tFd = 1;

namespace ProxiIOS { namespace Filesystem {
	Filesystem::Filesystem() : Module("file")
	{
		memset(OpenFiles, 0, sizeof(OpenFiles));
		Disk = NULL;
		Mounted = NULL;

		Timer_Init();

		Isfs = (new IsfsHandler())->Mount(NULL);
		tFd = 1;
	}

	int Filesystem::HandleIoctl(ipcmessage* message)
	{
		os_sync_before_read(message->ioctl.buffer_in, message->ioctl.length_in);

		switch (message->ioctl.command) {
			case Ioctl::InitDisc: {
				if (Mounted != NULL) {
					Mounted->Handler->Unmount(Mounted);
					delete Mounted->Handler;
					delete Mounted;
					Mounted = NULL;
				}
				if (Disk != NULL) {
					Disk->shutdown();
					Disk = NULL;
				}

				int disk = message->ioctl.buffer_in[0];
				switch (disk) {
					case Disks::SD:
						Disk = const_cast<DISC_INTERFACE*>(&__io_wiisd);
						break;
					case Disks::USB:
						Disk = const_cast<DISC_INTERFACE*>(&__io_usbstorage);
						break;
					//case Disks::USB2:
					//	Disk = const_cast<DISC_INTERFACE*>(&__io_usb2storage);
					//	break;
					default:
						return Errors::Unrecognized;
				}

				if (!Disk->startup()) {
					Disk = NULL;
					return Errors::DiskNotStarted;
				}
				if (!Disk->isInserted()) {
					Disk = NULL;
					return Errors::DiskNotInserted;
				}

				return Errors::Success;
				break; }
			case Ioctl::Mount: {
				if (Disk == NULL)
					return Errors::NotMounted;

				if (Mounted != NULL) {
					Mounted->Handler->Unmount(Mounted);
					delete Mounted->Handler;
					delete Mounted;
					Mounted = NULL;
				}

				FilesystemHandler* handler = NULL;
				int disk = message->ioctl.buffer_in[0];
				switch (disk) {
					case Filesystems::FAT:
						handler = new FatHandler();
						break;
					case Filesystems::ELM:
						break;
					case Filesystems::NTFS:
						break;
					case Filesystems::Ext2:
						break;
					case Filesystems::SMB:
						break;
				}

				if (handler == NULL)
					return Errors::Unrecognized;

				Mounted = handler->Mount(Disk);

				if (Mounted == NULL) {
					delete handler;
					return Errors::DiskNotMounted;
				}

				return Errors::Success; }
			case Ioctl::Stat: {
				int ret = Mounted->Handler->Stat((const char*)message->ioctl.buffer_in, (Stats*)message->ioctl.buffer_io);
				if (ret >= 0)
					os_sync_after_write(message->ioctl.buffer_io, message->ioctl.length_io);

				return ret; }
			case Ioctl::CreateFile:
				return Mounted->Handler->CreateFile((const char*)message->ioctl.buffer_in);
			case Ioctl::Delete:
				return Mounted->Handler->Delete((const char*)message->ioctl.buffer_in);
			case Ioctl::Rename:
				os_sync_before_read((void*)message->ioctl.buffer_in[0], IPC_MAXPATH_LEN);
				os_sync_before_read((void*)message->ioctl.buffer_in[1], IPC_MAXPATH_LEN);
				return Mounted->Handler->Rename((const char*)message->ioctl.buffer_in[0], (const char*)message->ioctl.buffer_in[1]);
			case Ioctl::CreateDir:
				return Mounted->Handler->CreateDir((const char*)message->ioctl.buffer_in);
			case Ioctl::OpenDir:
				return Mounted->Handler->OpenDir((const char*)message->ioctl.buffer_in);
			case Ioctl::NextDir: {
				int ret = Mounted->Handler->NextDir((int)message->ioctl.buffer_in[0], (char*)message->ioctl.buffer_io, (Stats*)message->ioctl.buffer_in[1]);
				//os_sync_after_write((void*)message->ioctl.buffer_in[1], strlen((const char*)message->ioctl.buffer_in[1]) + 1);
				os_sync_after_write((void*)message->ioctl.buffer_io, message->ioctl.length_io);
				os_sync_after_write((void*)message->ioctl.buffer_in[1], sizeof(Stats));
				return ret; }
			case Ioctl::CloseDir:
				return Mounted->Handler->CloseDir((int)message->ioctl.buffer_in[0]);
			default:
				return -1;
		}
	}

	int Filesystem::HandleOpen(ipcmessage* message)
	{
		// If it exactly matches "file" then give an fd for ioctls (though any fd will technically do)
		if (!strcmp(message->open.device, "file"))
			//return Module::HandleOpen(message);
			return tFd++;

		if (Mounted == NULL)
			return Errors::NotMounted;

		if (strstr(message->open.device, "isfs\\")) {
			return (int)Isfs->Handler->Open(NULL, message->open.device + 4 + 5, message->open.mode);
		}

		// Strip "file" from the path
		return (int)Mounted->Handler->Open(Mounted, message->open.device + 4, message->open.mode);
	}

	int Filesystem::HandleClose(ipcmessage* message)
	{
		//if ((int)message->fd == Fd)
			//return Module::HandleClose(message);
		if (message->fd < 0x20)
			return 1;

		FileInfo* file = (FileInfo*)message->fd;
		return file->System->Handler->Close(file);
	}

	int Filesystem::HandleSeek(ipcmessage* message)
	{
		FileInfo* file = (FileInfo*)message->fd;
		switch (message->seek.origin) {
			case Ioctl::Tell:
				return file->System->Handler->Tell(file);
			case Ioctl::Sync:
				return file->System->Handler->Sync(file);
			default:
				return file->System->Handler->Seek(file, message->seek.offset, message->seek.origin);
		}
	}

	int Filesystem::HandleRead(ipcmessage* message)
	{
		FileInfo* file = (FileInfo*)message->fd;
		int ret = file->System->Handler->Read(file, (u8*)message->read.data, message->read.length);
		os_sync_after_write(message->read.data, message->read.length);
		return ret;
	}

	int Filesystem::HandleWrite(ipcmessage* message)
	{
		os_sync_before_read(message->write.data, message->write.length);
		FileInfo* file = (FileInfo*)message->fd;
		return file->System->Handler->Write(file, (const u8*)message->write.data, message->write.length);
	}
} }
