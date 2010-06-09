#define _CRT_SECURE_NO_DEPRECATE 1 // fuck off
#define _CRT_NONSTDC_NO_DEPRECATE 1 // you too

#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <vorbis/vorbisfile.h> // for OV_CALLBACKS
#include "RawkAudio.h"

#define BINK_FLAG_STEREO			(1<<(13+16))
#define BINK_FLAG_DCT				(1<<(12+16))

#define BLOCK_MAX_SIZE ((1<<11)*2)

// forward declarations
void ddct(int n, int isgn, double *a, int *ip, double *w);
void rdft(int n, int isgn, double *a, int *ip, double *w);

typedef struct rawk_bitstream
{
	int bits;
	int bitpos;
	const unsigned char *bitptr;
	const unsigned char *bitend;
} rawk_bitstream;

static __inline int bitbookmark(rawk_bitstream *p)
{
	return (int)p->bitptr*8 + p->bitpos;
}

static __inline int biteof(rawk_bitstream *p)
{
	return p->bitptr >= p->bitend;
}

#define bitflush(p, n) (p)->bitpos += n;
#define bitbegin_pos(p)								\
	int bitpos = (p)->bitpos;						\
	int bits = (p)->bits;
#define bitend_pos(p)								\
	(p)->bitpos = bitpos;
#define bitload_pos(p)								\
	if (bitpos >= 8) {								\
		const unsigned char *bitptr = (p)->bitptr;	\
		do {										\
			bits = ((unsigned int)bits>>8) | (*bitptr++<<24);			\
			bitpos -= 8;							\
		} while (bitpos >= 8);						\
		(p)->bits = bits;							\
		(p)->bitptr = bitptr;						\
	}
#define bitshow_pos(p, n) ((unsigned int)(bits >> bitpos) & (((1<<(n))-1)))
//#define bitshow_pos(p, n) ((unsigned int)(bits << bitpos) >> (32-(n)))
#define bitflush_pos(p, n) bitpos += n;

static void bitload(rawk_bitstream* p)
{
	bitbegin_pos(p);
	bitload_pos(p);
	bitend_pos(p);
}

static int bitread(rawk_bitstream* p, int n)
{
	int i;
	bitbegin_pos(p);
	bitload_pos(p);
	i = bitshow_pos(p, n);
	bitflush_pos(p, n);
	bitend_pos(p);
	return i;
}

static void bitinit(rawk_bitstream *p, const unsigned char *stream, size_t len)
{
	p->bits = 0;
	p->bitpos = 32;
	p->bitptr = stream;
	p->bitend = stream+len+4;
}

static void bitalign32(rawk_bitstream *p)
{
	int n = (-bitbookmark(p)) & 31;
	if (n)
		bitflush(p, n);
	bitload(p);
}

typedef struct rawkbink_dec_stream
{
	int rate;
	int stereo; // bool
	int dct; // 1 = use dct, 0 = use fft
	size_t max_decoded_size; // size of largest decoded frame
	int track_no;
	int64_t samples;
	int frame_len;
	int overlap_len;
	int num_bands;
	unsigned int *bands;
	double root;
	short window[BLOCK_MAX_SIZE/16];
	short *decoded;
	int first;
	rawk_bitstream p;
	double coeffs[BLOCK_MAX_SIZE];
	double ddct_w[BLOCK_MAX_SIZE*5/4];
	double rdft_w[BLOCK_MAX_SIZE/2];
	int fft4g_ip[48];
	short *output[2];
	int output_samples;
} rawkbink_dec_stream;

typedef struct rawkbink_dec_file
{
	rawk_callbacks cb;
	int frames;
	int tracks;
	size_t *frame_index;
	rawkbink_dec_stream *streams;
	int channels;
	unsigned char *frame_data_buffer;
	int current_frame;
	rawkbink_dec_stream **index;
	int64_t samples;
	int encrypted;
	unsigned int key[4];
	uint64_t nonce[2];
	uint64_t enc_start;
	int64_t enc_offset;
	rawk_callbacks enc_cb;
	uint64_t dec_bytes[2];
	int dec_bytes_left;
} rawkbink_dec_file;

#pragma pack(push, 1)
typedef struct bink_header
{
	char BIK_sig[3];
	char rev;
	int file_size_minus_eight;
	int frames;
	int largest_frame;
	int unused; // frames again
	int width;
	int height;
	int fps_num;
	int fps_den;
	int vid_flags;
	int audio_tracks;
} bink_header;
#pragma pack(pop)

static const unsigned short critical_freqs[25] =
{
	  100,   200,   300,   400,   510,   630,   770,   920,
     1080,  1270,  1480,  1720,  2000,  2320,  2700,  3150,
	 3700,  4400,  5300,  6400,  7700,  9500, 12000, 15500,
	 24500
};

static const unsigned char rle_length_tab[16] =
{
	 2,  3,  4,  5 , 6,  8,  9, 10,
	11, 12, 13, 14, 15, 16, 32, 64
};

