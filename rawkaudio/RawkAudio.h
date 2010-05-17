#ifndef _RAWKAUDIO_H
#define _RAWKAUDIO_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#ifdef RAWKAUDIO_EXPORTS
#define RAWKAUDIO_API __declspec(dllexport) __cdecl
#else
#define RAWKAUDIO_API __declspec(dllimport) __cdecl
#endif
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#else
#ifdef _LP64
#define RAWKAUDIO_API
#else
#define RAWKAUDIO_API __attribute__((cdecl))
#endif
#include <stdint.h>
#endif

#define rawk_min(a, b) (((a)>(b)) ? (b) : (a))
#define rawk_max(a, b) (((a)>(b)) ? (a) : (b))

// An invalid input parameter was given
#define RAWKERROR_INVALID_PARAM		-1
// An out of memory error occurred
#define RAWKERROR_MEMORY			-2
// A file error occurred
#define RAWKERROR_IO				-3
// The input file couldn't be identified as an Ogg Vorbis file
#define RAWKERROR_NOTVORBIS			-4
// The input file couldn't be identified as a BINK file
#define RAWKERROR_NOTBINK			-5
// BINK streams with different sampling rates are not supported
#define RAWKERROR_BINK_SAMPLING		-6
#define RAWKERROR_VGS_SAMPLING		-6
// The input file couldn't be identified as a FSB file
#define RAWKERROR_NOTFSB			-7
// The input file couldn't be identified as a VGS file
#define RAWKERROR_NOTVGS			-8
// The input file isn't a compatible WAV file
#define RAWKERROR_NOTWAV			-9
// A miscellaneous error occurred
#define RAWKERROR_UNKNOWN			-127

typedef void* vb_enc_stream;
typedef void* vb_dec_stream;
typedef void* bk_dec_stream;
typedef void* fsb_dec_stream;
typedef void* vgs_dec_stream;
typedef void* wav_enc_stream;
typedef void* wav_dec_stream;

// first four functions must match the definitions in ov_callbacks
// Do not add more data to the beginning of this struct!
typedef struct rawk_callbacks
{
	size_t (*read_func)(void *ptr, size_t size, size_t count, void *datasource);
	int (*seek_func)(void *datasource, int64_t offset, int whence);
	int (*close_func)(void *datasource);
	long (*tell_func)(void *datasource);
	size_t (*write_func)(const void *ptr, size_t size, size_t count, void *datasource);
	void *datasource;
} rawk_callbacks;


/* Creates an encoding object
* output_name = name of the file to write to (will be created)
* channels = number of audio channels
* s_rate = input sampling rate in Hz
* target_s_rate = output sampling rate in Hz
* bitrate = bitrate to use per channel (in bps), 0 means use the default (72000)
* stream = (out) a pointer to a variable that will hold the encoding object
* returns 0 on success or a RAWKERROR on failure
*/
int RAWKAUDIO_API rawk_vorbis_enc_create(char *output_name, int channels, int s_rate, int target_s_rate, int bitrate, vb_enc_stream *stream);

/* The same function, but using callback functions
* The callbacks are stored internally, the callback struct passed to this function does not need
* to be static and can be freed after the function has returned.
*/
int RAWKAUDIO_API rawk_vorbis_enc_create_cb(rawk_callbacks *cb, int channels, int s_rate, int target_s_rate, int bitrate, vb_enc_stream *stream);

/* Finalize and destroy an encoding object
* stream = the encoding object to destroy
*/
void RAWKAUDIO_API rawk_vorbis_enc_destroy(vb_enc_stream stream);

/* Encode audio samples from an array of buffers and add them to the output file
* stream = encoding object
* samples = an array of buffers filled with samples from each channel
* sample_count = the number of samples (per channel) to encode
* returns 0 on success or a RAWKERROR on failure
*/
int RAWKAUDIO_API rawk_vorbis_enc_compress(vb_enc_stream stream, short **samples, int sample_count);

/* Creates a decoding object
* input_name = name of the file to read from (must exist)
* channels = (out) receives the number of channels in the input file
* rate = (out) receives the sampling rate of the input file (in Hz)
* samples = (out) receives the total number of samples in the input file
* stream = (out) a pointer to a variable that will hold the decoding object
* returns 0 on success or a RAWKERROR on failure
* the input file can be a regular .ogg vorbis file or an unencrypted .mogg file
*/
int RAWKAUDIO_API rawk_vorbis_dec_create(char *input_name, int *channels, int *rate, int64_t *samples, vb_dec_stream *stream);
/* The same function, but using callback functions
* The callbacks are stored internally, the callback struct passed to this function does not need
* to be static and can be freed after the function has returned.
*/
int RAWKAUDIO_API rawk_vorbis_dec_create_cb(rawk_callbacks *cb, int *channels, int *rate, int64_t *samples, vb_dec_stream *stream);

