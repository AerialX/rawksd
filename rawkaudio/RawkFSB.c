#define _CRT_SECURE_NO_DEPRECATE 1 // fuck off
#define _CRT_NONSTDC_NO_DEPRECATE 1 // you too

#include <stdlib.h>
#include <memory.h>
#include "RawkAudio.h"
#include <vorbis/vorbisfile.h> // for OV_CALLBACKS

/*
from http://www.fmod.org/forum/viewtopic.php?t=1551
*/

#pragma pack(2)

#define F_FLOAT     float

#define FSOUND_FSB_NAMELEN  30

/*
    Defines for FSOUND_FSB_HEADER->mode field
*/
#define FSOUND_FSB_SOURCE_FORMAT        0x00000001  /* all samples stored in their original compressed format */
#define FSOUND_FSB_SOURCE_BASICHEADERS  0x00000002  /* samples should use the basic header structure */

/*
    Defines for FSOUND_FSB_HEADER->version field
*/
#define FSOUND_FSB_VERSION_3_1          0x00030001  /* FSB version 3.1 */

/* 
    16 byte header. 
*/ 
typedef struct FSOUND_FSB_HEADER_FSB2
{ 
    char                id[4];          /* 'FSB2' */ 
    int                 numsamples;     /* number of samples in the file */ 
    int                 shdrsize;       /* size in bytes of all of the sample headers including extended information */ 
    int                 datasize;       /* size in bytes of compressed sample data */ 
} FSOUND_FSB_HEADER_FSB2; 

/* 
    24 byte header. 
*/ 
typedef struct FSOUND_FSB_HEADER_FSB3
{ 
    char                id[4];          /* 'FSB3' */ 
    int                 numsamples;     /* number of samples in the file */ 
    int                 shdrsize;       /* size in bytes of all of the sample headers including extended information */ 
    int                 datasize;       /* size in bytes of compressed sample data */ 
    unsigned int        version;        /* extended fsb version */ 
    unsigned int        mode;           /* flags that apply to all samples in the fsb */ 
} FSOUND_FSB_HEADER_FSB3;

typedef struct FSOUND_FSB_HEADER
{
    char        id[4];      /* 'FSB2', 'FSB3', 'FSB4' */
    int32_t     numsamples; /* number of samples in the file */
    int32_t     shdrsize;   /* size in bytes of all of the sample headers including extended information */
    int32_t     datasize;   /* size in bytes of compressed sample data */
    uint32_t    version;    /* extended fsb version */
    uint32_t    mode;       /* flags that apply to all samples in the fsb */
    char        zero[8];    /* ??? */
    uint8_t     hash[16];   /* hash??? */
} FSOUND_FSB_HEADER;

/*
    8 byte sample header.
*/
typedef struct FSOUND_FSB_SAMPLE_HEADER_BASIC
{
    uint32_t    lengthsamples;
    uint32_t    lengthcompressedbytes;   
} FSOUND_FSB_SAMPLE_HEADER_BASIC; 

/* 
    64 byte sample header. 
*/ 
typedef struct FSOUND_FSB_SAMPLE_HEADER2
{ 
    unsigned short  size; 
    char            name[FSOUND_FSB_NAMELEN]; 

    unsigned int    lengthsamples; 
    unsigned int    lengthcompressedbytes; 
    unsigned int    loopstart; 
    unsigned int    loopend; 

    unsigned int    mode; 
    int             deffreq; 
    unsigned short  defvol; 
    short           defpan; 
    unsigned short  defpri; 
    unsigned short  numchannels; 
    
} FSOUND_FSB_SAMPLE_HEADER2; 