static const struct
{
	unsigned int id[4];
	unsigned int key[4];
} xtea_keys[] = {
	{ // apunk.bik
		{ 0xED5809FB, 0x6F04009D, 0xB6ECFDA4, 0x2C823719 },
		{ 0x97227381, 0xE6C19BE7, 0x97B2593E, 0xE2CB5A12 }
	},
	{ // accidentallyinlove.bik
		{ 0x3D30B113, 0x870E37A9, 0x0EC5F37F, 0x3A12FEE3 },
		{ 0xB0BD3C9E, 0x0A83BA24, 0x83487EF2, 0x2CF1856E },
	},
	{ // aliensexist.bik
		{ 0xBD50F05A, 0xBF0393B1, 0x239AEDC2, 0x8C5BC14F },
		{ 0xB0E444EE, 0x7EC318FC, 0x8C97657F, 0x613C4C3E }
	},
	{ // breakout.bik
		{ 0xF497FDF2, 0x961177CC, 0x5B7A2E75, 0x2D41B187 },
		{ 0x593A505F, 0x43815061, 0x44F8E8BA, 0x1EE5D1B6 }
	},
	{ // checkyesjuliet.bik
		{ 0x3446A077, 0x77C02CA6, 0x880B4BB3, 0xBD193B5A },
		{ 0xBFCD2BFC, 0xB44BA72D, 0x74A2EB68, 0x1A4BC751 }
	},
	{ // crash.bik
		{ 0xB8EA3BCA, 0x65EEB703, 0x343D90F4, 0xC12F4748 },
		{ 0xF75AC195, 0xDA333D78, 0x5DFBD222, 0xF1A37610 }
	},
	{ // crocodilerock.bik
		{ 0xDD6EA5C0, 0xA8F734EC, 0xC8D56088, 0x08B9F0BB },
		{ 0xF93CF792, 0xB2BF493A, 0x6A9421E9, 0xC6C05219 }
	},
	{ // dig.bik
		{ 0x8A8F4406, 0x4785B743, 0xEE25FAAC, 0x59E91E3E },
		{ 0xB2B77C3E, 0x7FBD8F7B, 0x461DC294, 0x4CDB38DD }
	},
	{ // dreamingofyou.bik
		{ 0x06BD4777, 0xE56DE317, 0x27F2D52A, 0x6749C72D },
		{ 0x7D780DA9, 0xD119C6C5, 0xB0D15F61, 0x9ECC09DC }
	},
	{ // everylittlething.bik
		{ 0x5FB6E7E0, 0xC7ABDFB8, 0x6AF1ECCC, 0xDFECD2DB },
		{ 0x2FC69790, 0x157975C8, 0xE43E00F7, 0x2D7B915B }
	},
	{ // fire.bik
		{ 0x7474099E, 0xEA8395C0, 0x11BB2F83, 0x8C456FEB },
		{ 0xE8A25617, 0x1676AED3, 0x195F9946, 0x7F2C96DC }
	},
	{ // freefallin.bik
		{ 0xB121211B, 0x237F4141, 0x69EB24FF, 0x2B20BE96 },
		{ 0x4EDEDEE4, 0xDC80BEBE, 0x9614DB00, 0x2E91EB69 }
	},
	{ // ghostbusters.bik
		{ 0x5B915A1F, 0x6A54174C, 0x2691F753, 0xFDC1DC23 },
		{ 0x85D6232C, 0xF34E958A, 0x60AFBCC8, 0x5B4C50EB }
	},
	{ // girlsandboys.bik
		{ 0x3119CA1B, 0x65BAF78B, 0xC24E47AA, 0x57A093B4 },
		{ 0x156CBFA4, 0x4B6AD3AF, 0x2E55BB84, 0xB558303B }
	},
	{ // grace.bik
		{ 0xC4211E35, 0xBA270F17, 0xBBBD87BE, 0x137D241E },
		{ 0xD5890120, 0xCAA9A087, 0xDDDF653B, 0x25628194 }
	},
	{ // intoodeep.bik
		{ 0x1BA4F217, 0xA90E0A9D, 0x98ED9BEC, 0x819F1D13 },
		{ 0x96297F9A, 0x24838710, 0x15601661, 0x0C12909E }
	},
	{ // iwantyouback.bik
		{ 0x9ECFCD54, 0x36EE9B03, 0xCB6891EC, 0xDE66FA3C },
		{ 0x85D3D64F, 0x2DF58018, 0xD0738AF7, 0x3A7DE127 }
	},
	{ // justfortonight.bik
		{ 0x02D0C613, 0xA46FC1DF, 0x53D2EB70, 0xB41DC1CC },
		{ 0xF246105E, 0xE95FCE95, 0x9B2596F2, 0x74973601 }
	},
	{ // kungfufighting.bik
		{ 0x72EB2EB8, 0xD843DE45, 0x5221E078, 0xAAD13C37 },
		{ 0x319169E2, 0x44419ECF, 0x69A81C12, 0x5389D68A }
	},
	{ // letsdance.bik
		{ 0x53CBC174, 0x02B888D4, 0x9CBCEC1E, 0x78C0BA3D },
		{ 0x0A3C6057, 0xE28326B7, 0x0B40EFFE, 0xF2B361E6 }
	},
	{ // lifeisahighway.bik
		{ 0x592E3D84, 0x2C0745BE, 0x0F9DDBA1, 0xAE3C94D7 },
		{ 0x6D1A09B0, 0x1833718A, 0x28A9EF95, 0x54544814 }
	},
	{ // makemesmile.bik
		{ 0xACFD24A7, 0xF9C87FB7, 0x37D617A2, 0x1B98F81B },
		{ 0x6D9EA8C7, 0x85CEDF01, 0x89B15819, 0xAF9F7B1F }
	},
	{ // monster.bik
		{ 0x8C7DBE7E, 0x81F73D16, 0x54BC31A7, 0x2D13BA93 },
		{ 0x2F9EBAD3, 0x67E9B0CF, 0x29D045E3, 0x2D1BACAA }
	},
	{ // naive.bik
		{ 0xC2A2D95C, 0xBAA3C717, 0xCC035FE0, 0xFDD3D2EC },
		{ 0x1EFDC972, 0x63E3F084, 0xEC41327F, 0x927FC6E8 }
	},
	{ // realwildchild.bik
		{ 0xCFFD5CC7, 0x50A36461, 0x2D730883, 0x76FC064D },
		{ 0xE4F54D57, 0x8CDEBC4A, 0xF8D093A1, 0x48854ABE }
	},
	{ // rideawhiteswan.bik
		{ 0x5C0CB895, 0xAF7699C6, 0x224FEA8D, 0xF40BAACE },
		{ 0x3D6DD9F4, 0x5E17F8A7, 0x9B124FA3, 0x34DFF4C7 }
	},
	{ // rooftops.bik
		{ 0x20E4A39A, 0x10FBE4CF, 0x43371126, 0x5CD845D0 },
		{ 0x20E4A39A, 0x10FBE4CF, 0x43371126, 0x5CD845D0 }
	},
	{ // ruby.bik
		{ 0x57D8869C, 0x432C1665, 0x048A1A35, 0x61F9AD86 },
		{ 0xDFF4A220, 0x34229EED, 0xE06E4F42, 0x52093506 }
	},
	{ // shortandsweet.bik
		{ 0xC1F92F04, 0xEA21FC92, 0xBB54E64A, 0xB1AD43DA },
		{ 0xE1D90F24, 0xCA01DCB2, 0x9B74C66A, 0x03D2A3FA }
	},
	{ // song2.bik
		{ 0x90007DB8, 0xF56B0140, 0x6A5B957B, 0xF1D99ABC },
		{ 0xB0205D98, 0xD54B2160, 0x427BB55B, 0x7BD8C5A6 }
	},
	{ // sowhat.bik
		{ 0xCFFD5CC7, 0x50A36461, 0x2D730883, 0x76FC064D },
		{ 0xE4F54D57, 0x8CDEBC4A, 0xF8D093A1, 0x48854ABE }
	},
	{ // stumbleandfall.bik
		{ 0x8E141983, 0x7F8CD036, 0x53DDD847, 0xF3A8355D },
		{ 0x7EB6EF68, 0xEFA560AA, 0x1FF0CA55, 0x7E37F7F2 }
	},
	{ // suddenlyisee.bik
		{ 0x43E11C83, 0xE7CAD997, 0x6CFFFB82, 0xF99BC459 },
		{ 0x0BA954CB, 0xAF8291DF, 0x82362ECA, 0xBE108778 }
	},
	{ // summerof69.bik
		{ 0x9188AE00, 0x9D50794F, 0x1F46625B, 0x095D884D },
		{ 0x39200EA8, 0x3811F1EF, 0xAEF1EDDC, 0xD461A601 }
	},
	{ // swingswing.bik
		{ 0xF4E96BA5, 0xDFAFA044, 0x08F75EA2, 0x2A546D23 },
		{ 0xCDA09777, 0x28958AC3, 0xF6079B6E, 0x7AB44709 }
	},
	{ // thefinalcountdown.bik
		{ 0x94B157DD, 0x77CD7BFB, 0x4FE2E291, 0x8055C487 },
		{ 0x587D9B11, 0x44431837, 0x7D6E4B95, 0xB7B58B80 }
	},
	{ // thepassenger.bik
		{ 0x10742781, 0x1385F564, 0x12C2A898, 0x494DA34B },
		{ 0x4CFD8684, 0x12D0F21F, 0x824D3F22, 0xFBD08151 }
	},
	{ // thunder.bik
		{ 0xA1887852, 0x4BA195F6, 0x75E8BE83, 0x8DF6B639 },
		{ 0x2642D4A3, 0x3410FB4A, 0xCE035724, 0x307D44A3 }
	},
	{ // ticktickboom.bik
		{ 0xF041C402, 0xDF5FF0C0, 0x3AF46475, 0x2F3C4478 },
		{ 0xC677F234, 0x8909A6F6, 0xAB06D723, 0xB5F49301 }
	},
	{ // twoprinces.bik
		{ 0xEE0617E7, 0x8FA1FDEF, 0x98D26F71, 0x78BE108F },
		{ 0x0AE2F303, 0x6B45190B, 0x7C368B95, 0x9C5AF46B }
	},
	{ // valerie.bik
		{ 0x00D89E28, 0xA615D0CD, 0xB9A8E7F8, 0xB385F1CE },
		{ 0x00D89E28, 0xA615D0CD, 0xB9A8E7F8, 0xE62E3CCE }
	},
	{ // walkingonsunshine.bik
		{ 0xCBEDDA55, 0x8BEBF465, 0xEC3FCC4D, 0x2FF6BA0D },
		{ 0x290F38B7, 0x69091687, 0x0EDD2EAF, 0xD5A767EF }
	},
	{ // wearethechampions.bik
		{ 0x424A0E62, 0x019C094D, 0x0745DACE, 0x2C490C87 },
		{ 0xC1C98DE1, 0x821F8ACE, 0x88C6594D, 0x93D7D44A }
	},
	{ // wewillrockyou1.bik
		{ 0x37FA41C3, 0x7640537D, 0xFA0841DE, 0xF2D52C61 },
		{ 0xA504BE70, 0xC0B65FA0, 0xAA25A98A, 0x5E1243F8 }
	},
	{ // wordup.bik
		{ 0x592E3D84, 0x2C0745BE, 0x0F9DDBA1, 0xAE3C94D7 },
		{ 0x6D1A09B0, 0x1833718A, 0x28A9EF95, 0x54544814 }
	},
	{ // yougiveloveabadname.bik
		{ 0x3532B2DE, 0xDAD0BD75, 0x1B8783D4, 0x97A5C341 },
		{ 0xC3C44452, 0x73FB9675, 0xC38BDE22, 0xF725B331 }
	},
	{ // 21stcenturybreakdown.bik
		{ 0x8F21C9B4, 0xBBCFED22, 0x0F9C7A8E, 0x7A0A3CF6 },
		{ 0x38E33B0E, 0x2A7C7256, 0x0A9962AA, 0xF35B35EB }
	},
	{ // americaneulogy.bik
		{ 0x83A19E9F, 0x3616A9BF, 0xCC2E517E, 0x5F8F575C },
		{ 0xB6384ECA, 0x476C66C1, 0x4EEF94C5, 0xBB967616 }
	},
	{ // americanidiot.bik
		{ 0x697D832F, 0xFB7F2FF3, 0xCBD00E66, 0xC9F8C776 },
		{ 0x96827CD0, 0x0480D00C, 0x342FF199, 0x42F7B889 }
	},
	{ // arewethewaitingstjimmy.bik
		{ 0x8487A32F, 0x6E640EC0, 0xF25EB1AF, 0x5C0368C7 },
		{ 0xC56847BE, 0xE2024EF2, 0x52B27D60, 0x3E38E287 }
	},
	{ // basketcase.bik
		{ 0x45603707, 0xC2CEFA4B, 0x43A7EEB0, 0x11323E9E },
		{ 0xEC9F00A5, 0xBC3318DD, 0x989AEBF2, 0x0DAF56D9 }
	},
	{ // boulevardof.bik
		{ 0xA5A599D3, 0x29E3D964, 0x8829F566, 0x70DADA3B },
		{ 0x45457933, 0x78033D84, 0x325D5D90, 0x0E45ADA5 }
	},
	{ // brainstewjaded.bik
		{ 0xED5809FB, 0x6F04009D, 0xB6ECFDA4, 0x2C823719 },
		{ 0x97227381, 0xE6C19BE7, 0x97B2593E, 0xE2CB5A12 }
	},
	{ // burnout.bik
		{ 0x1CED7625, 0x53988A92, 0xFC60A677, 0x5891ECD1 },
		{ 0xD829B2E1, 0x975C4E56, 0xB8FC9AB3, 0x73B4DF32 }
	},
	{ // chump.bik
		{ 0x7B642F63, 0x7ED21C60, 0x3F2596FC, 0xBC55E011 },
		{ 0x03541688, 0xBA29CBDE, 0xA9141F1C, 0xDCC9A9B4 }
	},
	{ // comingclean.bik
		{ 0x7B752C54, 0x78E868A2, 0xA1DFA62B, 0x98FA0DF7 },
		{ 0x1F114830, 0x1C8C0CC6, 0xC5BBC24F, 0xE2B1D293 }
	},
	{ // emeniussleepus.bik
		{ 0x0AA0D2EE, 0x6C7EFF73, 0xDAB62B99, 0xFD9295C6 },
		{ 0xDE74063A, 0xB8AA2BA7, 0xF29C2E4D, 0x1159CC1C }
	},
	{ // extraordinarygirl.bik
		{ 0x4C493E58, 0x12D89F98, 0x00BCE1E9, 0xE0FCE386 },
		{ 0x5E65E82A, 0xE512B843, 0x6255E194, 0x45757A7D }
	},
	{ // fod.bik
		{ 0x864F670B, 0x3F3FF930, 0x49CA967E, 0x75D0FC11 },
		{ 0x15DCF498, 0xACAC6AA3, 0xDA5905ED, 0xA6B95D82 }
	},
	{ // geekstink.bik
		{ 0x23627303, 0xDCB05F42, 0x6AF28066, 0xAAD83C60 },
		{ 0xE58AD286, 0x7222006B, 0xE91BD1F2, 0x63D1DF77 }
	},
	{ // goodriddance.bik
		{ 0x69497806, 0x895A58B5, 0xB396F565, 0x45F9AAAF },
		{ 0x292B0D73, 0x0A413D36, 0x0F908197, 0x5DDE32A5 }
	},
	{ // havingablast.bik
		{ 0xC45A5580, 0x82F3709F, 0x3381BA0F, 0xC6F4EB1E },
		{ 0xA33D32E7, 0xE59417F8, 0xAEE6DD68, 0x44848DDB }
	},
	{ // hitchinaride.bik
		{ 0xDF9B4CF5, 0xC515AF40, 0x579C6AF6, 0xA0D3031E },
		{ 0x7B3FE851, 0x5E0939E4, 0xEF1193AF, 0x48207BA6 }
	},
	{ // holiday.bik
		{ 0x55E551EE, 0x7DD7FBB5, 0x840F2E06, 0xE814836B },
		{ 0xDE6EDA65, 0xF65C703E, 0x0F84A58D, 0x65C383E0 }
	},
	{ // homecoming.bik
		{ 0xE92A1075, 0xFD95AD23, 0x1AEE1CAC, 0x3E7D6ABE },
		{ 0x3965C0A5, 0xEB95AD55, 0x1413E751, 0xDD464F30 }
	},
	{ // horseshoes.bik
		{ 0x819F2235, 0xAEA9B7E6, 0xB7664BAA, 0x8FEFE467 },
		{ 0x4EA7D1FD, 0xE61EF7BA, 0x52AC49ED, 0x6F4EB264 }
	},
	{ // intheend.bik
		{ 0x9FBF7845, 0xADB0A0AC, 0x14C0E1EE, 0x23D6209D },
		{ 0x894559E0, 0x8E4401C5, 0x1E0FECA2, 0x1B9DFDA3 }
	},
	{ // jesusofsuburbia.bik
		{ 0x2F26240F, 0xAC7C2ED3, 0x96C95AE8, 0xA76EF73F },
		{ 0x2F26240F, 0xAC7C2ED3, 0x96C95AE8, 0xCB6EF73F }
	},
	{ // lastnightonearth.bik
		{ 0xC83F2B36, 0x23D3E168, 0x411B8A1B, 0xC1404235 },
		{ 0xF3BAFC9F, 0xD012EBDD, 0x5B95F8C4, 0x1A5854E2 }
	},
	{ // letterbomb.bik
		{ 0x2F2A259C, 0xF5CCA470, 0x2BA227D3, 0x3A32A69B },
		{ 0xA2A7A811, 0x784129FD, 0xA62FAA5E, 0xB7A02B16 }
	},
	{ // littlegirl.bik
		{ 0x9297B147, 0xC70A6A5B, 0xF0CD7A88, 0xB8FD02E8 },
		{ 0x717452A4, 0x24E989B8, 0xB0C0B06B, 0x04EAF648 }
	},
	{ // lobotomy.bik
		{ 0x11C17C8C, 0xF7FAE263, 0x75097941, 0x365EAC6C },
		{ 0x7B1A50DA, 0x82478809, 0xB81F93C9, 0x881A1899 }
	},
	{ // longview.bik
		{ 0x5B915A1F, 0x6A54174C, 0x2691F753, 0xFDC1DC23 },
		{ 0x85D6232C, 0xF34E958A, 0x60AFBCC8, 0x5B4C50EB }
	},
	{ // minority.bik
		{ 0x87097D1A, 0x715497AD, 0xBC3C418A, 0x86EF07F6 },
		{ 0x941A6E09, 0x624784BE, 0xAF2F5299, 0x613958E5 }
	},
	{ // murdercity.bik
		{ 0x615993F0, 0x5213BC99, 0x47F9F66C, 0x6B8ECA8D },
		{ 0x7E8B3073, 0xAC51AAD0, 0x00BEB1AC, 0x049E2270 }
	},
	{ // niceguys.bik
		{ 0xC2A88211, 0x9DB6B1B7, 0xD71DCFB1, 0x5D9321E3 },
		{ 0x399CF078, 0x80215EA3, 0x125E923B, 0xEC191BA8 }
	},
	{ // novacaineshesarebel.bik
		{ 0x95019A7E, 0x07635445, 0x50598210, 0xCF4852B8 },
		{ 0xD7F0403C, 0xE95B6C89, 0xA6503006, 0xB6F93841 }
	},
	{ // peacemaker.bik
		{ 0xEAA46CD1, 0xDA24A8E1, 0xFB0F228E, 0x3EE730AA },
		{ 0xC8417DC0, 0x1F1B7C1A, 0xCD1DF623, 0xC90C8C39 }
	},
	{ // pullingteeth.bik
		{ 0x9E8E8AC5, 0xC44AF40F, 0x063FF5CB, 0xF8F9A98A },
		{ 0x4151551A, 0x1B952BD0, 0xB917CC14, 0x272F3F7E }
	},
	{ // restlessheart.bik
		{ 0x235BB33C, 0x18313FF7, 0xA2A63E48, 0xD172175A },
		{ 0x94AB3037, 0xD6CBCA64, 0xCDBE6FBA, 0x5083F535 }
	},
	{ // sassafrasroots.bik
		{ 0x090103C4, 0x05A8A867, 0xFFC8406A, 0xA49A69F4 },
		{ 0xCDC5C700, 0xEEDBDBA3, 0xDB5E95C6, 0x447A3B62 }
	},
	{ // seethelight.bik
		{ 0xC7A8AAEF, 0x260F1219, 0x0D7D7792, 0x337C1757 },
		{ 0x1C737134, 0xFDD4C9C2, 0xD6A6AC49, 0xCFA7CC8C }
	},
	{ // she.bik
		{ 0x37685E02, 0x1716CEA2, 0x59E68563, 0xD1A32427 },
		{ 0x86D9EFB3, 0xBDA77F13, 0xE6CFAA7E, 0x5F967A55 }
	},
	{ // songofthecentury.bik
		{ 0x01813ACC, 0x94DA5889, 0x2D7878EC, 0x96A50327 },
		{ 0xB863FC30, 0x214AAE2A, 0xE526CF64, 0x82AD7FBE }
	},
	{ // thestaticage.bik
		{ 0x4477716F, 0xFF81C08F, 0x8FEF4AC3, 0x7FF3B2B2 },
		{ 0x9616AB73, 0x8D53125D, 0x970538B1, 0xABB3CE4F }
	},
	{ // wakemeup.bik
		{ 0x0D8CB064, 0x675ED27B, 0xD5D124F4, 0x8A014275 },
		{ 0x1796AA7E, 0x1444C861, 0x4593E8D4, 0xD555595A }
	},
	{ // warning.bik
		{ 0x34E133E5, 0xC1E3C187, 0xE7D5F7EC, 0xB670CBE9 },
		{ 0xC411C315, 0x99C9F677, 0xADA9B758, 0xE07792CF }
	},
	{ // welcometoparadise.bik
		{ 0x0C395356, 0x7F02C6F8, 0x9F39BED3, 0xC47E764C },
		{ 0x453C5604, 0xB3478FB1, 0xF8A33158, 0x4DED8B94 }
	},
	{ // whatsername.bik
		{ 0x6EF88296, 0xC4633732, 0xE8B0A92C, 0xC9504A17 },
		{ 0xA533495D, 0x0823BFF9, 0xD584987F, 0x130F9BDF }
	},
	{ // whenicomearound.bik
		{ 0xF15EF22B, 0xCADB8DE8, 0xDD72C6AC, 0xE3BE83B5 },
		{ 0x89268A53, 0xB2A3F590, 0xACE7A0D4, 0x855B3EDF }
	}
};

