#include "emu.h"

static char DeviceName[] __attribute__ ((aligned (32))) = "emu";

static u32 bins_titleid = 0;
static u8 bins[512]; // Yeah yeah, a bitfield can come later

static u8 Heapspace[0x8000] __attribute__ ((aligned (32)));
static void* Queue;
static u32 QueueHandle;
static s32 EmuFd = -1;
static ipcmessage ProxyMessage;

static tmd* meta = null;
static void* raw_meta = null;

#define MIN(a, b) ((a) < (b) ? (a) : (b))

typedef struct {
	int Fd;
	BinFileRead* Bin;
	
	// Post-open Params
	u8 Mode;
	char* Path;

	u32 Type;
	/* 0 - File Not Open
	   1 - Direct FAT reads
	   2 - .bin handling
	*/
} File;

static File OpenFiles[MAX_FILES_OPEN];

static int FilesystemHook(ipcmessage* message, int* result);
int HandleFsMessage(ipcmessage* message, int* ret);
void Loop();

void TMD_Free()
{
	Dealloc(raw_meta);

	raw_meta = null;
	meta = null;
}

void TMD_Realloc(u32 newsize, u32 oldsize)
{
	void *newptr = Alloc(newsize);
	if (raw_meta) {
		if (newptr && oldsize)
			memcpy(newptr, raw_meta, MIN(oldsize, newsize));
		Dealloc(raw_meta);
	}
	raw_meta = newptr;

	meta = (tmd*)SIGNATURE_PAYLOAD((signed_blob*)raw_meta);
}

void Initialize()
{
	os_thread_set_priority(os_get_thread_id(), 1);
	
	InitializeHeap(Heapspace, sizeof(Heapspace), 32);

	Queue = Alloc(0x20);
	
	QueueHandle = os_message_queue_create(Queue, 8);

	os_device_register(DeviceName, QueueHandle);

	Timer_Init();

	memset(OpenFiles, 0, sizeof(OpenFiles));

	memset(bins, 0, sizeof(bins));

	File_Init();
	
	LogInit();
	
	InitBinHandler();
}

int main()
{
	Initialize();

	Loop();

	return 0;
}

void Loop()
{
	while (true) {
		ipcmessage* message;
		int result = 1;
		bool acknowledge = true;
		
		os_message_queue_receive(QueueHandle, (u32*)&message, 0);

		switch (message->command) {
			case IOS_OPEN:
				if (strcmp(message->open.device, DeviceName))
					result = ERROR_OPEN_FAILURE;
				else
					result = message->open.resultfd;
				break;
			case IOS_CLOSE:
				result = 0;
				break;
			case IOS_IOCTL: {
				u32* inbuffer = message->ioctl.buffer_in;
				u32* outbuffer = message->ioctl.buffer_io;
				u32 insize = message->ioctl.length_in;
				//u32 outsize = message->ioctl.length_io;

				if (insize)
					os_sync_before_read(inbuffer, insize);
				//if (outsize)
				//	os_sync_before_read(outbuffer, outsize); // It's an 'io' buffer

				switch (message->ioctl.command) {
					case IOCTL_GET_FS_HANDLER:
						result = (int)FilesystemHook;
						break;
					case IOCTL_PROXY_MESSAGE:
						result = HandleFsMessage((ipcmessage*)inbuffer, (int*)outbuffer);
						break;
					case IOCTL_LWP_INFO:
						result = HeapInfo();
						break;
					default:
						result = -1;
				}

				//if (outsize)
				//	os_sync_after_write(outbuffer, outsize);
				break;
			}
		}

		// Respond
		if (acknowledge)
			os_message_queue_ack((void*)message, result);
	}
}

bool IsHookPath(const char* path)
{
	if ((strstr(path, "/title/00010005/") && strstr(path, ".app")))
		return true;
	return false;
}

int GetEmuFd(int fd)
{
	if (fd >= (int)&OpenFiles[0] && fd < (int)&OpenFiles[MAX_FILES_OPEN ])
		return fd;
	return -1;
}


static const char HEX_CHARS[] = "0123456789abcdef";
int HexToInt(char* hex, int length)
{
	int ret = 0;
	for (int i = length - 1; i >= 0; i--)
		ret |= ((int)strchr(HEX_CHARS, hex[i]) - (int)HEX_CHARS) << (4*(length - 1 - i));
	return ret;
}

