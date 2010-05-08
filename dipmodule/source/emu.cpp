#include "emu.h"

#include <files.h>
#include <mem.h>

#include <ch341.h>
#include <print.h>

u32 FS_ioctl_vect=0;
u32 FS_ret;

static char str[1024];

extern "C" void LogPrintf(const char *fmt, ...)
{
	int heapfree = HeapInfo();
	va_list arg;
	_sprintf(str, "(%X): ", heapfree);
	ch341_send(str, strlen(str));

	va_start(arg, fmt);
	_vsprintf(str, fmt, arg);
	va_end(arg);
	ch341_send(str, strlen(str));
}

void LogMessage(ipcmessage* message)
{
	switch (message->command) {
		case IOS_OPEN:
			if (!strncmp(message->open.device, "/dev/fs", 7) || strncmp(message->open.device, "/dev", 4)) {
				LogPrintf("IOS_Open: \"%s\" - %d - %d - %04X\n", message->open.device, message->open.mode, message->open.uid, message->open.gid);
				LogPrintf("Potential FD: %08X\n", message->result);
			}
			break;
		case IOS_CLOSE:
			LogPrintf("IOS_Close: 0x%X\n", message->fd);
			break;
		case IOS_READ:
			LogPrintf("IOS_Read: %p 0x%X\n", message->fd, message->read.length);
			break;
		case IOS_WRITE:
			LogPrintf("IOS_Write: 0x%X\n", message->write.length);
			break;
		case IOS_SEEK:
			LogPrintf("IOS_Seek: %p 0x%X - 0x%X\n", message->fd, message->seek.offset, message->seek.origin);
			break;
		case IOS_IOCTL:
			LogPrintf("IOS_Ioctl: %d - 0x%X\n", message->ioctl.command, message->fd);
			break;
		case IOS_IOCTLV:
			LogPrintf("IOS_Ioctlv: fd=0x%X, command=0x%X\n", message->fd, message->ioctlv.command);
			break;
		default:
			LogPrintf("Dunno what ipc msg this is: %d (fd=0x%X)\n", message->command, message->fd);
	}
}

static s32 emu_fd = -1;
static osqueue_t fs_internal_queue = -1;
static u32 fs_internal_msgs[8];
static ipcmessage ProxyMessage;
static ipcmessage CBMessage;
typedef s32 (*NANDFS_Func)(const ipcmessage*);

// IOS37 specific stuff
static const struct ProxiIOS::EMU::ISFSFile *FS_Files = (struct ProxiIOS::EMU::ISFSFile*)0x200499A4;
static const NANDFS_Func NAND_Funcs[7] = {
	// these are thumb functions so add 1 to the pointers
	(NANDFS_Func)(0x200055A8+1), // handle_fs_open
	(NANDFS_Func)(0x20005D34+1), // handle_fs_close
	(NANDFS_Func)(0x2000564C+1), // handle_fs_read
	(NANDFS_Func)(0x200057A8+1), // handle_fs_write
	(NANDFS_Func)(0x2000598C+1), // handle_fs_seek
	(NANDFS_Func)(0x200059E8+1), // handle_fs_ioctl
	(NANDFS_Func)(0x20005C4C+1), // handle_fs_ioctlv
};

static int FilesystemHook(const ipcmessage* message, int* result)
{
	// this should never (can't?) happen
	if (!message)
		return 0;

	if (fs_internal_queue >= 0 && emu_fd >= 0) {
		memcpy(&ProxyMessage, message, sizeof(ipcmessage));
		// get the pointer that will be the fd if /dev/fs opens this device/file
		if (message->command==IOS_OPEN) {
			for (int i=0; i < MAX_EMU_OPEN; i++) {
				if (!FS_Files[i].in_use) {
					ProxyMessage.result = (u32)(FS_Files+i);
					break;
				}
			}
		}

		if (os_ioctl_async(emu_fd, ProxiIOS::EMU::Ioctl::FSMessage, &ProxyMessage, sizeof(ipcmessage), result, result ? sizeof(*result) : 0, fs_internal_queue, &CBMessage)>=0) {
			while (os_message_queue_receive(fs_internal_queue, (u32*)&message, 0)==0) {
				s32 result = -1;

				if (message->command==IOS_IOCTL && message->ioctl.command==ProxiIOS::EMU::Ioctl::NANDFSMessage) {
					// call internal /dev/fs functions
					ipcmessage *emu_message = (ipcmessage*)message->ioctl.buffer_in;
					result = NAND_Funcs[emu_message->command-1](emu_message);

				} else if (message->command==IOS_OPEN && !strcmp(message->open.device, FS_INTERNAL_NAME))
					result = 1;
				else if (message->command==IOS_CLOSE && message->fd==1)
					result = 0;
				/* note that emu only opens this device when it has to,
				 * rather than keeping a handle constantly open */

				os_message_queue_ack(message, result);
				if (&CBMessage==message)
					return message->result;
			}
		}
		// somehow the queue died or the async ioctl failed
		return 0;
	}

	if (fs_internal_queue < 0) {
		fs_internal_queue = os_message_queue_create(fs_internal_msgs, sizeof(fs_internal_msgs)/sizeof(u32));
		if (fs_internal_queue >= 0) {
			// emu calls this device when it wants to access the nand fs
			os_device_register(FS_INTERNAL_NAME, fs_internal_queue);
			// open emu so we have a handle to send requests to
			emu_fd = os_open(EMU_MODULE_NAME, 0);
		}
	}

	return 0;
}

