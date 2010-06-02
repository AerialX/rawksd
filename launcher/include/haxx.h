#pragma once

#include <vector>

#include <gctypes.h>

// taken from MINI
typedef struct
{
	u8 boot1_hash[20];
	u8 common_key[16];
	u32 ng_id;
	union {
		struct {
			u8 ng_priv[30];
			u8 _wtf1[18];
		};
		struct {
			u8 _wtf2[28];
			u8 nand_hmac[20];
		};
	};
	u8 nand_key[16];
	u8 prng_key[16];
	u32 unk1;
	u32 unk2; // 0x00000007
} __attribute__((packed)) otp_t;

typedef struct
{
	u8 boot2version;
	u8 unknown1;
	u8 unknown2;
	u8 pad;
	u32 update_tag;
	u16 checksum;
} __attribute__((packed)) eep_ctr_t;

typedef struct
{
	union {
		struct {
			u32 dunno0; // 0x2 = MS
			u32 dunno1; // 0x1 = CA
			u32 ng_key_id;
			u8 ng_sig[60];
			eep_ctr_t counters[2];
			u8 fill[0x18];
			u8 korean_key[16];
		};
		u8 data[256];
	};
} __attribute__((packed)) seeprom_t;

extern otp_t otp;
//extern seeprom_t seeprom;
#define SYS_CERTS_SIZE      2560
extern u8 sys_certs[SYS_CERTS_SIZE];

int get_certs();
int Haxx_Init();
void Haxx_Mount(std::vector<int>* mounted);
bool forge_sig(u8 *data, u32 length);
int check_cert_chain(const u8 *data, const u32 data_len);

#define HAXX_IOS 0x0000000100000025ULL
#define HAXX_IOS_REVISION 3869