uint64_t Encipher(uint64_t block, const unsigned int *key)
{
	const unsigned int delta = 0x9E3779B9;
	unsigned int v1 = (block>>32), v0 = (unsigned int)block, sum=0;
	int i;

	for (i=0; i < 4; i++)
	{
		v0 += (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]);
		sum += delta;
		v1 += (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum>>11) & 3]);
	}

	return ((uint64_t)v1 << 32) | v0;
}

size_t enc_read(void *ptr, size_t size, size_t count, void *datasource)
{
	rawkbink_dec_file *f = (rawkbink_dec_file*)datasource;
	size_t bytes_read=0, bytes_to_read = size*count;

	if (!f->enc_start || f->enc_offset < f->enc_start)
	{
		size_t readed = f->enc_cb.read_func(ptr, size, count, f->enc_cb.datasource);
		if (readed>0)
			f->enc_offset += readed*size;
		return readed;
	}

	while (bytes_read < bytes_to_read)
	{
		int bytes_to_copy;
		if (f->dec_bytes_left<=0)
		{
			if (f->enc_cb.read_func(f->dec_bytes, 16, 1, f->enc_cb.datasource)==1)
			{
				uint64_t nonce[2];
				int wasted = (f->enc_offset - f->enc_start) & 0xF;
				nonce[0] = f->nonce[0] + ((f->enc_offset - f->enc_start)>>4);
				nonce[1] = f->nonce[1] + ((f->enc_offset - f->enc_start)>>4);
				// decrypt it
				f->dec_bytes[0] ^= Encipher(nonce[0], f->key);
				f->dec_bytes[1] ^= Encipher(nonce[1], f->key);
				if (wasted)
					memmove(f->dec_bytes, (unsigned char*)f->dec_bytes+wasted, 16 - wasted);
				f->dec_bytes_left = 16 - wasted;
			}
			else
			{
				f->dec_bytes_left = f->enc_cb.read_func(f->dec_bytes, 1, 16, f->enc_cb.datasource);
				if (f->dec_bytes_left <= 0)
				{
					if (bytes_read==0)
						return f->dec_bytes_left;
					break;
				}
			}
		}
		bytes_to_copy = rawk_min(f->dec_bytes_left, bytes_to_read - bytes_read);
		memcpy((unsigned char*)ptr + bytes_read, f->dec_bytes, bytes_to_copy);
		f->enc_offset += bytes_to_copy;
		bytes_read += bytes_to_copy;
		f->dec_bytes_left -= bytes_to_copy;
		if (f->dec_bytes_left)
			memmove(f->dec_bytes, (unsigned char*)f->dec_bytes + bytes_to_copy, f->dec_bytes_left);
	}

	return (bytes_read / size);
}

