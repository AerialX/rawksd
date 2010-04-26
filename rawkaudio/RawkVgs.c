#define _CRT_SECURE_NO_DEPRECATE 1 // fuck off
#define _CRT_NONSTDC_NO_DEPRECATE 1 // you too

#include <stdlib.h>
#include <memory.h>
#include "RawkAudio.h"
#include <vorbis/vorbisfile.h> // for OV_CALLBACKS

#define VGS_HEADER_LEN		128
#define VGS_HEADER_MAGIC    '!SgV'

#define CLAMP(x) (rawk_min(rawk_max(x, -32768), 32767))

static const int xa_table[5][2] =
{
	{0, 0},
	{60, 0},
	{115, -52},
	{98, -55},
	{122, -60}
};

typedef struct rawkvgs_dec_file
{
	rawk_callbacks cb;
	int channels;
	int samples;
	unsigned char *in_buf;
	short *out_buf;
	short *output[60]; // 60 channels max(!)
	int state[60*2];
	int out_buf_samples;
	int sample_pos;
	int downsampled_last;
} rawkvgs_dec_file;

void XA_Decode(int *state, unsigned char *in, short **out, const int channels)
{
	int i, j;
	int predict1, predict2, shift;
	int s1, s2;

	for (i=0; i<channels; i++)
	{
		int pred = rawk_min(in[0]>>4,4);
		predict1 = xa_table[pred][0];
		predict2 = xa_table[pred][1];
		shift = in[0]&0x0F;
		in += 2;
		s1 = state[i*2];
		s2 = state[i*2+1];
		for (j=0; j<28; j+=2,in++)
		{
			int low = ((short)(*in<<12))>>shift;
			int high = ((short)((*in&0xF0)<<8))>>shift;
			s2 = low + ((s1*predict1 + s2*predict2+32)>>6);
			out[i][j] = s2;
			s1 = high + ((s2*predict1 + s1*predict2+32)>>6);
			out[i][j+1] = s1;
		}
		state[i*2] = s1;
		state[i*2+1] = s2;
	}
}

int vgs_dec_close(rawkvgs_dec_file *f)
{
	void *f_in;
	int (*close_func)(void*);

	if (f==NULL)
		return EOF;

	free(f->in_buf);
	free(f->out_buf);

	f_in = f->cb.datasource;
	close_func = f->cb.close_func;
	free(f);
	if (close_func)
		return close_func(f_in);
	return 0;
}

int RAWKAUDIO_API rawk_vgs_dec_create_cb(rawk_callbacks *cb, int *channels, int *rate, int64_t *samples, vgs_dec_stream *stream)
{
	int ret=0;
	rawkvgs_dec_file *f;
	unsigned int vgs_header[VGS_HEADER_LEN/sizeof(int)];
	int i;

	if (cb==NULL||cb->read_func==NULL||channels==NULL||rate==NULL||samples==NULL||stream==NULL)
		return RAWKERROR_INVALID_PARAM;
	*stream = NULL;
	*rate=0;

	f = (rawkvgs_dec_file*)malloc(sizeof(rawkvgs_dec_file));
	if (f==NULL)
	{
		ret = RAWKERROR_MEMORY;
		goto rawk_vgs_dec_done;
	}
	memset(f, 0, sizeof(rawkvgs_dec_file));
	memcpy(&f->cb, cb, sizeof(rawk_callbacks));

	if (f->cb.read_func(vgs_header, sizeof(vgs_header), 1, f->cb.datasource)!=1)
	{
		ret = RAWKERROR_IO;
		goto rawk_vgs_dec_done;
	}
	if (vgs_header[0] != VGS_HEADER_MAGIC || vgs_header[1] != 2)
	{
		ret = RAWKERROR_NOTVGS;
		goto rawk_vgs_dec_done;
	}

	for (i=2;vgs_header[i]!=0;i+=2)
	{
		if (*rate==0)
			*rate = vgs_header[i];
		else if (vgs_header[i] != *rate)
		{
			if ((vgs_header[i]<<1) != *rate)
			{
				ret = RAWKERROR_VGS_SAMPLING;
				goto rawk_vgs_dec_done;
			}
			f->downsampled_last++;
		}
		f->samples = rawk_max(f->samples, vgs_header[i+1]*28);
		f->channels++;
	}
	*samples = f->samples;
	*channels = f->channels;

	f->in_buf = (unsigned char*)malloc(f->channels*16);
	if (f->in_buf==NULL)
	{
		ret = RAWKERROR_MEMORY;
		goto rawk_vgs_dec_done;
	}
	f->out_buf = (short*)malloc((f->channels+f->downsampled_last)*sizeof(short)*28);
	if (f->out_buf==NULL)
	{
		ret = RAWKERROR_MEMORY;
		goto rawk_vgs_dec_done;
	}

	f->cb.seek_func(f->cb.datasource, VGS_HEADER_LEN, SEEK_SET);

	*stream = (vgs_dec_stream)f;
rawk_vgs_dec_done:
	if (ret)
		vgs_dec_close(f);
	return ret;
}

