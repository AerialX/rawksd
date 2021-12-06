/*-------------------------------------------------------------

network_wii.c -- Wii network subsystem

Copyright (C) 2008 bushing

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

#define MAX_IP_RETRIES		100
#define MAX_INIT_RETRIES	20

#define debug_printf(fmt, args...)
#define net_malloc(a) 				Memalign(32, (a))
#define net_free(a) 				Dealloc(a)

#include <string.h>
#include <fcntl.h>
#include <ctype.h>

#include "gctypes.h"
#include "gcutil.h"
#include "ipc.h"
#include "network.h"
#include "syscalls.h"
#include "mem.h"
#include "timer.h"
#include "gpio.h"

#define MEM_PHYSICAL_TO_K0(x) x

#define IOS_O_NONBLOCK				0x04			//(O_NONBLOCK >> 16) - it's in octal representation, so this shift leads to 0 and hence nonblocking sockets didn't work. changed it to the right value.

#define IOCTL_NWC24_STARTUP			0x06

#define IOCTL_NCD_SETIFCONFIG3		0x03
#define IOCTL_NCD_SETIFCONFIG4		0x04
#define IOCTL_NCD_GETLINKSTATUS		0x07

#define NET_UNKNOWN_ERROR_OFFSET	-10000

#define SO_TIMER_MESSAGE			0x1337

enum {
	IOCTL_SO_ACCEPT	= 1,
	IOCTL_SO_BIND,
	IOCTL_SO_CLOSE,
	IOCTL_SO_CONNECT,
	IOCTL_SO_FCNTL,
	IOCTL_SO_GETPEERNAME, // todo
	IOCTL_SO_GETSOCKNAME, // todo
	IOCTL_SO_GETSOCKOPT,  // todo    8
	IOCTL_SO_SETSOCKOPT,
	IOCTL_SO_LISTEN,
	IOCTL_SO_POLL,        // todo    b
	IOCTLV_SO_RECVFROM,
	IOCTLV_SO_SENDTO,
	IOCTL_SO_SHUTDOWN,    // todo    e
	IOCTL_SO_SOCKET,
	IOCTL_SO_GETHOSTID,
	IOCTL_SO_GETHOSTBYNAME,
	IOCTL_SO_GETHOSTBYADDR,// todo
	IOCTLV_SO_GETNAMEINFO, // todo   13
	IOCTL_SO_UNK14,        // todo
	IOCTL_SO_INETATON,     // todo
	IOCTL_SO_INETPTON,     // todo
	IOCTL_SO_INETNTOP,     // todo
	IOCTLV_SO_GETADDRINFO, // todo
	IOCTL_SO_SOCKATMARK,   // todo
	IOCTLV_SO_UNK1A,       // todo
	IOCTLV_SO_UNK1B,       // todo
	IOCTLV_SO_GETINTERFACEOPT, // todo
	IOCTLV_SO_SETINTERFACEOPT, // todo
	IOCTL_SO_SETINTERFACE,     // todo
	IOCTL_SO_STARTUP,           // 0x1f
	IOCTL_SO_ICMPSOCKET =	0x30, // todo
	IOCTLV_SO_ICMPPING,         // todo
	IOCTL_SO_ICMPCANCEL,        // todo
	IOCTL_SO_ICMPCLOSE          // todo
};

struct bind_params {
	u32 socket;
	u32 has_name;
	u8 name[28];
};

struct connect_params {
	u32 socket;
	u32 has_addr;
	u8 addr[28];
};

struct sendto_params {
	u32 socket;
	u32 flags;
	u32 has_destaddr;
	u8 destaddr[28];
};

struct setsockopt_params {
	u32 socket;
	u32 level;
	u32 optname;
	u32 optlen;
	u8 optval[20];
};

// 0 means we don't know what this error code means
// I sense a pattern here...
static u8 _net_error_code_map[] = {
	0, // 0
 	E2BIG,
 	EACCES,
 	EADDRINUSE,
 	EADDRNOTAVAIL,
 	EAFNOSUPPORT, // 5
	EAGAIN,
	EALREADY,
	EBADFD,
 	EBADMSG,
 	EBUSY, // 10
 	ECANCELED,
 	ECHILD,
 	ECONNABORTED,
 	ECONNREFUSED,
 	ECONNRESET, // 15
 	EDEADLK,
 	EDESTADDRREQ,
 	EDOM,
 	EDQUOT,
 	EEXIST, // 20
 	EFAULT,
 	EFBIG,
 	EHOSTUNREACH,
 	EIDRM,
 	EILSEQ, // 25
	EINPROGRESS,
 	EINTR,
 	EINVAL,
 	EIO,
	EISCONN, // 30
 	EISDIR,
 	ELOOP,
 	EMFILE,
 	EMLINK,
 	EMSGSIZE, // 35
 	EMULTIHOP,
 	ENAMETOOLONG,
 	ENETDOWN,
 	ENETRESET,
 	ENETUNREACH, // 40
 	ENFILE,
 	ENOBUFS,
 	ENODATA,
 	ENODEV,
 	ENOENT, // 45
 	ENOEXEC,
 	ENOLCK,
 	ENOLINK,
 	ENOMEM,
 	ENOMSG, // 50
 	ENOPROTOOPT,
 	ENOSPC,
 	ENOSR,
 	ENOSTR,
 	ENOSYS, // 55
 	ENOTCONN,
 	ENOTDIR,
 	ENOTEMPTY,
 	ENOTSOCK,
 	ENOTSUP, // 60
 	ENOTTY,
 	ENXIO,
 	EOPNOTSUPP,
 	EOVERFLOW,
 	EPERM, // 65
 	EPIPE,
 	EPROTO,
 	EPROTONOSUPPORT,
 	EPROTOTYPE,
 	ERANGE, // 70
 	EROFS,
 	ESPIPE,
 	ESRCH,
 	ESTALE,
 	ETIME, // 75
 	ETIMEDOUT,
};

/* NetBios structs and defines */