int enc_seek(void *datasource, int64_t offset, int whence)
{
	rawkbink_dec_file *f = (rawkbink_dec_file*)datasource;
	int new_pos;
	int move_back;

	if (whence==SEEK_CUR)
	{
		whence = SEEK_SET;
		offset += f->enc_offset;
	}
	if (whence==SEEK_SET)
		offset += 0x38; // sizeof KIBE header


	f->dec_bytes_left = 0;

	new_pos = f->enc_cb.seek_func(f->enc_cb.datasource, offset, whence);
	if (new_pos)
		return new_pos;
	new_pos = f->enc_cb.tell_func(f->enc_cb.datasource) - 0x38;
	if (new_pos<0) // too far back
		return -1;
	if (f->enc_start && new_pos > f->enc_start)
	{
		move_back = ((new_pos - f->enc_start)&0xF);
		if (move_back)
		{
			f->enc_cb.seek_func(f->enc_cb.datasource, -move_back, SEEK_CUR);
		}
	}
	f->enc_offset = new_pos;
	return 0;
}

int enc_close(void *datasource)
{
	rawkbink_dec_file *f = (rawkbink_dec_file*)datasource;
	if (f->enc_cb.close_func)
		return f->enc_cb.close_func(f->enc_cb.datasource);
	return 0;
}

long enc_tell(void *datasource)
{
	rawkbink_dec_file *f = (rawkbink_dec_file*)datasource;
	return f->enc_offset;
}

static float get_float(rawk_bitstream *p)
{
	int power = bitread(p, 5);
	int x = bitread(p, 23);
	float f = ldexp((float)x, power-23);
	if (bitread(p, 1))
		f = -f;
	return f;
}

static void decode_block(rawkbink_dec_stream *s, short **out)
{
	int ch, k;
	int i, j;
	float q, quant[25];
	int width, coeff;

	if (s->dct)
		bitflush(&s->p, 2);

	for (ch=0; ch<=s->stereo; ch++)
	{
		q=0.0;
		s->coeffs[0] = get_float(&s->p);
		s->coeffs[1] = get_float(&s->p);

		for (i=0; i<s->num_bands; i++)
		{
			int value = bitread(&s->p, 8);
			quant[i] = pow(10.0, rawk_min(value, 95) * 0.066399999);
		}

		// find band (k)
		for (k=0; s->bands[k]*2 < 2; k++)
			q = quant[k];

		i=2;
		while (i<s->frame_len)
		{
			if (bitread(&s->p, 1))
				j = i + rle_length_tab[bitread(&s->p, 4)]*8;
			else
				j = i + 8;

			if (j>s->frame_len)
				j = s->frame_len;

			width = bitread(&s->p, 4);
			if (width==0)
			{
				memset(s->coeffs+i, 0, (j-i)*sizeof(double));
				i=j;
				while (s->bands[k]*2 < i)
					q = quant[k++];
			}
			else
			{
				while (i<j)
				{
					if (s->bands[k]*2 == i)
						q = quant[k++];
					coeff = bitread(&s->p, width);
					if (coeff)
					{
						if (bitread(&s->p, 1))
							s->coeffs[i] = -q*coeff;
						else
							s->coeffs[i] = q*coeff;
					}
					else
						s->coeffs[i] = 0.0;
					i++;
				}
			}
		}

		if (s->dct)
			ddct(s->frame_len, 1, s->coeffs, s->fft4g_ip, s->ddct_w);
		else
			rdft(s->frame_len, -1, s->coeffs, s->fft4g_ip, s->rdft_w);

		for (i=0; i<s->frame_len; i++)
			out[ch][i] = rawk_min(rawk_max(s->coeffs[i]*s->root, -32767), 32767);
	}

	// windowing
	if (!s->first)
	{
		int count = s->overlap_len<<s->stereo;
		if (!s->stereo)
		{
			for (i=0; i<count; i++)
				out[0][i] = (s->window[i]*(count-i) + out[0][i]*i) / count;
		}
		else
		{
			for (i=0; i < (count>>1); i++)
			{
				out[0][i] = (s->window[i]*(count-(i<<1)) + out[0][i]*(i<<1)) / count;
				out[1][i] = (s->window[i+s->overlap_len]*(count-(i<<1)-1) + out[1][i]*((i<<1)+1)) / count;
			}
		}
	}

	memcpy(s->window, out[0]+(s->frame_len-s->overlap_len), s->overlap_len*sizeof(short));
	if (s->stereo)
		memcpy(s->window+s->overlap_len, out[1]+(s->frame_len-s->overlap_len), s->overlap_len*sizeof(short));

	s->first=0;

	out[0] += (s->frame_len - s->overlap_len);
	if (s->stereo)
		out[1] += (s->frame_len - s->overlap_len);
}

size_t bink_decode_frame(rawkbink_dec_stream *s, const unsigned char *buf, size_t buf_size)
{
	unsigned short *samples[2];
	unsigned int decoded_samples = *(unsigned int*)buf>>1;
	samples[0] = s->output[0] = s->decoded;
	samples[1] = s->output[1] = s->decoded + (decoded_samples>>1) + s->overlap_len;

	if (s->max_decoded_size < (sizeof(short)*(decoded_samples+(s->overlap_len<<s->stereo))))
		return RAWKERROR_UNKNOWN;

	memset(samples[0], 0, (sizeof(short)*(decoded_samples+(s->overlap_len<<s->stereo))));
	s->output_samples = decoded_samples>>s->stereo;

	bitinit(&s->p, buf+4, buf_size-4);
	while (!biteof(&s->p))
	{
		decode_block(s, samples);
		bitalign32(&s->p);
	}

	return 0;
}

