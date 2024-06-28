#ifndef __EMBEDDEDDEF_H__
#define __EMBEDDEDDEF_H__


#include "stdbool.h"
#include "stdint.h"

typedef int32_t SOCKET_T;


#define ST_OFFSET_OF(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

#define ST_CONTAINER_OF(ptr, type, member) \
((type*)((char *)(ptr) - ST_OFFSET_OF(type, member)))


#ifndef __WORDSIZE
#define __WORDSIZE (64)
#endif

#endif


#if defined(__unix__) || defined(__linux__)

#define ATTRIBUTE_WEAK __attribute__((weak))

#elif defined(_WIN32)  
#define ATTRIBUTE_WEAK 
#pragma warning(disable  : 4996)

#elif defined(__APPLE__)


#else
#define ATTRIBUTE_WEAK __attribute__((weak))

#endif
