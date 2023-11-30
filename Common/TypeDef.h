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

#define ST_OFFSET_OF(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

#define ST_CONTAINER_OF(ptr, type, member) \
({\
    typeof( ((type *)0)->member ) *__mptr = (ptr);\
    (type *)( (S8_T *)__mptr - ST_OFFSET_OF(type,member) );\
})

#ifndef __WORDSIZE
#define __WORDSIZE (64)
#endif

#endif