#define OPCODE_QUERY		0
#define QUESTION_TYPE_NB    0x0020 // general name request
#define QUESTION_CLASS_IN   0x0001 // internet class
#define RRTYPE_NB			0x0020
#define RRCLASS_IN			0x0001

// reversed for big-endian (bitfields suck)
typedef struct {
	u16 fResponse:1;
	u16 OpCode:4;
	u16 fNM_AA:1;
	u16 fNM_TC:1;
	u16 fNM_RD:1;
	u16 fNM_RA:1;
	u16 fNM_00:2;
	u16 fNM_B:1;
	u16 RCode:4;
} OPCODEFLAGSRCODE;

typedef struct {
	u8 Name[34]; // compressed name
	u16 Type;    // question type
	u16 Class;   // question class (always type IN - Internet)
} QUESTION;

typedef struct
{
	QUESTION question;
	u32 TTL ATTRIBUTE_PACKED; // Time to live
	u16 RDLength ATTRIBUTE_PACKED; // length of following resource data
} RESOURCERECORDHEADER;

typedef struct {
	u16 TransactionID;		// transaction id, responses match original packet, requests are random/sequential
	u16 OpcodeFlagsRcode;	// opcode, flags and rcode
	u16 QDCount;			// number of questions
	u16 ANCount;			// number of answer resource records
	u16 NSCount;			// number of name service resource records
	u16 ARCount;			// number of authoratative resource records
} NBNAMEHEADER;

typedef struct {
	struct hostent he;
	char Name[16];
	char *alias_none;
	char *addr_list[2];
	// nameheader should be 32-byte aligned
	u8 padding[20];
	NBNAMEHEADER nameheader;
	RESOURCERECORDHEADER rrheader;
	u8 nbflags[2];
	u8 ip_addr[4];
} nb_lookup;

/* End NetBios */

static s32 net_ip_top_fd = -1;
static ipcmessage cbdata ATTRIBUTE_ALIGN(32);

static const char __manage_fs[] = "/dev/net/ncd/manage";
static const char __iptop_fs[] = "/dev/net/ip/top";
static const char __kd_fs[] = "/dev/net/kd/request";

static s32 _net_convert_error(s32 ios_retval)
{
//	return ios_retval;
	if (ios_retval >= 0) return ios_retval;
	if (ios_retval < -sizeof(_net_error_code_map)
		|| !_net_error_code_map[-ios_retval])
			return NET_UNKNOWN_ERROR_OFFSET + ios_retval;
	return -_net_error_code_map[-ios_retval];
}

static s32 _open_manage_fd(void)
{
	s32 ncd_fd;
	int i;

	for(i=0;i < MAX_INIT_RETRIES; i++)
	{
		ncd_fd = _net_convert_error(os_open(__manage_fs, 0));
		if (ncd_fd < 0)
			usleep(100000);
		else
			break;
	}

	return ncd_fd;
}

s32 NCDGetLinkStatus(void)
{
	s32 ret, ncd_fd;
	STACK_ALIGN(u8, linkinfo, 0x20, 32);
	STACK_ALIGN(ioctlv, vec, 1, 32);

	ncd_fd = _open_manage_fd();
	if (ncd_fd < 0) return ncd_fd;

	vec[0].data = linkinfo;
	vec[0].len = 0x20;

	ret = _net_convert_error(os_ioctlv(ncd_fd, IOCTL_NCD_GETLINKSTATUS, 0, 1, vec));

	os_close(ncd_fd);

  	if (ret < 0) debug_printf("NCDGetLinkStatus returned error %d\n", ret);

	return ret;
}

