#ifndef __BINFILE_H__
#define __BINFILE_H__

#include "gctypes.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    u8 buf[16];
    s32 handle;
    u32 pos;
    u32 data_size;
    u32 header_size;
    u8 iv[16];
    int key_index;
    u16 index;
    u8 mode;
} BinFile;

BinFile* OpenBinRead(s32 file);
BinFile* CreateBinFile(u16 index, u32* tmd, u32 tmd_size, s32 file);
void CloseBin(BinFile* file);
s32 SeekBin(BinFile* file, s32 where, u32 origin);
s32 ReadBin(BinFile* file, u8* buffer, u32 numbytes);
s32 WriteBin(BinFile* file, u8* buffer, u32 numbytes);

#ifdef __cplusplus
}
#endif

#endif
