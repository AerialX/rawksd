#define _CRT_SECURE_NO_DEPRECATE 1 // fuck off
#define _CRT_NONSTDC_NO_DEPRECATE 1 // you too

// RawkAudio
#define OUTSIDE_SPEEX
#define RANDOM_PREFIX rawk
#define FLOATING_POINT
#define EXPORT

#define RESAMPLE_MAX_BLOCK 4096
#define OGG_SAMPLE_RATE_OUT 28000



#include <stdlib.h>
#include <memory.h>
#include <time.h>
#include "RawkAudio.h"
#include <vorbis/vorbisenc.h>
#include <vorbis/vorbisfile.h>
#include "speex_resampler.h"


// structures
typedef struct rawkvorbis_enc_stream
{
	vorbis_info vi;
	vorbis_comment vc;
	vorbis_dsp_state vd;
	vorbis_block vb;

	ogg_stream_state os;

	rawk_callbacks cb;
	SpeexResamplerState *rs;
	float* resample_in_buf;
	size_t resample_out_size;
} rawkvorbis_enc_stream;

typedef struct rawkvorbis_dec_file
{
	rawk_callbacks cb;
	unsigned int offset;
} rawkvorbis_dec_file;

// constants
#define MAX_INPUT_CHANNELS 16

void vorbis_enc_close(rawkvorbis_enc_stream *vbs)
{
	if (vbs)
	{
		ogg_stream_clear(&vbs->os);
		vorbis_block_clear(&vbs->vb);
		vorbis_dsp_clear(&vbs->vd);
		vorbis_comment_clear(&vbs->vc);
		vorbis_info_clear(&vbs->vi);
		if (vbs->cb.close_func)
			vbs->cb.close_func(vbs->cb.datasource);
		if (vbs->rs)
			rawk_resampler_destroy(vbs->rs);
		free(vbs->resample_in_buf);
		free(vbs);
	}
}

int vorbis_enc_encode(rawkvorbis_enc_stream *vbs)
{
	ogg_packet op;
	ogg_page og;
	int eos=0;

	while(vorbis_analysis_blockout(&vbs->vd,&vbs->vb)==1)
	{
		// analysis
		vorbis_analysis(&vbs->vb,NULL);
		// can skip this call ?
		vorbis_bitrate_addblock(&vbs->vb);

		while (vorbis_bitrate_flushpacket(&vbs->vd, &op))
		{
        
			// add this packet to the bitstream
			ogg_stream_packetin(&vbs->os, &op);

			// write ogg pages out to file
			while (!eos && ogg_stream_pageout(&vbs->os, &og))
			{
				vbs->cb.write_func(og.header, 1, og.header_len, vbs->cb.datasource);
				vbs->cb.write_func(og.body, 1, og.body_len, vbs->cb.datasource);

				if (ogg_page_eos(&og)) // ??
					eos=1;
			}
		}
	}
	return 0;
}

