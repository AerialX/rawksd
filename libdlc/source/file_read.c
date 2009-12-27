#include "byteswap.h"
#include "file_read.h"

s8 WriteU64(s32 file, u64 value)
{
   value = bswap_64( value );
   return (fwrite(&value, 8, 1, file) == 1 ? 1 : 0);
}

s8 ReadU64(s32 file, u64* value )
{
    if(fread(value, 8, 1, file) != 1)
        return 0;
    *value = bswap_64(*value);
    return 1;
}

s8 WriteU32(s32 file, u32 value)
{
   value = bswap_32( value );
   return (fwrite(&value, 4, 1, file) == 1 ? 1 : 0);
}

s8 ReadU32(s32 file, u32* value )
{
    if(fread(value, 4, 1, file) != 1)
        return 0;
    *value = bswap_32(*value);
    return 1;
}

s8 WriteU16(s32 file, u16 value)
{
   value = bswap_16( value );
   return (fwrite(&value, 2, 1, file) == 1 ? 1 : 0);
}

s8 ReadU16(s32 file, u16* value )
{
    if(fread(value, 2, 1, file) != 1)
        return 0;
    *value = bswap_16(*value);
    return 1;
}

s8 WriteU8(s32 file, u8 value)
{
   return (fwrite(&value, 1, 1, file) == 1 ? 1 : 0);
}

s8 ReadU8(s32 file, u8* value)
{
    return (fread(value, 1, 1, file) == 1 ? 1 : 0);
}

s8 WriteBytes(s32 file, u8* pointer, u32 num)
{
    return (fwrite(pointer, 1, num, file) == num ? 1 : 0);
}

s8 ReadBytes(s32 file, u8* pointer, u32 num)
{
    return (fread(pointer, 1, num, file) == num ? 1 : 0);
}

s8 SkipBytes(s32 file, u32 num)
{
    //return (fseek(file, num, 1) == 0 ? 1 : 0);
	fseek(file, num, 1);
	return 1;
}

s8 FileSeek(s32 file, u32 pos, u32 orgin)
{
    //return (fseek(file, pos, orgin) == 0 ? 1 : 0);
	fseek(file, pos, orgin);
	return 1;
}

s8 FilePadding(s32 file, u32 pad)
{
    //return (fseek(file, ((ftell(file)+pad-1)&~(pad-1)), 0) == 0 ? 1 : 0);
	fseek(file, ((ftell(file)+pad-1)&~(pad-1)), 0);
	return 1;
}

void HexDump(u8* buffer, u32 num)
{
#if 0
    u32 i;
    for(i = 0; i < num; i++)
    {
        printf("%c ", *(buffer+i));
        if((i + 1) % 16 == 0)
            printf("\n");
    }
    printf("\n\n");
#endif
}

