#ifndef __WIISTRUCT_H__
#define __WIISTRUCT_H__

#include "gctypes.h"
#include "file_read.h"
#include "byteswap.h"

typedef struct
{
    u32 size;
    u32 magic;
    u32 NG_id;
    u8  unknown1[8];
    u32 tmd_size;
    u32 contents_size;
    u32 content_bin_size;
    u8  content_mask[0x40];
    u32 title_id_1;
    u32 title_id_2;
    u8  unknown2[8];
} __attribute__((packed)) BK_Header;

typedef struct
{
    u32 signature_type;
    u8  signature[256];
    u8  padding1[60];
    u8  issuer[64];
    u8  version;
    u8  ca_crl_version;
    u8  signer_crl_version;
    u8  padding2;
    u32 ios_id_1;
    u32 ios_id_2;
    u32 title_id_1;
    u32 title_id_2;
    u32 title_type;
    u16 group_id;
    u8  reserved[62];
    u32 access_rights;
    u16 title_version;
    u16 contents_number;
    u16 boot_index;
    u16 padding3;
} __attribute__((packed)) TMD_Header;

typedef struct
{
    u32 content_id;
    u16 index;
    u16 type;
    u64 size;
    u8  hash[20];
} __attribute__((packed)) TMD_Content;

s32 ReadBK(s32 file, BK_Header* header);
s32 ReadTMD(s32 file, TMD_Header* header);
s32 ReadTMDContent(s32 file, TMD_Content* content);

s32 SkipBKHeader(s32 file);
s32 SkipTMD(s32 file);
s32 SkipTMDContent(s32 file);

s32 WriteBK(s32 file, BK_Header* header);
s32 WriteTMD(s32 file, TMD_Header* header);
s32 WriteTMDContent(s32 file, TMD_Content* content);


#endif