int bink_dec_close(rawkbink_dec_file *f)
{
	int i;

	if (f==NULL)
		return EOF;

	if (f->streams)
	{
		for (i=0; i<f->tracks; i++)
		{
			// close f->streams[i]
			free(f->streams[i].decoded);
			free(f->streams[i].bands);
		}
		free(f->streams);
	}
	free(f->index);
	free(f->frame_index);
	free(f->frame_data_buffer);

	if (f->cb.close_func)
		i = f->cb.close_func(f->cb.datasource);
	else
		i = 0;

	free(f);
	return i;
}

int RAWKAUDIO_API rawk_bink_dec_create_cb(rawk_callbacks *cb, int *channels, int *rate, int64_t *samples, bk_dec_stream *stream)
{
	int ret=0;
	rawkbink_dec_file *f = NULL;
	bink_header bk_head;
	int i, j;

	if (cb==NULL||cb->read_func==NULL||channels==NULL||rate==NULL||samples==NULL||stream==NULL)
		return RAWKERROR_INVALID_PARAM;
	*stream = NULL;
	*rate = 0;
	*samples = 0;

	f = (rawkbink_dec_file*)malloc(sizeof(rawkbink_dec_file));
	if (f==NULL)
	{
		ret = RAWKERROR_MEMORY;
		goto rawk_bink_dec_done;
	}
	memset(f, 0, sizeof(rawkbink_dec_file));
	memcpy(&f->cb, cb, sizeof(rawk_callbacks));

	if (f->cb.read_func(&bk_head, sizeof(bk_head), 1, f->cb.datasource)!=1)
	{
		ret = RAWKERROR_IO;
		goto rawk_bink_dec_done;
	}

	if (!memcmp(bk_head.BIK_sig, "KIB", 3) && bk_head.file_size_minus_eight==2)
	{
		unsigned int crypt_key[4];

		f->encrypted = 1;
		f->cb.seek_func(f->cb.datasource, 0x18, SEEK_SET);
		f->cb.read_func(f->nonce, 8, 2, f->cb.datasource);
		f->cb.read_func(crypt_key, 4, 4, f->cb.datasource);
		if (bk_head.rev == 'A')
		{
			f->encrypted = 2;
			memcpy(f->key, crypt_key, 16);
		}
		else if (bk_head.rev == 'E')
		{
			for (i=0; i < (sizeof(xtea_keys)/sizeof(xtea_keys[0])); i++)
			{
				if (!memcmp(crypt_key, xtea_keys[i].id, sizeof(crypt_key)))
				{
					f->encrypted = 2;
					memcpy(f->key, xtea_keys[i].key, 16);
					break;
				}
			}
		}
		if (f->encrypted == 2)
		{
			f->enc_cb = f->cb;
			f->cb.datasource = f;
			f->cb.close_func = enc_close;
			f->cb.read_func = enc_read;
			f->cb.seek_func = enc_seek;
			f->cb.tell_func = enc_tell;
		}
		f->cb.read_func(&bk_head, sizeof(bk_head), 1, f->cb.datasource);
	}

	// check signature and filesize (if possible)
	// should this allow revisions other than 'i'?
	if (((memcmp(bk_head.BIK_sig, "BIK", 3) || bk_head.rev != 'i') && memcmp(bk_head.BIK_sig, "RAWK", 4)) || \
	  (f->cb.seek_func && f->cb.tell_func && f->cb.seek_func(f->cb.datasource, 0, SEEK_END)==0 && ((bk_head.file_size_minus_eight+8) > f->cb.tell_func(f->cb.datasource))))
	{
		ret = RAWKERROR_NOTBINK;
		goto rawk_bink_dec_done;
	}

	f->frame_data_buffer = (unsigned char*)malloc(bk_head.largest_frame+4);
	if (f->frame_data_buffer==NULL)
	{
		ret = RAWKERROR_MEMORY;
		goto rawk_bink_dec_done;
	}

	f->frame_index = (size_t*)malloc(sizeof(size_t)*(bk_head.frames+1));
	if (f->frame_index==NULL)
	{
		ret = RAWKERROR_MEMORY;
		goto rawk_bink_dec_done;
	}
	f->frame_index[bk_head.frames] = bk_head.file_size_minus_eight+8;
	memset(f->frame_index, 0, sizeof(size_t)*bk_head.frames);
	f->frames = (f->encrypted==1) ? 0 : bk_head.frames;


	f->streams = (rawkbink_dec_stream*)malloc(sizeof(rawkbink_dec_stream)*bk_head.audio_tracks);
	f->index = (rawkbink_dec_stream**)malloc(sizeof(rawkbink_dec_stream*)*bk_head.audio_tracks);
	if (f->streams==NULL || f->index==NULL)
	{
		ret = RAWKERROR_MEMORY;
		goto rawk_bink_dec_done;
	}
	memset(f->streams, 0, sizeof(rawkbink_dec_stream)*bk_head.audio_tracks);
	memset(f->index, 0, sizeof(rawkbink_dec_stream*)*bk_head.audio_tracks);
	f->channels = f->tracks = bk_head.audio_tracks;

	if (f->cb.seek_func)
	{
		f->cb.seek_func(f->cb.datasource, sizeof(bink_header), SEEK_SET);
		if (f->encrypted==1)
			f->cb.seek_func(f->cb.datasource, 0x38, SEEK_CUR);
	}

	for (i=0; i<f->tracks; i++)
	{
		f->cb.read_func(&f->streams[i].max_decoded_size, sizeof(size_t), 1, f->cb.datasource);
		f->streams[i].track_no = -1;
	}

	for (i=0; i<f->tracks; i++)
	{
		unsigned int x;
		rawkbink_dec_stream *s = f->streams+i;
		int sample_rate_half;
		f->cb.read_func(&x, sizeof(x), 1, f->cb.datasource);
		s->rate = (unsigned short)x;
		s->dct = !!(x&BINK_FLAG_DCT);
		if (x&BINK_FLAG_STEREO)
		{
			s->stereo = 1;
			f->channels++;
		}
		if (*rate==0)
			*rate = s->rate;
		else if (*rate != s->rate)
		{
			ret = RAWKERROR_BINK_SAMPLING;
			goto rawk_bink_dec_done;
		}

		// determine frame length
		// this will be the same for all streams since we reject any files where streams don't
		// share the same rate (see directly above)
		if (s->rate<22050)
			s->frame_len = 1<<9;
		else if (s->rate<44100)
			s->frame_len = 1<<10;
		else
			s->frame_len = 1<<11;

		if (!s->dct)
		{
			s->frame_len <<= s->stereo;
			s->rate <<= s->stereo;
		}
		else
			s->fft4g_ip[0] = s->fft4g_ip[1] = 0;

		s->first = 1;
		s->output_samples = 0;

		s->overlap_len = s->frame_len / 16;
		sample_rate_half = (s->rate+1)/2;
		s->root = 2.0 / sqrt(s->frame_len);

		s->max_decoded_size += (s->overlap_len*sizeof(short))<<s->stereo;
		s->decoded = (short*)malloc(s->max_decoded_size);
		if (s->decoded==NULL)
		{
			ret = RAWKERROR_MEMORY;
			goto rawk_bink_dec_done;
		}

		// calculate number of bands
		for (s->num_bands=1; s->num_bands<25; s->num_bands++)
		{
			if (sample_rate_half <= critical_freqs[s->num_bands-1])
				break;
		}

		s->bands = (unsigned int*)malloc(sizeof(unsigned int)*(s->num_bands+1));
		if (s->bands==NULL)
		{
			ret = RAWKERROR_MEMORY;
			goto rawk_bink_dec_done;
		}
		s->bands[0] = 1;
		for (j=1; j<s->num_bands; j++)
		{
			s->bands[j] = critical_freqs[j-1] * (s->frame_len/2) / sample_rate_half;
		}
		s->bands[j] = s->frame_len/2;
	}

	for (i=0; i<f->tracks; i++)
	{
		// this value seems to be a track index of some description
		f->cb.read_func(&f->streams[i].track_no, sizeof(int), 1, f->cb.datasource);
		if (f->streams[i].track_no>f->tracks || f->streams[i].track_no<0)
		{
			f->streams[i].track_no = 0;
		}
		f->index[f->streams[i].track_no] = f->streams+i;
	}

	for (i=0; i<f->frames; i++)
	{
		int offset;
		f->cb.read_func(&offset, sizeof(offset), 1, f->cb.datasource);
		f->frame_index[i] = offset&(~1) + (f->encrypted==1 ? 0x38 : 0); // clear bit 0
	}

	if (f->encrypted==2)
		f->enc_start = f->frame_index[0];

	// get total number of samples
	for (i=0; i<f->frames; i++)
	{
		f->cb.seek_func(f->cb.datasource, f->frame_index[i], SEEK_SET);
		for (j=0; j<f->tracks; j++)
		{
			int frame_samples;
			int frame_length;
			f->cb.read_func(&frame_length, sizeof(frame_length), 1, f->cb.datasource);
			if (!frame_length)
				continue;
			f->cb.read_func(&frame_samples, sizeof(frame_samples), 1, f->cb.datasource);
			f->streams[j].samples += frame_samples>>1;
			f->cb.seek_func(f->cb.datasource, frame_length-4, SEEK_CUR);
		}
	}

	f->cb.seek_func(f->cb.datasource, f->frame_index[0], SEEK_SET);
	f->current_frame = 0;

	if (f->encrypted!=1)
	{
		f->samples = f->streams[0].samples>>f->streams[0].stereo;
		for (i=1; i<f->tracks; i++)
		{
			f->samples = rawk_min(f->samples, f->streams[i].samples>>f->streams[i].stereo);
		}

		*samples = f->samples;
	}
	else
	{
		f->samples = *samples = 0;
	}

	*channels = f->channels;
	*stream = (bk_dec_stream)f;
rawk_bink_dec_done:
	if (ret)
		bink_dec_close(f);
	return ret;
}

