#include <stdlib.h>
#include <string.h>

#include "wiistruct.h"
#include "macros.h"

s32 ReadBK(s32 file, BK_Header* header)
{
    ASSERT( ReadU32(file, &(header->size)) != 0 );
    ASSERT( ReadU32(file, &(header->magic)) != 0 );
    ASSERT( ReadU32(file, &(header->NG_id)) != 0 );
    ASSERT( ReadBytes(file, (header->unknown1), 8) != 0);
    ASSERT( ReadU32(file, &(header->tmd_size)) != 0 );
    ASSERT( ReadU32(file, &(header->contents_size)) != 0 );
    ASSERT( ReadU32(file, &(header->content_bin_size)) != 0 );
    ASSERT( ReadBytes(file, (header->content_mask), 0x40) != 0);
    ASSERT( ReadU32(file, &(header->title_id_1)) != 0 );
    ASSERT( ReadU32(file, &(header->title_id_2)) != 0 );
    ASSERT( ReadBytes(file, (header->unknown2), 8) != 0);

    return 1;
}

s32 ReadTMD(s32 file, TMD_Header* header)
{
    ASSERT( ReadU32(file, &(header->signature_type)) != 0 );
    ASSERT( ReadBytes(file, (header->signature), 256) != 0);
    ASSERT( ReadBytes(file, (header->padding1), 60) != 0);
    ASSERT( ReadBytes(file, (header->issuer), 64) != 0);
    ASSERT( ReadU8(file, &(header->version)) != 0 );
    ASSERT( ReadU8(file, &(header->ca_crl_version)) != 0 );
    ASSERT( ReadU8(file, &(header->signer_crl_version)) != 0 );
    ASSERT( ReadU8(file, &(header->padding2)) != 0 );
    ASSERT( ReadU32(file, &(header->ios_id_1)) != 0 );
    ASSERT( ReadU32(file, &(header->ios_id_2)) != 0 );
    ASSERT( ReadU32(file, &(header->title_id_1)) != 0 );
    ASSERT( ReadU32(file, &(header->title_id_2)) != 0 );
    ASSERT( ReadU32(file, &(header->title_type)) != 0 );
    ASSERT( ReadU16(file, &(header->group_id)) != 0 );
    ASSERT( ReadBytes(file, (header->reserved), 62) != 0);
    ASSERT( ReadU32(file, &(header->access_rights)) != 0 );
    ASSERT( ReadU16(file, &(header->title_version)) != 0 );
    ASSERT( ReadU16(file, &(header->contents_number)) != 0 );
    ASSERT( ReadU16(file, &(header->boot_index)) != 0 );
    ASSERT( ReadU16(file, &(header->padding3)) != 0 );

    return 1;
}

s32 ReadTMDContent(s32 file, TMD_Content* content)
{
    ASSERT( ReadU32(file, &(content->content_id)) != 0 );
    ASSERT( ReadU16(file, &(content->index)) != 0 );
    ASSERT( ReadU16(file, &(content->type)) != 0 );
    ASSERT( ReadU64(file, &(content->size)) != 0 );
    ASSERT( ReadBytes(file, (content->hash), 20) != 0);

    return 1;
}

s32 SkipBKHeader(s32 file)
{
    ASSERT( SkipBytes(file, sizeof(BK_Header)) != 0 );

    return 1;
}

s32 SkipTMD(s32 file)
{
    TMD_Header tmd_header;
    ASSERT_N( ReadTMD(file, &tmd_header) != 0);
    ASSERT( SkipBytes(file, tmd_header.contents_number * sizeof(BK_Header)) != 0 );

    return 1;
}

s32 SkipTMDContent(s32 file)
{
    ASSERT( SkipBytes(file, sizeof(TMD_Content)) != 0 );

    return 1;
}

s32 WriteBK(s32 file, BK_Header* header)
{
    ASSERT( WriteU32(file, (header->size)) != 0 );
    ASSERT( WriteU32(file, (header->magic)) != 0 );
    ASSERT( WriteU32(file, (header->NG_id)) != 0 );
    ASSERT( WriteBytes(file, (header->unknown1), 8) != 0);
    ASSERT( WriteU32(file, (header->tmd_size)) != 0 );
    ASSERT( WriteU32(file, (header->contents_size)) != 0 );
    ASSERT( WriteU32(file, (header->content_bin_size)) != 0 );
    ASSERT( WriteBytes(file, (header->content_mask), 0x40) != 0);
    ASSERT( WriteU32(file, (header->title_id_1)) != 0 );
    ASSERT( WriteU32(file, (header->title_id_2)) != 0 );
    ASSERT( WriteBytes(file, (header->unknown2), 8) != 0);

    return 1;
}

s32 WriteTMD(s32 file, TMD_Header* header)
{
    ASSERT( WriteU32(file, (header->signature_type)) != 0 );
    ASSERT( WriteBytes(file, (header->signature), 256) != 0);
    ASSERT( WriteBytes(file, (header->padding1), 60) != 0);
    ASSERT( WriteBytes(file, (header->issuer), 64) != 0);
    ASSERT( WriteU8(file, (header->version)) != 0 );
    ASSERT( WriteU8(file, (header->ca_crl_version)) != 0 );
    ASSERT( WriteU8(file, (header->signer_crl_version)) != 0 );
    ASSERT( WriteU8(file, (header->padding2)) != 0 );
    ASSERT( WriteU32(file, (header->ios_id_1)) != 0 );
    ASSERT( WriteU32(file, (header->ios_id_2)) != 0 );
    ASSERT( WriteU32(file, (header->title_id_1)) != 0 );
    ASSERT( WriteU32(file, (header->title_id_2)) != 0 );
    ASSERT( WriteU32(file, (header->title_type)) != 0 );
    ASSERT( WriteU16(file, (header->group_id)) != 0 );
    ASSERT( WriteBytes(file, (header->reserved), 62) != 0);
    ASSERT( WriteU32(file, (header->access_rights)) != 0 );
    ASSERT( WriteU16(file, (header->title_version)) != 0 );
    ASSERT( WriteU16(file, (header->contents_number)) != 0 );
    ASSERT( WriteU16(file, (header->boot_index)) != 0 );
    ASSERT( WriteU16(file, (header->padding3)) != 0 );

    return 1;
}

s32 WriteTMDContent(s32 file, TMD_Content* content)
{
    ASSERT( WriteU32(file, (content->content_id)) != 0 );
    ASSERT( WriteU16(file, (content->index)) != 0 );
    ASSERT( WriteU16(file, (content->type)) != 0 );
    ASSERT( WriteU64(file, (content->size)) != 0 );
    ASSERT( WriteBytes(file, (content->hash), 20) != 0);

    return 1;
}
