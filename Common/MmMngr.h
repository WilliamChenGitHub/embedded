#ifndef __MMMNGR_H__
#define __MMMNGR_H__

#include "TypeDef.h"

#ifdef __cplusplus
extern "C" 
{
#endif


/*when the callback is needed, the MmMngrMalloc could be implemented in the user file*/
VOID_T *MmMngrMalloc(U32_T sz);

/*when the callback is needed, the MmMngrFree could be implemented in the user file*/
VOID_T MmMngrFree(VOID_T *pMem);



#ifdef __cplusplus
}
#endif

#endif



