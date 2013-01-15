#include <ssl.h>
#include <network.h>
#include <errno.h>
#include <string.h>

static const char ssl_dev[] = "/dev/net/ssl";
static s32 __ssl_hid = -1;

#define SSL_HEAP_SIZE 32*1024

enum {
	IOCTLV_SSL_NEW = 1,
	IOCTLV_SSL_CONNECT,
	IOCTLV_SSL_HANDSHAKE,
	IOCTLV_SSL_READ,
	IOCTLV_SSL_WRITE,
	IOCTLV_SSL_SHUTDOWN,
	IOCTLV_SSL_SETCLIENTCERT,
	IOCTLV_SSL_SETCLIENTDEFAULTCERT,
	IOCTLV_SSL_REMOVECLIENTCERT,
	IOCTLV_SSL_SETROOTCA,
	IOCTLV_SSL_SETROOTCADEFAULT,
	IOCTLV_SSL_HANDSHAKEEX,
	IOCTLV_SSL_SETBUILTINROOTCA,
	IOCTLV_SSL_SETBUILTINCLIENTCERT,
	IOCTLV_SSL_DISABLEVERIFYOPTIONFORDEBUG,
	IOCTLV_SSL_DEBUGGETVERSION = 20,
	IOCTLV_SSL_DEBUGGETTIME
};

SSL::SSL(const char *hostname)
{
	if (__ssl_hid < 0)
		__ssl_hid = iosCreateHeap(SSL_HEAP_SIZE);

	// assign internal pointers
	host = (char*)(((u32)aligned_mem+31)&~31);
	index = (u32*)host;
	result = (s32*)(index+8);
	ssl_context = result+8;

	socket = net_socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
	if (socket >= 0)
	{
		struct hostent *ipData;
		struct sockaddr_in saddr;

		saddr.sin_family = AF_INET;
		saddr.sin_port = 443;
		host[255] = '\0';
		strncpy(host, hostname, 255);

		ipData = net_gethostbyname(hostname);
		if (ipData == NULL || ipData->h_length != sizeof(saddr.sin_addr) || ipData->h_addrtype != PF_INET || ipData->h_addr_list==NULL || ipData->h_addr_list[0]==NULL)
			saddr.sin_addr.s_addr = 0;
		else
			memcpy(&saddr.sin_addr, ipData->h_addr_list[0], ipData->h_length);

		if (net_connect(socket, (struct sockaddr*)&saddr, sizeof(saddr)) < 0)
		{
			net_close(socket);
			socket = -1;
		}
	}

	ssl_context[0] = -1;
}

SSL::~SSL()
{
	if (ssl_context[0] >= 1)
	{
		s32 fd = IOS_Open(ssl_dev, 0);
		if (fd >= 0)
		{
			result[0] = -1;
			IOS_IoctlvFormat(__ssl_hid, fd, IOCTLV_SSL_SHUTDOWN, "d:d", result, 32, ssl_context, 32);
			IOS_Close(fd);
		}
	}

	if (socket >= 0)
		net_close(socket);
}

s32 SSL::SetVerify(s32 verify_options)
{
	s32 ret, fd;

	if (socket < 0)
		return socket;

	if (ssl_context[0] >= 0)
		return 0;

	fd = IOS_Open(ssl_dev, 0);
	if (fd < 0)
		return fd;

	result[0] = verify_options;
	ret = IOS_IoctlvFormat(__ssl_hid, fd, IOCTLV_SSL_NEW, "d:dd", ssl_context, 32, result, 32, host, 256);

	IOS_Close(fd);
	return ret;
}

s32 SSL::SetBuiltinClientCert(u32 ind)
{
	s32 ret, fd;

	if (ssl_context[0] < 0)
		return ssl_context[0];

	fd = IOS_Open(ssl_dev, 0);
	if (fd < 0)
		return fd;

	result[0] = -1;
	index[0] = ind;
	ret = IOS_IoctlvFormat(__ssl_hid, fd, IOCTLV_SSL_SETBUILTINCLIENTCERT, "d:dd", result, 32, ssl_context, 32, index, 32);
	if (ret==0)
		ret = result[0];

	IOS_Close(fd);
	return ret;
}

s32 SSL::SetBuiltinRootCA(u32 ind)
{
	s32 ret, fd;

	if (ssl_context[0] < 0)
		return ssl_context[0];

	fd = IOS_Open(ssl_dev, 0);
	if (fd < 0)
		return fd;

	result[0] = -1;
	index[0] = ind;
	ret = IOS_IoctlvFormat(__ssl_hid, fd, IOCTLV_SSL_SETBUILTINROOTCA, "d:dd", result, 32, ssl_context, 32, index, 32);
	if (ret==0)
		ret = result[0];

	IOS_Close(fd);
	return ret;
}