static s32 NWC24iStartupSocket(void)
{
	s32 kd_fd, ret;
	STACK_ALIGN(u8, kd_buf, 0x20, 32);

	kd_fd = _net_convert_error(os_open(__kd_fs, 0));
	if (kd_fd < 0) {
		debug_printf("IOS_Open(%s) failed with code %d\n", __kd_fs, kd_fd);
		return kd_fd;
	}

	ret = _net_convert_error(os_ioctl(kd_fd, IOCTL_NWC24_STARTUP, NULL, 0, kd_buf, 0x20));
	if (ret < 0) debug_printf("IOS_Ioctl(6)=%d\n", ret);
  	os_close(kd_fd);
  	return ret;
}

u32 net_gethostip(void)
{
	u32 ip_addr=0;
	int retries;

	if (net_ip_top_fd < 0) return 0;
	for (retries=0, ip_addr=0; !ip_addr && retries < MAX_IP_RETRIES; retries++) {
		ip_addr = os_ioctl(net_ip_top_fd, IOCTL_SO_GETHOSTID, 0, 0, 0, 0);
		if (!ip_addr) usleep(100000);
	}

	return ip_addr;
}

s32 net_init(void)
{
	s32 ret;
	u32 ip_addr = 0;
	u8 *octets;

	if (net_ip_top_fd >= 0) return 0;

	Timer_Init();

	ret = NCDGetLinkStatus();  // this must be called as part of initialization
	debug_printf("NCDGetLinkStatus returned %d\n", ret);
	if (ret < 0) {
		return ret;
	}

	net_ip_top_fd = _net_convert_error(os_open(__iptop_fs, 0));
	debug_printf("IOS_Open(/dev/net/ip/top)=%d\n", net_ip_top_fd);
	if (net_ip_top_fd < 0) {
		return net_ip_top_fd;
	}

	ret = NWC24iStartupSocket(); // this must also be called during init
	debug_printf("NWC24iStartupSocket returned %d\n", ret);
	if (ret < 0) {
		goto error;
	}

	ret = _net_convert_error(os_ioctl(net_ip_top_fd, IOCTL_SO_STARTUP, 0, 0, 0, 0));
	debug_printf("IOCTL_SO_STARTUP returned %d\n", ret);
	if (ret < 0) {
		goto error;
	}

	ip_addr=net_gethostip();

	if (!ip_addr) {
		debug_printf("Unable to obtain IP address\n");
		ret = -ETIMEDOUT;
		goto error;
	}

	octets = (u8 *) &ip_addr;
	debug_printf(" %d.%d.%d.%d\n", octets[0], octets[1], octets[2], octets[3]);
	(void)octets;

	return 0;

error:
	net_deinit();
	return ret;
}

void net_deinit() {
	if (net_ip_top_fd >= 0)
		os_close(net_ip_top_fd);
	net_ip_top_fd = -1;
}

struct hostent * net_gethostbyname(const char *addrString)
{
	return net_gethostbyname_async(addrString, 0);
}

static u8 ipBuffer[0x460] ATTRIBUTE_ALIGN(32);
/* Returned value is a static buffer -- this function is not threadsafe! */
struct hostent * net_gethostbyname_async(const char *addrString, u32 timeout)
{
	s32 len, i, ret = -ETIMEDOUT;
	u8 *params;
	struct hostent *ipData;
	u32 addrOffset;

	memset(ipBuffer, 0, sizeof(ipBuffer));

	if (net_ip_top_fd < 0) {
		errno = -ENXIO;
		return NULL;
	}

	if (addrString==NULL)
	{
		errno = -EINVAL;
		return NULL;
	}

	len = strlen(addrString) + 1;
	if ((u32)addrString & 0x1F)
	{
		params = net_malloc(len);
		if (params==NULL) {
			errno = IPC_ENOMEM;
			return NULL;
		}
		memcpy(params, addrString, len);
	}
	else
		params = (u8*)addrString;

	os_sync_after_write(params, len);

	if (timeout == 0) {
		ret = _net_convert_error(os_ioctl(net_ip_top_fd, IOCTL_SO_GETHOSTBYNAME, params, len, ipBuffer, 0x460));
	} else {
		u32 queuedata[0x02];
		int queue = os_message_queue_create(queuedata, sizeof(queuedata)/sizeof(u32));
		int timer = os_create_timer(timeout, 0, queue, SO_TIMER_MESSAGE);
		u32 message;

		os_ioctl_async(net_ip_top_fd, IOCTL_SO_GETHOSTBYNAME, params, len, ipBuffer, 0x460, queue, &cbdata);

		while (!(ret = os_message_queue_receive(queue, &message, 0))) {
			if (message == SO_TIMER_MESSAGE) {
				ret = -ETIMEDOUT;
				break;
			}
			else if (&cbdata == (ipcmessage*)message) {
				ret = _net_convert_error(cbdata.result);
				os_message_queue_ack(&cbdata, 0);
				break;
			}
		}
		os_destroy_timer(timer);
		os_message_queue_destroy(queue);
	}

