#pragma once

#ifdef __cplusplus
   extern "C" {
#endif

typedef unsigned char u8;									///< 8bit unsigned integer
typedef unsigned short u16;								///< 16bit unsigned integer
typedef unsigned int u32;									///< 32bit unsigned integer
typedef unsigned long long u64;						///< 64bit unsigned integer
typedef signed char s8;										///< 8bit signed integer
typedef signed short s16;									///< 16bit signed integer
typedef signed int s32;										///< 32bit signed integer
typedef signed long long s64;							///< 64bit signed integer
typedef volatile unsigned char vu8;				///< 8bit unsigned volatile integer
typedef volatile unsigned short vu16;			///< 16bit unsigned volatile integer
typedef volatile unsigned int vu32;				///< 32bit unsigned volatile integer
typedef volatile unsigned long long vu64;	///< 64bit unsigned volatile integer
typedef volatile signed char vs8;					///< 8bit signed volatile integer
typedef volatile signed short vs16;				///< 16bit signed volatile integer
typedef volatile signed int vs32;					///< 32bit signed volatile integer
typedef volatile signed long long vs64;		///< 64bit signed volatile integer
typedef float f32;
typedef double f64;
typedef volatile float vf32;
typedef volatile double vf64;

// fixed point math typedefs
typedef s16 sfp16;                              ///< 1:7:8 fixed point
typedef s32 sfp32;                              ///< 1:19:8 fixed point
typedef u16 ufp16;                              ///< 8:8 fixed point
typedef u32 ufp32;                              ///< 24:8 fixed point

typedef s32 osqueue_t; // ipc message queue
typedef s32 ostimer_t; // ipc timer

// bool is a standard type in cplusplus, but not in c.
#ifndef __cplusplus
typedef u8 bool;
enum { false, true };
#endif

typedef unsigned int BOOL;
#define FIXED s32                                   ///< Alias type for sfp32
#ifndef boolean
	#define boolean  bool
#endif
#ifndef TRUE
	#define TRUE 1                                  ///< True
#endif
#ifndef FALSE
	#define FALSE 0                                 ///< False
#endif
#ifndef NULL
	#define NULL 0                                  ///< Pointer to 0
#endif
#ifndef null
	#define null 0                                  ///< Pointer to 0
#endif
	#ifndef LITTLE_ENDIAN
	#define LITTLE_ENDIAN  3412
#endif
#ifndef BIG_ENDIAN
	#define BIG_ENDIAN     1234
#endif
#ifndef BYTE_ORDER
	#define BYTE_ORDER     BIG_ENDIAN
#endif

#ifndef MAX
	#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
	#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifdef __cplusplus
	}
#endif

#include "gcutil.h"
