#pragma once

#include "stdio.h"
#include "gctypes.h"

#ifdef __cplusplus
   extern "C" {
#endif

void aes_set_key(u8 *key);

void aes_decrypt(u8 *iv, u8 *inbuf, u8 *outbuf, u64 len);
void aes_decrypt2(u8 *iv, u8 *inbuf, u8 *outbuf, u64 len);

void aes_encrypt(u8 *iv, u8 *inbuf, u8 *outbuf, u64 len);

#ifdef __cplusplus
   }
#endif