namespace ProxiIOS { namespace EMU {
	EMU::EMU() : Module(EMU_MODULE_NAME)
	{
		ch341_open();
		memset(open_files, 0, sizeof(open_files));

		const int stacksize = 0x2000;
		u8* stack = (u8*)Memalign(32, stacksize);

		stack += stacksize;
		loop_thread = os_thread_create(emu_thread, this, stack, stacksize, 0x48, 0);
		LogPrintf("EMU thread created\n");
	}

	u32 EMU::emu_thread(void* _p) {
		ProxiIOS::EMU::EMU *p = (ProxiIOS::EMU::EMU*)_p;

		LogPrintf("EMU thread started\n");

		s32 fd = os_open("/dev/fs", 0);
		if (fd >= 0) {
			FS_ioctl_vect = (u32)FilesystemHook;
			os_ioctl(fd, ProxiIOS::EMU::Ioctl::ActivateHook, NULL, 0, NULL, 0);
			LogPrintf("FS ioctl called on fd %d\n", fd);
			// this close call will open "emu" inside FilesystemHook and setup other stuff
			os_close_async(fd, p->queuehandle, &ProxyMessage);
		}
		else // something is very wrong here
			return 0;

		return (u32)p->Loop();
	}

	int EMU::Start()
	{
		return os_thread_continue(loop_thread);
	}

	s32 EMU::FS_IPC(ipcmessage *msg)
	{
		s32 ret, fd = os_open(FS_INTERNAL_NAME, 0);
		if (fd < 0) {
			LogPrintf("FS_IPC failed to open %d\n", fd);
			return fd;
		} else
			LogPrintf("FS_IPC opened %d\n", fd);

		ret = os_ioctl(fd, Ioctl::NANDFSMessage, msg, sizeof(ipcmessage), NULL, 0);
		os_close(fd);
		return ret;
	}

	s32 EMU::FS_IPC(const char *device, u32 mode, u32 uid, u16 gid)
	{
		ipcmessage msg;

		msg.command = Ios::Open;
		msg.result = 0;
		msg.fd = 0;
		msg.open.device = device;
		msg.open.mode = mode;
		msg.open.uid = uid;
		msg.open.gid = gid;

		return FS_IPC(&msg);
	}

	s32 EMU::FS_IPC(s32 fd)
	{
		ipcmessage msg;

		msg.command = Ios::Close;
		msg.fd = fd;

		return FS_IPC(&msg);
	}

	s32 EMU::FS_IPC(s32 fd, void* data, u32 length)
	{
		ipcmessage msg;

		msg.command = Ios::Read;
		msg.fd = fd;
		msg.read.data = data;
		msg.read.length = length;

		return FS_IPC(&msg);
	}

	s32 EMU::FS_IPC(s32 fd, const void* data, u32 length)
	{
		ipcmessage msg;

		msg.command = Ios::Write;
		msg.fd = fd;
		msg.write.data = data;
		msg.write.length = length;

		return FS_IPC(&msg);
	}

	s32 EMU::FS_IPC(s32 fd, s32 offset, s32 origin)
	{
		ipcmessage msg;

		msg.command = Ios::Seek;
		msg.fd = fd;
		msg.seek.offset = offset;
		msg.seek.origin = origin;

		return FS_IPC(&msg);
	}

