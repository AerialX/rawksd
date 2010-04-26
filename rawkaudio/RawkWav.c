#define _CRT_SECURE_NO_DEPRECATE 1 // fuck off
#define _CRT_NONSTDC_NO_DEPRECATE 1 // you too

#include <stdlib.h>
#include <memory.h>
#include "RawkAudio.h"
#include <vorbis/vorbisfile.h> // for OV_CALLBACKS

typedef struct // very minimal wav header
{
	char RIFF[4];
	uint32_t riff_size;
	char WAVE[4];
	char fmt[4];
	uint32_t fmt_size;
	uint16_t wFormatTag;
	uint16_t nChannels;
	uint32_t nSamplesPerSec;
	uint32_t nAvgBytesPerSec;
	uint16_t nBlockAlign;
	uint16_t wBitsPerSample;
	// screw the cbSize field. It's not necessary and causes alignment problems.
	//uint16_t cbSize;
	char data[4];
	uint32_t data_size;
} wav_header_t;

typedef struct rawkwav_file
{
	rawk_callbacks cb;
	int channels;
	int samples;
} rawkwav_file;

int wav_dec_close(rawkwav_file *f)
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

int wav_enc_close(rawkwav_file *f)
{
	void *f_in;
	int (*close_func)(void*);

	if (f==NULL)
		return EOF;

	if (f->cb.seek_func(f->cb.datasource, 4, SEEK_SET)==0)
	{
		unsigned int byte_size = (f->samples * 2 * f->channels) + sizeof(wav_header_t)-8;
		fwrite(&byte_size, 4, 1, f->cb.datasource);
		byte_size -= sizeof(wav_header_t)-8;
		if (f->cb.seek_func(f->cb.datasource, sizeof(wav_header_t)-4, SEEK_SET)==0)
			fwrite(&byte_size, 4, 1, f->cb.datasource);
	}

	f_in = f->cb.datasource;
	close_func = f->cb.close_func;
	free(f);
	if (close_func)
		return close_func(f_in);
	return 0;
}

int RAWKAUDIO_API rawk_wav_dec_create_cb(rawk_callbacks *cb, int *channels, int *rate, int64_t *samples, wav_dec_stream *stream)
{
	int ret = 0;
	rawkwav_file *f;
	wav_header_t wav_header;

	if (cb==NULL||cb->read_func==NULL||channels==NULL||rate==NULL||samples==NULL||stream==NULL)
		return RAWKERROR_INVALID_PARAM;
	*stream = NULL;

	f = (rawkwav_file*)malloc(sizeof(rawkwav_file));
	if (f==NULL)
	{
		ret = RAWKERROR_MEMORY;
		goto rawk_wav_dec_done;
	}
	memset(f, 0, sizeof(rawkwav_file));
	memcpy(&f->cb, cb, sizeof(rawk_callbacks));

	if (f->cb.read_func(&wav_header, sizeof(wav_header), 1, f->cb.datasource)!=1)
	{
		ret = RAWKERROR_IO;
		goto rawk_wav_dec_done;
	}
	if (memcmp(wav_header.data, "data", 4) || memcmp(wav_header.fmt, "fmt ", 4) || \
		memcmp(wav_header.RIFF, "RIFF", 4) || memcmp(wav_header.WAVE, "WAVE", 4) || \
		wav_header.wBitsPerSample != 16 || wav_header.wFormatTag != 1 || \
		wav_header.fmt_size != ((uint32_t)(&wav_header.data) - (uint32_t)(&wav_header.wFormatTag)) || \
		wav_header.riff_size != (wav_header.data_size+sizeof(wav_header_t)-8) || \
		wav_header.nAvgBytesPerSec != (wav_header.nSamplesPerSec*2*wav_header.nChannels) || \
		wav_header.nBlockAlign != (wav_header.nChannels*2))
	{
		ret = RAWKERROR_NOTWAV;
		goto rawk_wav_dec_done;
	}

	*channels  = f->channels = wav_header.nChannels;
	*rate = wav_header.nSamplesPerSec;
	*samples = f->samples = wav_header.data_size / wav_header.nBlockAlign;

	*stream = (wav_dec_stream)f;
rawk_wav_dec_done:
	if (ret)
		wav_dec_close(f);
	return ret;
}

int RAWKAUDIO_API rawk_wav_dec_create(char *input_name, int *channels, int *rate, int64_t *samples, wav_dec_stream *stream)
{
	rawk_callbacks cb;

	if (input_name==NULL)
		return RAWKERROR_INVALID_PARAM;

	memset(&cb, 0, sizeof(cb));

	cb.datasource = fopen(input_name, "rb");
	if (cb.datasource==NULL)
		return RAWKERROR_IO;

	memcpy(&cb, &OV_CALLBACKS_DEFAULT, sizeof(OV_CALLBACKS_DEFAULT));

	return rawk_wav_dec_create_cb(&cb, channels, rate, samples, stream);
}

