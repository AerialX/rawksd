// Basic I/O.

static inline u32 read32(u32 addr)
{
	u32 x;
	asm volatile("lwz %0,0(%1) ; sync" : "=r"(x) : "b"(0xc0000000 | addr));
	return x;
}

static inline void write32(u32 addr, u32 x)
{
	asm("stw %0,0(%1) ; eieio" : : "r"(x), "b"(0xc0000000 | addr));
}

// USB Gecko.

void usb_flush(int chn);
int usb_sendbuffer(int chn,const void *buffer,int size);

// Version string.

extern const char version[];

// Debug: blink the tray led.

static inline void blink(void)
{
	write32(0x0d8000c0, read32(0x0d8000c0) ^ 0x20);
}

void debug_printf(const char *fmt, ...);
void hexdump(void *d, int len);

extern "C" {
void aes_set_key(u8 *key);
void aes_decrypt(u8 *iv, u8 *inbuf, u8 *outbuf, unsigned long long len);
void aes_encrypt(u8 *iv, u8 *inbuf, u8 *outbuf, unsigned long long len);
}

void installer_downgrade(bool skipdowngrade);
void installer_go(bool skipdowngrade);
void installer_init(bool skipdowngrade);
void installer_cleanup();

#define TRACE(x) debug_printf("%s / %d: %d\n", __FUNCTION__, __LINE__, (x))

#define ISFS_ACCESS_READ 1
#define ISFS_ACCESS_WRITE 2

// OMG MASTER HAXX
//#define EHCUSB

#define DOWNGRADED_IOS 15
#define DOWNGRADED_VERSION 257
#define PATCHED_IOS 37
#define HAXXED_IOS 36
//#define HAXXED_VERSION 1042
#define HAXXED_VERSION 3351
#ifdef RAWKHAXX
#define HAXXED_NEW_IOS 243
#endif
#ifdef RIIVOLUTION
#define HAXXED_NEW_IOS 242
#endif
#define SD_IOS 56
#define SD_VERSION 5405
#define SD_CONTENT 4
#define HAXXED_NEW_VERSION 4

#define HAXXED_NEW_TITLEID (0x0000000100000000ULL | (u64)HAXXED_NEW_IOS)