/* Still don't trust it >.>
int HexToInt(char* hex, int length)
{
	int ret = 0;
	for (int i = 0; i < length; i++)
		ret = (ret << 4) + hex[i] - ((hex[i] > '9') ? ('a' - 10) : '0');
	return ret;
}
*/
int DecimalToInt(char* dec, int length)
{
	int ret = 0;
	for (int i = 0; i < length; i++)
		 ret = (ret * 10) + dec[i] - '0';
	return ret;
}
void PrintDlcPath(char* path, u32 titleid, int index)
{
	char* title = (char*)&titleid;
	_sprintf(path, "/private/wii/data/%c%c%c%c/%03d.bin", title[0], title[1], title[2], title[3], index);
}

void GetTitleMetadata(u32 titleid)
{
	LogPrintf("GetTitleMetadata(%u);\n", titleid);

	char path[MAXPATHLEN];
	PrintDlcPath(path, titleid, 000);

	TMD_Realloc(sizeof(tmd) + sizeof(sig_rsa2048), 0);
	
	int fd = File_Open(path, O_RDONLY);
	if (fd < 0) { // It doesn't exist; don't try opening it again
		meta->num_contents = 0;
		meta->title_id = titleid;
		return;
	}

	File_Seek(fd, 0x80, 0);
	File_Read(fd, raw_meta, sizeof(tmd) + sizeof(sig_rsa2048));
	
	TMD_Realloc(TMD_SIZE(meta) + sizeof(sig_rsa2048), sizeof(tmd) + sizeof(sig_rsa2048));

	File_Read(fd, (u8*)meta->contents, sizeof(tmd_content) * meta->num_contents);

	File_Close(fd);
}

int GetIndexFromCid(u32 titleid, u32 cid)
{
	if ((titleid & 0xFFFFFF00) == 0x63524200) // cRB?
		return cid; // For RawkSD, cid == index

	if (meta == null || (u32)meta->title_id != titleid)
		GetTitleMetadata(titleid);
	
	for (int i = 0; i < meta->num_contents; i++)
		if (meta->contents[i].cid == cid)
			return i;

	// Didn't find a match
	return -1;
}

void PostOpenFile(int fd)
{
	if (OpenFiles[fd].Type & 0xF0) {
		OpenFiles[fd].Fd = File_Open(OpenFiles[fd].Path, OpenFiles[fd].Mode);
		Dealloc(OpenFiles[fd].Path);

		if (OpenFiles[fd].Type == 0xF2)
			OpenFiles[fd].Bin = OpenBinRead(OpenFiles[fd].Fd);

		OpenFiles[fd].Type ^= 0xF0;
	}
}

void GetDirectoryInfo(char* dir, char* file, const char* path)
{
	int i;
	for (i = strlen(path) - 1; i >= 0; i--)
		if (path[i] == '/')
			break;
	if (i >= 0) {
		strncpy(dir, path, i);
		dir[i] = '\0';
		strcpy(file, path + i + 1); // + 1 for the '/'
	}
}

void UpdateBinList(const char* path, u32 titleid)
{
	LogPrintf("UpdateBinList(%s, %u);\n", path, titleid);

	static char dir[MAXPATHLEN];
	static char file[MAXPATHLEN];
	GetDirectoryInfo(dir, file, path);

	memset(bins, 0, sizeof(bins));

	s32 d = File_OpenDir(dir);
	if (d < 0)
		return;
	Stats st;
	while (File_NextDir(d, file, &st) == 0) {
		if (file[0] == '.') // Ignore relative "." and ".." filenames
			continue;
		
		if (strlen(file) == 7 && !strcasecmp(file + 3, ".bin")) { // xxx.bin
			int idx = DecimalToInt(file, 3);
			if (idx >= 0 && idx < 512)
				bins[idx] = 1;
		}
	}
	File_CloseDir(d);

	bins_titleid = titleid;
}

bool BinExists(const char* path, u32 titleid, s32 cidx)
{
	if (bins_titleid == titleid)
		return bins[cidx] == 1;

	if (cidx == 0) {
		UpdateBinList(path, titleid);
		return bins[cidx] == 1;
	}

	return File_Stat(path, null) == 0;
}

bool FileExists(const char* path)
{
	return File_Stat(path, null) == 0;
}

