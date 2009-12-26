#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "syscalls.h"
#include "mem.h"

#define IOS_MAXFMT_PARAMS		32

struct ioctlvfmt_bufent
{
	void *ipc_buf;
	void *io_buf;
	s32 copy_len;
};

struct ioctlvfmt_cbdata
{
	ipccallback user_cb;
	void *user_data;
	s32 num_bufs;
	u32 hId;
	struct ioctlvfmt_bufent *bufs;
};

static s32 _ioctlvfmtCB(s32 result, void *userdata)
{
	ipccallback user_cb;
	void *user_data;
	struct ioctlvfmt_cbdata *cbdata;
	struct ioctlvfmt_bufent *pbuf;

	cbdata = (struct ioctlvfmt_cbdata*)userdata;

	// deal with data buffers
	if(cbdata->bufs) {
		pbuf = cbdata->bufs;
		while(cbdata->num_bufs--) {
			if(pbuf->ipc_buf) {
				// copy data if needed
				if(pbuf->io_buf && pbuf->copy_len)
					memcpy(pbuf->io_buf, pbuf->ipc_buf, pbuf->copy_len);
				// then free the buffer
				Dealloc(pbuf->ipc_buf);
			}
			pbuf++;
		}
	}

	user_cb = cbdata->user_cb;
	user_data = cbdata->user_data;

	// free buffer list
	Dealloc(cbdata->bufs);

	// free callback data
	Dealloc(cbdata);

	// call the user callback
	if(user_cb)
		return user_cb(result, user_data);

	return result;
}


