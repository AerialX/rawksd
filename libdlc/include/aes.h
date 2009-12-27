#pragma once

#include <rijndael.h>
#include "file_read.h"
#include <string.h>

void aes_decrypt3(u8 *iv, s32 infile, u8* outbuf, u32 bound1, u32 len);

void aes_encrypt_file(u8 *iv, s32 infile, s32 outfile, u64 len);