	s32 EMU::FS_IPC(s32 fd, u32 command, const void* buffer_in, u32 length_in, void* buffer_io, u32 length_io)
	{
		ipcmessage msg;

		msg.command = Ios::Ioctl;
		msg.fd = fd;
		msg.ioctl.command = command;
		msg.ioctl.buffer_in = buffer_in;
		msg.ioctl.length_in = length_in;
		msg.ioctl.buffer_io = buffer_io;
		msg.ioctl.length_io = length_io;

		return FS_IPC(&msg);
	}

	s32 EMU::FS_IPC(s32 fd, u32 command, u32 num_in, u32 num_io, ioctlv* vector)
	{
		ipcmessage msg;

		msg.command = Ios::Ioctlv;
		msg.fd = fd;
		msg.ioctlv.command = command;
		msg.ioctlv.num_in = num_in;
		msg.ioctlv.num_io = num_io;
		msg.ioctlv.vector = vector;

		return FS_IPC(&msg);
	}

	int EMU::HandleIoctl(ipcmessage* message)
	{
		switch (message->ioctl.command) {
			case Ioctl::FSMessage:
				return HandleFSMessage((ipcmessage*)message->ioctl.buffer_in, (int*)message->ioctl.buffer_io);
			default:
				LogPrintf("wtf emu ioctl is this: %d\n", message->ioctl.command);
		}
		return -1;
	}

	RiivFile* EMU::TryOpen(const char *name, u32 mode)
	{
		if (!strcmp(name, "/title/00010005/735a4145/content/00000043.app")) {
			LogPrintf("Opening 067.bin\n");
			return new AppFile("/private/wii/data/sZAE/067.bin");
		} else if (!strcmp(name, "/title/00010005/735a4145/content/00000044.app")) {
			LogPrintf("Opening 068.bin\n");
			return new AppFile("/private/wii/data/sZAE/068.bin");
		}

		return NULL;
	}

	int EMU::HandleFSMessage(ipcmessage* message, int* result)
	{
		int i;
		LogMessage(message);

		if (message->command == Ios::Open) {
			// find next free slot
			for (i=0; i < MAX_EMU_OPEN; i++) {
				if (open_files[i]==NULL)
					break;
			}
			if (i==MAX_EMU_OPEN)
				return 0;

			RiivFile *f = TryOpen(message->open.device, message->open.mode);
			if (f) {
				LogPrintf("Got RiivFile, %p %d\n", f, i);
				open_files[i] = f;
				*result = (u32)f;
				return 1;
			}

			return 0;
		}

		// check if it's one of our files
		for (i=0; i < MAX_EMU_OPEN; i++) {
			if ((u32)open_files[i] == message->fd) {
				LogPrintf("open_file %d\n", i);
				switch (message->command) {
					case Ios::Close:
						delete open_files[i];
						open_files[i] = NULL;
						*result = 0;
						break;
					case Ios::Read:
						*result = open_files[i]->Read(message->read.data, message->read.length);
						break;
					case Ios::Write:
						*result = open_files[i]->Write(message->write.data, message->write.length);
						break;
					case Ios::Seek:
						*result = open_files[i]->Seek(message->seek.offset, message->seek.origin);
						break;
					case Ios::Ioctl:
						if (message->ioctl.command==Ioctl::GetFileStats && message->ioctl.length_io>=8) {
							s32 *stats = (s32*)message->ioctl.buffer_io;
							// position
							stats[1] = open_files[i]->Seek(0, SEEK_CUR);
							// length
							stats[0] = open_files[i]->Seek(0, SEEK_END);
							// restore
							LogPrintf("GetFileStats, length %d pos %d\n", stats[0], stats[1]);
							open_files[i]->Seek(stats[1], SEEK_SET);
							*result = FSErrors::OK;
						}
					default:
						*result = FSErrors::InvalidArgument;
				}
				return 1;
			}
		}

		// otherwise assume the fd is /dev/fs and handle only suitable commands
		if (message->command == Ios::Ioctl || message->command == Ios::Ioctlv) {
			s32 ret;
			ISFS::FSattr *attrib = (ISFS::FSattr*)message->ioctl.buffer_in;
			switch (message->ioctl.command) {
				case Ioctl::Format:
					LogPrintf("ISFS Format\n");
					break;
				case Ioctl::GetStats:
					LogPrintf("ISFS GetStats\n");
					break;
				case Ioctl::CreateDir:
					LogPrintf("ISFS CreateDir %s %d %d %d %d\n", attrib->path, attrib->attributes, attrib->ownerperm, attrib->groupperm, attrib->otherperm);
					break;
				case Ioctl::ReadDir:
					LogPrintf("ISFS ReadDir %s %d\n", message->ioctlv.vector[0].data, message->ioctlv.num_in);
					break;
				case Ioctl::SetAttrib:
					LogPrintf("ISFS SetAttrib %s %d %d %d %d\n", attrib->path, attrib->attributes, attrib->ownerperm, attrib->groupperm, attrib->otherperm);
					break;
				case Ioctl::GetAttrib:
					LogPrintf("ISFS GetAttrib %s\n", (char*)message->ioctl.buffer_in);
					break;
				case Ioctl::Delete:
					LogPrintf("ISFS Delete %s\n", (char*)message->ioctl.buffer_in);
					break;
				case Ioctl::Move:
					LogPrintf("ISFS Move %s -> %s\n", (char*)message->ioctl.buffer_in, (char*)message->ioctl.buffer_in+ISFS_MAXPATH_LEN);
					break;
				case Ioctl::CreateFile:
					LogPrintf("ISFS CreateFile %s %d %d %d %d\n", attrib->path, attrib->attributes, attrib->ownerperm, attrib->groupperm, attrib->otherperm);
					break;
				case Ioctl::SetFileVerCtrl:
					LogPrintf("ISFS SetFileVerCtrl\n");
					break;
				case Ioctl::GetFileStats: // this is called for files, not /dev/fs
					LogPrintf("ISFS GetFileStats %p %d\n", message->ioctl.buffer_io, message->ioctl.length_io);
					ret = FS_IPC(message->fd, message->ioctl.command, message->ioctl.buffer_in, message->ioctl.length_in, message->ioctl.buffer_io, message->ioctl.length_io);
					if (ret>=0) {
						s32 *stats = (s32*)message->ioctl.buffer_io;
						LogPrintf("Length %x, Pos %x\n", stats[0], stats[1]);
					} else
						LogPrintf("FS_IPC failed, %d\n", ret);
					return 0;
				case Ioctl::GetUsage:
					LogPrintf("ISFS GetUsage %s\n", (char*)message->ioctlv.vector[0].data);
					break;
				case Ioctl::Shutdown:
					LogPrintf("ISFS Shutdown\n");
					break;
			}
		}
		return 0;
	}

