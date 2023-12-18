#ifndef __TRANSFERFACTORY_H__
#define __TRANSFERFACTORY_H__

#include "Transfer.h"


struct __TransferFactoryVtbl;

typedef struct __TransferFactory
{
    struct __TransferFactoryVtbl *vPtr;
}TRANSFER_FACTORY_ST;


typedef TRANSFER_ST *(*TRANSFER_CREAT_FT)(TRANSFER_FACTORY_ST *pTransFactory);

typedef struct __TransferFactoryVtbl
{
    TRANSFER_CREAT_FT pTransferCreate;
}TRANSFER_FACTORY_VTBL_ST;


#ifdef __cplusplus
extern "C"{
#endif

static inline TRANSFER_ST *TransferCreate(TRANSFER_FACTORY_ST *pTransFactory)
{
    return pTransFactory->vPtr->pTransferCreate(pTransFactory);
}

static inline VOID_T TransferDestory(TRANSFER_ST *pTrans)
{
    TransferDeinit(pTrans);
    MmMngrFree(pTrans);
}

#ifdef __cplusplus
}
#endif

#endif

