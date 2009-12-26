#ifndef __FILE_READ_H__
#define __FILE_READ_H__

#include <stdio.h>
#include "gctypes.h"
#include <files.h>

#define fread(x, y, z, i) (File_Read(i, (u8*)x, (y)*(z)) / y)
#define fwrite(x, y, z, i) (File_Write(i, (const u8*)x, (y)*(z)) / y)
#define fseek(x, y, z) File_Seek(x, y, z)
#define ftell(x) File_Tell(x)

s8 WriteU64(s32 file, u64 value);
s8 ReadU64(s32 file, u64* value);

s8 WriteU32(s32 file, u32 value);
s8 ReadU32(s32 file, u32* value);

s8 WriteU16(s32 file, u16 value);
s8 ReadU16(s32 file, u16* value);

s8 WriteU8(s32 file, u8 value);
s8 ReadU8(s32 file, u8* value);

s8 WriteBytes(s32 file, u8* pointer, u32 num);
s8 ReadBytes(s32 file, u8* pointer, u32 num);
s8 SkipBytes(s32 file, u32 num);

s8 FileSeek(s32 file, u32 pos, u32 orgin);

s8 FilePadding(s32 file, u32 pad);

void HexDump(u8* buffer, u32 num);

#endif
