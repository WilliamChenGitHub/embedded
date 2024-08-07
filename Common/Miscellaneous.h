#ifndef __MISCELLANEOUS_H__
#define __MISCELLANEOUS_H__

#include "EmbeddedDef.h"

#ifdef __cplusplus
extern "C" 
{
#endif

uint64_t GetRealTickNs(void);

uint64_t GetMonotonicTickNs(void);

uint64_t GetBootTickNs(void);


uint16_t GetLibEmbeddedBCDVer(void); // BCD code


#ifdef __cplusplus
}
#endif

#endif