int RAWKAUDIO_API rawk_bink_dec_create(char *input_name, int *channels, int *rate, int64_t *samples, bk_dec_stream *stream)
{
	rawk_callbacks cb;

	if (input_name==NULL)
		return RAWKERROR_INVALID_PARAM;

	memset(&cb, 0, sizeof(cb));

	cb.datasource = fopen(input_name, "rb");
	if (cb.datasource==NULL)
		return RAWKERROR_IO;

	memcpy(&cb, &OV_CALLBACKS_DEFAULT, sizeof(OV_CALLBACKS_DEFAULT));

	return rawk_bink_dec_create_cb(&cb, channels, rate, samples, stream);
}

void RAWKAUDIO_API rawk_bink_dec_destroy(bk_dec_stream stream)
{
	rawkbink_dec_file *f = (rawkbink_dec_file*)stream;
	bink_dec_close(f);
}

int RAWKAUDIO_API rawk_bink_dec_decompress(bk_dec_stream stream, short **samples, int length)
{
	rawkbink_dec_file *f = (rawkbink_dec_file*)stream;
	int written=0;
	int i;

	if (f==NULL||length<0||(f->encrypted==1 && length))
		return RAWKERROR_INVALID_PARAM;

	// only the first stream is checked - assume the streams are synchronized at all times.
	while (length)
	{
		if (f->streams[0].output_samples)
		{
			int samples_to_output = rawk_min(f->streams[0].output_samples, length);
			rawkbink_dec_stream *s = f->index[0];
			for (i=0; i < f->channels; i++)
			{
				if (samples)
				{
					memcpy(samples[i]+written, s->output[0], sizeof(short)*samples_to_output);
					if (s->stereo)
						memcpy(samples[i+1]+written, s->output[1], sizeof(short)*samples_to_output);
				}
				s->output[0] += samples_to_output;
				if (s->stereo)
				{
					s->output[1] += samples_to_output;
					i++;
				}
				s->output_samples -= samples_to_output;
				if (s->track_no+1 < f->tracks)
					s = f->index[s->track_no+1];
			}
			written += samples_to_output;
			length -= samples_to_output;
		}

		if (f->streams[0].output_samples<=0 && length)
		{
			if (f->current_frame >= f->frames)
				return written ? written : RAWKERROR_IO;

			f->cb.seek_func(f->cb.datasource, f->frame_index[f->current_frame], SEEK_SET);

			for (i=0; i < f->tracks; i++)
			{
				size_t frame_length;
				f->cb.read_func(&frame_length, sizeof(frame_length), 1, f->cb.datasource);
				if (!frame_length)
					continue;
				f->cb.read_func(f->frame_data_buffer, frame_length, 1, f->cb.datasource);
				bink_decode_frame(f->streams+i, f->frame_data_buffer, frame_length);
				// check if we decoded anything, abort if we didn't
				if (f->streams[i].output_samples<=0)
					break;
			}

			f->current_frame++;
		}
	}

	return written;
}

void rawk_bink_dec_reset(bk_dec_stream stream)
{
	rawkbink_dec_file *f = (rawkbink_dec_file*)stream;
	int i;

	if (f==NULL)
		return;

	for (i=0; i<f->tracks; i++)
	{
		rawkbink_dec_stream *s = f->streams+i;
		s->output_samples = 0;
		s->first = 1;
	}
	f->current_frame = 0;
}

int RAWKAUDIO_API rawk_bink_dec_seek(bk_dec_stream stream, int64_t sample_pos)
{
	rawkbink_dec_file *f = (rawkbink_dec_file*)stream;
	int ret;

	if (f==NULL||sample_pos<0||sample_pos>f->samples)
		return RAWKERROR_INVALID_PARAM;

	rawk_bink_dec_reset(stream);

	ret = rawk_bink_dec_decompress(stream, NULL, sample_pos);

	return (ret<0) ? ret : 0;
}


void rdft(int n, int isgn, double *a, int *ip, double *w)
{
    void makewt(int nw, int *ip, double *w);
    void makect(int nc, int *ip, double *c);
    void bitrv2(int n, int *ip, double *a);
    void cftfsub(int n, double *a, double *w);
    void cftbsub(int n, double *a, double *w);
    void rftfsub(int n, double *a, int nc, double *c);
    void rftbsub(int n, double *a, int nc, double *c);
    int nw, nc;
    double xi;

    nw = ip[0];
    if (n > (nw << 2)) {
        nw = n >> 2;
        makewt(nw, ip, w);
    }
    nc = ip[1];
    if (n > (nc << 2)) {
        nc = n >> 2;
        makect(nc, ip, w + nw);
    }
    if (isgn >= 0) {
        if (n > 4) {
            bitrv2(n, ip + 2, a);
            cftfsub(n, a, w);
            rftfsub(n, a, nc, w + nw);
        } else if (n == 4) {
            cftfsub(n, a, w);
        }
        xi = a[0] - a[1];
        a[0] += a[1];
        a[1] = xi;
    } else {
        a[1] = 0.5 * (a[0] - a[1]);
        a[0] -= a[1];
        if (n > 4) {
            rftbsub(n, a, nc, w + nw);
            bitrv2(n, ip + 2, a);
            cftbsub(n, a, w);
        } else if (n == 4) {
            cftfsub(n, a, w);
        }
    }
}

void ddct(int n, int isgn, double *a, int *ip, double *w)
{
    void makewt(int nw, int *ip, double *w);
    void makect(int nc, int *ip, double *c);
    void bitrv2(int n, int *ip, double *a);
    void cftfsub(int n, double *a, double *w);
    void cftbsub(int n, double *a, double *w);
    void rftfsub(int n, double *a, int nc, double *c);
    void rftbsub(int n, double *a, int nc, double *c);
    void dctsub(int n, double *a, int nc, double *c);
    int j, nw, nc;
    double xr;

    nw = ip[0];
    if (n > (nw << 2)) {
        nw = n >> 2;
        makewt(nw, ip, w);
    }
    nc = ip[1];
    if (n > nc) {
        nc = n;
        makect(nc, ip, w + nw);
    }
    if (isgn < 0) {
        xr = a[n - 1];
        for (j = n - 2; j >= 2; j -= 2) {
            a[j + 1] = a[j] - a[j - 1];
            a[j] += a[j - 1];
        }
        a[1] = a[0] - xr;
        a[0] += xr;
        if (n > 4) {
            rftbsub(n, a, nc, w + nw);
            bitrv2(n, ip + 2, a);
            cftbsub(n, a, w);
        } else if (n == 4) {
            cftfsub(n, a, w);
        }
    }
    dctsub(n, a, nc, w + nw);
    if (isgn >= 0) {
        if (n > 4) {
            bitrv2(n, ip + 2, a);
            cftfsub(n, a, w);
            rftfsub(n, a, nc, w + nw);
        } else if (n == 4) {
            cftfsub(n, a, w);
        }
        xr = a[0] - a[1];
        a[0] += a[1];
        for (j = 2; j < n; j += 2) {
            a[j - 1] = a[j] - a[j + 1];
            a[j] += a[j + 1];
        }
        a[n - 1] = xr;
    }
}

void makewt(int nw, int *ip, double *w)
{
    void bitrv2(int n, int *ip, double *a);
    int j, nwh;
    double delta, x, y;

    ip[0] = nw;
    ip[1] = 1;
    if (nw > 2) {
        nwh = nw >> 1;
        delta = atan(1.0) / nwh;
        w[0] = 1;
        w[1] = 0;
        w[nwh] = cos(delta * nwh);
        w[nwh + 1] = w[nwh];
        if (nwh > 2) {
            for (j = 2; j < nwh; j += 2) {
                x = cos(delta * j);
                y = sin(delta * j);
                w[j] = x;
                w[j + 1] = y;
                w[nw - j] = y;
                w[nw - j + 1] = x;
            }
            bitrv2(nw, ip + 2, w);
        }
    }
}


void makect(int nc, int *ip, double *c)
{
    int j, nch;
    double delta;

    ip[1] = nc;
    if (nc > 1) {
        nch = nc >> 1;
        delta = atan(1.0) / nch;
        c[0] = cos(delta * nch);
        c[nch] = 0.5 * c[0];
        for (j = 1; j < nch; j++) {
            c[j] = 0.5 * cos(delta * j);
            c[nc - j] = 0.5 * sin(delta * j);
        }
    }
}

