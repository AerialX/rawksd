#include <stdio.h>
#include <string.h>

#include "binfile.h"
#include "gcutil.h"
#include "syscalls.h"
#include "es.h"
#include "mem.h"
#include "files.h"

#define setAccessMask(mask, bit) (mask[bit>>3] |= (1 << (bit&7)))
#define unsetAccessMask(mask, bit) (mask[bit>>3] &= ~(1 << (bit&7)))
#define checkAccessMask(mask, bit) (mask[bit>>3] & (1 << (bit&7)))

#if 0
void LogPrintf(const char *fmt, ...);
#define debug_printf LogPrintf
#else
#define debug_printf(fmt, args...)
#define LogPrintf debug_printf
#endif

#define BIN_READ  1
#define BIN_WRITE 2

#define FSERR_EINVAL	-103

void dlc_aes_decrypt(u8 *iv, u8 *inbuf, u8 *outbuf, u32 len);
void dlc_aes_encrypt(u8 *iv, u8 *inbuf, u8 *outbuf, u32 len);

int verify_bk(BK_Header *bk)
{
	int failed=0;
	u32 wii_id = 0;
	os_get_key(ES_KEY_CONSOLE, &wii_id);

	if (bk->size != 0x70)
	{
		debug_printf("bk_header.size incorrect %d\n", bk->size);
		failed = 1;
	}
	if (bk->magic != 0x426B)
	{
		debug_printf("bk_header.magic incorrect %d\n", bk->magic);
		failed = 1;
	}
	if (bk->version != 1)
	{
		debug_printf("bk_header.version incorrect %d\n", bk->version);
		failed = 1;
	}
	if (bk->NG_id != wii_id)
	{
		debug_printf("bk_header.NG_id incorrect %08X\n", bk->NG_id);
		failed = 1;
	}
	if (bk->zeroes != 0)
	{
		debug_printf("bk_header.zeroes not zero %08X\n", bk->zeroes);
		failed = 1;
	}
	if (bk->title_id_1 != 0x00010000)
	{
		debug_printf("bk_header.title_id_1 incorrect %08X\n", bk->title_id_1);
		failed =1;
	}
	if (bk->title_id_2 != *(u32*)0)
	{
		debug_printf("bk_header.title_id_2 incorrect got=%08X expected=%08X\n", bk->title_id_2, *(u32*)0);
		failed = 1;
	}
	if (bk->padding[0] || bk->padding[1] || bk->padding[2])
	{
		debug_printf("bk_header.padding incorrect\n");
		failed =1;
	}

	return failed;
}

int FileRead(BinFile* file, void *buf, u32 size)
{
	if (size==0)
		return 0;
	if (File_Read(file->handle, buf, size)!=size)
		return 1;
	file->pos += size;
	return 0;
}

int FileWrite(BinFile* file, void *buf, u32 size)
{
	if (size==0)
		return 0;
	if (File_Write(file->handle, buf, size)!=size)
		return 1;
	file->pos += size;
	return 0;
}

int FileSeek(BinFile* file, s32 where)
{
	if (file->pos==where)
		return 0;
	if (File_Seek(file->handle, where, SEEK_SET)!=where)
		return 0;
	file->pos = where;
	return 0;
}

BinFile* OpenBinRead(s32 file)
{
	u32 i, found=0;;
    BK_Header bk_header;
    tmd tmd_header;
    tmd_content content_rec;
    BinFile* binfile = Alloc(sizeof(BinFile));

    if (binfile==NULL)
    	return NULL;

    memset(binfile, 0, sizeof(BinFile));
    binfile->handle = file;
    binfile->mode = BIN_READ;

    if (FileSeek(binfile, 0) ||
		FileRead(binfile, &bk_header, sizeof(bk_header)) ||
    	FileSeek(binfile, 0x1C0) ||
    	FileRead(binfile, &tmd_header, sizeof(tmd)))
    	{
			debug_printf("DLC Open failed reading bk_header or tmd_header\n");
    		goto open_error;
		}

    if (verify_bk(&bk_header))
    	goto open_error;

    binfile->header_size = ROUND_UP(bk_header.tmd_size+sizeof(bk_header), 64);
    binfile->data_size = bk_header.contents_size;

    for(i = 0; i < tmd_header.num_contents; i++)
    {
        if(checkAccessMask(bk_header.content_mask, i))
        {
			if (FileRead(binfile, &content_rec, sizeof(tmd_content)))
				goto open_error;
			binfile->index = content_rec.index;
			binfile->iv[0] = content_rec.index>>8;
			binfile->iv[1] = (u8)content_rec.index;
			if (bk_header.contents_size != ROUND_UP(content_rec.size, 64) ||
				content_rec.type != 0x4001)
			{
				debug_printf("DLC Open size mismatch (%d vs. %d) or contents not DLC (%04X)\n", bk_header.contents_size, ROUND_UP(content_rec.size, 64), content_rec.type);
				goto open_error;
			}
			found = 1;
			break;
        }
        else
            if (FileSeek(binfile, binfile->pos+sizeof(tmd_content)))
            	goto open_error;
    }

    if (!found)
    {
		debug_printf("DLC Open failed to find index in TMD\n");
    	goto open_error;
	}

    if (SeekBin(binfile, 0, SEEK_SET)>=0)
	    return binfile;

open_error:
	Dealloc(binfile);
	return NULL;
}