	if(params != (u8*)addrString)
		net_free(params);

	if (ret < 0) {
		errno = ret;
		return NULL;
	}

	ipData = ((struct hostent*)ipBuffer);
	addrOffset = (u32)MEM_PHYSICAL_TO_K0(ipData->h_name) - ((u32)ipBuffer + 0x10);

	ipData->h_name = MEM_PHYSICAL_TO_K0(ipData->h_name) - addrOffset;
	ipData->h_aliases = MEM_PHYSICAL_TO_K0(ipData->h_aliases) - addrOffset;

	for (i=0; (i < 0x40) && (ipData->h_aliases[i] != 0); i++) {
		ipData->h_aliases[i] = MEM_PHYSICAL_TO_K0(ipData->h_aliases[i]) - addrOffset;
	}

	ipData->h_addr_list = MEM_PHYSICAL_TO_K0(ipData->h_addr_list) - addrOffset;

	for (i=0; (i < 0x40) && (ipData->h_addr_list[i] != 0); i++) {
		ipData->h_addr_list[i] = MEM_PHYSICAL_TO_K0(ipData->h_addr_list[i]) - addrOffset;
	}

	os_sync_before_read(ipBuffer, sizeof(ipBuffer));

	errno = 0;
	return ipData;
}

static s32 FormNameQueryPacket(nb_lookup *lookup)
{
	int i;
	OPCODEFLAGSRCODE Wcode;

	lookup->nameheader.TransactionID = os_time_now(); // Transaction ID

	Wcode.RCode = 0;
	Wcode.fNM_B = 1; // always broadcast
	Wcode.fNM_00 = 0;
	Wcode.fNM_RA = 0;
	Wcode.fNM_RD = 1;
	Wcode.fNM_TC = 0;
	Wcode.fNM_AA = 0;
	Wcode.OpCode = OPCODE_QUERY;
	Wcode.fResponse = 0;

	lookup->nameheader.OpcodeFlagsRcode = htons(*(u16*)&Wcode);

	lookup->nameheader.QDCount = htons(1);
	//lookup->nameheader.ANCount = 0;
	//lookup->nameheader.ARCount = 0;
	//lookup->nameheader.NSCount = 0;

	lookup->rrheader.question.Name[0] = 32;
	//lookup->rrheader.question.Name[33] = 0;

	// do the weird name encoding
	for(i=0; i < 16; i++) {
		lookup->rrheader.question.Name[1+i*2] = 'A' + (lookup->Name[i]>>4);
		lookup->rrheader.question.Name[2+i*2] = 'A' + (lookup->Name[i]&0xF);
	}

	debug_printf("NB Encoded name: %s\n", lookup->rrheader.question.Name);

	lookup->rrheader.question.Type = htons(QUESTION_TYPE_NB);
	lookup->rrheader.question.Class = htons(QUESTION_CLASS_IN);

	return (s32)(sizeof(NBNAMEHEADER)+sizeof(QUESTION));
}

struct hostent* ProcessPacket(nb_lookup *reply, s32 packetsize, struct sockaddr_in *sockaddr)
{
	OPCODEFLAGSRCODE Wcode;
	u16 w, QDCount, ANCount, NSCount, ARCount;

	if (packetsize <= sizeof(NBNAMEHEADER))
		return NULL;

	QDCount = ntohs(reply->nameheader.QDCount);
	ANCount = ntohs(reply->nameheader.ANCount);
	ARCount = ntohs(reply->nameheader.ARCount);
	NSCount = ntohs(reply->nameheader.NSCount);

	w = ntohs(reply->nameheader.OpcodeFlagsRcode);
	memcpy(&Wcode, &w, sizeof(w));

	if (Wcode.RCode || Wcode.OpCode!=OPCODE_QUERY || !Wcode.fResponse || QDCount || ANCount!=1 || ARCount || NSCount)
	{
		debug_printf("Packet Wcode is bad\n");
		return NULL;
	}

	packetsize -= sizeof(NBNAMEHEADER);

	if (packetsize < sizeof(RESOURCERECORDHEADER)+6)
	{
		debug_printf("packetsize %d expected %d\n", packetsize, sizeof(RESOURCERECORDHEADER)+6);
		return NULL;
	}

	if (reply->rrheader.question.Name[0] != 32)
	{
		debug_printf("Packet Name[0] unsupported (%d)\n", reply->rrheader.question.Name[0]);
		return NULL;
	}

	if (ntohs(reply->rrheader.question.Type) != RRTYPE_NB) {
		debug_printf("Bad rrheader Type (%d)\n", ntohs(reply->rrheader.question.Type));
		return NULL;
	}