int RAWKAUDIO_API rawk_vorbis_enc_compress(vb_enc_stream stream, short **samples, int sample_count)
{
	rawkvorbis_enc_stream *vbs = (rawkvorbis_enc_stream*)stream;
	float **buffer = NULL;
	int i, j;
	int samples_processed=0;

	if (vbs==NULL || samples==NULL || sample_count <= 0)
		return RAWKERROR_INVALID_PARAM;

	for (i=0; i < vbs->vi.channels; i++)
		if (samples[i]==NULL)
			return RAWKERROR_INVALID_PARAM;

	while (samples_processed<sample_count)
	{
		int samples_to_process = rawk_min(sample_count-samples_processed, RESAMPLE_MAX_BLOCK);
		int out_samples = vbs->resample_out_size;
		int in_len, out_len;
		buffer = vorbis_analysis_buffer(&vbs->vd, out_samples);
		for (i=0; i < vbs->vi.channels; i++)
		{
			short *channel = samples[i]+samples_processed;
			if (buffer[i]==NULL)
				return RAWKERROR_MEMORY;
			in_len = samples_to_process;
			out_len = out_samples;
			for (j=0; j < samples_to_process; j++)
				vbs->resample_in_buf[j] = *channel++/32767.f;
			rawk_resampler_process_float(vbs->rs, i, vbs->resample_in_buf, &in_len, buffer[i], &out_len);
		}
		samples_processed += in_len;
		if (out_len)
		{
			vorbis_analysis_wrote(&vbs->vd, out_len);
			vorbis_enc_encode(vbs);
		}
	}

	return 0;


	buffer = vorbis_analysis_buffer(&vbs->vd, sample_count);
	// make sure all buffers are valid
	for (i=0; i < vbs->vi.channels; i++)
	{
		if (buffer[i]==NULL)
			return RAWKERROR_MEMORY;
		if (samples[i]==NULL)
			return RAWKERROR_INVALID_PARAM;
	}

	for (i=0; i < vbs->vi.channels; i++)
	{
		short *channel = samples[i];
		for (j=0; j < sample_count; j++)
		{
			buffer[i][j] = *channel++/32767.f;
		}
	}

	vorbis_analysis_wrote(&vbs->vd, sample_count);

	vorbis_enc_encode(vbs);
	return 0;
}

int RAWKAUDIO_API rawk_vorbis_enc_create_cb(rawk_callbacks *cb, int channels, int s_rate, int target_s_rate, int bitrate, vb_enc_stream *stream)
{
	int ret=0;
	rawkvorbis_enc_stream *vbs=NULL;
	ogg_packet header, header_comm, header_code;
	ogg_page og;
	uint32_t num, den;

	if (stream==NULL||cb==NULL||cb->write_func==NULL||channels<=0||s_rate<=0||target_s_rate<0||bitrate<0)
		return RAWKERROR_INVALID_PARAM;
	*stream = NULL;

	vbs = (rawkvorbis_enc_stream*)malloc(sizeof(rawkvorbis_enc_stream));
	if (vbs==NULL)
		return RAWKERROR_MEMORY;
	memset(vbs, 0, sizeof(rawkvorbis_enc_stream));

	// copy callbacks
	memcpy(&vbs->cb, cb, sizeof(rawk_callbacks));
	
	// initialize the vorbis stream
	vorbis_info_init(&vbs->vi);

	if (!target_s_rate)
		target_s_rate = OGG_SAMPLE_RATE_OUT;

	// vorbis documentation says max_bitrate and min_bitrate should be set to -1 if not used
	// but that doesn't match the headers in RB1's .mogg files, so use 0 (if bitrate not specified)

	if (bitrate)
		ret = vorbis_encode_init(&vbs->vi, channels, target_s_rate, -1, bitrate*channels, -1);
	else
		ret = vorbis_encode_init(&vbs->vi, channels, target_s_rate, 0, 72000*channels, 0);

	if (ret)
	{
		ret = RAWKERROR_UNKNOWN;
		goto vorbis_enc_create_done;
	}

	// add comments
	vorbis_comment_init(&vbs->vc);
	vorbis_comment_add_tag(&vbs->vc,"ENCODER","RawkAudio");

	// setup analysis and storage
	vorbis_analysis_init(&vbs->vd,&vbs->vi);
	vorbis_block_init(&vbs->vd,&vbs->vb);

	// write out the headers
	srand((unsigned int)time(NULL));
	ogg_stream_init(&vbs->os, rand());

	ret = vorbis_analysis_headerout(&vbs->vd, &vbs->vc, &header, &header_comm, &header_code);
	if (ret)
	{
		ret = RAWKERROR_UNKNOWN;
		goto vorbis_enc_create_done;
	}

	ogg_stream_packetin(&vbs->os,&header);
    ogg_stream_packetin(&vbs->os,&header_comm);
    ogg_stream_packetin(&vbs->os,&header_code);

	while (ogg_stream_flush(&vbs->os,&og)!=0)
	{
		vbs->cb.write_func(og.header, 1, og.header_len, vbs->cb.datasource);
		vbs->cb.write_func(og.body, 1, og.body_len, vbs->cb.datasource);
	}

	// setup a resampler
	vbs->rs = rawk_resampler_init(channels, s_rate, target_s_rate, SPEEX_RESAMPLER_QUALITY_MAX, NULL);
	if (vbs->rs==NULL)
	{
		ret = RAWKERROR_MEMORY;
		goto vorbis_enc_create_done;
	}
	// account for resampler latency
	rawk_resampler_skip_zeros(vbs->rs);
	rawk_resampler_get_ratio(vbs->rs, &num, &den);
	vbs->resample_out_size = (RESAMPLE_MAX_BLOCK*den)/num+1;
	vbs->resample_in_buf = (float*)malloc(RESAMPLE_MAX_BLOCK*sizeof(float));
	if (vbs->resample_in_buf==NULL)
	{
		ret = RAWKERROR_MEMORY;
		goto vorbis_enc_create_done;
	}

	*stream = (vb_enc_stream)vbs;
vorbis_enc_create_done:
	if (ret)
		vorbis_enc_close(vbs);
	return ret;
}