	s32 RiivFile::Open()
	{
		if (file<0) {
			file = File_Open(file_name, file_mode);
			LogPrintf("Post-Open %s returned %d\n", file_name, file);
		}
		return file;
	}

	s32 RiivFile::Read(void *dest, s32 length)
	{
		if (file<0 && Open()<0)
			return FSErrors::IOError;

		return File_Read(file, dest, length);
	}

	s32 RiivFile::Write(const void *src, s32 length)
	{
		if (file<0 && Open()<0)
			return FSErrors::IOError;

		return File_Write(file, src, length);
	}

	s32 RiivFile::Seek(s32 where, s32 whence)
	{
		if (file<0 && Open()<0)
			return FSErrors::IOError;

		return File_Seek(file, where, whence);
	}

	int RiivFile::IsWatching(u32 fd)
	{
		return !!(watched_fd==fd);
	}

	RiivFile::RiivFile(const char *name, s32 mode, u32 watching)
	{
		file_name = (char*)Alloc(strlen(name)+1);
		strcpy(file_name, name);
		file_mode = mode;

		file = -1;
		watched_fd = watching;
	}

	RiivFile::~RiivFile()
	{
		Dealloc(file_name);

		if (file >= 0)
			File_Close(file);
	}

	s32 AppFile::Open()
	{
		if (binfile==NULL)
		{
			if (file<0)
				RiivFile::Open();

			if (file>=0)
			{
				// TODO: CreateBinFile
				binfile = OpenBinRead(file);
				if (binfile==NULL)
				{
					File_Close(file);
					file = -1;
				}
			}
		}
		return file;
	}

	s32 AppFile::Read(void *dest, s32 length)
	{
		if (file<0 && Open()<0)
			return FSErrors::IOError;

		return ReadBin(binfile, (u8*)dest, length);
	}

	s32 AppFile::Write(const void *src, s32 length)
	{
		if (file<0 && Open()<0)
			return FSErrors::IOError;

		return WriteBin(binfile, (const u8*)src, length);
	}

	s32 AppFile::Seek(s32 where, s32 whence)
	{
		if (file<0 && Open()<0)
			return FSErrors::IOError;

		return SeekBin(binfile, where, (u32)whence);
	}

	AppFile::AppFile(const char *name) :
	RiivFile(name, O_RDONLY)
	{
		binfile = NULL;
	}

	AppFile::~AppFile()
	{
		if (binfile)
			CloseBin(binfile);
	}

} }