void RAWKAUDIO_API rawk_wav_dec_destroy(wav_dec_stream stream)
{
	rawkwav_file *f = (rawkwav_file*)stream;
	wav_dec_close(f);
}

int RAWKAUDIO_API rawk_wav_dec_decompress(wav_dec_stream stream, short **samples, int length)
{
	rawkwav_file *f = (rawkwav_file*)stream;
	int i, j;

	if (f==NULL||length<0)
		return RAWKERROR_INVALID_PARAM;

	for (j=0; j < length; j++)
	{
		for (i=0; i < f->channels; i++)
		{
			if (f->cb.read_func(samples[i]+j, 2, 1, f->cb.datasource)!= 1)
				return j ? j : RAWKERROR_IO;
		}
	}

	return j;
}

int RAWKAUDIO_API rawk_wav_dec_seek(wav_dec_stream stream, int64_t sample_pos)
{
	rawkwav_file *f = (rawkwav_file*)stream;
	int ret;

	if (f==NULL||sample_pos<0||sample_pos>f->samples)
		return RAWKERROR_INVALID_PARAM;

	ret = f->cb.seek_func(f->cb.datasource, sizeof(wav_header_t)+f->channels*2*sample_pos, SEEK_SET);

	return ret ? RAWKERROR_IO : 0;
}

int RAWKAUDIO_API rawk_wav_enc_create_cb(rawk_callbacks *cb, int channels, int s_rate, wav_enc_stream *stream)
{
	int ret = 0;
	rawkwav_file *f;
	wav_header_t wav_header;

	if (stream==NULL||cb==NULL||cb->write_func==NULL||channels<=0||s_rate<=0||cb->seek_func==NULL)
		return RAWKERROR_INVALID_PARAM;
	*stream = NULL;

	f = (rawkwav_file*)malloc(sizeof(rawkwav_file));
	if (f==NULL)
		return RAWKERROR_MEMORY;
	memset(f, 0, sizeof(rawkwav_file));

	// copy callbacks
	memcpy(&f->cb, cb, sizeof(rawk_callbacks));

	memset(&wav_header, 0, sizeof(wav_header));
	memcpy(wav_header.data, "data", 4);
	memcpy(wav_header.fmt, "fmt ", 4);
	memcpy(wav_header.RIFF, "RIFF", 4);
	memcpy(wav_header.WAVE, "WAVE", 4);
	// constant
	wav_header.fmt_size = (uint32_t)(&wav_header.data) - (uint32_t)(&wav_header.wFormatTag);
	wav_header.wBitsPerSample = 16;
	wav_header.wFormatTag = 1;

	wav_header.nChannels = channels;
	wav_header.nSamplesPerSec = s_rate;
	wav_header.nAvgBytesPerSec = s_rate*channels*2;
	wav_header.nBlockAlign = 2*channels;

	if (f->cb.write_func(&wav_header, sizeof(wav_header), 1, f->cb.datasource)!=1)
	{
		ret = RAWKERROR_IO;
		goto rawk_wav_enc_done;
	}
	f->channels = channels;

	*stream = (wav_enc_stream)f;
rawk_wav_enc_done:
	if (ret)
		wav_enc_close(f);
	return ret;
}

int RAWKAUDIO_API rawk_wav_enc_create(char *output_name, int channels, int s_rate, wav_enc_stream *stream)
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
	// need a seek function to write the total samples when closing
	cb.seek_func = (int (*)(void*,int64_t,int))fseek;

	return rawk_wav_enc_create_cb(&cb, channels, s_rate, stream);
}

void RAWKAUDIO_API rawk_wav_enc_destroy(wav_enc_stream stream)
{
	rawkwav_file *f = (rawkwav_file*)stream;
	wav_enc_close(f);
}

int RAWKAUDIO_API rawk_wav_enc_compress(wav_enc_stream stream, short **samples, int sample_count)
{
	rawkwav_file *f = (rawkwav_file*)stream;
	int i, j;

	if (f==NULL||sample_count<=0)
		return RAWKERROR_INVALID_PARAM;

	for (j=0; j < sample_count; j++)
	{
		for (i=0; i < f->channels; i++)
		{
			if (f->cb.write_func(samples[i]+j, 2, 1, f->cb.datasource)!= 1)
				return RAWKERROR_IO;
		}
		f->samples++;
	}

	return 0;
}
