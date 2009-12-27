#include "aes.h"
#include "file_read.h"

void encrypt(u8 *buff);
void decrypt(u8 *buff);

void aes_decrypt3(u8 *iv, s32 infile, u8* outbuf, u32 bound1, u32 len)
{
	static u8 block[0x10] __attribute__ ((aligned(0x20)));
	static u8 block2[0x10] __attribute__ ((aligned(0x20)));
	unsigned int blockno = 0, i;

	for (blockno = 0; blockno <= ((len + bound1) / sizeof(block)); blockno++) {
		unsigned int fraction;
		if (blockno == ((len + bound1) / sizeof(block))) { // last block
			fraction = (len + bound1) % sizeof(block);
			if (fraction == 0)
				break;
			memset(block, 0, sizeof(block));
		} else
			fraction = 0x10;

		File_Read(infile, block, 0x10);
		memcpy(block2, block, 0x10);
		decrypt(block);

		for(i=bound1; i < fraction; i++) {
			*outbuf = block[i] ^ iv[i];
			outbuf++;
		}
		bound1 = 0;
		memcpy(iv, block2, 0x10);
	}
}

// CBC mode encryption
void aes_encrypt_file(u8 *iv, s32 infile, s32 outfile, u64 len)
{
    u8 block[16];
    unsigned int blockno = 0, i;

    for (blockno = 0; blockno <= (len / sizeof(block)); blockno++)
    {
        unsigned int fraction;
        if (blockno == (len / sizeof(block)))
        {
            // last block
            fraction = len % sizeof(block);
            if (fraction == 0)
                break;
            memset(block, 0, sizeof(block));
        }
        else
            fraction = 16;

		fread(block, fraction, 1, infile);

        for(i=0; i < fraction; i++)
            block[i] ^= iv[i];

        encrypt(block);
        memcpy(iv, block, sizeof(block));
		fwrite(block, 16, 1, outfile);
    }
}
