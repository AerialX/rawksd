#include "emu.h"

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
#else
#define LogPrintf(...)
#define ch341_open()
#endif

u32 FS_ioctl_vect=0;
u32 FS_ret;

void LogMessage(ipcmessage* message)
{
	switch (message->command) {
		case IOS_OPEN:
			if (!strncmp(message->open.device, "/dev/fs", 7) || strncmp(message->open.device, "/dev", 4)) {
				LogPrintf("IOS_Open: \"%s\" - %d - %d - %04X (%08X)\n", message->open.device, message->open.mode, message->open.uid, message->open.gid, message->result);
			}
			break;
		case IOS_CLOSE:
			LogPrintf("IOS_Close: 0x%X\n", message->fd);
			break;
		case IOS_READ:
			//LogPrintf("IOS_Read: %p 0x%X\n", message->fd, message->read.length);
			break;
		case IOS_WRITE:
			//LogPrintf("IOS_Write: %p 0x%X\n", message->fd, message->write.length);
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

	s32 EMU::FS_IPC(ipcmessage *msg)
	{
		s32 ret, fd = os_open(FS_INTERNAL_NAME, 0);
		if (fd < 0)
			return fd;

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
				return 1;
			}
		}
		return -1;
	}

	int EMU::TryOpen(const char *name, u32 mode, RiivFile **x)
	{
		std::vector<RiivDir*>::iterator it = DataDirs.begin();
		for (;it != DataDirs.end(); it++) {
			char *path = (*it)->GetTranslatedPath(name);
			if (path) {
				Stats st;
				LogPrintf("TryOpen translated path: %s\n", path);
				// TODO: replace this stat with a RiivDir function
				if (File_Stat(path, &st)>=0)
					*x = new RiivFile(path, mode);
				Dealloc(path);
				return 1;
			}
		}

		return -1;
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

			RiivFile *f=NULL;
			if (TryOpen(message->open.device, message->open.mode, &f)>=0) {
				if (f) {
					open_files[i] = f;
					*result = (u32)f;
				} else
					*result = FSErrors::FileNotFound;
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
						if (message->ioctl.command==Ioctl::GetFileStats && message->ioctl.length_io>=8) {
							s32 *stats = (s32*)message->ioctl.buffer_io;
							// position
							stats[1] = open_files[i]->Seek(0, SEEK_CUR);
							// length
							stats[0] = open_files[i]->Seek(0, SEEK_END);
							// restore
							open_files[i]->Seek(stats[1], SEEK_SET);
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

			LogPrintf("FS Ioctl path: %s\n", path);
			if (path2)
				LogPrintf("FS Ioctl path2: %s\n", path2);

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
							*result = File_CreateFile(new_path);
							break;
						case Ioctl::Delete:
							*result = File_Delete(new_path);
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
								*result = ReadDir(new_path, out_count, names, max_count);
								os_sync_after_write(out_count, sizeof(u32));
								if (names && out_count[0])
									os_sync_after_write(names, 13*out_count[0]);
							}
							break;
						case Ioctl::Move:
							if (new_path && new_path2) {
								*result = File_Rename(new_path, new_path2);
							} else if (new_path) {
								// Unimplemented
								LogPrintf("Unimplemented Move!\n");
								//*result = MoveFrom(new_path, path2);
								*result = FSErrors::InvalidArgument;
							} else if (new_path2) {
								*result = MoveTo(path, new_path2);
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
									*result = GetUsage(new_path, files, blocks, next_name);
									if (*result>=0) {
										*blocks >>= 14; // 1 block = 0x4000 bytes
										os_sync_after_write(blocks, sizeof(u32));
										os_sync_after_write(files, sizeof(u32));
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

					Dealloc(new_path);
					Dealloc(new_path2);
					return 1;
				}
			}
		}
		return 0;
	}

	int EMU::MoveTo(const char* nand_path, const char* ext_path)
	{
		void *buf;
		int ret = FSErrors::InvalidArgument;
		int dest = -1;

		LogPrintf("Moving %s from NAND to external path: %s\n", nand_path, ext_path);

		buf = Alloc(0x8000);
		if (buf==NULL)
			return FSErrors::OutOfMemory;

		// TODO: Handle moving directories
		int src = FS_IPC(nand_path, IPC_OPEN_READ);
		if (src<0) {
			Dealloc(buf);
			return FSErrors::FileNotFound;
		}

		dest = File_Open(ext_path, O_CREAT|O_WRONLY|O_TRUNC);
		if (dest>=0) {
			int size = FS_IPC(src, 0, SEEK_END);
			ret = FS_IPC(src, 0, SEEK_SET);
			while (size>0) {
				int readed = FS_IPC(src, buf, MIN(0x8000, size));
				if (readed<=0 || File_Write(dest, buf, readed)!=readed) {
					ret = FSErrors::IOError;
					break;
				}
				size -= readed;
			}
		}

		if (dest>=0)
			File_Close(dest);
		if (src>=0)
			FS_IPC(src);
		Dealloc(buf);

		// clean up old file
		if (ret==FSErrors::OK) {
			src = FS_IPC("/dev/fs", 0);
			if (src>=0) {
				FS_IPC(src, Ioctl::Delete, nand_path, ISFS_MAXPATH_LEN, NULL, 0);
				FS_IPC(src);
			}
		}

		return ret;
	}

	int EMU::GetUsage(const char* ext_path, u32 *files, u32 *bytes, char *next_name)
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

	int EMU::ReadDir(const char* ext_path, u32 *out_count, char *names, const u32 *max_count)
	{
		Stats st;
		int ret = FSErrors::OK;
		char *next_name = (char*)Alloc(1024);
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
			if (st.Mode & S_IFDIR && next_name[0]=='.')
				continue;

			if (out) {
				if (count==max_count[0]) {
					ret = FSErrors::TooManyFiles;
					break;
				}
				else {
					strncpy(out, next_name, 12); // maximum ISFS filename is 12 chars
					out[13] = 0;
					out += strlen(out)+1;
				}
			}
			count++;
		}

		*out_count = count;

		if (names) {
			if (count)
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


	s32 RiivFile::Open()
	{
		if (file<0)
			file = File_Open(file_name, file_mode);

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
		file_mode = mode&3;
		if (file_mode)
			file_mode -= 1;

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

	RiivDir::RiivDir(const char* _nand_dir, const char* _ext_dir)
	{
		nand_dir = (char*)Alloc(strlen(_nand_dir)+1);
		ext_dir = (char*)Alloc(strlen(_ext_dir)+1);

		if (nand_dir)
			strcpy(nand_dir, _nand_dir);
		if (ext_dir)
			strcpy(ext_dir, _ext_dir);
	}

	RiivDir::~RiivDir()
	{
		Dealloc(nand_dir);
		Dealloc(ext_dir);
	}

	char* RiivDir::GetTranslatedPath(const char *path)
	{
		char *new_path = NULL;
		if (!strncmp(path, nand_dir, strlen(nand_dir)))
			new_path = (char*)Alloc(strlen(ext_dir)+strlen(path)-strlen(nand_dir)+1);
		if (new_path) {
			strcpy(new_path, ext_dir);
			strcat(new_path, path+strlen(nand_dir));
		}
		return new_path;
	}

} }
