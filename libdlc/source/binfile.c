#include <stdlib.h>
#include <string.h>

#include "binfile.h"
#include "macros.h"
#include "byteswap.h"
#include "aes.h"

#include <files.h>

typedef u32 sigtype;
typedef sigtype sig_header;
typedef sig_header signed_blob;

typedef u8 sha1[20];
typedef u8 aeskey[16];

typedef struct _sig_rsa2048 {
	sigtype type;
	u8 sig[256];
	u8 fill[60];
} __attribute__((packed)) sig_rsa2048;

typedef struct _sig_rsa4096 {
	sigtype type;
	u8 sig[512];
	u8 fill[60];
}  __attribute__((packed)) sig_rsa4096;

typedef char sig_issuer[0x40];

typedef struct _tmd_content {
	u32 cid;
	u16 index;
	u16 type;
	u64 size;
	sha1 hash;
}  __attribute__((packed)) tmd_content;

typedef struct _tmd {
	sig_issuer issuer;
	u8 version;
	u8 ca_crl_version;
	u8 signer_crl_version;
	u8 fill2;
	u64 sys_version;
	u64 title_id;
	u32 title_type;
	u16 group_id;
	u16 zero; // part of u32 region, but always zero. split due to misalignment
	u16 region;
	u8 ratings[16];
	u8 reserved[42];
	u32 access_rights;
	u16 title_version;
	u16 num_contents;
	u16 boot_index;
	u16 fill3;
	// content records follow
	// C99 flexible array
	tmd_content contents[];
} __attribute__((packed)) tmd;

void LogPrintf(const char *fmt, ...);

s32 InitBinHandler()
{
    u8 magic_key[16] = {0x00, 0x00, 0x00, 0x00,
                 0x00, 0x00, 0x00, 0x00,
                 0x00, 0x00, 0x00, 0x00,
                 0x00, 0x00, 0x00, 0x00};

  	aes_set_key(magic_key);

  	return 1;
}

BinFileRead* OpenBinRead(s32 file)
{
    if(!file)
        return NULL;

    u32 i;

    BK_Header bk_header;
    tmd tmd_header;
	
	File_Read(file, &bk_header, sizeof(BK_Header));
	File_Seek(file, 0x80 + sizeof(sig_rsa2048), SEEK_SET);
	File_Read(file, &tmd_header, sizeof(tmd));
	
	BinFileRead* binfile = Alloc(sizeof(BinFileRead));

	File_Seek(file, 0, SEEK_END);
	s32 filesize = File_Tell(file);
	
	for(i = 0; i < tmd_header.num_contents; i++) {
		if (checkAccessMask(bk_header.content_mask, i) != 0) {
			binfile->content_num = i;
			break;
		}
	}
	
	binfile->content_position = (0x80 + sizeof(sig_rsa2048) + sizeof(tmd) + sizeof(tmd_content) * tmd_header.num_contents + 0x40 - 1) & ~(0x40 - 1);
	
    binfile->handle = file;
    binfile->size = (filesize - binfile->content_position + 15) & ~15;

	SeekBin(binfile, 0, SEEK_SET); // Set up the initial IV and seek
	
    return binfile;
}

void CloseReadBin(BinFileRead* file)
{
    Dealloc(file);
}

s32 SeekBin(BinFileRead* file, s32 where, u32 orgin)
{
    switch (orgin) {
        case SEEK_SET:
            if (where < 0 || where > file->size)
                return 0;
			break;
        case SEEK_CUR:
			where += file->position;
			break;
        case SEEK_END:
            if (where > 0 || file->size + where < 0)
                return 0;
            where += file->size;
			break;
        default:
            return 0;
    }
	s32 pos = (where % 0x10 == 0) ? where : (((where + 0x0F) & (~0x0F)) - 0x10);
	if (pos == 0) {
		memset(file->iv_buffer, 0, 0x10);
		memcpy(file->iv_buffer, &(file->content_num), 2);
		File_Seek(file->handle, file->content_position, SEEK_SET);
	} else {
		File_Seek(file->handle, pos - 0x10 + file->content_position, SEEK_SET);
		File_Read(file->handle, file->iv_buffer, 0x10);
	}
	file->position = where;
	
	return file->position;
}

s32 ReadBin(BinFileRead* file, u8* buffer, u32 numbytes)
{
	if (file->position + numbytes >= file->size)
		numbytes = file->size - file->position;

	aes_decrypt3(file->iv_buffer, file->handle, buffer, file->position % 0x10, numbytes);
	SeekBin(file, file->position + numbytes, SEEK_SET);
	return numbytes;
}


s32 CreateBin(u32 title, u32 content_id, void* tmdFile, s32 appFile, s32 binFile)
{
    s32 ret = 0;
    u32 i;

    ASSERT_JMP( tmdFile && appFile && binFile , err );

    TMD_Header tmd_header = *(TMD_Header*)tmdFile;
    TMD_Content tmd_content, my_content;
    u16 content_num = -1;

    ASSERT_JMP( FileSeek(binFile, sizeof(BK_Header), 0) != 0 , err );
    ASSERT_JMP( FilePadding(binFile, 0x40) != 0 , err );

    //ASSERT_JMP( ReadTMD(tmdFile, &tmd_header) != 0 , err );
    ASSERT_JMP( WriteTMD(binFile, &tmd_header) != 0 , err ); //no changes to tmd for now
	my_content.size = 0;
    for(i = 0; i < tmd_header.contents_number; i++)
    {
		tmd_content = *((TMD_Content*)((u8*)tmdFile + sizeof(TMD_Header)) + i);
        //ASSERT_JMP( ReadTMDContent(tmdFile, &tmd_content) != 0 , err );
        ASSERT_JMP( WriteTMDContent(binFile, &tmd_content) != 0 , err );

        if(tmd_content.content_id == content_id)
        {
            content_num = bswap_16(i);
            my_content = tmd_content;
        }
    }

    ASSERT_JMP( content_num != -1 , err );

    ASSERT_JMP( FilePadding(binFile, 0x40) != 0 , err );

    u8 iv[16];
    memset(iv, 0, 16);
    memcpy(iv, &content_num, 2);

    aes_encrypt_file(iv, appFile, binFile, my_content.size);

    ASSERT_JMP( FilePadding(binFile, 0x40) != 0 , err );

    BK_Header bk_header;
    bk_header.size = sizeof(BK_Header);
    bk_header.magic = 1114308609;
    bk_header.NG_id = 70225589;
    memset(bk_header.unknown1, 0, 8);
    bk_header.tmd_size = sizeof(TMD_Header) + tmd_header.contents_number * sizeof(TMD_Content);
    bk_header.contents_size = ((my_content.size + 63) & ~63);
    bk_header.content_bin_size = ftell(binFile);
    memset(bk_header.content_mask, 0, 0x40);
    setAccessMask(bk_header.content_mask, bswap_16(content_num));

    u32 t = tmd_header.title_id_1;
    *(u8*)(&t) = 0x00;
    bk_header.title_id_1 = t;

    t = tmd_header.title_id_2;
    *((u8*)&t + 3) = 'S';
    bk_header.title_id_2 = t;

    memset(bk_header.unknown2, 0, 8);

    FileSeek(binFile, 0, 0);

    WriteBK(binFile, &bk_header);

    ret = 1;
err:
    return ret;
}