static void CloseWriteBin(BinFile* file)
{
	u8 zero=0;
	u32 total_size;
	u32 i;

	file->pos &= 0xF;
	if (file->pos)
	{
		LogPrintf("Flushing %u bytes\n", file->pos);
		dlc_aes_encrypt(file->iv, file->buf, file->buf, file->pos);
		FileWrite(file, file->buf, file->pos);
	}

	file->pos = file->data_size;
	file->data_size = ROUND_UP(file->data_size, 64);
	total_size = file->header_size + file->data_size;

	if (file->data_size > file->pos)
		LogPrintf("Padding with %u zeroes\n", file->data_size-file->pos);

	for (i=file->pos; i < file->data_size; i++)
		FileWrite(file, &zero, 1);

	if (!FileSeek(file, 0x18))
	{
		if (!FileWrite(file, &file->data_size, sizeof(file->data_size)))
			FileWrite(file, &total_size, sizeof(total_size));
	}

	LogPrintf("Closed a WriteBin file\n");
}

void CloseBin(BinFile* file)
{
	if (file && file->mode == BIN_WRITE)
		CloseWriteBin(file);
    Dealloc(file);
}

s32 SeekBin(BinFile* file, s32 where, u32 origin)
{
	s32 result = FSERR_EINVAL;

	if (file && file->mode==BIN_READ)
	{
		switch(origin)
		{
			case SEEK_SET:
			{
				where += file->header_size;
				break;
			}
			case SEEK_CUR:
			{
				where += file->pos;
				break;
			}
			case SEEK_END:
			{
				where += file->header_size + file->data_size;
			}
			default:
				return result;
		}

		if (where == file->pos) // check if we're already there
			return where - file->header_size;

		// make sure we're not before the beginning, then seek
		// 16 bytes backwards to read the iv
		if (where >= file->header_size && !FileSeek(file, (where&~15)-16))
		{
			// use index for iv?
			if (file->pos < file->header_size+16)
			{
				memset(file->iv, 0, 16);
				file->iv[0] = file->index>>8;
				file->iv[1] = (u8)file->index;
				if (FileSeek(file, file->pos+16))
					return result;
			}
			else if (FileRead(file, file->iv, 16))
				return result;

			// decrypt initial bytes of block
			if (where&0xF)
			{
				if (FileRead(file, file->buf, 16))
					return result;
				file->pos -= (16-(where&0xF));
				dlc_aes_decrypt(file->iv, file->buf, file->buf, 16);
				memmove(file->buf, file->buf+(where&0xF), 16-(where&0xF));
			}
			result = where - file->header_size;
		}
	}

	return result;
}