/*
    80 byte sample header.
*/
typedef union FSOUND_FSB_SAMPLE_HEADER
{
	FSOUND_FSB_SAMPLE_HEADER_BASIC basic;
	struct
	{
		uint16_t    size;
		char        name[FSOUND_FSB_NAMELEN];

		uint32_t    lengthsamples;
		uint32_t    lengthcompressedbytes;
		uint32_t    loopstart;
		uint32_t    loopend;

		uint32_t    mode;
		int32_t     deffreq;
		uint16_t    defvol;
		int16_t     defpan;
		uint16_t    defpri;
		uint16_t    numchannels;

		F_FLOAT     mindistance;
		F_FLOAT     maxdistance;
		int32_t     varfreq;
		uint16_t    varvol;
		int16_t     varpan;
	};
} FSOUND_FSB_SAMPLE_HEADER;

#define FSOUND_LOOP_OFF         0x00000001 /* For non looping samples. */
#define FSOUND_LOOP_NORMAL      0x00000002 /* For forward looping samples. */
#define FSOUND_LOOP_BIDI        0x00000004 /* For bidirectional looping samples. (no effect if in hardware). */
#define FSOUND_8BITS            0x00000008 /* For 8 bit samples. */
#define FSOUND_16BITS           0x00000010 /* For 16 bit samples. */
#define FSOUND_MONO             0x00000020 /* For mono samples. */
#define FSOUND_STEREO           0x00000040 /* For stereo samples. */
#define FSOUND_UNSIGNED         0x00000080 /* For user created source data containing unsigned samples. */
#define FSOUND_SIGNED           0x00000100 /* For user created source data containing signed data. */
#define FSOUND_DELTA            0x00000200 /* For user created source data stored as delta values. */
#define FSOUND_IT214            0x00000400 /* For user created source data stored using IT214 compression. */
#define FSOUND_IT215            0x00000800 /* For user created source data stored using IT215 compression. */
#define FSOUND_HW3D             0x00001000 /* Attempts to make samples use 3d hardware acceleration. (if the card supports it) */
#define FSOUND_2D               0x00002000 /* Tells software (not hardware) based sample not to be included in 3d processing. */
#define FSOUND_STREAMABLE       0x00004000 /* For a streamimg sound where you feed the data to it. */
#define FSOUND_LOADMEMORY       0x00008000 /* "name" will be interpreted as a pointer to data for streaming and samples. */
#define FSOUND_LOADRAW          0x00010000 /* Will ignore file format and treat as raw pcm. */
#define FSOUND_MPEGACCURATE     0x00020000 /* For FSOUND_Stream_Open - for accurate FSOUND_Stream_GetLengthMs/FSOUND_Stream_SetTime. WARNING, see FSOUND_Stream_Open for inital opening time performance issues. */
#define FSOUND_FORCEMONO        0x00040000 /* For forcing stereo streams and samples to be mono - needed if using FSOUND_HW3D and stereo data - incurs a small speed hit for streams */
#define FSOUND_HW2D             0x00080000 /* 2D hardware sounds. allows hardware specific effects */
#define FSOUND_ENABLEFX         0x00100000 /* Allows DX8 FX to be played back on a sound. Requires DirectX 8 - Note these sounds cannot be played more than once, be 8 bit, be less than a certain size, or have a changing frequency */
#define FSOUND_MPEGHALFRATE     0x00200000 /* For FMODCE only - decodes mpeg streams using a lower quality decode, but faster execution */
#define FSOUND_IMAADPCM         0x00400000 /* Contents are stored compressed as IMA ADPCM */
#define FSOUND_VAG              0x00800000 /* For PS2 only - Contents are compressed as Sony VAG format */
#define FSOUND_XMA              0x01000000
#define FSOUND_GCADPCM          0x02000000 /* For Gamecube only - Contents are compressed as Gamecube DSP-ADPCM format */
#define FSOUND_MULTICHANNEL     0x04000000 /* For PS2 and Gamecube only - Contents are interleaved into a multi-channel (more than stereo) format */
#define FSOUND_USECORE0         0x08000000 /* For PS2 only - Sample/Stream is forced to use hardware voices 00-23 */
#define FSOUND_USECORE1         0x10000000 /* For PS2 only - Sample/Stream is forced to use hardware voices 24-47 */
#define FSOUND_LOADMEMORYIOP    0x20000000 /* For PS2 only - "name" will be interpreted as a pointer to data for streaming and samples. The address provided will be an IOP address */
#define FSOUND_IGNORETAGS       0x40000000 /* Skips id3v2 etc tag checks when opening a stream, to reduce seek/read overhead when opening files (helps with CD performance) */
#define FSOUND_STREAM_NET       0x80000000 /* Specifies an internet stream */
#define FSOUND_NORMAL           (FSOUND_16BITS | FSOUND_SIGNED | FSOUND_MONO)

