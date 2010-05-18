#include "emu.h"
#include "es.h"

#include <files.h>
#include <mem.h>

#if 0
#include <ch341.h>
#include <print.h>

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
				if (strncmp(message->open.device, "/title/00010005/735a", 20))
					LogPrintf("IOS_Open: \"%s\" - %d - %d - %04X (%08X)\n", message->open.device, message->open.mode, message->open.uid, message->open.gid, message->result);
			}
			break;
		case IOS_CLOSE:
			LogPrintf("IOS_Close: 0x%X\n", message->fd);
			break;
		case IOS_READ:
			LogPrintf("IOS_Read: %p 0x%X\n", message->fd, message->read.length);
			break;
		case IOS_WRITE:
			LogPrintf("IOS_Write: %p 0x%X\n", message->fd, message->write.length);
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

#else
#define LogPrintf(...)
#define ch341_open()
#define LogMessage(a)
#endif

u32 FS_ioctl_vect=0;
u32 FS_ret;

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
	static s32 FS_IPC(ipcmessage *msg)
	{
		s32 ret, fd = os_open(FS_INTERNAL_NAME, 0);
		if (fd < 0)
			return fd;

		ret = os_ioctl(fd, Ioctl::NANDFSMessage, msg, sizeof(ipcmessage), NULL, 0);
		os_close(fd);
		return ret;
	}

	static s32 FS_Open(const char *device, u32 mode, u32 uid=0, u16 gid=0)
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

	static s32 FS_Close(s32 fd)
	{
		ipcmessage msg;

		msg.command = Ios::Close;
		msg.fd = fd;

		return FS_IPC(&msg);
	}

	static s32 FS_Read(s32 fd, void* data, u32 length)
	{
		ipcmessage msg;

		msg.command = Ios::Read;
		msg.fd = fd;
		msg.read.data = data;
		msg.read.length = length;

		return FS_IPC(&msg);
	}

	static s32 FS_Write(s32 fd, const void* data, u32 length)
	{
		ipcmessage msg;

		msg.command = Ios::Write;
		msg.fd = fd;
		msg.write.data = data;
		msg.write.length = length;

		return FS_IPC(&msg);
	}

	static s32 FS_Seek(s32 fd, s32 offset, s32 origin)
	{
		ipcmessage msg;

		msg.command = Ios::Seek;
		msg.fd = fd;
		msg.seek.offset = offset;
		msg.seek.origin = origin;

		return FS_IPC(&msg);
	}

	static s32 FS_Ioctl(s32 fd, u32 command, const void* buffer_in, u32 length_in, void* buffer_io, u32 length_io)
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

	static s32 FS_Ioctlv(s32 fd, u32 command, u32 num_in, u32 num_io, ioctlv* vector)
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

	EMU::EMU() : Module(EMU_MODULE_NAME)
	{
		ch341_open();
		memset(open_files, 0, sizeof(open_files));

		const int stacksize = 0x2000;
		u8* stack = (u8*)Memalign(32, stacksize);

		stack += stacksize;
		loop_thread = os_thread_create(emu_thread, this, stack, stacksize, 0x48, 0);

		/* FIXME: Put these somewhere sensible, they don't belong here
		 * and they only work if Riivolution doesn't unmount the usb drive */

		RiivDir *d = new AppDir("/title/00010005/735a4145/content", "/mnt/usb/private/wii/data/sZAE");
		if (d) {
			LogPrintf("Added custom DLC path\n");
			DataDirs.push_back(d);
		}
		d = new AppDir("/title/00010005/735a4245/content", "/mnt/usb/private/wii/data/sZBE");
		if (d) {
			LogPrintf("Added custom DLC path\n");
			DataDirs.push_back(d);
		}
		d = new AppDir("/title/00010005/735a4345/content", "/mnt/usb/private/wii/data/sZCE");
		if (d) {
			LogPrintf("Added custom DLC path\n");
			DataDirs.push_back(d);
		}
		d = new AppDir("/title/00010005/735a4445/content", "/mnt/usb/private/wii/data/sZDE");
		if (d) {
			LogPrintf("Added custom DLC path\n");
			DataDirs.push_back(d);
		}
#if 0
		d = new AppDir("/title/00010005/63524241/content", "/mnt/sd/private/wii/data/cRBA");
		if (d) {
			LogPrintf("Added custom DLC path\n");
			DataDirs.push_back(d);
		}
#endif
	}

	u32 EMU::emu_thread(void* _p) {
		ProxiIOS::EMU::EMU *p = (ProxiIOS::EMU::EMU*)_p;

		s32 fd = os_open("/dev/fs", 0);
		if (fd >= 0) {
			FS_ioctl_vect = (u32)FilesystemHook;
			os_ioctl(fd, ProxiIOS::EMU::Ioctl::ActivateHook, NULL, 0, NULL, 0);
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

	int EMU::HandleIoctl(ipcmessage* message)
	{
		if (message->ioctl.command == Ioctl::FSMessage)
			return HandleFSMessage((ipcmessage*)message->ioctl.buffer_in, (int*)message->ioctl.buffer_io);
		LogPrintf("EMU: Unknown ioctl %u\n", message->ioctl.command);
		return -1;
	}

	int EMU::HandleIoctlv(ipcmessage* message)
	{
		if (message->ioctlv.command == Ioctl::RedirectDir && message->ioctlv.num_in>=2) {
			LogPrintf("New EMU dir, %s -> %s\n", message->ioctlv.vector[0].data, message->ioctlv.vector[1].data);
			RiivDir *d = new RiivDir((const char*)message->ioctlv.vector[0].data, (const char*)message->ioctlv.vector[1].data);
			if (d) {
				DataDirs.push_back(d);
			}
			return 1;
		}
		return -1;
	}

	int EMU::TryOpen(const char *name, u32 mode, RiivFile **x)
	{
		std::vector<RiivDir*>::iterator it = DataDirs.begin();
		for (;it != DataDirs.end(); it++) {
			char *path = (*it)->GetTranslatedPath(name);
			if (path) {
				//LogPrintf("TryOpen translated filename: %s\n", path);
				if ((*it)->Exists(path)>=0)
					*x = (*it)->OpenFile(path, mode);
				Dealloc(path);
				return 1;
			}
		}

		return -1;
	}

	int EMU::HandleFSMessage(ipcmessage* message, int* result)
	{
		int i;
		// this is commented out because it's too noisy
		//LogMessage(message);

		if (message->command == Ios::Open) {
			// find next free slot
			for (i=0; i < MAX_EMU_OPEN; i++) {
				if (open_files[i]==NULL)
					break;
			}
			if (i==MAX_EMU_OPEN)
				return 0;

			RiivFile *f=NULL;
			if (TryOpen(message->open.device, message->open.mode, &f)>=0) {
				if (f) {
					open_files[i] = f;
					*result = (u32)f;
				} else {
					LogPrintf("File not found\n");
					*result = FSErrors::FileNotFound;
				}
				return 1;
			}

			return 0;
		}

		// check if it's one of our files
		for (i=0; i < MAX_EMU_OPEN; i++) {
			if ((u32)open_files[i] == message->fd) {
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
						if (message->ioctl.command==Ioctl::GetFileStats && message->ioctl.length_io>=sizeof(ISFS::Stats)) {
							ISFS::Stats *stats = (ISFS::Stats*)message->ioctl.buffer_io;
							stats->Pos = open_files[i]->Seek(0, SEEK_CUR);
							stats->Length = open_files[i]->Seek(0, SEEK_END);
							open_files[i]->Seek(stats->Pos, SEEK_SET);
							*result = FSErrors::OK;
							break;
						}
					default:
						LogPrintf("Unhandled FS IPC %u\n", message->command);
						*result = FSErrors::InvalidArgument;
				}

				return 1;
			}
		}

		// otherwise assume the fd is /dev/fs and handle only suitable commands
		if (message->command == Ios::Ioctl || message->command == Ios::Ioctlv) {
			ISFS::FSattr *attrib = (ISFS::FSattr*)message->ioctl.buffer_in;
			const char *path;
			const char *path2 = NULL;
			std::vector<RiivDir*>::iterator it;

			switch (message->ioctl.command) {
				case Ioctl::CreateDir:
				case Ioctl::SetAttrib:
				case Ioctl::CreateFile:
					path = attrib->path;
					break;
				case Ioctl::GetAttrib:
				case Ioctl::Delete:
					path = (const char*)message->ioctl.buffer_in;
					break;
				case Ioctl::ReadDir:
				case Ioctl::GetUsage:
					path = (const char*)message->ioctlv.vector[0].data;
					break;
				case Ioctl::Move:
					path = (const char*)message->ioctl.buffer_in;
					path2 = path+ISFS_MAXPATH_LEN;
					break;
				default:
					return 0;
			}

			for(it = DataDirs.begin();it != DataDirs.end(); it++) {
				Stats st;
				char *new_path = (*it)->GetTranslatedPath(path);
				char *new_path2 = NULL;
				if (path2)
					new_path2 = (*it)->GetTranslatedPath(path2);
				if (new_path || new_path2) {
					if (new_path) {
						os_sync_after_write(new_path, strlen(new_path)+1);
						LogPrintf("Translated Path: %s\n", new_path);
					}
					if (new_path2) {
						os_sync_after_write(new_path2, strlen(new_path2)+1);
						LogPrintf("Translated Path2: %s\n", new_path2);
					}

					switch (message->ioctl.command) {
						case Ioctl::GetAttrib:
							if (message->ioctl.length_io < sizeof(ISFS::FSattr)) {
								*result = FSErrors::InvalidArgument;
								break;
							}
							attrib = (ISFS::FSattr*)message->ioctl.buffer_io;
							attrib->owner_id = 0;
							attrib->group_id = *(u16*)4; // read from disc id
							attrib->ownerperm = 3;
							attrib->groupperm = 3;
							attrib->otherperm = 3;
							attrib->attributes = 0;
							// fallthrough
						case Ioctl::SetAttrib:
							if (File_Stat(new_path, &st)<0)
								*result = FSErrors::FileNotFound;
							else
								*result = FSErrors::OK;
							break;
						case Ioctl::CreateDir:
							*result = File_CreateDir(new_path);
							break;
						case Ioctl::CreateFile:
							*result = (*it)->CreateFile(new_path);
							break;
						case Ioctl::Delete:
							*result = (*it)->Delete(new_path);
							break;
						case Ioctl::ReadDir:
							if (File_Stat(new_path, &st)<0)
								*result = FSErrors::FileNotFound;
							else {
								u32 *out_count;
								char *names = NULL;
								const u32 *max_count = NULL;
								if (message->ioctlv.num_in==2 && message->ioctlv.num_io==2) {
									names = (char*)message->ioctlv.vector[2].data;
									max_count = (u32*)message->ioctlv.vector[1].data;
									out_count = (u32*)message->ioctlv.vector[3].data;
									os_sync_before_read(max_count, sizeof(u32));
								} else if (message->ioctlv.num_in!=1 || message->ioctlv.num_io!=1) {
									*result = FSErrors::InvalidArgument;
									break;
								} else
									out_count = (u32*)message->ioctlv.vector[1].data;
								*result = (*it)->ReadDir(new_path, out_count, names, max_count);
								os_sync_after_write(out_count, sizeof(u32));
								if (names && out_count[0])
									os_sync_after_write(names, 13*out_count[0]);
							}
							break;
						case Ioctl::Move:
							if (new_path && new_path2) {
								*result = File_Rename(new_path, new_path2);
							} else if (new_path) {
								*result = (*it)->MoveFrom(new_path, path2);
							} else if (new_path2) {
								*result = (*it)->MoveTo(path, new_path2);
							} else {
								*result = FSErrors::InvalidArgument;
							}
							break;
						case Ioctl::GetUsage:
							if (message->ioctlv.num_in!=1 || message->ioctlv.num_io!=2) {
								*result = FSErrors::InvalidArgument;
								break;
							} else {
								u32 *blocks = (u32*)message->ioctlv.vector[1].data;
								u32 *files = (u32*)message->ioctlv.vector[2].data;
								char *next_name = (char*)Alloc(1024);
								if (next_name==NULL)
									*result = FSErrors::OutOfMemory;
								else {
									*blocks = 0;
									*files = 1;
									*result = (*it)->GetUsage(new_path, files, blocks, next_name);
									if (*result>=0) {
										*blocks >>= 14; // 1 block = 0x4000 bytes
										os_sync_after_write(blocks, sizeof(u32));
										os_sync_after_write(files, sizeof(u32));
										LogPrintf("Usage for %s: %u files, %u blocks\n", new_path, *files, *blocks);
									}
									Dealloc(next_name);
								}
							}
							break;
						default:
							*result = FSErrors::InvalidArgument;
					}
					if (*result > 0)
						*result = FSErrors::OK;

					LogPrintf("Returning %d\n", *result);

					Dealloc(new_path);
					Dealloc(new_path2);
					return 1;
				}
			}
		}
		return 0;
	}

	s32 RiivFile::Open()
	{
		if (file<0) {
			LogPrintf("Post-opening file: %s\n", file_name);
			file = File_Open(file_name, file_mode);
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

	RiivFile::RiivFile(const char *name, s32 mode)
	{
		file_name = (char*)Alloc(strlen(name)+1);
		strcpy(file_name, name);
		file_mode = mode;

		file = -1;
	}

	RiivFile::~RiivFile()
	{
		Dealloc(file_name);

		if (file >= 0)
			File_Close(file);
	}

	s32 AppFile::Open()
	{
		if (binfile==NULL) {
			if (file<0)
				RiivFile::Open();

			if (file>=0) {
				binfile = OpenBinRead(file);

				if (binfile==NULL) {
					File_Close(file);
					file = -1;
				} else
					LogPrintf("Post-opened BIN file\n");
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
		if (file<0)
			return FSErrors::IOError;

		return WriteBin(binfile, (u8*)src, length);
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

	AppFile::AppFile(const char *name, u16 index, u32 *tmd_buf) :
	RiivFile(name, O_CREAT|O_TRUNC|O_WRONLY)
	{
		RiivFile::Open();
		if (file>=0)
			binfile = CreateBinFile(index, (u8*)tmd_buf, SIGNED_TMD_SIZE(tmd_buf), file);
	}

	AppFile::~AppFile()
	{
		if (binfile)
			CloseBin(binfile);
	}

	char* RiivDir::GetTranslatedPath(const char *path)
	{
		char *new_path = NULL;
		if (!strncmp(path, nand_dir, strlen(nand_dir)))
			new_path = (char*)Memalign(32, strlen(ext_dir)+strlen(path)-strlen(nand_dir)+1);
		if (new_path) {
			strcpy(new_path, ext_dir);
			strcat(new_path, path+strlen(nand_dir));
		}
		return new_path;
	}

	RiivFile* RiivDir::OpenFile(const char *path, int mode)
	{
		mode = mode&3;
		if (mode)
			mode--;
		return new RiivFile(path, mode);
	}

	int RiivDir::CreateFile(const char *path)
	{
		return File_CreateFile(path);
	}

	int RiivDir::Delete(const char *path)
	{
		return File_Delete(path);
	}

	int RiivDir::ReadDir(const char* ext_path, u32 *out_count, char *names, const u32 *max_count)
	{
		Stats st;
		int ret = FSErrors::OK;
		char *next_name = (char*)Memalign(32, 1024);
		if (next_name==NULL)
			return FSErrors::OutOfMemory;
		char *out_names=NULL, *out=NULL;
		// use a temp variable, because out_count and max_count may point to the same thing
		u32 count = 0;
		if (names && max_count[0]) {
			// allocate temporary memory for the filenames to work around MEM1 word restriction
			out_names = (char*)Alloc(13*max_count[0]);
			if (out_names==NULL) {
				Dealloc(next_name);
				return FSErrors::OutOfMemory;
			}
			out = out_names;
		}

		s32 dir = File_OpenDir(ext_path);
		if (dir<0) {
			Dealloc(next_name);
			Dealloc(out_names);
			return FSErrors::InvalidArgument;
		}

		while (File_NextDir(dir, next_name, &st)>=0) {
			if (st.Mode&S_IFDIR && next_name[0]=='.')
				continue;

			if (out) {
				if (count==max_count[0]) {
					ret = FSErrors::TooManyFiles;
					break;
				} else {
					out[12] = 0;
					strncpy(out, next_name, 12); // maximum ISFS filename is 12 chars
					out += strlen(out)+1;
				}
			}
			count++;
		}
		*out_count = count;

		if (names && count) {
			memcpy(names, out_names, ((out-out_names)+3)&~3);
			LogPrintf("ReadDir: %s has %u files, %u names written\n", ext_path, *max_count, *out_count);
		}
		else
			LogPrintf("ReadDir: %s has %u files.\n", ext_path, *out_count);

		File_CloseDir(dir);
		Dealloc(next_name);
		Dealloc(out_names);

		return ret;
	}

	int RiivDir::MoveTo(const char* nand_path, const char* ext_path)
	{
		void *buf;
		int ret = FSErrors::InvalidArgument;
		RiivFile *dest=NULL;

		LogPrintf("Moving %s from NAND to external path: %s\n", nand_path, ext_path);

		buf = Memalign(32, 0x4000);
		if (buf==NULL)
			return FSErrors::OutOfMemory;

		// TODO: Handle moving directories
		int src = FS_Open(nand_path, IPC_OPEN_READ);
		if (src<0) {
			Dealloc(buf);
			return FSErrors::FileNotFound;
		}

		if (CreateFile(ext_path)>=0 && (dest=OpenFile(ext_path, ISFS_OPEN_WRITE))) {
			int size = FS_Seek(src, 0, SEEK_END);
			ret = FS_Seek(src, 0, SEEK_SET);
			while (size>0) {
				int readed = FS_Read(src, buf, MIN(0x4000, size));
				if (readed<=0 || dest->Write(buf, readed)!=readed) {
					ret = FSErrors::IOError;
					break;
				}
				size -= readed;
			}
		}

		delete dest;
		if (src>=0)
			FS_Close(src);
		Dealloc(buf);

		// clean up old file
		if (ret==FSErrors::OK) {
			src = FS_Open("/dev/fs", 0);
			if (src>=0) {
				FS_Ioctl(src, Ioctl::Delete, nand_path, ISFS_MAXPATH_LEN, NULL, 0);
				FS_Close(src);
			}
		}

		return ret;
	}

	int RiivDir::MoveFrom(const char* ext_path, const char* nand_path)
	{
		LogPrintf("Unimplemented Move (RiivDir::MoveFrom)!\n");
		return FSErrors::InvalidArgument;
	}

	int RiivDir::GetUsage(const char* ext_path, u32 *files, u32 *bytes, char *next_name)
	{
		s32 ret = FSErrors::OK;
		Stats st;

		s32 dir = File_OpenDir(ext_path);
		if (dir<0)
			return FSErrors::FileNotFound;

		while (File_NextDir(dir, next_name, &st)>=0) {
			if (!(st.Mode&S_IFDIR)) {
				files[0]++;
				*bytes += st.Size;
			} else if (next_name[0]=='.')
				continue;
			else {
				char *new_dir = (char*)Alloc(strlen(ext_path)+strlen(next_name)+2);
				if (new_dir==NULL) {
					ret = FSErrors::OutOfMemory;
					break;
				}
				strcpy(new_dir, ext_path);
				strcat(new_dir, "/");
				strcat(new_dir, next_name);
				ret = GetUsage(new_dir, files, bytes, next_name);
				Dealloc(new_dir);
				if (ret<0)
					break;
			}
		}

		File_CloseDir(dir);

		return ret;
	}

	int RiivDir::Exists(const char* path)
	{
		Stats st[2];
		if (File_Stat(path, st)>=0)
			return 1;
		return -1;
	}

	RiivDir::RiivDir(const char* _nand_dir, const char* _ext_dir)
	{
		nand_dir = (char*)Memalign(32, ISFS_MAXPATH_LEN);
		ext_dir = (char*)Memalign(32, strlen(_ext_dir)+1);

		if (nand_dir)
			strncpy(nand_dir, _nand_dir, ISFS_MAXPATH_LEN);
		if (ext_dir)
			strcpy(ext_dir, _ext_dir);
	}

	RiivDir::~RiivDir()
	{
		Dealloc(nand_dir);
		Dealloc(ext_dir);
	}

	s16 AppDir::AppToCID(const char *app_file)
	{
		int index=-1;
		if (strlen(app_file)==strlen("00000000.app") && !strcmp(app_file+8, ".app")) {
			index = 0;
			for (int i=4; i < 8; i++) {
				char c = app_file[i];
				if (c < '0' || c > 'f' || (c > '9' && c < 'a')) {
					LogPrintf("Bad character in appfile '%c'\n", c);
					return -1;
				}
				index = (index << 4) + c - ((c > '9') ? ('a' - 10) : '0');
			}
		}
		return index;
	}

	void AppDir::IndexToBin(int index, char *bin_file)
	{
		bin_file[2] = index%10 + '0';
		index /= 10;
		bin_file[1] = index%10 + '0';
		bin_file[0] = index/10 + '0';
		strcpy(bin_file+3, ".bin");
	}

	int AppDir::Initialize()
	{
		if (initialized)
			return 1;

		LogPrintf("Initializing %s\n", nand_dir);
		int i;
		char *tmd_path = (char*)Memalign(32, 1024);
		u32 *tmd_buf = (u32*)Memalign(32, MAX_SIGNED_TMD_SIZE);
		if (tmd_path && tmd_buf) {
			strcpy(tmd_path, nand_dir);
			strcat(tmd_path, "/title.tmd");
			s32 fd = FS_Open(tmd_path, ISFS_OPEN_READ);
			if (fd>=0) {
				tmd *title_tmd = NULL;
				s32 x = FS_Read(fd, tmd_buf, MAX_SIGNED_TMD_SIZE);
				FS_Close(fd);
				if (x>(s32)(sizeof(sig_rsa2048)+sizeof(tmd)) && x>=(s32)SIGNED_TMD_SIZE(tmd_buf))
					title_tmd = (tmd*)SIGNATURE_PAYLOAD(tmd_buf);
				if (title_tmd && title_tmd->num_contents <= 512) {
					LogPrintf("Found %d contents in TMD %s\n", title_tmd->num_contents, tmd_path);
					x = File_OpenDir(ext_dir);
					if (x>=0) {
						Stats st;
						while (File_NextDir(x, tmd_path, &st)>=0) {
							if (strlen(tmd_path)!=strlen("000.bin") || strcasecmp(tmd_path+3, ".bin"))
								continue;

							s32 index=0;
							for (i=0; i<3; i++) {
								if (tmd_path[i] < '0' || tmd_path[i] > '9')
									i=5;
								else
									index = index*10 + tmd_path[i]-'0';
							}
							if (i>3)
								continue;

							if (index < title_tmd->num_contents && title_tmd->contents[index].type&0x4000) {
								LogPrintf("Found .bin file: %s, index %d\n", tmd_path, index);
								content_map[index] = (s16)title_tmd->contents[index].cid;
							}
						}
						File_CloseDir(x);
					}
				}
			} else
				LogPrintf("Couldn't open TMD: %s\n", tmd_path);
			LogPrintf("AppDir for %s initialized\n", nand_dir);
			initialized = 1;
		}
		Dealloc(tmd_path);
		Dealloc(tmd_buf);

		return initialized;
	}

	char* AppDir::GetTranslatedPath(const char* path)
	{
		char *new_path = NULL;

		if (!strcmp(path, nand_dir)) {
			// strdup
			new_path = (char*)Memalign(32, strlen(ext_dir)+1);
			if (new_path)
				strcpy(new_path, ext_dir);
		} else if (!strncmp(path, nand_dir, strlen(nand_dir)) && strlen(path)==strlen(nand_dir)+strlen("/00000000.app") && Initialize()) {
			int i;
			s16 index = AppToCID(path+strlen(nand_dir)+1);
			if (index<0) {
				LogPrintf("AppToCID returned bad index for %s\n", path+strlen(nand_dir)+1);
				return NULL;
			}

			for (i=0; i < 512; i++) {
				if (content_map[i]==index)
					break;
			}
			if (i==512)
				return NULL;

			new_path = (char*)Memalign(32, strlen(ext_dir)+strlen("000.bin")+2);
			strcpy(new_path, ext_dir);
			strcat(new_path, "/");
			IndexToBin(i, new_path+strlen(ext_dir)+1);
		}
		return new_path;
	}

	RiivFile* AppDir::OpenFile(const char* path, int mode)
	{
		// TODO: Handle mode != ISFS_OPEN_READ
		if (mode != ISFS_OPEN_READ)
			return NULL;
		return new AppFile(path);
	}

	int AppDir::Delete(const char *path)
	{
		const char *filename = path+strlen(ext_dir)+1;
		int index = (filename[0]-'0')*100 + (filename[1]-'0')*10 + filename[2]-'0';
		if (index<512)
			content_map[index] = -1;
		return File_Delete(path);
	}

	int AppDir::ReadDir(const char*, u32 *out_count, char *names, const u32 *max_count)
	{
		LogPrintf("AppDir::ReadDir %s\n", nand_dir);
		// passthrough to /dev/fs
		s32 ret;
		ioctlv vec[4];
		s32 fd = FS_Open("/dev/fs", 0);
		if (fd<0)
			return fd;

		vec[0].data = nand_dir;
		vec[0].len = ISFS_MAXPATH_LEN;
		if (names==NULL) {
			LogPrintf("AppDir::ReadDir: Getting number of files\n");
			vec[1].data = out_count;
			vec[1].len = sizeof(u32);
			ret = FS_Ioctlv(fd, Ioctl::ReadDir, 1, 1, vec);
		} else {
			LogPrintf("AppDir::ReadDir: Getting %u file names\n", *max_count);
			vec[1].data = (void*)max_count;
			vec[1].len = sizeof(u32);
			vec[2].data = names;
			vec[2].len = *max_count*13;
			vec[3].data = out_count;
			vec[3].len = sizeof(u32);
			ret = FS_Ioctlv(fd, Ioctl::ReadDir, 2, 2, vec);
		}

		LogPrintf("AppDir::ReadDir returning %d (%u files)\n", ret, *out_count);
		FS_Close(fd);
		return ret;
	}

	// Warning: Huge unwieldy function ahead!
	int AppDir::MoveTo(const char* nand_path, const char*)
	{
		int ret;
		int i;
		ioctlv vec[4];
		s32 fd;
		u32 number_of_files;

		LogPrintf("AppDir::MoveTo start (%s -> %s)\n", nand_path, ext_dir);

		char *nand_file = (char*)Memalign(32, ISFS_MAXPATH_LEN*2);
		char *ext_file = (char*)Alloc(strlen(ext_dir)+14);
		char *file_names = (char*)Memalign(32, 13*512);
		u32 *tmd_buf = (u32*)Memalign(32, MAX_SIGNED_TMD_SIZE);
		void *buf = Memalign(32, 0x8000);
		if (nand_file==NULL || ext_file==NULL || file_names==NULL || buf==NULL || tmd_buf==NULL) {
			Dealloc(tmd_buf);
			Dealloc(buf);
			Dealloc(nand_file);
			Dealloc(ext_file);
			Dealloc(file_names);
			return FSErrors::OutOfMemory;
		}

		strcpy(nand_file, nand_path);
		strcat(nand_file, "/");

		// this will probably already exist, but just in case (might be a new DLC dir)
		File_CreateDir(ext_dir);

		// get number of files
		vec[0].data = (void*)nand_path;
		vec[0].len = ISFS_MAXPATH_LEN;
		vec[1].data = &number_of_files;
		vec[1].len = sizeof(u32);

		fd = FS_Open("/dev/fs", 0);
		if (fd<0)
			ret = fd;
		else {
			ret = FS_Ioctlv(fd, Ioctl::ReadDir, 1, 1, vec);
			if (ret>=0 && number_of_files>513)
				ret = FSErrors::TooManyFiles;
		}

		// read title.tmd first
		if (ret>=0) {
			LogPrintf("Source directory has %u files\n", number_of_files);
			strcpy(nand_file+strlen(nand_path)+1, "title.tmd");
			s32 tmd_in = FS_Open(nand_file, ISFS_OPEN_READ);
			if (tmd_in<0)
				ret = tmd_in;
			else {
				ret = FS_Read(tmd_in, tmd_buf, MAX_SIGNED_TMD_SIZE);
				LogPrintf("Read %d bytes of TMD\n", ret);

				FS_Close(tmd_in);
			}
		}

		if (ret>=0) {
			vec[2].data = file_names;
			vec[2].len = 13*number_of_files;
			vec[3].data = &number_of_files;
			vec[3].len = sizeof(u32);
			ret = FS_Ioctlv(fd, Ioctl::ReadDir, 2, 2, vec);
		}

		if (ret>=0) {
			char *current_file;
			LogPrintf("Read %u filenames\n", number_of_files);
			tmd *title_tmd = (tmd*)SIGNATURE_PAYLOAD(tmd_buf);
			strcpy(ext_file, ext_dir);
			strcat(ext_file, "/");

			for (current_file=file_names; ret>=0 && number_of_files; number_of_files--, current_file+=strlen(current_file)+1) {
				LogPrintf("Found local file %s\n", current_file);

				s16 cid = AppToCID(current_file);
				if (cid<0)
					continue;

				for (i=0; i < title_tmd->num_contents; i++) {
					if (title_tmd->contents[i].cid==(u32)cid) {
						strcpy(nand_file+strlen(nand_path)+1, current_file);

						IndexToBin(i, ext_file+strlen(ext_dir)+1);
						AppFile bin_out(ext_file, title_tmd->contents[i].index, tmd_buf);

						s32 app_in = FS_Open(nand_file, ISFS_OPEN_READ);
						if (app_in>=0) {
							int readed;
							while ((readed=FS_Read(app_in, buf, 0x8000))>0)
								bin_out.Write(buf, readed);

							FS_Close(app_in);
							LogPrintf("Moved %s to %s\n", nand_file, ext_file);
						}
						if (title_tmd->contents[i].type&0x4000) {
							if (FS_Ioctl(fd, Ioctl::Delete, nand_file, ISFS_MAXPATH_LEN, NULL, 0)>=0)
								LogPrintf("Deleted file %s\n", nand_file);
							else
								LogPrintf("Deleting .app file failed\n");
						} else
							LogPrintf("Skipped deleting non-dlc .app file\n");
						break;
					}
				}
			}
		}

		if (fd>=0) {
			if (ret>=0) {
				nand_file[strlen(nand_path)] = 0;
				strcpy(nand_file+ISFS_MAXPATH_LEN, nand_dir);
				ret = FS_Ioctl(fd, Ioctl::Move, nand_path, ISFS_MAXPATH_LEN*2, NULL, 0);
			}
			FS_Close(fd);
		}

		Dealloc(tmd_buf);
		Dealloc(buf);
		Dealloc(file_names);
		Dealloc(ext_file);
		Dealloc(nand_file);

		for (i=0; i < 512; i++)
			content_map[i] = -1;
		initialized = 0;

		LogPrintf("AppDir::MoveTo returning %d\n", ret);

		return (ret>0) ? FSErrors::OK : ret;
	}

	int AppDir::MoveFrom(const char*, const char* nand_path)
	{
		char *name_buf = (char*)Memalign(32, ISFS_MAXPATH_LEN*2);
		if (name_buf==NULL)
			return FSErrors::OutOfMemory;

		strncpy(name_buf, nand_dir, ISFS_MAXPATH_LEN);
		strncpy(name_buf+ISFS_MAXPATH_LEN, nand_path, ISFS_MAXPATH_LEN);

		s32 fd = FS_Open("/dev/fs", 0);
		if (fd<0)
			return fd;
		s32 ret = FS_Ioctl(fd, Ioctl::Move, name_buf, ISFS_MAXPATH_LEN*2, NULL, 0);
		Dealloc(name_buf);
		FS_Close(fd);
		LogPrintf("AppDir::MoveFrom %s -> %s returning %d\n", name_buf, name_buf+ISFS_MAXPATH_LEN, ret);
		return ret;
	}

	int AppDir::Exists(const char*)
	{
		// any path given to this function comes from AppDir::GetTranslatedPath,
		// and should already exist
		return 1;
	}

	AppDir::AppDir(const char* _nand_dir, const char* _ext_dir) :
	RiivDir(_nand_dir, _ext_dir)
	{
		for (int i=0; i < 512; i++)
			content_map[i] = -1;
		initialized = 0;
		//Initialize();
	}

} }
