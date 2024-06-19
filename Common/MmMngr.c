#include "MmMngr.h"
#include "stdlib.h"

ATTRIBUTE_WEAK void *MmMngrMalloc(uint32_t sz)
{
    /* NOTE: This function Should not be modified, when the callback is needed,
    the MmMngrMalloc could be implemented in the user file*/
    return malloc(sz);
}

ATTRIBUTE_WEAK void MmMngrFree(void *pMem)
{
    free(pMem);
}



