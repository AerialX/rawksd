#ifndef __BINFILE_H__
#define __BINFILE_H__

#include "gctypes.h"
#include "file_read.h"
#include "byteswap.h"
#include "wiistruct.h"

#include <mem.h>
#define malloc Alloc
#define free Dealloc

#define setAccessMask(mask, bit) (mask[bit / 8] = mask[bit / 8] | (1 << (bit % 8)))
#define unsetAccessMask(mask, bit) (mask[bit / 8] = mask[bit / 8] & ~(1 << (bit % 8)))
#define checkAccessMask(mask, bit) (mask[bit / 8] & (1 << (bit % 8)))

typedef struct
{
    s32 handle;
    u16 content_num;
    u32 content_position;
    u64 size;
	u8 iv_buffer[0x10];
	u32 position;
} BinFileRead;

s32 InitBinHandler();
BinFileRead* OpenBinRead(s32 file);
void CloseReadBin(BinFileRead* file);
s32 SeekBin(BinFileRead* file, s32 where, u32 orgin);
s32 ReadBin(BinFileRead* file, u8* buffer, u32 numbytes);

s32 CreateBin(u32 title, u32 content_id, void* tmdFile, s32 appFile, s32 binFile);

#if 0
typedef struct
{
    FILE* handle;
    u16 content_num;
    u32 content_position;

    u8 iv[16];
    u8 writeBuffer[16];
    u8 bufferPos;
} BinFileWrite;

BinFileWrite* CreateBinFile(u32 ticket, u32 content_id, char* tmdPath, char* binPath);
s32 FlushWriteBuffer(BinFileWrite* file);
s32 WriteBin(BinFileWrite* file, u8* buffer, u32 size);
#endif

#endif