#pragma pack()

static const int index_table[16] =
{
	-1, -1, -1, -1, 2, 4, 6, 8,
	-1, -1, -1, -1, 2, 4, 6, 8
};

static const int step_table[89] =
{
	    7,     8,     9,    10,    11,    12,    13,    14,    16,    17,
	   19,    21,    23,    25,    28,    31,    34,    37,    41,    45,
	   50,    55,    60,    66,    73,    80,    88,    97,   107,   118,
      130,   143,   157,   173,   190,   209,   230,   253,   279,   307,
	  337,   371,   408,   449,   494,   544,   598,   658,   724,   796,
	  876,   963,  1060,  1166,  1282,  1411,  1552,  1707,  1878,  2066,
     2272,  2499,  2749,  3024,  3327,  3660,  4026,  4428,  4871,  5358,
	 5894,  6484,  7132,  7845,  8630,  9493, 10442, 11487, 12635, 13899,
	15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

typedef struct adpcm_state
{
	int predictor;
	int step_index;
} adpcm_state;

int IMA_Calc(adpcm_state *s, int v)
{
	int diff, step;
	
	step = step_table[s->step_index];
	s->step_index = rawk_max(0, rawk_min(88, s->step_index+index_table[v]));

	diff = ((2*(v&7)+1)*step) >>3;

	s->predictor += (v&8) ? -diff : diff;
	s->predictor = rawk_max(-32768, rawk_min(32767, s->predictor));

	return s->predictor;
}

void IMA_decode(adpcm_state* s, unsigned char *in, short **out, const int channels)
{
	int i;
	unsigned char *end = in + (36*channels);
	short *left = out[0];
	short *right = out[1];

	s[0].predictor = (int16_t)(in[0] | (in[1]<<8));
	s[0].step_index = rawk_min(in[2], 88);
	in += 4;

	if (channels == 2)
	{
		s[1].predictor = (int16_t)(in[0] | (in[1]<<8));
		s[1].step_index = rawk_min(in[2], 88);
		in += 4;

		for (i=4;in<end;++in, left+=2, right+=2)
		{
			left[0] = (int16_t)IMA_Calc(s, in[0] & 0x0F);
			right[0] = (int16_t)IMA_Calc(s+1, in[4] & 0x0F);
			left[1] = (int16_t)IMA_Calc(s, in[0]>>4);
			right[1] = (int16_t)IMA_Calc(s+1, in[4]>>4);

			if (--i==0)
			{
				i=4;
				in += 4;
			}
		}
	}
	else
	{
		while (in<end)
		{
			left[0] = (int16_t)IMA_Calc(s, in[0] & 0x0F);
			left[1] = (int16_t)IMA_Calc(s, in[0]>>4);
			in++;
			left += 2;
		}
	}
}

typedef struct rawkfsb_dec_file
{
	rawk_callbacks cb;
	adpcm_state state[2];
	int channels;
	int samples;
	size_t data_offset;
	// assume block size of 72 (stereo) or 36 (mono)
	unsigned char in_buf[72];
	short out_buf[128];
	short *output[2];
	int out_buf_samples;
	int sample_pos;
} rawkfsb_dec_file;

int fsb_dec_close(rawkfsb_dec_file *f)
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

int RAWKAUDIO_API rawk_fsb_dec_create_cb(rawk_callbacks *cb, int *channels, int *rate, int64_t *samples, fsb_dec_stream *stream)
{
	int ret=0;
	rawkfsb_dec_file *f;
	FSOUND_FSB_HEADER header;
	FSOUND_FSB_SAMPLE_HEADER sample_header;
	int sample_header_size;

	if (cb==NULL||cb->read_func==NULL||channels==NULL||rate==NULL||samples==NULL||stream==NULL)
		return RAWKERROR_INVALID_PARAM;
	*stream = NULL;

	f = (rawkfsb_dec_file*)malloc(sizeof(rawkfsb_dec_file));
	if (f==NULL)
	{
		ret = RAWKERROR_MEMORY;
		goto rawk_fsb_dec_done;
	}
	memset(f, 0, sizeof(rawkfsb_dec_file));
	memcpy(&f->cb, cb, sizeof(rawk_callbacks));

	if (f->cb.read_func(&header.id, sizeof(FSOUND_FSB_HEADER_FSB2), 1, f->cb.datasource)!=1)
	{
		ret = RAWKERROR_IO;
		goto rawk_fsb_dec_done;
	}
	if (memcmp(header.id, "FSB", 3))
	{
		ret = RAWKERROR_NOTFSB;
		goto rawk_fsb_dec_done;
	}
	switch(header.id[3])
	{
	case '2':	// nothing to do
		f->data_offset = sizeof(FSOUND_FSB_HEADER_FSB2);
		sample_header_size = sizeof(FSOUND_FSB_SAMPLE_HEADER2);
		break;
	case '3':
		if (f->cb.read_func(&header.version, sizeof(FSOUND_FSB_HEADER_FSB3)-sizeof(FSOUND_FSB_HEADER_FSB2), 1, f->cb.datasource)!=1)
		{
			ret = RAWKERROR_IO;
			goto rawk_fsb_dec_done;
		}
		if (header.mode & FSOUND_FSB_SOURCE_BASICHEADERS)
			sample_header_size = sizeof(FSOUND_FSB_SAMPLE_HEADER_BASIC);
		else if (header.version == FSOUND_FSB_VERSION_3_1)
			sample_header_size = sizeof(FSOUND_FSB_SAMPLE_HEADER);
		else
			sample_header_size = sizeof(FSOUND_FSB_SAMPLE_HEADER2);
		f->data_offset = sizeof(FSOUND_FSB_HEADER_FSB3);
		break;
	case '4':
		if (f->cb.read_func(&header.version, sizeof(FSOUND_FSB_HEADER)-sizeof(FSOUND_FSB_HEADER_FSB2), 1, f->cb.datasource)!=1)
		{
			ret = RAWKERROR_IO;
			goto rawk_fsb_dec_done;
		}
		if (header.mode & FSOUND_FSB_SOURCE_BASICHEADERS)
			sample_header_size = sizeof(FSOUND_FSB_SAMPLE_HEADER_BASIC);
		else
			sample_header_size = sizeof(FSOUND_FSB_SAMPLE_HEADER);
		f->data_offset = sizeof(FSOUND_FSB_HEADER);
		break;
	default:
		ret = RAWKERROR_NOTFSB;
		goto rawk_fsb_dec_done;
	}

	f->data_offset += header.shdrsize;
	if (f->cb.read_func(&sample_header, sample_header_size, 1, f->cb.datasource)!=1)
	{
		ret = RAWKERROR_IO;
		goto rawk_fsb_dec_done;
	}
	if (sample_header_size==sizeof(FSOUND_FSB_SAMPLE_HEADER_BASIC))
	{
		//f->samples = *samples = sample_header.basic.lengthsamples;
		// Not supported - only 1 stream per file is handled here
		ret = RAWKERROR_UNKNOWN;
		goto rawk_fsb_dec_done;
	}
	else
	{
		if (!(sample_header.mode & FSOUND_IMAADPCM))
		{
			ret = RAWKERROR_NOTFSB;
			goto rawk_fsb_dec_done;
		}
		f->samples = *samples = sample_header.lengthsamples;
		f->channels = *channels = sample_header.numchannels;
		*rate = sample_header.deffreq;
		//f->cb.seek_func(f->cb.datasource, sample_header.size-f->sample_header_size, SEEK_CUR);
	}

	f->cb.seek_func(f->cb.datasource, f->data_offset, SEEK_SET);

	*stream = (bk_dec_stream)f;
rawk_fsb_dec_done:
	if (ret)
		fsb_dec_close(f);
	return ret;
}

int RAWKAUDIO_API rawk_fsb_dec_create(char *input_name, int *channels, int *rate, int64_t *samples, fsb_dec_stream *stream)
{
	rawk_callbacks cb;

	if (input_name==NULL)
		return RAWKERROR_INVALID_PARAM;

	memset(&cb, 0, sizeof(cb));

	cb.datasource = fopen(input_name, "rb");
	if (cb.datasource==NULL)
		return RAWKERROR_IO;

	memcpy(&cb, &OV_CALLBACKS_DEFAULT, sizeof(OV_CALLBACKS_DEFAULT));

	return rawk_fsb_dec_create_cb(&cb, channels, rate, samples, stream);
}

void RAWKAUDIO_API rawk_fsb_dec_destroy(fsb_dec_stream stream)
{
	rawkfsb_dec_file *f = (rawkfsb_dec_file*)stream;
	fsb_dec_close(f);
}

int RAWKAUDIO_API rawk_fsb_dec_decompress(fsb_dec_stream stream, short **samples, int length)
{
	int i;
	int written=0;
	rawkfsb_dec_file *f = (rawkfsb_dec_file*)stream;

	if (f==NULL||length<0)
		return RAWKERROR_INVALID_PARAM;

	while (length)
	{
		if (f->out_buf_samples)
		{
			int samples_to_output = rawk_min(f->out_buf_samples, length);
			for (i=0; i < f->channels; i++)
			{
				if (samples)
					memcpy(samples[i]+written, f->output[i], sizeof(short)*samples_to_output);
				f->output[i]+= samples_to_output;
			}
			written += samples_to_output;
			length -= samples_to_output;
			f->out_buf_samples -= samples_to_output;
		}

		if (f->out_buf_samples==0)
		{
			if (f->sample_pos >= f->samples)
				return written ? written : RAWKERROR_INVALID_PARAM;
			if (f->cb.read_func(f->in_buf, 36*f->channels, 1, f->cb.datasource) != 1)
				return written ? written : RAWKERROR_IO;
			f->output[0] = f->out_buf;
			f->output[1] = f->out_buf + ((sizeof(f->out_buf)/sizeof(f->out_buf[0]))>>1);
			f->out_buf_samples = 64;
			IMA_decode(f->state, f->in_buf, f->output, f->channels);
			f->sample_pos += 64;
		}
	}

	return written;
}

int RAWKAUDIO_API rawk_fsb_dec_seek(fsb_dec_stream stream, int64_t sample_pos)
{
	rawkfsb_dec_file *f = (rawkfsb_dec_file*)stream;
	int ret;

	if (f==NULL||sample_pos<0||sample_pos>f->samples)
		return RAWKERROR_INVALID_PARAM;

	f->out_buf_samples = 0;
	ret = f->cb.seek_func(f->cb.datasource, f->data_offset+(sample_pos/64)*36*f->channels, SEEK_SET);
	if (ret != 0)
		return RAWKERROR_IO;
	f->sample_pos = (sample_pos&~63);

	ret = rawk_fsb_dec_decompress(stream, NULL, sample_pos%64);
	return (ret<0) ? ret : 0;
}