static s32 __ios_ioctlvformat_parse(const char *format,va_list args,struct ioctlvfmt_cbdata *cbdata,s32 *cnt_in,s32 *cnt_io,struct _ioctlv **argv)
{
	s32 ret,i;
	void *pdata;
	void *iodata;
	char type,*ps;
	s32 len,maxbufs = 0;
	ioctlv *argp = NULL;
	struct ioctlvfmt_bufent *bufp;

	maxbufs = strnlen(format,IOS_MAXFMT_PARAMS);
	if(maxbufs>=IOS_MAXFMT_PARAMS) return IPC_EINVAL;

	cbdata->bufs = Alloc((sizeof(struct ioctlvfmt_bufent)*(maxbufs+1)));
	if(cbdata->bufs==NULL) return IPC_ENOMEM;

	argp = Alloc((sizeof(struct _ioctlv)*(maxbufs+1)));
	if(argp==NULL) {
		Dealloc(cbdata->bufs);
		return IPC_ENOMEM;
	}

	*argv = argp;
	bufp = cbdata->bufs;
	memset(argp,0,(sizeof(struct _ioctlv)*(maxbufs+1)));
	memset(bufp,0,(sizeof(struct ioctlvfmt_bufent)*(maxbufs+1)));

	cbdata->num_bufs = 1;
	bufp->ipc_buf = argp;
	bufp++;

	*cnt_in = 0;
	*cnt_io = 0;

	ret = IPC_OK;
	while(*format) {
		type = tolower(*format);
		switch(type) {
			case 'b':
				pdata = Alloc(sizeof(u8));
				if(pdata==NULL) {
					ret = IPC_ENOMEM;
					goto free_and_error;
				}
				*(u8*)pdata = va_arg(args,u32);
				argp->data = pdata;
				argp->len = sizeof(u8);
				bufp->ipc_buf = pdata;
				cbdata->num_bufs++;
				(*cnt_in)++;
				argp++;
				bufp++;
				break;
			case 'h':
				pdata = Alloc(sizeof(u16));
				if(pdata==NULL) {
					ret = IPC_ENOMEM;
					goto free_and_error;
				}
				*(u16*)pdata = va_arg(args,u32);
				argp->data = pdata;
				argp->len = sizeof(u16);
				bufp->ipc_buf = pdata;
				cbdata->num_bufs++;
				(*cnt_in)++;
				argp++;
				bufp++;
				break;
			case 'i':
				pdata = Alloc(sizeof(u32));
				if(pdata==NULL) {
					ret = IPC_ENOMEM;
					goto free_and_error;
				}
				*(u32*)pdata = va_arg(args,u32);
				argp->data = pdata;
				argp->len = sizeof(u32);
				bufp->ipc_buf = pdata;
				cbdata->num_bufs++;
				(*cnt_in)++;
				argp++;
				bufp++;
				break;
			case 'q':
				pdata = Alloc(sizeof(u64));
				if(pdata==NULL) {
					ret = IPC_ENOMEM;
					goto free_and_error;
				}
				*(u64*)pdata = va_arg(args,u64);
				argp->data = pdata;
				argp->len = sizeof(u64);
				bufp->ipc_buf = pdata;
				cbdata->num_bufs++;
				(*cnt_in)++;
				argp++;
				bufp++;
				break;
			case 'd':
				argp->data = va_arg(args, void*);
				argp->len = va_arg(args, u32);
				(*cnt_in)++;
				argp++;
				break;
			case 's':
				ps = va_arg(args, char*);
				len = strnlen(ps,256);
				if(len>=256) {
					ret = IPC_EINVAL;
					goto free_and_error;
				}

				pdata = Alloc((len+1));
				if(pdata==NULL) {
					ret = IPC_ENOMEM;
					goto free_and_error;
				}
				memcpy(pdata,ps,(len+1));
				argp->data = pdata;
				argp->len = (len+1);
				bufp->ipc_buf = pdata;
				cbdata->num_bufs++;
				(*cnt_in)++;
				argp++;
				bufp++;
				break;
			case ':':
				format++;
				goto parse_io_params;
			default:
				ret = IPC_EINVAL;
				goto free_and_error;
		}
		format++;
	}

parse_io_params:
	while(*format) {
		type = tolower(*format);
		switch(type) {
			case 'b':
				pdata = Alloc(sizeof(u8));
				if(pdata==NULL) {
					ret = IPC_ENOMEM;
					goto free_and_error;
				}
				iodata = va_arg(args,u8*);
				*(u8*)pdata = *(u8*)iodata;
				argp->data = pdata;
				argp->len = sizeof(u8);
				bufp->ipc_buf = pdata;
				bufp->io_buf = iodata;
				bufp->copy_len = sizeof(u8);
				cbdata->num_bufs++;
				(*cnt_io)++;
				argp++;
				bufp++;
				break;
			case 'h':
				pdata = Alloc(sizeof(u16));
				if(pdata==NULL) {
					ret = IPC_ENOMEM;
					goto free_and_error;
				}
				iodata = va_arg(args,u16*);
				*(u16*)pdata = *(u16*)iodata;
				argp->data = pdata;
				argp->len = sizeof(u16);
				bufp->ipc_buf = pdata;
				bufp->io_buf = iodata;
				bufp->copy_len = sizeof(u16);
				cbdata->num_bufs++;
				(*cnt_io)++;
				argp++;
				bufp++;
				break;
			case 'i':
				pdata = Alloc(sizeof(u32));
				if(pdata==NULL) {
					ret = IPC_ENOMEM;
					goto free_and_error;
				}
				iodata = va_arg(args,u32*);
				*(u32*)pdata = *(u32*)iodata;
				argp->data = pdata;
				argp->len = sizeof(u32);
				bufp->ipc_buf = pdata;
				bufp->io_buf = iodata;
				bufp->copy_len = sizeof(u32);
				cbdata->num_bufs++;
				(*cnt_io)++;
				argp++;
				bufp++;
				break;
			case 'q':
				pdata = Alloc(sizeof(u64));
				if(pdata==NULL) {
					ret = IPC_ENOMEM;
					goto free_and_error;
				}
				iodata = va_arg(args,u64*);
				*(u64*)pdata = *(u64*)iodata;
				argp->data = pdata;
				argp->len = sizeof(u64);
				bufp->ipc_buf = pdata;
				bufp->io_buf = iodata;
				bufp->copy_len = sizeof(u64);
				cbdata->num_bufs++;
				(*cnt_io)++;
				argp++;
				bufp++;
				break;
			case 'd':
				argp->data = va_arg(args, void*);
				argp->len = va_arg(args, u32);
				(*cnt_io)++;
				argp++;
				break;
			default:
				ret = IPC_EINVAL;
				goto free_and_error;
		}
		format++;
	}
	return IPC_OK;

free_and_error:
	for(i=0;i<cbdata->num_bufs;i++) {
		if(cbdata->bufs[i].ipc_buf!=NULL) Dealloc(cbdata->bufs[i].ipc_buf);
	}
	Dealloc(cbdata->bufs);
	return ret;
}


s32 IOS_IoctlvFormat(s32 fd, s32 ioctl, const char *format, ...)
{
	s32 ret;
	va_list args;
	s32 cnt_in,cnt_io;
	struct _ioctlv *argv;
	struct ioctlvfmt_cbdata *cbdata;

	cbdata = Alloc(sizeof(struct ioctlvfmt_cbdata));
	if (cbdata==NULL)
		return IPC_ENOMEM;

	memset(cbdata,0,sizeof(struct ioctlvfmt_cbdata));

	va_start(args,format);
	ret = __ios_ioctlvformat_parse(format, args, cbdata, &cnt_in, &cnt_io, &argv);
	va_end(args);
	if(ret < 0) {
		Dealloc(cbdata);
		return ret;
	}

	ret = os_ioctlv(fd,ioctl,cnt_in,cnt_io,argv);
	_ioctlvfmtCB(ret,cbdata);

	return ret;
}
