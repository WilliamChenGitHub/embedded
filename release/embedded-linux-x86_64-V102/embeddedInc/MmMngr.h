#ifndef __MMMNGR_H__
#define __MMMNGR_H__

#include "EmbeddedDef.h"

#ifdef __cplusplus
extern "C" 
{
#endif


/*when the callback is needed, the MmMngrMalloc could be implemented in the user file*/
void *MmMngrMalloc(uint32_t sz);

/*when the callback is needed, the MmMngrFree could be implemented in the user file*/
void MmMngrFree(void *pMem);



#ifdef __cplusplus
}
#endif

#endif