s32 SSL::SetRootCA(const void *root_cert_der, size_t length_cert)
{
	s32 ret, fd;
	void *root;

	if (ssl_context[0] < 0)
		return ssl_context[0];

	root = iosAlloc(__ssl_hid, length_cert);
	if (root==NULL)
		return -ENOMEM;

	fd = IOS_Open(ssl_dev, 0);
	if (fd < 0)
	{
		iosFree(__ssl_hid, root);
		return fd;
	}

	result[0] = -1;
	memcpy(root, root_cert_der, length_cert);

	ret = IOS_IoctlvFormat(__ssl_hid, fd, IOCTLV_SSL_SETROOTCA, "d:dd", result, 32, ssl_context, 32, root, length_cert);
	if (ret==0)
		ret = result[0];

	iosFree(__ssl_hid, root);
	IOS_Close(fd);
	return ret;
}

s32 SSL::Connect(void)
{
	s32 ret, fd;

	if (ssl_context[0] < 0)
		return ssl_context[0];

	fd = IOS_Open(ssl_dev, 0);
	if (fd < 0)
		return fd;

	result[0] = -1;
	index[0] = socket;

	ret = IOS_IoctlvFormat(__ssl_hid, fd, IOCTLV_SSL_CONNECT, "d:dd", result, 32, ssl_context, 32, index, 32);
	if (ret==0)
		ret = result[0];

	IOS_Close(fd);
	return ret;
}

s32 SSL::Handshake(void)
{
	s32 ret, fd;

	if (ssl_context[0] < 0)
		return ssl_context[0];

	fd = IOS_Open(ssl_dev, 0);
	if (fd < 0)
		return fd;

	result[0] = -1;

	ret = IOS_IoctlvFormat(__ssl_hid, fd, IOCTLV_SSL_HANDSHAKE, "d:d", result, 32, ssl_context, 32);
	if (ret==0)
		ret = result[0];

	/*  KNOWN ERRORS:
	 *  -9: CN of certificate doesn't match hostname
	 * -10: unable to validate cert/CA (no trusted root?)
	 */

	IOS_Close(fd);
	return ret;
}

s32 SSL::Write(const void *data, size_t length)
{
	s32 ret, fd;
	u8 *source = (u8*)data;
	void *buffer;
	s32 written = 0;

	if (length==0)
		return 0;

	if (ssl_context[0] < 0)
		return ssl_context[0];

	buffer = iosAlloc(__ssl_hid, length > 256 ? 256 : length);
	if (buffer == NULL)
		return -ENOMEM;

	fd = IOS_Open(ssl_dev, 0);
	if (fd < 0)
	{
		iosFree(__ssl_hid, buffer);
		return fd;
	}

	while (length)
	{
		size_t to_write = length > 256 ? 256 : length;
		result[0] = -1;
		memcpy(buffer, source+written, to_write);
		ret = IOS_IoctlvFormat(__ssl_hid, fd, IOCTLV_SSL_WRITE, "d:dd", result, 32, ssl_context, 32, buffer, to_write);
		if (ret==0)
		{
			if (result[0] < 0)
			{
				ret = result[0];
				break;
			}

			written += to_write;
			if ((length -= to_write)==0)
				ret = written;
		}
		else
			break;
	}

	iosFree(__ssl_hid, buffer);
	IOS_Close(fd);
	return ret;
}

s32 SSL::Read(void *data, size_t length)
{
	s32 ret, fd;
	void *buffer;

	if (length > 0x2000)
		length = 0x2000;

	if (length==0)
		return 0;

	if (ssl_context[0] < 0)
		return ssl_context[0];

	buffer = iosAlloc(__ssl_hid, length);
	if (buffer == NULL)
		return -ENOMEM;

	fd = IOS_Open(ssl_dev, 0);
	if (fd < 0)
	{
		iosFree(__ssl_hid, buffer);
		return fd;
	}

	result[0] = -1;
	ret = IOS_IoctlvFormat(__ssl_hid, fd, IOCTLV_SSL_READ, "dd:d", result, 32, buffer, length, ssl_context, 32);
	if (ret==0)
	{
		ret = result[0];
		memcpy(data, buffer, ret);
	}

	iosFree(__ssl_hid, buffer);
	IOS_Close(fd);
	return ret;
}