int RAWKAUDIO_API rawk_vorbis_enc_create(char *output_name, int channels, int s_rate, int target_s_rate, int bitrate, vb_enc_stream *stream)
{
	rawk_callbacks cb;

	if (output_name==NULL)
		return RAWKERROR_INVALID_PARAM;

	memset(&cb, 0, sizeof(cb));
	cb.datasource = (void*)fopen(output_name, "wb");
	if (cb.datasource==NULL)
		return RAWKERROR_IO;

	cb.close_func = (int (*)(void*))fclose;
	cb.write_func = (size_t (*)(const void*,size_t,size_t,void*))fwrite;

	return rawk_vorbis_enc_create_cb(&cb, channels, s_rate, target_s_rate, bitrate, stream);
}

void RAWKAUDIO_API rawk_vorbis_enc_destroy(vb_enc_stream stream)
{
	rawkvorbis_enc_stream *vbs = (rawkvorbis_enc_stream*)stream;
	if (vbs==NULL)
		return;

	// write end of stream
	vorbis_analysis_wrote(&vbs->vd, 0);

	// flush the encoder
	vorbis_enc_encode(vbs);

	// clean up
	vorbis_enc_close(vbs);
}

// these callbacks hide the RockBand mogg header from libvorbisfile
int vorbis_dec_fseek(rawkvorbis_dec_file *f, ogg_int64_t Offset, int Origin)
{
	if(f==NULL)return(-1);
	switch(Origin)
	{
	case SEEK_SET:
		Offset += f->offset;
	case SEEK_CUR:
	case SEEK_END:
		break;
	default:
		return -1;
	}

	// call the real callback
	return f->cb.seek_func(f->cb.datasource, Offset, Origin);
}

long vorbis_dec_ftell(rawkvorbis_dec_file *f)
{
	if (f==NULL)
		return -1;
	return f->cb.tell_func(f->cb.datasource)-f->offset;
}

size_t vorbis_dec_fread(void *dst, size_t el_size, size_t count, rawkvorbis_dec_file *f)
{
	if (f==NULL)
		return 0;
	return f->cb.read_func(dst, el_size, count, f->cb.datasource);
}

int vorbis_dec_fclose(rawkvorbis_dec_file *f)
{
	void *f_in;
	int (*close_func)(void*);
	if (f==NULL)
		return EOF;
	f_in = f->cb.datasource;
	close_func = f->cb.close_func;
	free(f);
	if (close_func)
		return close_func(f_in);
	return 0;
}

void vorbis_dec_close(OggVorbis_File *vf)
{
	if (vf)
	{
		ov_clear(vf);
		free(vf);
	}
}

