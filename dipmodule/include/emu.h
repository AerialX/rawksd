#pragma once

#include <proxiios.h>
#include "binfile.h"

#define EMU_MODULE_NAME "emu"
#define FS_INTERNAL_NAME "nandfs"
#define MAX_EMU_OPEN 16

namespace ProxiIOS { namespace EMU {
	namespace Ioctl {
		enum Enum {
			Format           = ISFS::Format,
			GetStats         = ISFS::GetStats,
			CreateDir        = ISFS::CreateDir,
			ReadDir          = ISFS::ReadDir,        // ioctlv
			SetAttrib        = ISFS::SetAttrib,
			GetAttrib        = ISFS::GetAttrib,
			Delete           = ISFS::Delete,
			Move             = ISFS::Rename,
			CreateFile       = ISFS::CreateFile,
			SetFileVerCtrl   = ISFS::SetFileVerCtrl,
			GetFileStats     = ISFS::GetFileStats,
			GetUsage         = ISFS::GetUsage,       // ioctlv
			Shutdown         = ISFS::Shutdown,

			FSMessage        = 0x60,
			RedirectDir,
			RedirectFile,
			NANDFSMessage,
			ActivateHook     = 0x64,
			DeactivateHook   = 0x6F
		};
	}

	namespace FSErrors {
		enum Enum {
			OK                   = IPC_OK,
			InvalidArgument      = -101,
			PermissionDenied     = -102,
			IOError              = -103,
			FileExists           = -105,
			FileNotFound         = -106,
			TooManyFiles         = -107,
			OutOfMemory          = -108,
			NameTooLong          = -110,
			DirNotEmpty          = -115,
			DirDepthExceeded     = -116
		};
	}

	struct ISFSFile {
		u16 in_use;           // 0x00 (boolean)
		u16 gid;              // 0x02 (ipcmessage.open)
		u32 uid;              // 0x04 (ipcmessage.open)
		u32 node_maybe;       // 0x08 (0xFFFF for /dev/fs device)
		u32 mode;             // 0x0C (ipcmessage.open)
		u32 written_bytes;    // 0x10 (initially zero, not sure about this)
		u32 pos;              // 0x14
		u32 length;           // 0x18
		u32 unk;              // 0x1C (initially zero)
		u32 error_state_maybe;// 0x20 (initially zero, makes reading/writing/seeking/closing fail if non-zero)
	};

	class RiivFile
	{
	private:
		char *file_name;
		u32 file_mode;
		u32 watched_fd;
	protected:
		s32 file;
		s32 Open();
	public:
		int IsWatching(u32 fd);
		virtual s32 Read(void *dest, s32 length);
		virtual s32 Write(const void *src, s32 length);
		virtual s32 Seek(s32 where, s32 whence);
		RiivFile(const char *name, s32 mode, u32 watching=0);
		virtual ~RiivFile();
	};

	class AppFile : public RiivFile
	{
	private:
		BinFile *binfile;
		s32 Open();
	public:
		virtual s32 Read(void *dest, s32 length);
		virtual s32 Write(const void *src, s32 length);
		virtual s32 Seek(s32 where, s32 whence);
		// TODO: overload constructor for creating
		AppFile(const char *name);
		virtual ~AppFile();
	};

	class EMU : public ProxiIOS::Module
	{
	private:
		int loop_thread;
		RiivFile* open_files[MAX_EMU_OPEN];
		RiivFile* TryOpen(const char *name, u32 mode);

		s32 FS_IPC(ipcmessage *msg);
		// open
		s32 FS_IPC(const char *device, u32 mode, u32 uid, u16 gid);
		// close
		s32 FS_IPC(s32 fd);
		// read
		s32 FS_IPC(s32 fd, void* data, u32 length);
		// write
		s32 FS_IPC(s32 fd, const void* data, u32 length);
		// seek
		s32 FS_IPC(s32 fd, s32 offset, s32 origin);
		// ioctl
		s32 FS_IPC(s32 fd, u32 command, const void* buffer_in, u32 length_in, void* buffer_io, u32 length_io);
		// ioctlv
		s32 FS_IPC(s32 fd, u32 command, u32 num_in, u32 num_io, ioctlv* vector);
	public:
		EMU();

		int Start();

		int HandleIoctl(ipcmessage* message);
		int HandleFSMessage(ipcmessage* message, int* result);

		static u32 emu_thread(void*);
	};

} }