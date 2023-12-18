#ifndef __TYPEDEF_H__
#define __TYPEDEF_H__


#include "stdbool.h"


typedef unsigned char       U8_T;
typedef char                S8_T;

typedef unsigned short      U16_T;
typedef short               S16_T;

typedef unsigned int        U32_T;
typedef int                 S32_T;

typedef unsigned long long  U64_T;
typedef long long           S64_T;


typedef void                VOID_T;
typedef bool                BOOL_T;

#ifndef TRUE
#define TRUE                true
#endif

#ifndef FALSE
#define FALSE               false
#endif

#define ST_OFFSET_OF(TYPE, MEMBER) ((U64_T) &((TYPE *)0)->MEMBER)

#define ST_CONTAINER_OF(ptr, type, member) \
((type*)((U64_T)(ptr) - ST_OFFSET_OF(type, member)))


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