s32 ReadBin(BinFile* file, u8* buffer, u32 numbytes)
{
	s32 result = FSERR_EINVAL;
	u32 i;

	if (file && file->mode==BIN_READ)
	{
		result = numbytes;

		// leading bytes, should already be decrypted in buffer
		i = MIN((16 - file->pos)&0xF, numbytes);
		memcpy(buffer, file->buf, i);
		numbytes -= i;
		buffer += i;
		file->pos += i;

		// middle bytes (16-byte blocks)
		i = numbytes & ~15;
		if (FileRead(file, buffer, i))
			return FSERR_EINVAL;
		dlc_aes_decrypt(file->iv, buffer, buffer, i);
		numbytes -= i;
		buffer += i;

		// trailing bytes
		if (numbytes)
		{
			if (FileRead(file, file->buf, 16))
				return FSERR_EINVAL;
			file->pos -= 16 - numbytes;
			dlc_aes_decrypt(file->iv, file->buf, file->buf, 16);
			memcpy(buffer, file->buf, numbytes);
			memmove(file->buf, file->buf+numbytes, 16-numbytes);
		}
	}

	if (result<0)
		LogPrintf("ReadBin failed\n");
	return result;
}

BinFile* CreateBinFile(u16 index, u8* tmd, u32 tmd_size, s32 file)
{
	BinFile *bin;

    BK_Header bk_header;

    LogPrintf("CreateBinFile index %u %p %u\n", index, tmd, tmd_size);

	if (!tmd || !tmd_size || index>=512)
		return NULL;

	bin = (BinFile*)Memalign(32, sizeof(BinFile));
	if (bin==NULL)
		return NULL;

	LogPrintf("Allocated BinFile header\n");

	memset(bin, 0, sizeof(BinFile));
	bin->handle = file;
	bin->mode = BIN_WRITE;
	bin->iv[0] = index >> 8;
	bin->iv[1] = (u8)index;
	bin->header_size = ROUND_UP(tmd_size+sizeof(bk_header), 64);

	memset(&bk_header, 0, sizeof(bk_header));
    bk_header.size = 0x70;
    bk_header.magic = 0x426B /*'Bk'*/;
    bk_header.version = 1;
	os_get_key(ES_KEY_CONSOLE, &bk_header.NG_id);
    bk_header.tmd_size = tmd_size;
    bk_header.title_id_1 = 0x00010000;
    bk_header.title_id_2 = *(u32*)0;
    setAccessMask(bk_header.content_mask, index);

	if (FileSeek(bin, 0))
		LogPrintf("Couldn't seek back to start\n");
	else if (FileWrite(bin, &bk_header, sizeof(bk_header)))
		LogPrintf("Couldn't write bk_header\n");
	else if (FileSeek(bin, ROUND_UP(bin->pos, 64)))
		LogPrintf("Couldn't seek to 64-byte boundary\n");
	else if (FileWrite(bin, tmd, tmd_size))
		LogPrintf("Couldn't write TMD\n");
	else if (FileSeek(bin, ROUND_UP(bin->pos, 64)))
		LogPrintf("Couldn't seek to 64-byte boundary after TMD\n");
	else {
		LogPrintf("Created .bin file\n");
		return bin;
	}

	Dealloc(bin);
	return NULL;
}

s32 WriteBin(BinFile* file, u8* buffer, u32 numbytes)
{
	s32 result = FSERR_EINVAL;

	if (file && file->mode == BIN_WRITE)
	{
		s32 bytes_left;
		result = numbytes;

		// leading bytes, up to 16-byte boundary
		bytes_left = MIN((16-file->pos)&0xF, numbytes);
		if (bytes_left)
		{
			memcpy(file->buf+(16-bytes_left), buffer, bytes_left);
			buffer += bytes_left;
			numbytes -= bytes_left;
			file->pos += bytes_left;
			if (!(file->pos&0xF))
			{
				file->pos -= 16;
				dlc_aes_encrypt(file->iv, file->buf, file->buf, 16);
				if (FileWrite(file, file->buf, 16))
				{
					LogPrintf("Error writing leading encrypted block\n");
					return FSERR_EINVAL;
				}
				LogPrintf("WriteBin: sent 16 bytes leadin\n");
			}
		}

		bytes_left = numbytes&~0xF;
		if (bytes_left)
		{
			dlc_aes_encrypt(file->iv, buffer, buffer, bytes_left);
			if (FileWrite(file, buffer, bytes_left))
			{
				LogPrintf("Error writing bulk encrypted block\n");
				return FSERR_EINVAL;
			}
			buffer += bytes_left;
		}

		bytes_left = numbytes&0xF;
		if (bytes_left)
		{
			memcpy(file->buf, buffer, bytes_left);
			file->pos += bytes_left;
		}

		file->data_size += numbytes;
	} else
		LogPrintf("WriteBin: Missing file or file not opened for writing\n");

	return result;
}
