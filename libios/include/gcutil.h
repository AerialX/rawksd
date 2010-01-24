#pragma once

#ifndef ATTRIBUTE_ALIGN
#	define ATTRIBUTE_ALIGN(v)					__attribute__((aligned(v)))
#endif
#ifndef ATTRIBUTE_PACKED
#	define ATTRIBUTE_PACKED					__attribute__((packed))
#endif

#define ROUND_UP(a, b) ((((u32)(a)) + (b)-1)&~((b)-1))

/* Stack align */
#define STACK_ALIGN(type, name, cnt, alignment) \
	u8 _al__##name[(sizeof(type)*(cnt)) + (alignment)]; \
	type *name = (type*)ROUND_UP(_al__##name, alignment)

#define SWAP32(a) ((((u32)(a) >> 24) & 0x000000FF) | (((u32)(a) >> 8)  & 0x0000FF00)|\
                  (((u32)(a) << 8)  & 0x00FF0000) | (((u32)(a) << 24) & 0xFF000000))

#define SWAP16(a) ((u16)((((u32)(a) >> 8) & 0xFF) | (((u32)(a) << 8) & 0xFF00)))