static u32 fake_fd = 0;
static s32 fake_file = 0;
static u32 fake_type = 0;
static u32 fake_cid = 0;
static u8* fake_tmd = NULL;
/* fake_type
	0 = tmd (fake_file = position/size)
	1 = app/bin (fake_file = File_Open)
	2 = app/bin (fake_file = File_Open) -- Passthrough
*/

// Whee /dev/fs hook
// store value to return in *result and return non-zero to prevent /dev/fs from handling this message
// otherwise return 0 to let /dev/fs handle it
int HandleFsMessage(ipcmessage* message, int* ret)
{
	LogMessage(message);

	int fd = message->fd - 0x1000;

	if ((fd < 0 || fd >= MAX_FILES_OPEN) && message->command != IOS_OPEN)
		return false;
/*
	if ((fd < 0 || fd >= MAX_FILES_OPEN) && (message->command != IOS_OPEN || strstr(message->open.device, "/dev")) &&
		message->command != IOS_IOCTL &&
		message->command != IOS_IOCTLV &&
		(!fake_fd || message->fd != fake_fd))
			return false;

	if (fake_fd && message->fd == fake_fd) {
		if (message->command == IOS_WRITE) {
			os_sync_before_read(message->write.data, message->write.length);
			if (fake_type == 0) {
				TMD_Realloc(fake_file + message->write.length, fake_file);
				memcpy((u8*)raw_meta + fake_file, message->write.data, message->write.length);

				fake_file += message->write.length;
			} else if (fake_type == 1 || fake_type == 2) {
				if (fake_file) {
					*ret = File_Write(fake_file, message->write.data, message->write.length);
					if (fake_type == 1)
						return true;
				}
			}
		} else if (message->command == IOS_CLOSE) {
			if (fake_type == 1 || fake_type == 2) {
				if (fake_file) {
					char binpath[MAXPATHLEN];
					PrintDlcPath(binpath, (u32)meta->title_id, GetIndexFromCid(meta->title_id, fake_cid));
					s32 binfd = File_Open(binpath, O_CREAT | O_TRUNC | O_WRONLY);
					File_Seek(fake_file, 0, 0);
					CreateBin((u32)meta->title_id, fake_cid, raw_meta, fake_file, binfd);
					File_Close(binfd);
					File_Close(fake_file);
					//File_Delete(TEMP_APP_PATH);
					bins_titleid = 0; // Force us to refresh the list
				}
			}

			fake_file = 0;
			fake_fd = 0;
			fake_cid = 0;
			*ret = 0;

			if (fake_type == 1)
				return true;
		}

		return false;
	}
*/
	switch (message->command) {
		case IOS_OPEN: {
			u32 mode = message->open.mode;
			if (mode > 0)
				mode--; // change ISFS_READ|ISFS_WRITE to O_RDONLY|O_WRONLY|O_RDWR

			// Only hook for some files (like apps/bins)
			if (!IsHookPath(message->open.device)) {
				/*
				if (strstr(message->open.device, "/tmp")) {
					if (strstr(message->open.device, "title.tmd")) {
						fake_file = 0;
						fake_type = 0;
					} else if (strstr(message->open.device, ".app")) {
						// TODO: File_CreateDir()
						fake_file = File_Open(TEMP_APP_PATH, O_CREAT | O_TRUNC | O_RDWR);
						fake_cid = HexToInt(message->open.device + 5, 8);
						if (meta->contents[0].cid == fake_cid)
							fake_type = 2; // Passthrough for 000.bin
						else {
							fake_type = 1; // Any other bin shouldn't be on NAND
							fake_fd = 0x123;
							*ret = fake_fd;
							return true;
						}
					} else
						return false;

					*ret = (int)&fake_fd;
					return EMU_CATCH_WRITES;
				}
				*/
				return false;
			}

			for (fd = 0; OpenFiles[fd].Type != 0 && fd < MAX_FILES_OPEN; fd++)
				;

			if (fd >= MAX_FILES_OPEN)
				return false;

			if (strstr(message->open.device, ".app")) {
				// strlen("/title/00010005/") = 16
				// strlen("/title/00010005/00000000/content/") = 33
				u32 titleid = (u32)HexToInt(message->open.device + 16, 8);
				int cid = HexToInt(message->open.device + 33, 8);
				int cidx = GetIndexFromCid(titleid, cid);

				if (cidx < 0)
					return false; // We don't know about this app

				char path[MAXPATHLEN];
				PrintDlcPath(path, titleid, cidx);
				if (!BinExists(path, titleid, cidx))
					return false;

				OpenFiles[fd].Path = Alloc(strlen(path) + 1);
				strcpy(OpenFiles[fd].Path, path);

				OpenFiles[fd].Type = 0xF2;
			}

			OpenFiles[fd].Mode = mode;

			*ret = fd + 0x1000;
			return true;
		}
		case IOS_CLOSE:
			switch(OpenFiles[fd].Type) {
				case 0: // Already closed
					*ret = -1;
					break;
				case 2: // bin
					CloseReadBin(OpenFiles[fd].Bin);
				case 1: // FAT ; Note the fallthrough
					File_Close(OpenFiles[fd].Fd);
					*ret = 0;
					break;
				case 0xF2:
				case 0xF1: // Closing a never-used file
					Dealloc(OpenFiles[fd].Path);
					*ret = 0;
					break;
			}

			OpenFiles[fd].Type = 0;
			return true;
		case IOS_READ:
			PostOpenFile(fd);

			switch(OpenFiles[fd].Type) {
				case 0: // Closed
					*ret = -1;
					break;
				case 1: // FAT
					*ret = File_Read(OpenFiles[fd].Fd, message->read.data, message->read.length);
					break;
				case 2: // bin
					*ret = ReadBin(OpenFiles[fd].Bin, (u8*)message->read.data, message->read.length);
					break;
			}

			os_sync_after_write(message->read.data, message->read.length);

			return true;
		case IOS_WRITE:
			os_sync_before_read(message->write.data, message->write.length);

			PostOpenFile(fd);
			
			switch(OpenFiles[fd].Type) {
				case 0: // Closed
					*ret = -1;
					return true;
				case 1: // FAT
					*ret = File_Write(OpenFiles[fd].Fd, message->write.data, message->write.length);
					break;
				case 2: // bin
					*ret = -1;
					break;
			}

			return true;
		case IOS_SEEK:
			PostOpenFile(fd);

			switch(OpenFiles[fd].Type) {
				case 0: // Closed
					*ret = -1;
					return true;
				case 1: // FAT
					*ret = File_Seek(OpenFiles[fd].Fd, message->seek.offset, message->seek.origin);
					break;
				case 2: // bin
					*ret = SeekBin(OpenFiles[fd].Bin, message->seek.offset, message->seek.origin);
					break;
			}

			return true;
		case IOS_IOCTL: {
			isfs_ioctl *data = (isfs_ioctl*)message->ioctl.buffer_in;
			switch (message->ioctl.command) {
				case FSIOCTL_RENAME:
					if (strstr(data->fsrename.filepathOld, "/tmp") && strstr(data->fsrename.filepathOld, ".app")) {
						*ret = 0;
						return true;
					}
					break;
			}
			return false; }
		case IOS_IOCTLV:
			// Fuck what do I do?
			return false;
		default:
			return false;
	}

	return false;
}

static int FilesystemHook(ipcmessage* message, int* result)
{
	if (message) {
		if (EmuFd < 0)
			EmuFd = os_open(DeviceName, 0);

		if (EmuFd >= 0) {
			memcpy(&ProxyMessage, message, sizeof(ipcmessage));
			os_sync_after_write(&ProxyMessage, sizeof(ipcmessage));
			int ret = os_ioctl(EmuFd, IOCTL_PROXY_MESSAGE, &ProxyMessage, sizeof(ipcmessage), result, result ? sizeof(int) : 0);
			if (ret == EMU_CATCH_WRITES) {
				// check what will be returned by /dev/fs
				int i;
				for (i=0; i < MAX_FILES_OPEN; i++) {
					u16 *file_opened = (u16*)(DEV_FS_FILESTRUCTS+i*36); 
					if (*file_opened==0) {
						u32 *fd_temp = (u32*)*result; 
						// do stuff 
						*fd_temp = DEV_FS_FILESTRUCTS+i*36; 
						os_sync_after_write(fd_temp, sizeof(fd_temp)); 
						break; 
					}
				}
				return false;
			}
			
			return ret;
		}
	}
	return false;
}