void bitrv2(int n, int *ip, double *a)
{
    int j, j1, k, k1, l, m, m2;
    double xr, xi, yr, yi;

    ip[0] = 0;
    l = n;
    m = 1;
    while ((m << 3) < l) {
        l >>= 1;
        for (j = 0; j < m; j++) {
            ip[m + j] = ip[j] + l;
        }
        m <<= 1;
    }
    m2 = 2 * m;
    if ((m << 3) == l) {
        for (k = 0; k < m; k++) {
            for (j = 0; j < k; j++) {
                j1 = 2 * j + ip[k];
                k1 = 2 * k + ip[j];
                xr = a[j1];
                xi = a[j1 + 1];
                yr = a[k1];
                yi = a[k1 + 1];
                a[j1] = yr;
                a[j1 + 1] = yi;
                a[k1] = xr;
                a[k1 + 1] = xi;
                j1 += m2;
                k1 += 2 * m2;
                xr = a[j1];
                xi = a[j1 + 1];
                yr = a[k1];
                yi = a[k1 + 1];
                a[j1] = yr;
                a[j1 + 1] = yi;
                a[k1] = xr;
                a[k1 + 1] = xi;
                j1 += m2;
                k1 -= m2;
                xr = a[j1];
                xi = a[j1 + 1];
                yr = a[k1];
                yi = a[k1 + 1];
                a[j1] = yr;
                a[j1 + 1] = yi;
                a[k1] = xr;
                a[k1 + 1] = xi;
                j1 += m2;
                k1 += 2 * m2;
                xr = a[j1];
                xi = a[j1 + 1];
                yr = a[k1];
                yi = a[k1 + 1];
                a[j1] = yr;
                a[j1 + 1] = yi;
                a[k1] = xr;
                a[k1 + 1] = xi;
            }
            j1 = 2 * k + m2 + ip[k];
            k1 = j1 + m2;
            xr = a[j1];
            xi = a[j1 + 1];
            yr = a[k1];
            yi = a[k1 + 1];
            a[j1] = yr;
            a[j1 + 1] = yi;
            a[k1] = xr;
            a[k1 + 1] = xi;
        }
    } else {
        for (k = 1; k < m; k++) {
            for (j = 0; j < k; j++) {
                j1 = 2 * j + ip[k];
                k1 = 2 * k + ip[j];
                xr = a[j1];
                xi = a[j1 + 1];
                yr = a[k1];
                yi = a[k1 + 1];
                a[j1] = yr;
                a[j1 + 1] = yi;
                a[k1] = xr;
                a[k1 + 1] = xi;
                j1 += m2;
                k1 += m2;
                xr = a[j1];
                xi = a[j1 + 1];
                yr = a[k1];
                yi = a[k1 + 1];
                a[j1] = yr;
                a[j1 + 1] = yi;
                a[k1] = xr;
                a[k1 + 1] = xi;
            }
        }
    }
}

void cftfsub(int n, double *a, double *w)
{
    void cft1st(int n, double *a, double *w);
    void cftmdl(int n, int l, double *a, double *w);
    int j, j1, j2, j3, l;
    double x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i;

    l = 2;
    if (n > 8) {
        cft1st(n, a, w);
        l = 8;
        while ((l << 2) < n) {
            cftmdl(n, l, a, w);
            l <<= 2;
        }
    }
    if ((l << 2) == n) {
        for (j = 0; j < l; j += 2) {
            j1 = j + l;
            j2 = j1 + l;
            j3 = j2 + l;
            x0r = a[j] + a[j1];
            x0i = a[j + 1] + a[j1 + 1];
            x1r = a[j] - a[j1];
            x1i = a[j + 1] - a[j1 + 1];
            x2r = a[j2] + a[j3];
            x2i = a[j2 + 1] + a[j3 + 1];
            x3r = a[j2] - a[j3];
            x3i = a[j2 + 1] - a[j3 + 1];
            a[j] = x0r + x2r;
            a[j + 1] = x0i + x2i;
            a[j2] = x0r - x2r;
            a[j2 + 1] = x0i - x2i;
            a[j1] = x1r - x3i;
            a[j1 + 1] = x1i + x3r;
            a[j3] = x1r + x3i;
            a[j3 + 1] = x1i - x3r;
        }
    } else {
        for (j = 0; j < l; j += 2) {
            j1 = j + l;
            x0r = a[j] - a[j1];
            x0i = a[j + 1] - a[j1 + 1];
            a[j] += a[j1];
            a[j + 1] += a[j1 + 1];
            a[j1] = x0r;
            a[j1 + 1] = x0i;
        }
    }
}

void cftbsub(int n, double *a, double *w)
{
    void cft1st(int n, double *a, double *w);
    void cftmdl(int n, int l, double *a, double *w);
    int j, j1, j2, j3, l;
    double x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i;

    l = 2;
    if (n > 8) {
        cft1st(n, a, w);
        l = 8;
        while ((l << 2) < n) {
            cftmdl(n, l, a, w);
            l <<= 2;
        }
    }
    if ((l << 2) == n) {
        for (j = 0; j < l; j += 2) {
            j1 = j + l;
            j2 = j1 + l;
            j3 = j2 + l;
            x0r = a[j] + a[j1];
            x0i = -a[j + 1] - a[j1 + 1];
            x1r = a[j] - a[j1];
            x1i = -a[j + 1] + a[j1 + 1];
            x2r = a[j2] + a[j3];
            x2i = a[j2 + 1] + a[j3 + 1];
            x3r = a[j2] - a[j3];
            x3i = a[j2 + 1] - a[j3 + 1];
            a[j] = x0r + x2r;
            a[j + 1] = x0i - x2i;
            a[j2] = x0r - x2r;
            a[j2 + 1] = x0i + x2i;
            a[j1] = x1r - x3i;
            a[j1 + 1] = x1i - x3r;
            a[j3] = x1r + x3i;
            a[j3 + 1] = x1i + x3r;
        }
    } else {
        for (j = 0; j < l; j += 2) {
            j1 = j + l;
            x0r = a[j] - a[j1];
            x0i = -a[j + 1] + a[j1 + 1];
            a[j] += a[j1];
            a[j + 1] = -a[j + 1] - a[j1 + 1];
            a[j1] = x0r;
            a[j1 + 1] = x0i;
        }
    }
}

void cft1st(int n, double *a, double *w)
{
    int j, k1, k2;
    double wk1r, wk1i, wk2r, wk2i, wk3r, wk3i;
    double x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i;

    x0r = a[0] + a[2];
    x0i = a[1] + a[3];
    x1r = a[0] - a[2];
    x1i = a[1] - a[3];
    x2r = a[4] + a[6];
    x2i = a[5] + a[7];
    x3r = a[4] - a[6];
    x3i = a[5] - a[7];
    a[0] = x0r + x2r;
    a[1] = x0i + x2i;
    a[4] = x0r - x2r;
    a[5] = x0i - x2i;
    a[2] = x1r - x3i;
    a[3] = x1i + x3r;
    a[6] = x1r + x3i;
    a[7] = x1i - x3r;
    wk1r = w[2];
    x0r = a[8] + a[10];
    x0i = a[9] + a[11];
    x1r = a[8] - a[10];
    x1i = a[9] - a[11];
    x2r = a[12] + a[14];
    x2i = a[13] + a[15];
    x3r = a[12] - a[14];
    x3i = a[13] - a[15];
    a[8] = x0r + x2r;
    a[9] = x0i + x2i;
    a[12] = x2i - x0i;
    a[13] = x0r - x2r;
    x0r = x1r - x3i;
    x0i = x1i + x3r;
    a[10] = wk1r * (x0r - x0i);
    a[11] = wk1r * (x0r + x0i);
    x0r = x3i + x1r;
    x0i = x3r - x1i;
    a[14] = wk1r * (x0i - x0r);
    a[15] = wk1r * (x0i + x0r);
    k1 = 0;
    for (j = 16; j < n; j += 16) {
        k1 += 2;
        k2 = 2 * k1;
        wk2r = w[k1];
        wk2i = w[k1 + 1];
        wk1r = w[k2];
        wk1i = w[k2 + 1];
        wk3r = wk1r - 2 * wk2i * wk1i;
        wk3i = 2 * wk2i * wk1r - wk1i;
        x0r = a[j] + a[j + 2];
        x0i = a[j + 1] + a[j + 3];
        x1r = a[j] - a[j + 2];
        x1i = a[j + 1] - a[j + 3];
        x2r = a[j + 4] + a[j + 6];
        x2i = a[j + 5] + a[j + 7];
        x3r = a[j + 4] - a[j + 6];
        x3i = a[j + 5] - a[j + 7];
        a[j] = x0r + x2r;
        a[j + 1] = x0i + x2i;
        x0r -= x2r;
        x0i -= x2i;
        a[j + 4] = wk2r * x0r - wk2i * x0i;
        a[j + 5] = wk2r * x0i + wk2i * x0r;
        x0r = x1r - x3i;
        x0i = x1i + x3r;
        a[j + 2] = wk1r * x0r - wk1i * x0i;
        a[j + 3] = wk1r * x0i + wk1i * x0r;
        x0r = x1r + x3i;
        x0i = x1i - x3r;
        a[j + 6] = wk3r * x0r - wk3i * x0i;
        a[j + 7] = wk3r * x0i + wk3i * x0r;
        wk1r = w[k2 + 2];
        wk1i = w[k2 + 3];
        wk3r = wk1r - 2 * wk2r * wk1i;
        wk3i = 2 * wk2r * wk1r - wk1i;
        x0r = a[j + 8] + a[j + 10];
        x0i = a[j + 9] + a[j + 11];
        x1r = a[j + 8] - a[j + 10];
        x1i = a[j + 9] - a[j + 11];
        x2r = a[j + 12] + a[j + 14];
        x2i = a[j + 13] + a[j + 15];
        x3r = a[j + 12] - a[j + 14];
        x3i = a[j + 13] - a[j + 15];
        a[j + 8] = x0r + x2r;
        a[j + 9] = x0i + x2i;
        x0r -= x2r;
        x0i -= x2i;
        a[j + 12] = -wk2i * x0r - wk2r * x0i;
        a[j + 13] = -wk2i * x0i + wk2r * x0r;
        x0r = x1r - x3i;
        x0i = x1i + x3r;
        a[j + 10] = wk1r * x0r - wk1i * x0i;
        a[j + 11] = wk1r * x0i + wk1i * x0r;
        x0r = x1r + x3i;
        x0i = x1i - x3r;
        a[j + 14] = wk3r * x0r - wk3i * x0i;
        a[j + 15] = wk3r * x0i + wk3i * x0r;
    }
}

