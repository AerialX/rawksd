#include <gccore.h>

class SSL
{
private:
	u8 aligned_mem[256+32+32+31];
	char *host;
	u32 *index;
	s32 *result;
	s32 *ssl_context;

	s32 socket;
public:
	SSL(const char *hostname);
	~SSL();

	s32 SetVerify(s32 verify_options);
	s32 SetBuiltinClientCert(u32 index);
	s32 SetBuiltinRootCA(u32 index);
	s32 SetRootCA(const void *root_cert_der, size_t length_cert);
	s32 Connect(void);
	s32 Handshake(void);
	s32 Write(const void *data, size_t length);
	s32 Read(void *data, size_t length);
};