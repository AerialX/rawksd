/*-------------------------------------------------------------

es.h -- tik services

Copyright (C) 2008
Michael Wiedenbauer (shagkur)
Dave Murphy (WinterMute)
Hector Martin (marcan)

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1.	The origin of this software must not be misrepresented; you
must not claim that you wrote the original software. If you use
this software in a product, an acknowledgment in the product
documentation would be appreciated but is not required.

2.	Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3.	This notice may not be removed or altered from any source
distribution.

-------------------------------------------------------------*/

#ifndef __ES_H__
#define __ES_H__

#include <gctypes.h>
#include <gcutil.h>

#define ES_EINVAL			-0x1004
#define ES_ENOMEM			-0x100C
#define ES_ENOTINIT			-0x1100
#define ES_EALIGN			-0x1101

#define ES_SIG_RSA4096		0x10000
#define ES_SIG_RSA2048		0x10001
#define ES_SIG_ECC			0x10002

#define ES_CERT_RSA4096		0
#define ES_CERT_RSA2048		1
#define ES_CERT_ECC			2

#define ES_KEY_CONSOLE		1
#define ES_KEY_COMMON		4
#define ES_KEY_PRNG         5
#define ES_KEY_SDCARD		6

#ifdef __cplusplus
   extern "C" {
#endif /* __cplusplus */

typedef u32 sigtype;
typedef sigtype sig_header;
typedef sig_header signed_blob;

typedef u8 sha1[20];
typedef u8 aeskey[16];

typedef struct _BK_Header
{
    u32 size;
    u16 magic;
    u16 version;
    u32 NG_id;
    u64 zeroes;
    u32 tmd_size;
    u32 contents_size;
    u32 content_bin_size;
    u8  content_mask[0x40];
    u32 title_id_1;
    u32 title_id_2;
    u64 padding[3];
} __attribute__((packed)) BK_Header;

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

typedef struct _tiklimit {
	u32 tag;
	u32 value;
} __attribute__((packed)) tiklimit;

typedef struct _tikview {
	u32 view;
	u64 ticketid;
	u32 devicetype;
	u64 titleid;
	u16 access_mask;
	u8 reserved[0x3c];
	u8 cidx_mask[0x40];
	u16 padding;
	tiklimit limits[8];
} __attribute__((packed)) tikview;

typedef struct _tik {
	sig_issuer issuer;
	u8 fill[63]; //TODO: not really fill
	aeskey cipher_title_key;
	u8 fill2;
	u64 ticketid;
	u32 devicetype;
	u64 titleid;
	u16 access_mask;
	u8 reserved[0x3c];
	u8 cidx_mask[0x40];
	u16 padding;
	tiklimit limits[8];
} __attribute__((packed)) tik;

typedef struct _tmd_content {
	u32 cid;
	u16 index;
	u16 type;
	u64 size;
	sha1 hash;
}  __attribute__((packed)) tmd_content;

typedef struct _tmd {
	sig_issuer issuer;              //0x140
	u8 version;                     //0x180
	u8 ca_crl_version;              //0x181
	u8 signer_crl_version;          //0x182
	u8 vwii_title;                  //0x183
	u64 sys_version;                //0x184
	u64 title_id;                   //0x18c
	u32 title_type;                 //0x194
	u16 group_id;                   //0x198
	u16 zero;                       //0x19a
	u16 region;                     //0x19c
	u8 ratings[16];                 //0x19e
	u8 reserved[12];                //0x1ae
	u8 ipc_mask[12];                //0x1ba
	u8 reserved2[18];               //0x1c6
	u32 access_rights;              //0x1d8
	u16 title_version;              //0x1dc
	u16 num_contents;               //0x1de
	u16 boot_index;                 //0x1e0
	u16 fill2;
	// content records follow
	// C99 flexible array
	tmd_content contents[];
} __attribute__((packed)) tmd;

typedef struct _tmd_view_content
{
  u32 cid;
  u16 index;
  u16 type;
  u64 size;
} __attribute__((packed)) tmd_view_content;

typedef struct _tmdview
{
	u8 version; // 0x0000;
	u8 filler[3];
	u64 sys_version; //0x0004
	u64 title_id; // 0x00c
	u32 title_type; //0x0014
	u16 group_id; //0x0018
	u8 reserved[0x3e]; //0x001a this is the same reserved 0x3e bytes from the tmd
	u16 title_version; //0x0058
	u16 num_contents; //0x005a
	tmd_view_content contents[]; //0x005c
}__attribute__((packed)) tmd_view;

typedef struct _cert_header {
	sig_issuer issuer;
	u32 cert_type;
	char cert_name[64];
	u32 cert_id; //???
} __attribute__((packed)) cert_header;

typedef struct _cert_rsa2048 {
	sig_issuer issuer;
	u32 cert_type;
	char cert_name[64];
	u32 cert_id;
	u8 modulus[256];
	u32 exponent;
	u8 pad[0x34];
} __attribute__((packed)) cert_rsa2048;

typedef struct _cert_rsa4096 {
	sig_issuer issuer;
	u32 cert_type;
	char cert_name[64];
	u32 cert_id;
	u8 modulus[512];
	u32 exponent;
	u8 pad[0x34];
} __attribute__((packed)) cert_rsa4096;

#define TMD_SIZE(x) (((x)->num_contents)*sizeof(tmd_content) + sizeof(tmd))
// backwards compatibility
#define TMD_CONTENTS(x) ((x)->contents)

//TODO: add ECC stuff

#define IS_VALID_SIGNATURE(x) (((*(x))==ES_SIG_RSA2048) || ((*(x))==ES_SIG_RSA4096))

#define SIGNATURE_SIZE(x) (\
	((*(x))==ES_SIG_RSA2048) ? sizeof(sig_rsa2048) : ( \
	((*(x))==ES_SIG_RSA4096) ? sizeof(sig_rsa4096) : 0 ))

#define SIGNATURE_SIG(x) (((u8*)x)+4)

#define IS_VALID_CERT(x) ((((x)->cert_type)==ES_CERT_RSA2048) || (((x)->cert_type)==ES_CERT_RSA4096))

#define CERTIFICATE_SIZE(x) (\
	(((x)->cert_type)==ES_CERT_RSA2048) ? sizeof(cert_rsa2048) : ( \
	(((x)->cert_type)==ES_CERT_RSA4096) ? sizeof(cert_rsa4096) : 0 ))

#define SIGNATURE_PAYLOAD(x) ((void *)(((u8*)(x)) + SIGNATURE_SIZE(x)))

#define SIGNED_TMD_SIZE(x) ( TMD_SIZE((tmd*)SIGNATURE_PAYLOAD(x)) + SIGNATURE_SIZE(x))
#define SIGNED_TIK_SIZE(x) ( sizeof(tik) + SIGNATURE_SIZE(x) )
#define SIGNED_CERT_SIZE(x) ( CERTIFICATE_SIZE((cert_header*)SIGNATURE_PAYLOAD(x)) + SIGNATURE_SIZE(x))

#define STD_SIGNED_TIK_SIZE ( sizeof(tik) + sizeof(sig_rsa2048) )

#define MAX_NUM_TMD_CONTENTS 512

#define MAX_TMD_SIZE ( sizeof(tmd) + MAX_NUM_TMD_CONTENTS*sizeof(tmd_content) )
#define MAX_SIGNED_TMD_SIZE ( MAX_TMD_SIZE + sizeof(sig_rsa2048) )


#ifdef __cplusplus
   }
#endif

#endif