void cftmdl(int n, int l, double *a, double *w)
{
    int j, j1, j2, j3, k, k1, k2, m, m2;
    double wk1r, wk1i, wk2r, wk2i, wk3r, wk3i;
    double x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i;

    m = l << 2;
    for (j = 0; j < l; j += 2) {
        j1 = j + l;
        j2 = j1 + l;
        j3 = j2 + l;
        x0r = a[j] + a[j1];
        x0i = a[j + 1] + a[j1 + 1];
        x1r = a[j] - a[j1];
        x1i = a[j + 1] - a[j1 + 1];
        x2r = a[j2] + a[j3];
        x2i = a[j2 + 1] + a[j3 + 1];
        x3r = a[j2] - a[j3];
        x3i = a[j2 + 1] - a[j3 + 1];
        a[j] = x0r + x2r;
        a[j + 1] = x0i + x2i;
        a[j2] = x0r - x2r;
        a[j2 + 1] = x0i - x2i;
        a[j1] = x1r - x3i;
        a[j1 + 1] = x1i + x3r;
        a[j3] = x1r + x3i;
        a[j3 + 1] = x1i - x3r;
    }
    wk1r = w[2];
    for (j = m; j < l + m; j += 2) {
        j1 = j + l;
        j2 = j1 + l;
        j3 = j2 + l;
        x0r = a[j] + a[j1];
        x0i = a[j + 1] + a[j1 + 1];
        x1r = a[j] - a[j1];
        x1i = a[j + 1] - a[j1 + 1];
        x2r = a[j2] + a[j3];
        x2i = a[j2 + 1] + a[j3 + 1];
        x3r = a[j2] - a[j3];
        x3i = a[j2 + 1] - a[j3 + 1];
        a[j] = x0r + x2r;
        a[j + 1] = x0i + x2i;
        a[j2] = x2i - x0i;
        a[j2 + 1] = x0r - x2r;
        x0r = x1r - x3i;
        x0i = x1i + x3r;
        a[j1] = wk1r * (x0r - x0i);
        a[j1 + 1] = wk1r * (x0r + x0i);
        x0r = x3i + x1r;
        x0i = x3r - x1i;
        a[j3] = wk1r * (x0i - x0r);
        a[j3 + 1] = wk1r * (x0i + x0r);
    }
    k1 = 0;
    m2 = 2 * m;
    for (k = m2; k < n; k += m2) {
        k1 += 2;
        k2 = 2 * k1;
        wk2r = w[k1];
        wk2i = w[k1 + 1];
        wk1r = w[k2];
        wk1i = w[k2 + 1];
        wk3r = wk1r - 2 * wk2i * wk1i;
        wk3i = 2 * wk2i * wk1r - wk1i;
        for (j = k; j < l + k; j += 2) {
            j1 = j + l;
            j2 = j1 + l;
            j3 = j2 + l;
            x0r = a[j] + a[j1];
            x0i = a[j + 1] + a[j1 + 1];
            x1r = a[j] - a[j1];
            x1i = a[j + 1] - a[j1 + 1];
            x2r = a[j2] + a[j3];
            x2i = a[j2 + 1] + a[j3 + 1];
            x3r = a[j2] - a[j3];
            x3i = a[j2 + 1] - a[j3 + 1];
            a[j] = x0r + x2r;
            a[j + 1] = x0i + x2i;
            x0r -= x2r;
            x0i -= x2i;
            a[j2] = wk2r * x0r - wk2i * x0i;
            a[j2 + 1] = wk2r * x0i + wk2i * x0r;
            x0r = x1r - x3i;
            x0i = x1i + x3r;
            a[j1] = wk1r * x0r - wk1i * x0i;
            a[j1 + 1] = wk1r * x0i + wk1i * x0r;
            x0r = x1r + x3i;
            x0i = x1i - x3r;
            a[j3] = wk3r * x0r - wk3i * x0i;
            a[j3 + 1] = wk3r * x0i + wk3i * x0r;
        }
        wk1r = w[k2 + 2];
        wk1i = w[k2 + 3];
        wk3r = wk1r - 2 * wk2r * wk1i;
        wk3i = 2 * wk2r * wk1r - wk1i;
        for (j = k + m; j < l + (k + m); j += 2) {
            j1 = j + l;
            j2 = j1 + l;
            j3 = j2 + l;
            x0r = a[j] + a[j1];
            x0i = a[j + 1] + a[j1 + 1];
            x1r = a[j] - a[j1];
            x1i = a[j + 1] - a[j1 + 1];
            x2r = a[j2] + a[j3];
            x2i = a[j2 + 1] + a[j3 + 1];
            x3r = a[j2] - a[j3];
            x3i = a[j2 + 1] - a[j3 + 1];
            a[j] = x0r + x2r;
            a[j + 1] = x0i + x2i;
            x0r -= x2r;
            x0i -= x2i;
            a[j2] = -wk2i * x0r - wk2r * x0i;
            a[j2 + 1] = -wk2i * x0i + wk2r * x0r;
            x0r = x1r - x3i;
            x0i = x1i + x3r;
            a[j1] = wk1r * x0r - wk1i * x0i;
            a[j1 + 1] = wk1r * x0i + wk1i * x0r;
            x0r = x1r + x3i;
            x0i = x1i - x3r;
            a[j3] = wk3r * x0r - wk3i * x0i;
            a[j3 + 1] = wk3r * x0i + wk3i * x0r;
        }
    }
}

void rftfsub(int n, double *a, int nc, double *c)
{
    int j, k, kk, ks, m;
    double wkr, wki, xr, xi, yr, yi;

    m = n >> 1;
    ks = 2 * nc / m;
    kk = 0;
    for (j = 2; j < m; j += 2) {
        k = n - j;
        kk += ks;
        wkr = 0.5 - c[nc - kk];
        wki = c[kk];
        xr = a[j] - a[k];
        xi = a[j + 1] + a[k + 1];
        yr = wkr * xr - wki * xi;
        yi = wkr * xi + wki * xr;
        a[j] -= yr;
        a[j + 1] -= yi;
        a[k] += yr;
        a[k + 1] -= yi;
    }
}

void rftbsub(int n, double *a, int nc, double *c)
{
    int j, k, kk, ks, m;
    double wkr, wki, xr, xi, yr, yi;

    a[1] = -a[1];
    m = n >> 1;
    ks = 2 * nc / m;
    kk = 0;
    for (j = 2; j < m; j += 2) {
        k = n - j;
        kk += ks;
        wkr = 0.5 - c[nc - kk];
        wki = c[kk];
        xr = a[j] - a[k];
        xi = a[j + 1] + a[k + 1];
        yr = wkr * xr + wki * xi;
        yi = wkr * xi - wki * xr;
        a[j] -= yr;
        a[j + 1] = yi - a[j + 1];
        a[k] += yr;
        a[k + 1] = yi - a[k + 1];
    }
    a[m + 1] = -a[m + 1];
}

void dctsub(int n, double *a, int nc, double *c)
{
    int j, k, kk, ks, m;
    double wkr, wki, xr;

    m = n >> 1;
    ks = nc / n;
    kk = 0;
    for (j = 1; j < m; j++) {
        k = n - j;
        kk += ks;
        wkr = c[kk] - c[nc - kk];
        wki = c[kk] + c[nc - kk];
        xr = wki * a[j] - wkr * a[k];
        a[j] = wkr * a[j] + wki * a[k];
        a[k] = xr;
    }
    a[m] *= c[0];
}