	if (ntohs(reply->rrheader.question.Class) != RRCLASS_IN) {
		debug_printf("Bad rrheader Class (%d)\n", ntohs(reply->rrheader.question.Class));
		return NULL;
	}

	if (ntohs(reply->rrheader.RDLength) < 6) {
		debug_printf("Bad rrheader RDLength (%d)\n", ntohs(reply->rrheader.RDLength));
		return NULL;
	}

	debug_printf("Flags: %02X%02X, IP: %d.%d.%d.%d\n", reply->nbflags[0], reply->nbflags[1], reply->ip_addr[0], reply->ip_addr[1], reply->ip_addr[2], reply->ip_addr[3]);

	reply->he.h_addrtype = PF_INET;
	reply->he.h_name = reply->Name;
	reply->he.h_length = 4;
	reply->he.h_aliases = &reply->alias_none;
	reply->he.h_addr_list = reply->addr_list;
	reply->addr_list[0] = (char*)&reply->ip_addr[0];
	// if there's more than one IP in the record, get the IP from the datagram instead
	if (ntohs(reply->rrheader.RDLength > 6))
		memcpy(reply->ip_addr, &sockaddr->sin_addr, reply->he.h_length);

	return &reply->he;
}

struct hostent* net_getnbhostbyname_async(const char *addrString, u32 timeout)
{
	int udpsock;
	u32 sock_opt = 1;
	struct sockaddr_in sockaddr;
	nb_lookup *reply = (nb_lookup*)ipBuffer;
	s32 d;
	socklen_t size;
	int i;

	// TODO: Set errno properly for errors

	memset(reply, 0, sizeof(*ipBuffer));

	if (addrString==NULL || addrString[0]=='\0' || strlen(addrString)>15)
		return NULL;

	memset(reply->Name, ' ', 15);
	memcpy(reply->Name, addrString, strnlen(addrString, 15));
	for (i=0; i < 15; i++)
		reply->Name[i] = toupper((int)reply->Name[i]);

	udpsock = net_socket(AF_INET, SOCK_DGRAM, 0);
	if (udpsock < 0)
		return NULL;

	// set non-blocking
	if (net_ioctl(udpsock, FIONBIO, &sock_opt) < 0) {
		net_close(udpsock);
		return NULL;
	}

	sockaddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
	sockaddr.sin_port = htons(137); // NetBios name port
	sockaddr.sin_family = AF_INET;

	d = FormNameQueryPacket(reply);

	// this will implicitly bind the socket to a free port
	if (net_sendto(udpsock, &reply->nameheader, d, 0, (struct sockaddr*)&sockaddr, 8) < d) {
		net_close(udpsock);
		return NULL;
	}

	size = sizeof(sockaddr);
	// No timeout is a really bad idea if the host doesn't exist. You have been warned.
	if (timeout==0)
	{
		// set blocking
		sock_opt = 0;
		d = net_ioctl(udpsock, FIONBIO, &sock_opt);
		if (d>=0)
			d = net_recvfrom(udpsock, &reply->nameheader, sizeof(ipBuffer)-64, 0, (struct sockaddr*)&sockaddr, &size);
	} else {
		u32 queuedata[1];
		int queue = os_message_queue_create(queuedata, sizeof(queuedata)/sizeof(u32));
		int timer = os_create_timer(timeout, 0, queue, SO_TIMER_MESSAGE);
		u32 message;

		if (queue>=0 && timer>=0) {
			do {
				d = net_recvfrom(udpsock, &reply->nameheader, sizeof(ipBuffer)-64, 0, (struct sockaddr*)&sockaddr, &size);
				if (d != -EAGAIN)
					break;
			} while (os_message_queue_receive(queue, &message, 1) || message != SO_TIMER_MESSAGE);

			os_destroy_timer(timer);
			os_message_queue_destroy(queue);
		}
		else
			d = -1; // something fucked up, don't really care what
	}

	net_close(udpsock);

	if (d<=0)
		return NULL;

	return ProcessPacket(reply, d, &sockaddr);
}

s32 net_socket(u32 domain, u32 type, u32 protocol)
{
	s32 ret;
	STACK_ALIGN(u32, params, 3, 32);

	if (net_ip_top_fd < 0) return -ENXIO;

	params[0] = domain;
	params[1] = type;
	params[2] = protocol; // 0

	ret = _net_convert_error(os_ioctl(net_ip_top_fd, IOCTL_SO_SOCKET, params, 12, NULL, 0));
	if(ret>=0 && type==SOCK_STREAM) // set tcp window size to 32kb
	{
		int window_size = 32768; // max recv_buf
		net_setsockopt(ret, SOL_SOCKET, SO_RCVBUF, (char *) &window_size, sizeof(window_size));
		window_size = 8192; // max snd_buf
		net_setsockopt(ret, SOL_SOCKET, SO_SNDBUF, (char *) &window_size, sizeof(window_size));
		window_size = 1;
		net_setsockopt(ret, IPPROTO_TCP, /*TCP_NODELAY*/0x2001, &window_size, sizeof(window_size));
	}
	debug_printf("net_socket(%d, %d, %d)=%d\n", domain, type, protocol, ret);
	return ret;
}