int RAWKAUDIO_API rawk_vgs_dec_create(char *input_name, int *channels, int *rate, int64_t *samples, vgs_dec_stream *stream)
{
	rawk_callbacks cb;

	if (input_name==NULL)
		return RAWKERROR_INVALID_PARAM;

	memset(&cb, 0, sizeof(cb));

	cb.datasource = fopen(input_name, "rb");
	if (cb.datasource==NULL)
		return RAWKERROR_IO;

	memcpy(&cb, &OV_CALLBACKS_DEFAULT, sizeof(OV_CALLBACKS_DEFAULT));

	return rawk_vgs_dec_create_cb(&cb, channels, rate, samples, stream);
}

void RAWKAUDIO_API rawk_vgs_dec_destroy(vgs_dec_stream stream)
{
	rawkvgs_dec_file *f = (rawkvgs_dec_file*)stream;
	vgs_dec_close(f);
}

int RAWKAUDIO_API rawk_vgs_dec_decompress(vgs_dec_stream stream, short **samples, int length)
{
	int i;
	int written=0;
	rawkvgs_dec_file *f = (rawkvgs_dec_file*)stream;

	if (f==NULL||length<0)
		return RAWKERROR_INVALID_PARAM;

	while (length)
	{
		if (f->out_buf_samples)
		{
			int samples_to_output = rawk_min(f->out_buf_samples, length);
			for (i=0; i < f->channels; i++)
			{
				if (samples && samples[i])
					memcpy(samples[i]+written, f->output[i], sizeof(short)*samples_to_output);
				f->output[i]+= samples_to_output;
			}
			written += samples_to_output;
			length -= samples_to_output;
			f->out_buf_samples -= samples_to_output;
		}

		if (f->out_buf_samples==0)
		{
			int shortchannels = !!(f->sample_pos%(28<<1))*f->downsampled_last;
			if (f->sample_pos >= f->samples)
				return written ? written : RAWKERROR_INVALID_PARAM;
			if (f->cb.read_func(f->in_buf, 16*(f->channels-shortchannels), 1, f->cb.datasource) != 1)
				return written ? written : RAWKERROR_IO;
			for (i=0;i<(f->channels-f->downsampled_last);i++)
				f->output[i] = f->out_buf+28*i;
			if (f->downsampled_last && !shortchannels)
			{
				for (i=0; i<f->downsampled_last; i++)
					f->output[f->channels-f->downsampled_last+i] = f->out_buf+28*(f->channels-f->downsampled_last)+56*i;
			}
			XA_Decode(f->state, f->in_buf, f->output, f->channels-shortchannels);
			if (f->downsampled_last && !shortchannels)
			{
				// cheap bilinear resampling
				int i, j;
				for (j=f->channels-f->downsampled_last;j<f->channels;j++)
				{
					short *interp_samples = f->output[j];
					for(i=27;i>0;i--)
					{
						interp_samples[i<<1] = interp_samples[i];
					}
					for(i=1;i<55;i+=2)
					{
						interp_samples[i] = ((int)interp_samples[i-1] + interp_samples[i+1]+1)>>1;
					}
					interp_samples[55] = interp_samples[54];
				}
			}
			f->out_buf_samples = 28;
			f->sample_pos += 28;
		}
	}

	return written;
}

int RAWKAUDIO_API rawk_vgs_dec_seek(vgs_dec_stream stream, int64_t sample_pos)
{
	rawkvgs_dec_file *f = (rawkvgs_dec_file*)stream;
	int ret;

	if (f==NULL||sample_pos<0||sample_pos>f->samples)
		return RAWKERROR_INVALID_PARAM;

	f->out_buf_samples = 0;
	f->sample_pos = 0;
	ret = f->cb.seek_func(f->cb.datasource, VGS_HEADER_LEN, SEEK_SET);
	if (ret != 0)
		return RAWKERROR_IO;

	ret = rawk_vgs_dec_decompress(stream, NULL, sample_pos);
	return (ret<0) ? ret : 0;
}