/* Finalize and destroy a decoding object
* stream = the decoding object to destroy
*/
void RAWKAUDIO_API rawk_vorbis_dec_destroy(vb_dec_stream stream);

/* Seeks to a new position in the input file
* stream = decoding object
* sample_pos = the number of the output sample position to seek to
* returns 0 on success or a RAWKERROR on failure
*/
int RAWKAUDIO_API rawk_vorbis_dec_seek(vb_dec_stream stream, int64_t sample_pos);

/* Retrieve decoded samples from the input file and store them separate buffers for each channel
* stream = decoding object
* samples = an array of buffers for the output samples (one buffer for each channel)
* length = maximum number of samples the output buffers can hold
* returns the number of samples (per channel) placed in the output buffers, or a RAWKERROR on failure
* note that the output buffers may not always be completely filled when this function returns
*/
int RAWKAUDIO_API rawk_vorbis_dec_decompress(vb_dec_stream stream, short **samples, int length);

/* Mixes an array of input channel buffers into an array of output channel buffers
* in = the input buffers containing samples for each channel
* in_channels = number of input channels
* out = the output buffers for each channel
* out_channels = number of output channels
* masks = an array of bitmasks, for each output channel that specifies which input channels will be included in the mix
* samples = the number of samples (per channel) to process
* normalize = specifies whether to scale the output based on the number of input channels
* returns 0 on success or a RAWKERROR on failure
*/
int RAWKAUDIO_API rawk_downmix(short **in, int in_channels, short **out, int out_channels, unsigned short *masks, int samples, int normalize);

int RAWKAUDIO_API rawk_bink_dec_create(char *input_name, int *channels, int *rate, int64_t *samples, bk_dec_stream *stream);
int RAWKAUDIO_API rawk_bink_dec_create_cb(rawk_callbacks *cb, int *channels, int *rate, int64_t *samples, bk_dec_stream *stream);
void RAWKAUDIO_API rawk_bink_dec_destroy(bk_dec_stream stream);
int RAWKAUDIO_API rawk_bink_dec_decompress(bk_dec_stream stream, short **samples, int length);
int RAWKAUDIO_API rawk_bink_dec_seek(bk_dec_stream stream, int64_t sample_pos);

int RAWKAUDIO_API rawk_fsb_dec_create_cb(rawk_callbacks *cb, int *channels, int *rate, int64_t *samples, fsb_dec_stream *stream);
int RAWKAUDIO_API rawk_fsb_dec_create(char *input_name, int *channels, int *rate, int64_t *samples, fsb_dec_stream *stream);
void RAWKAUDIO_API rawk_fsb_dec_destroy(fsb_dec_stream stream);
int RAWKAUDIO_API rawk_fsb_dec_decompress(fsb_dec_stream stream, short **samples, int length);
int RAWKAUDIO_API rawk_fsb_dec_seek(fsb_dec_stream stream, int64_t sample_pos);

int RAWKAUDIO_API rawk_vgs_dec_create_cb(rawk_callbacks *cb, int *channels, int *rate, int64_t *samples, vgs_dec_stream *stream);
int RAWKAUDIO_API rawk_vgs_dec_create(char *input_name, int *channels, int *rate, int64_t *samples, vgs_dec_stream *stream);
void RAWKAUDIO_API rawk_vgs_dec_destroy(vgs_dec_stream stream);
int RAWKAUDIO_API rawk_vgs_dec_decompress(vgs_dec_stream stream, short **samples, int length);
int RAWKAUDIO_API rawk_vgs_dec_seek(vgs_dec_stream stream, int64_t sample_pos);

int RAWKAUDIO_API rawk_wav_dec_create_cb(rawk_callbacks *cb, int *channels, int *rate, int64_t *samples, wav_dec_stream *stream);
int RAWKAUDIO_API rawk_wav_dec_create(char *input_name, int *channels, int *rate, int64_t *samples, wav_dec_stream *stream);
void RAWKAUDIO_API rawk_wav_dec_destroy(wav_dec_stream stream);
int RAWKAUDIO_API rawk_wav_dec_decompress(wav_dec_stream stream, short **samples, int length);
int RAWKAUDIO_API rawk_wav_dec_seek(wav_dec_stream stream, int64_t sample_pos);

int RAWKAUDIO_API rawk_wav_enc_create(char *output_name, int channels, int s_rate, wav_enc_stream *stream);
int RAWKAUDIO_API rawk_wav_enc_create_cb(rawk_callbacks *cb, int channels, int s_rate, wav_enc_stream *stream);
void RAWKAUDIO_API rawk_wav_enc_destroy(wav_enc_stream stream);
int RAWKAUDIO_API rawk_wav_enc_compress(wav_enc_stream stream, short **samples, int sample_count);

#ifdef __cplusplus
}
#endif

#endif // _RAWKAUDIO_H