int RAWKAUDIO_API rawk_vorbis_dec_create_cb(rawk_callbacks *cb, int *channels, int *rate, int64_t *samples, vb_dec_stream *stream)
{
	OggVorbis_File *vf=NULL;
	int ret=0;
	int header_check[2];
	vorbis_info *vi=NULL;
	rawkvorbis_dec_file *f;
	ov_callbacks ovcb;
	size_t header_bytes;

	if (cb==NULL||cb->read_func==NULL||channels==NULL||rate==NULL||samples==NULL||stream==NULL)
		return RAWKERROR_INVALID_PARAM;
	*stream = NULL;

	vf = (OggVorbis_File*)malloc(sizeof(OggVorbis_File));
	if (vf==NULL)
		return RAWKERROR_MEMORY;
	memset(vf, 0, sizeof(OggVorbis_File));

	f = (rawkvorbis_dec_file*)malloc(sizeof(rawkvorbis_dec_file));
	if (f==NULL)
	{
		ret = RAWKERROR_MEMORY;
		goto rawk_vorbis_dec_done;
	}
	memcpy(&f->cb, cb, sizeof(rawk_callbacks));
	f->offset=0;

	header_bytes = f->cb.read_func(header_check, 1, sizeof(header_check), f->cb.datasource);
	if (header_bytes==sizeof(header_check) && header_check[0]==10)
	{
		// unencrypted .mogg header found
		long filesize;
		f->offset = header_check[1];
		// if we can't tell how long the file is, just assume it's a .mogg
		if (f->cb.tell_func && f->cb.seek_func)
		{
			f->cb.seek_func(f->cb.datasource, 0, SEEK_END);
			filesize = f->cb.tell_func(f->cb.datasource);
			if (filesize <= header_check[1])
				// header is too big
				ret = OV_ENOTVORBIS;
		}
		else if (f->offset>header_bytes)
		{
			int i = f->offset - header_bytes;
			while (i>0 && f->cb.read_func(header_check, 1, 1, f->cb.datasource)==1)
				i--;
		}
	}
	
	if (!ret)
	{
		memset(&ovcb, 0, sizeof(ovcb));
		ovcb.read_func = vorbis_dec_fread;
		ovcb.close_func = vorbis_dec_fclose;
		if (f->cb.tell_func)
			ovcb.tell_func = vorbis_dec_ftell;
		if (f->cb.seek_func)
		{
			ovcb.seek_func = vorbis_dec_fseek;
			// rewind
			f->cb.seek_func(f->cb.datasource, f->offset, SEEK_SET);
			ret = ov_open_callbacks(f, vf, NULL, 0, ovcb);
		}
		else
			ret = ov_open_callbacks(f, vf, (f->offset) ? NULL:(char*)header_check, (f->offset) ? 0:header_bytes, ovcb);
	}

	if (ret)
	{
		switch(ret)
		{
		case OV_EREAD:
			ret = RAWKERROR_IO;
			break;
		case OV_ENOTVORBIS:
			ret = RAWKERROR_NOTVORBIS;
			break;
		default:
			ret = RAWKERROR_UNKNOWN;
		}
		goto rawk_vorbis_dec_done;
	}

	vi = ov_info(vf, -1);
	if (vi==NULL)
	{
		ret = RAWKERROR_UNKNOWN;
		goto rawk_vorbis_dec_done;
	}

	// only 1 logical bitstream supported
	if (ov_streams(vf)!=1)
	{
		ret = RAWKERROR_NOTVORBIS;
		goto rawk_vorbis_dec_done;
	}

	*channels = vi->channels;
	*rate = vi->rate;
	*samples = ov_pcm_total(vf, -1);

	*stream = (vb_dec_stream)vf;

	// 3 SECOND HACK
	//*samples -= *rate * 3;
	//rawk_vorbis_dec_seek(*stream, 0);

rawk_vorbis_dec_done:
	if (ret)
		vorbis_dec_close(vf);
	return ret;
}

