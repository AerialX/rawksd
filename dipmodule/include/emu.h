#pragma once

#include <proxiios.h>
#include "binfile.h"

#include <vector>

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
			SetFileVerCtrl   = ISFS::SetFileVerCtrl, // what is this I don't even
			GetFileStats     = ISFS::GetFileStats,
			GetUsage         = ISFS::GetUsage,       // ioctlv
			Shutdown         = ISFS::Shutdown,

			FSMessage        = 0x60,
			RedirectDir,                             // ioctlv
			//RedirectFile,
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
		u16 gid;              // 0x02 (from ipcmessage.open)
		u32 uid;              // 0x04 (from ipcmessage.open)
		u32 node_maybe;       // 0x08 (0xFFFF for /dev/fs device)
		u32 mode;             // 0x0C (from ipcmessage.open)
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
	protected:
		s32 file;
		s32 Open();
	public:
		virtual s32 Read(void *dest, s32 length);
		virtual s32 Write(const void *src, s32 length);
		virtual s32 Seek(s32 where, s32 whence);
		RiivFile(const char *name, s32 mode);
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
		AppFile(const char *name);
		AppFile(const char *name, u16 index, u32 *tmd_buf);
		virtual ~AppFile();
	};

	class RiivDir
	{
	protected:
		char *nand_dir, *ext_dir;
	public:
		virtual char* GetTranslatedPath(const char *path);
		virtual RiivFile* OpenFile(const char *path, int mode);
		int CreateFile(const char *path);
		virtual int Delete(const char *path);
		virtual int ReadDir(const char* ext_path, u32 *out_count, char *names, const u32 *max_count);
		virtual int MoveTo(const char* nand_path, const char* ext_path);
		virtual int MoveFrom(const char* ext_path, const char* nand_path);
		int GetUsage(const char* ext_path, u32 *files, u32 *blocks, char* next_name);
		virtual int Exists(const char *path);
		RiivDir(const char* _nand_dir, const char* _ext_dir);
		~RiivDir();
	};

	class AppDir : public RiivDir
	{
	private:
		u8 initialized;
		s16 content_map[512];
		s16 AppToCID(const char *app_file);
		void IndexToBin(int index, char *bin_file);
		int Initialize();
	public:
		virtual char* GetTranslatedPath(const char *path);
		virtual RiivFile* OpenFile(const char *path, int mode);
		virtual int Delete(const char *path);
		virtual int ReadDir(const char*, u32 *out_count, char *names, const u32 *max_count);
		virtual int MoveTo(const char* nand_path, const char*);
		virtual int MoveFrom(const char*, const char* nand_path);
		virtual int Exists(const char*);
		AppDir(const char* _nand_dir, const char* _ext_dir);
	};

	class EMU : public ProxiIOS::Module
	{
	private:
		int loop_thread;
		std::vector<RiivDir*> DataDirs;

		RiivFile* open_files[MAX_EMU_OPEN];
		int TryOpen(const char *name, u32 mode, RiivFile **x);
	public:
		EMU();

		int Start();

		int HandleIoctl(ipcmessage* message);
		int HandleIoctlv(ipcmessage* message);
		int HandleFSMessage(ipcmessage* message, int* result);

		static u32 emu_thread(void*);
	};

} }