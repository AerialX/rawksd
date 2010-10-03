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
#endif

#define BIN_READ  1
#define BIN_WRITE 2

#define VERIFY_ERROR_SIZE		   0x00000001
#define VERIFY_ERROR_MAGIC		   0x00000002
#define VERIFY_ERROR_VERSION	   0x00000004
#define VERIFY_ERROR_WII_ID		   0x00000008
#define VERIFY_ERROR_ZEROES		   0x00000010
#define VERIFY_ERROR_TITLE1		   0x00000020
#define VERIFY_ERROR_TITLE2        0x00000040
#define VERIFY_ERROR_TITLE2_REGION 0x00000080
#define VERIFY_ERROR_PADDING       0x00000100

#define FSERR_EINVAL	-103

void dlc_aes_decrypt(u8 *iv, u8 *inbuf, u8 *outbuf, u32 len);
void dlc_aes_encrypt(u8 *iv, u8 *inbuf, u8 *outbuf, u32 len);

inline unsigned int verify_bk(BK_Header *bk)
{
	unsigned int failed=0;
	u32 wii_id = 0;
	os_get_4byte_key(ES_KEY_CONSOLE, &wii_id);

	if (bk->size != 0x70)
	{
		debug_printf("bk_header.size incorrect %d\n", bk->size);
		failed |= VERIFY_ERROR_SIZE;
	}
	if (bk->magic != 0x426B)
	{
		debug_printf("bk_header.magic incorrect %d\n", bk->magic);
		failed |= VERIFY_ERROR_MAGIC;
	}
	if (bk->version != 1)
	{
		debug_printf("bk_header.version incorrect %d\n", bk->version);
		failed |= VERIFY_ERROR_VERSION;
	}
	if (bk->NG_id != wii_id)
	{
		debug_printf("bk_header.NG_id incorrect %08X\n", bk->NG_id);
		failed |= VERIFY_ERROR_WII_ID;
	}
	if (bk->zeroes != 0)
	{
		debug_printf("bk_header.zeroes not zero %08X\n", bk->zeroes);
		failed |= VERIFY_ERROR_ZEROES;
	}
	if (bk->title_id_1 != 0x00010000)
	{
		debug_printf("bk_header.title_id_1 incorrect %08X\n", bk->title_id_1);
		failed |= VERIFY_ERROR_TITLE1;
	}
	if ((bk->title_id_2>>8) != ((*(u32*)0)>>8))
	{
		debug_printf("bk_header.title_id_2 front incorrect got %06X expected %06X\n", bk->title_id_2>>8, (*(u32*)0)>>8);
		failed |= VERIFY_ERROR_TITLE2;
	}
	if ((u8)bk->title_id_2 != *(u8*)3)
	{
		debug_printf("bk_header.title_id_2 region incorrect %02X %02X\n", (u8)bk->title_id_2, *(u8*)3);
		failed |= VERIFY_ERROR_TITLE2_REGION;
	}
	if (bk->padding[0] || bk->padding[1] || bk->padding[2])
	{
		debug_printf("bk_header.padding incorrect\n");
		failed |= VERIFY_ERROR_PADDING;
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

int FileWrite(BinFile* file, const void *buf, u32 size)
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
		return 1;
	file->pos = where;
	return 0;
}

BinFile* OpenBinRead(s32 file)
{
	u32 i, found=0;
	struct {
    	BK_Header bk_header;
    	sig_rsa2048 sig;
    	tmd tmd_header;
	} bin_header;
    tmd_content content_rec;
    BinFile* binfile = (BinFile*)Memalign(32, sizeof(BinFile));

    if (binfile==NULL)
    	return NULL;

    memset(binfile, 0, sizeof(BinFile));
    binfile->handle = file;
    binfile->mode = BIN_READ;

    if (FileSeek(binfile, 0) || FileRead(binfile, &bin_header, sizeof(bin_header)))
	{
		debug_printf("DLC Open failed reading bk_header or tmd_header\n");
		goto open_error;
	}

	i = verify_bk(&bin_header.bk_header);
	if (i) {
		u32 disc_title = (*(u32*)0)>>8;
		switch (disc_title) {
			case 0x00523336: // GDRB can use RB2 DLC (region must match)
				if (i & ~VERIFY_ERROR_TITLE2 || (bin_header.bk_header.title_id_2>>8) != 0x00535A41)
					goto open_error;
				break;
			case 0x00535A41: // RB2 customs
				if (i & ~(VERIFY_ERROR_TITLE2_REGION|VERIFY_ERROR_WII_ID) || (((u32)bin_header.tmd_header.title_id)>>8)!=0x00635242)
					goto open_error;
				break;
			default:
				goto open_error;
		}
	}

    binfile->header_size = ROUND_UP(bin_header.bk_header.tmd_size+sizeof(BK_Header), 64);
    binfile->data_size = bin_header.bk_header.contents_size;

    for(i = 0; i < bin_header.tmd_header.num_contents; i++)
    {
        if(checkAccessMask(bin_header.bk_header.content_mask, i))
        {
			if (FileSeek(binfile, sizeof(bin_header)+sizeof(tmd_content)*i) || FileRead(binfile, &content_rec, sizeof(tmd_content)))
				goto open_error;
			binfile->index = content_rec.index;
			binfile->iv[0] = content_rec.index>>8;
			binfile->iv[1] = (u8)content_rec.index;
			if (content_rec.type != 0x4001) {
				debug_printf("BIN contents not DLC\n");
				goto open_error;
			}
			if (bin_header.bk_header.contents_size != ROUND_UP(content_rec.size, 64))
			{
				if (((u32)bin_header.tmd_header.title_id>>8)==0x00635242)
					binfile->data_size = ROUND_UP(content_rec.size, 64);
				else
				{
					debug_printf("DLC Open size mismatch (%d vs. %d)\n", bin_header.bk_header.contents_size, ROUND_UP(content_rec.size, 64));
					goto open_error;
				}
			}
			found = 1;
			break;
        }
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
	static const u8 padding[49] ATTRIBUTE_ALIGN(32) = "BananaBananaBananaBananaBananaBananaBananaBanana";
	u8 padding_count;
	u32 total_size;

	file->pos &= 0xF;
	if (file->pos)
	{
		debug_printf("Flushing %u bytes\n", file->pos);
		dlc_aes_encrypt(file->iv, file->buf, file->buf, file->pos);
		FileWrite(file, file->buf, 16);
	}

	file->pos = ROUND_UP(file->data_size, 16);
	file->data_size = ROUND_UP(file->data_size, 64);
	total_size = file->header_size + file->data_size;
	padding_count = file->data_size - file->pos;

	if (padding_count) {
		debug_printf("Padding with %u bytes\n", padding_count);
		FileWrite(file, padding, padding_count);
	}

	if (!FileSeek(file, 0x18))
	{
		if (!FileWrite(file, &file->data_size, sizeof(file->data_size)))
			FileWrite(file, &total_size, sizeof(total_size));
	}

	debug_printf("Closed a WriteBin file\n");
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
		u8 *tempbuffer;
		result = numbytes;

		// leading bytes, should already be decrypted in buffer
		i = MIN((16 - file->pos)&0xF, numbytes);
		if (i) {
			memcpy(buffer, file->buf, i);
			numbytes -= i;
			buffer += i;
			file->pos += i;
			tempbuffer = (u8*)Memalign(32, ROUND_UP(numbytes, 16));
			if (tempbuffer==NULL)
				return -108; // FSErrors::OutOfMemory
		} else
			tempbuffer = buffer;

		// middle bytes (16 byte blocks)
		i = ROUND_UP(numbytes, 16);
		if (i) {
			if (FileRead(file, tempbuffer, i)) {
				if (tempbuffer != buffer)
					Dealloc(tempbuffer);
				return FSERR_EINVAL;
			}
			dlc_aes_decrypt(file->iv, tempbuffer, tempbuffer, i);
		}

		if (tempbuffer != buffer) {
			memcpy(buffer, tempbuffer, numbytes);
			if (numbytes&0xF) {
				file->pos -= i - numbytes;
				memcpy(file->buf, tempbuffer+numbytes, i-numbytes);
			}
			Dealloc(tempbuffer);
		}

	}

	if (result<0)
		debug_printf("ReadBin failed\n");
	return result;
}

BinFile* CreateBinFile(u16 index, u32* tmd_buf, u32 tmd_size, s32 file)
{
	BinFile *bin;

    BK_Header bk_header;

    debug_printf("CreateBinFile index %u %p %u\n", index, tmd, tmd_size);

	if (!tmd_buf || !tmd_size || index>=512)
		return NULL;

	tmd_content *content = ((tmd*)SIGNATURE_PAYLOAD(tmd_buf))->contents+index;

	bin = (BinFile*)Memalign(32, sizeof(BinFile));
	if (bin==NULL)
		return NULL;

	debug_printf("Allocated BinFile header\n");

	memset(bin, 0, sizeof(BinFile));
	bin->handle = file;
	bin->mode = BIN_WRITE;
	bin->iv[0] = content->index>>8;
	bin->iv[1] = (u8)content->index;
	bin->header_size = ROUND_UP(tmd_size+sizeof(bk_header), 64);

	memset(&bk_header, 0, sizeof(bk_header));
    bk_header.size = 0x70;
    bk_header.magic = 0x426B /*'Bk'*/;
    bk_header.version = 1;
	os_get_4byte_key(ES_KEY_CONSOLE, &bk_header.NG_id);
    bk_header.tmd_size = tmd_size;
    bk_header.title_id_1 = 0x00010000;
    bk_header.title_id_2 = *(u32*)0;
    setAccessMask(bk_header.content_mask, index);

	if (FileSeek(bin, 0))
		debug_printf("Couldn't seek back to start\n");
	else if (FileWrite(bin, &bk_header, sizeof(bk_header)))
		debug_printf("Couldn't write bk_header\n");
	else if (FileSeek(bin, ROUND_UP(bin->pos, 64)))
		debug_printf("Couldn't seek to 64-byte boundary\n");
	else if (FileWrite(bin, tmd_buf, tmd_size))
		debug_printf("Couldn't write TMD\n");
	else if (FileSeek(bin, ROUND_UP(bin->pos, 64)))
		debug_printf("Couldn't seek to 64-byte boundary after TMD\n");
	else {
		debug_printf("Created .bin file\n");
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

		// leading bytes, up to 16 (is this ever needed?)
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
					debug_printf("Error writing leading encrypted block\n");
					return FSERR_EINVAL;
				}
				debug_printf("WriteBin: sent 16 bytes leadin\n");
			}
		}

		bytes_left = numbytes&~0xF;
		if (bytes_left)
		{
			dlc_aes_encrypt(file->iv, buffer, buffer, bytes_left);
			if (FileWrite(file, buffer, bytes_left))
			{
				debug_printf("Error writing bulk encrypted block\n");
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
		debug_printf("WriteBin: Missing file or file not opened for writing\n");

	return result;
}
