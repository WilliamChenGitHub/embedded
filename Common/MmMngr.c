#include "MmMngr.h"
#include "stdlib.h"

__attribute__((weak)) VOID_T *MmMngrMalloc(U32_T sz)
{
    /* NOTE: This function Should not be modified, when the callback is needed,
    the MmMngrMalloc could be implemented in the user file*/
    return malloc(sz);
}

__attribute__((weak)) VOID_T MmMngrFree(VOID_T *pMem)
{
    free(pMem);
}



