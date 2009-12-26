#pragma once

#ifndef ATTRIBUTE_ALIGN
#	define ATTRIBUTE_ALIGN(v)					__attribute__((aligned(v)))
#endif
#ifndef ATTRIBUTE_PACKED
#	define ATTRIBUTE_PACKED					__attribute__((packed))
#endif

/* Stack align */
#define STACK_ALIGN(type, name, cnt, alignment)	\
	u8 _al__##name[((sizeof(type)*(cnt)) + (alignment) + (((sizeof(type)*(cnt))%(alignment)) > 0 ? ((alignment) - ((sizeof(type)*(cnt))%(alignment))) : 0))]; \
	type *name = (type*)(((u32)(_al__##name)) + ((alignment) - (((u32)(_al__##name))&((alignment)-1))))