int RAWKAUDIO_API rawk_vorbis_dec_create(char *input_name, int *channels, int *rate, int64_t *samples, vb_dec_stream *stream)
{
	rawk_callbacks cb;

	if (input_name==NULL)
		return RAWKERROR_INVALID_PARAM;

	memset(&cb, 0, sizeof(cb));

	cb.datasource = fopen(input_name, "rb");
	if (cb.datasource==NULL)
		return RAWKERROR_IO;

	memcpy(&cb, &OV_CALLBACKS_DEFAULT, sizeof(OV_CALLBACKS_DEFAULT));

	return rawk_vorbis_dec_create_cb(&cb, channels, rate, samples, stream);
}

void RAWKAUDIO_API rawk_vorbis_dec_destroy(vb_dec_stream stream)
{
	OggVorbis_File *vf = (OggVorbis_File*)stream;
	vorbis_dec_close(vf);
}

int RAWKAUDIO_API rawk_vorbis_dec_seek(vb_dec_stream stream, int64_t sample_pos)
{
	OggVorbis_File *vf = (OggVorbis_File*)stream;

	// 3 SECOND SEEK HACK
	//vorbis_info *vi=NULL;
	//vi = ov_info(vf, -1);
	//sample_pos += vi->rate*3;
	
	if (vf==NULL || sample_pos < 0)
		return RAWKERROR_INVALID_PARAM;

	if (ov_pcm_seek(vf, sample_pos))
		return RAWKERROR_UNKNOWN;

	return 0;
}

int RAWKAUDIO_API rawk_vorbis_dec_decompress(vb_dec_stream stream, short **samples, int sample_count)
{
	OggVorbis_File *vf = (OggVorbis_File*)stream;
	long samples_out=0;
	int bitstream;
	int channels=1;
	short buffer[16384];
	int i, j;
	int decoded=0;

	if (vf==NULL||samples==NULL||sample_count<=0)
		return RAWKERROR_INVALID_PARAM;

	channels = ov_info(vf,-1)->channels;

	while (sample_count>0)
	{
		short *buf_out = buffer;
		int out_size = channels*sample_count*sizeof(short);
		samples_out = ov_read(vf, (char*)buffer, rawk_min(sizeof(buffer), out_size), 0, 2, 1, &bitstream);
		if (samples_out<=0)
			break;
		samples_out /= (channels<<1);
		sample_count -= samples_out;
		for (i=0; i < samples_out; i++)
		{
			for (j=0;j<channels;j++)
			{
				samples[j][i+decoded] = buf_out[i*channels+j];
			}
		}
		decoded += samples_out;
	}

	if (samples_out<0)
		return RAWKERROR_UNKNOWN;

	return decoded;
}

int RAWKAUDIO_API rawk_downmix(short **in, int in_channels, short **out, int out_channels, unsigned short *masks, int samples, int normalize)
{
	int chan;
	char input_jump[MAX_INPUT_CHANNELS];
	int i, j;

	if (in==NULL||in_channels<=0||in_channels>MAX_INPUT_CHANNELS||out==NULL||out_channels<=0||masks==NULL||samples<0)
		return RAWKERROR_INVALID_PARAM;

	for (chan=0; chan < out_channels; chan++)
	{
		int total_inputs=0;
		int scale=0;
		for (i=0; i<in_channels; i++)
		{
			if (masks[chan] & (1<<i))
			{
				input_jump[total_inputs++] = i;
			}
		}
		if (total_inputs==0)
		{
			memset(out[chan], 0, sizeof(short)*samples);
		}
		else if (total_inputs==1)
		{
			memcpy(out[chan], in[input_jump[0]], sizeof(short)*samples);
		}
		else
		{
			scale = (1<<11)/total_inputs;
			for (i=0; i<samples; i++)
			{
				int mix = 0;
				for (j=0;  j<total_inputs; j++)
				{
					mix += in[input_jump[j]][i];
				}
				if (normalize)
					out[chan][i] = (short)((mix*scale+(1<<10))>>11);
				else
					out[chan][i] = (short)mix;
			}
		}
	}
	return 0;
}