s32 net_shutdown(s32 s, u32 how)
{
	s32 ret;
	STACK_ALIGN(u32, params, 2, 32);

	if (net_ip_top_fd < 0) return -ENXIO;

	params[0] = s;
	params[1] = how;
	ret = _net_convert_error(os_ioctl(net_ip_top_fd, IOCTL_SO_SHUTDOWN, params, 8, NULL, 0));

	debug_printf("net_shutdown(%d, %d)=%d\n", s, how, ret);
	return ret;
}

s32 net_bind(s32 s, struct sockaddr *name, socklen_t namelen)
{
	s32 ret;
	STACK_ALIGN(struct bind_params,params,1,32);

	if (net_ip_top_fd < 0) return -ENXIO;
	if (name->sa_family != AF_INET) return -EAFNOSUPPORT;

	name->sa_len = 8;

	memset(params, 0, sizeof(struct bind_params));
	params->socket = s;
	params->has_name = 1;
	memcpy(params->name, name, 8);

	ret = _net_convert_error(os_ioctl(net_ip_top_fd, IOCTL_SO_BIND, params, sizeof (struct bind_params), NULL, 0));
	debug_printf("net_bind(%d, %p)=%d\n", s, name, ret);

	return ret;
}

s32 net_listen(s32 s, u32 backlog)
{
	s32 ret;
	STACK_ALIGN(u32, params, 2, 32);

	if (net_ip_top_fd < 0) return -ENXIO;

	params[0] = s;
	params[1] = backlog;

	debug_printf("calling ios_ioctl(%d, %d, %p, %d)\n", net_ip_top_fd, IOCTL_SO_SOCKET, params, 8);

	ret = _net_convert_error(os_ioctl(net_ip_top_fd, IOCTL_SO_LISTEN, params, 8, NULL, 0));
  	debug_printf("net_listen(%d, %d)=%d\n", s, backlog, ret);
	return ret;
}

s32 net_accept(s32 s, struct sockaddr *addr, socklen_t *addrlen)
{
	s32 ret;
	STACK_ALIGN(u32, _socket, 1, 32);

	debug_printf("net_accept()\n");

	if (net_ip_top_fd < 0) return -ENXIO;

	if (!addr) return -EINVAL;
	addr->sa_len = 8;
	addr->sa_family = AF_INET;

	if (!addrlen) return -EINVAL;

	if (*addrlen < 8) return -ENOMEM;

	*addrlen = 8;

	*_socket = s;
	debug_printf("calling ios_ioctl(%d, %d, %p, %d)\n", net_ip_top_fd, IOCTL_SO_ACCEPT, _socket, 4);
	ret = _net_convert_error(os_ioctl(net_ip_top_fd, IOCTL_SO_ACCEPT, _socket, 4, addr, *addrlen));

	debug_printf("net_accept(%d, %p)=%d\n", s, addr, ret);
	return ret;
}

s32 net_connect(s32 s, struct sockaddr *addr, socklen_t addrlen)
{
	s32 ret;
	STACK_ALIGN(struct connect_params,params,1,32);

	if (net_ip_top_fd < 0) return -ENXIO;
	if (addr->sa_family != AF_INET) return -EAFNOSUPPORT;
	if (addrlen < 8) return -EINVAL;

	addr->sa_len = 8;

	memset(params, 0, sizeof(struct connect_params));
	params->socket = s;
	params->has_addr = 1;
	memcpy(&params->addr, addr, addrlen);

	ret = _net_convert_error(os_ioctl(net_ip_top_fd, IOCTL_SO_CONNECT, params, sizeof(struct connect_params), NULL, 0));
	if (ret < 0) {
    	debug_printf("SOConnect(%d, %p)=%d\n", s, addr, ret);
	}

  	return ret;
}

s32 net_write(s32 s, const void *data, s32 size)
{
    return net_send(s, data, size, 0);
}

s32 net_send(s32 s, const void *data, s32 size, u32 flags)
{
	return net_sendto(s, data, size, flags, NULL, 0);
}

