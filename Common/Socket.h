#ifndef ___SOCKET_H__
#define ___SOCKET_H__

#include "TypeDef.h"

typedef enum
{
    SOCKET_DOMAIN_INET4,
    SOCKET_DOMAIN_INET6,
    SOCKET_DOMAIN_LOCAL,
}SOCKET_DOMAIN_ET;

typedef enum
{
    SOCKET_TYPE_STREAM,
    SOCKET_TYPE_DGRAM,
    SOCKET_TYPE_RAW,
}SOCKET_TYPE_ET;

typedef enum
{
    SOCKET_PROTOCAL_BUTT,
}SOCKET_PROTOCAL_ET;


//typedef int32_t SOCKET_T;


#ifdef __cplusplus
extern "C" 
{
#endif



#ifdef __cplusplus
}
#endif

#endif



