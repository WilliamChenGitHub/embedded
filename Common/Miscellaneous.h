#ifndef __MISCELLANEOUS_H__
#define __MISCELLANEOUS_H__

#include "TypeDef.h"

#ifdef __cplusplus
extern "C" 
{
#endif

U64_T GetRealTickNs(VOID_T);

U64_T GetMonotonicTickNs(VOID_T);

U16_T GetLibEmbeddedBCDVer(VOID_T); // BCD code


#ifdef __cplusplus
}
#endif

#endif