s32 net_sendto(s32 s, const void *data, s32 len, u32 flags, struct sockaddr *to, socklen_t tolen)
{
	s32 ret;
	u8 * message_buf = NULL;
	STACK_ALIGN(struct sendto_params,params,1,32);
	STACK_ALIGN(ioctlv, vec, 2, 32);

	if (net_ip_top_fd < 0) return -ENXIO;
	if (tolen > 28) return -EOVERFLOW;

	if ((u32)data & 0x1F) // ensure message buf is aligned
	{
		message_buf = net_malloc(len);
		if (message_buf == NULL) {
			debug_printf("net_send: failed to alloc %d bytes\n", len);
			return IPC_ENOMEM;
		}
		memcpy(message_buf, data, len);
	}
	else
		message_buf = (u8*)data;

	debug_printf("net_sendto(%d, %p, %d, %d, %p, %d)\n", s, data, len, flags, to, tolen);

	if (to && to->sa_len != tolen) {
		debug_printf("warning: to->sa_len was %d, setting to %d\n",	to->sa_len, tolen);
		to->sa_len = tolen;
	}

	memset(params, 0, sizeof(struct sendto_params));

	params->socket = s;
	params->flags = flags;
	if (to) {
		params->has_destaddr = 1;
		memcpy(params->destaddr, to, to->sa_len);
	} else {
		params->has_destaddr = 0;
	}

	os_sync_after_write(message_buf, len);
	os_sync_after_write(params, sizeof(struct sendto_params));

	vec[0].data = message_buf;
	vec[0].len = len;
	vec[1].data = params;
	vec[1].len = sizeof(struct sendto_params);

	gpio_set_toggle(GPIO_OSLOT);

	ret = _net_convert_error(os_ioctlv(net_ip_top_fd, IOCTLV_SO_SENDTO, 2, 0, vec));
	debug_printf("net_send returned %d\n", ret);

	gpio_set_toggle(GPIO_OSLOT);

	if(message_buf != data)
		net_free(message_buf);
	return ret;
}

s32 net_recv(s32 s, void *mem, s32 len, u32 flags)
{
    return net_recvfrom(s, mem, len, flags, NULL, NULL);
}

s32 net_recvfrom(s32 s, void *mem, s32 len, u32 flags, struct sockaddr *from, socklen_t *fromlen)
{
	s32 ret;
	u8* message_buf = NULL;
	STACK_ALIGN(u32, params, 2, 32);
	STACK_ALIGN(ioctlv, vec, 3, 32);

	if (net_ip_top_fd < 0) return -ENXIO;
	if (len<=0) return -EINVAL;

	if (fromlen && from->sa_len != *fromlen) {
		debug_printf("warning: from->sa_len was %d, setting to %d\n",from->sa_len, *fromlen);
		from->sa_len = *fromlen;
	}

	if ((u32)mem & 0x1F || (u32)mem < 0x10000000)
	{
		message_buf = net_malloc(len);
		if (message_buf == NULL) {
			gpio_set_on(GPIO_OSLOT);
			return IPC_ENOMEM;
		}
	}
	else
		message_buf = (u8*)mem;

	debug_printf("net_recvfrom(%d, '%s', %d, %d, %p, %d)\n", s, (char *)mem, len, flags, from, fromlen?*fromlen:0);

	params[0] = s;
	params[1] = flags;

	if (fromlen)
		os_sync_after_write(from, *fromlen);
	os_sync_after_write(params, sizeof(u32) * 2);

	vec[0].data = params;
	vec[0].len = 8;
	vec[1].data = message_buf;
	vec[1].len = len;
	vec[2].data = from;
	vec[2].len = (fromlen ? *fromlen : 0);

	gpio_set_toggle(GPIO_OSLOT);

	ret = _net_convert_error(os_ioctlv(net_ip_top_fd, IOCTLV_SO_RECVFROM, 1, 2, vec));
	debug_printf("net_recvfrom returned %d\n", ret);

	gpio_set_toggle(GPIO_OSLOT);

	if (ret > 0) {
		if (ret > len) {
			gpio_set_on(GPIO_OSLOT);
			ret = -EOVERFLOW;
			goto done;
		}

		// ???
		//os_sync_before_read(message_buf, ret);
		os_sync_after_write(message_buf, ret);

		if (mem != message_buf) {
			memcpy(mem, message_buf, (ret+3)&~3);
			os_sync_after_write(mem, (ret+3)&~3);
		}
	}

	if (fromlen && from) *fromlen = from->sa_len;

done:
	if (mem != message_buf)
		net_free(message_buf);
	return ret;
}

s32 net_read(s32 s, void *mem, s32 len)
{
	return net_recvfrom(s, mem, len, 0, NULL, NULL);
}

s32 net_close(s32 s)
{
	s32 ret;
	STACK_ALIGN(u32, _socket, 1, 32);

	if (net_ip_top_fd < 0) return -ENXIO;

	*_socket = s;
	ret = _net_convert_error(os_ioctl(net_ip_top_fd, IOCTL_SO_CLOSE, _socket, 4, NULL, 0));

	if (ret < 0) debug_printf("net_close(%d)=%d\n", s, ret);

	return ret;
}

