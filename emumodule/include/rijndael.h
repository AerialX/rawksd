#ifndef __RIJNDAEL_H__
#define __RIJNDAEL_H__

#include "gctypes.h"

#ifdef __cplusplus
extern "C" {
#endif

void dlc_aes_decrypt(u8 *iv, u8 *inbuf, u8 *outbuf, u32 len);
void dlc_aes_encrypt(u8 *iv, u8 *inbuf, u8 *outbuf, u32 len);

#ifdef __cplusplus
}
#endif

#endif
