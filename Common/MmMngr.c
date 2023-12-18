#include "MmMngr.h"
#include "stdlib.h"

ATTRIBUTE_WEAK VOID_T *MmMngrMalloc(U32_T sz)
{
    /* NOTE: This function Should not be modified, when the callback is needed,
    the MmMngrMalloc could be implemented in the user file*/
    return malloc(sz);
}

ATTRIBUTE_WEAK VOID_T MmMngrFree(VOID_T *pMem)
{
    free(pMem);
}