s32 net_setsockopt(s32 s, u32 level, u32 optname, const void *optval, socklen_t optlen)
{
	s32 ret;
	STACK_ALIGN(struct setsockopt_params,params,1,32);

	if (net_ip_top_fd < 0) return -ENXIO;
	if (optlen < 0 || optlen > 20) return -EINVAL;

	memset(params, 0, sizeof(struct setsockopt_params));
	params->socket = s;
	params->level = level;
	params->optname = optname;
	params->optlen = optlen;
	if (optval && optlen) memcpy (params->optval, optval, optlen);

	ret = _net_convert_error(os_ioctl(net_ip_top_fd, IOCTL_SO_SETSOCKOPT, params, sizeof(struct setsockopt_params), NULL, 0));

	debug_printf("net_setsockopt(%d, %u, %u, %p, %d)=%d\n",	s, level, optname, optval, optlen, ret);
	return ret;
}

s32 net_ioctl(s32 s, u32 cmd, void *argp)
{
	u32 flags;
	const u32 *intp = (const u32 *)argp;

	if (net_ip_top_fd < 0) return -ENXIO;
	if (!intp) return -EINVAL;

	switch (cmd) {
		case FIONBIO:
			flags = net_fcntl(s, F_GETFL, 0);
			flags &= ~IOS_O_NONBLOCK;
			if (*intp) flags |= IOS_O_NONBLOCK;
			return net_fcntl(s, F_SETFL, flags);
		default:
			return -EINVAL;
	}
}

s32 net_fcntl(s32 s, u32 cmd, u32 flags)
{
	s32 ret;
	STACK_ALIGN(u32, params, 3, 32);

	if (net_ip_top_fd < 0) return -ENXIO;
	if (cmd != F_GETFL && cmd != F_SETFL) return -EINVAL;


	params[0] = s;
	params[1] = cmd;
	params[2] = flags;

	ret = _net_convert_error(os_ioctl(net_ip_top_fd, IOCTL_SO_FCNTL, params, 12, NULL, 0));

	debug_printf("net_fcntl(%d, %d, %x)=%d\n", params[0], params[1], params[2], ret);

	return ret;
}


/*!
 * \fn s32 net_poll(struct pollsd *sds, u32 nsds, s64 timeout)
 * \brief Poll a set of sockets for a set of events.
 *
 * \param[in] sds a pointer to an array of pollsd structures
 * \param[in] nsds the number of elements in the sds array
 * \param[in] time in milliseconds before the function should timeout
 *
 * \return the number of structures in sds that now have non-zero revent fields
 */
s32 net_poll(struct pollsd *sds,s32 nsds,s32 timeout)
{
	union ullc {
		u64 ull;
		u32 ul[2];
	};

	s32 ret;
	union ullc outv;
	struct pollsd *psds;
	STACK_ALIGN(u64,params,1,32);

	if(net_ip_top_fd<0) return -ENXIO;
	if(sds==NULL || nsds==0) return -EINVAL;

	psds = net_malloc((nsds*sizeof(struct pollsd)));
	if(psds==NULL) {
		debug_printf("net_poll: failed to alloc %d bytes\n", nsds * sizeof(struct pollsd));
		return IPC_ENOMEM;
	}

	outv.ul[0] = 0;
	outv.ul[1] = timeout;
	params[0] = outv.ull;
	memcpy(psds,sds,(nsds*sizeof(struct pollsd)));

	ret = _net_convert_error(os_ioctl(net_ip_top_fd, IOCTL_SO_POLL, params, 8, psds, (nsds * sizeof(struct pollsd))));

	memcpy(sds,psds,(nsds*sizeof(struct pollsd)));

	net_free(psds);

	debug_printf("net_poll(sds, %d, %lld)=%d\n", nsds, params[0], ret);

	return ret;
}

s32 if_config(char *local_ip, char *netmask, char *gateway,boolean use_dhcp)
{
	s32 i,ret;
	struct in_addr hostip;

	if ( use_dhcp != true ) return -EINVAL;

	for(i=0;i<MAX_INIT_RETRIES && (ret=net_init())==-EAGAIN;i++);
	if(ret<0) return ret;

	hostip.s_addr = net_gethostip();
	if ( local_ip!=NULL && hostip.s_addr ) {
		strcpy(local_ip, inet_ntoa(hostip));
		return 0;
	}

	return -1;
}

s32 if_configex(struct in_addr *local_ip, struct in_addr *netmask, struct in_addr *gateway,boolean use_dhcp)
{
	s32 i,ret;
	struct in_addr hostip;

	if ( use_dhcp != true ) return -EINVAL;

	for(i=0;i<MAX_INIT_RETRIES && (ret=net_init())==-EAGAIN;i++);
	if(ret<0) return ret;

	hostip.s_addr = net_gethostip();
	if ( local_ip!=NULL && hostip.s_addr )
	{
		*local_ip = hostip;
		return 0;
	}

	return -1;
}
