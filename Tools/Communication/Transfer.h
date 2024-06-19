#ifndef __TRANSFER_H__
#define __TRANSFER_H__

#include "EmbeddedDef.h"
#include "Queue.h"
#include "Thread.h"
#include "Log.h"

#define GET_RX_DATA_TO_MAX          (500) // ms

struct __transferVtbl;

typedef struct __TransferAttr
{
    struct __transferVtbl *vptr;
    QUEUE_ST *pTxQueue;
    QUEUE_ST *pRxQueue;

    MUTEX_T txMutex;
    EVENT_T rxEvent;
    bool bInited;
}TRANSFER_ST;

typedef struct
{
    void *pBuf;
    uint32_t bLen;
}TRANSFER_BUF_ST;

typedef int32_t (*TRANSFER_TX_FT)(TRANSFER_ST *pTrans, TRANSFER_BUF_ST *pBuf, int32_t totalLen);
typedef int32_t (*TRANSFER_READRXDAT_FT)(TRANSFER_ST *pTrans, void *pBuf, int32_t len);
typedef void (*TRANSFER_OP_FT)(TRANSFER_ST *pTrans);


typedef struct __transferVtbl
{
    TRANSFER_TX_FT pTransferTx;
    TRANSFER_READRXDAT_FT pTransferReadRxDat;
    TRANSFER_OP_FT pTransferClrBuf;
    TRANSFER_OP_FT pTransferDeinit;
}TRANSFER_VTBL_ST;

#ifdef __cplusplus
extern "C"{
#endif


static inline int32_t TransferTx(TRANSFER_ST *pTrans, TRANSFER_BUF_ST *pBuf, int32_t totalLen)
{
    return pTrans->vptr->pTransferTx(pTrans, pBuf, totalLen);
}

static inline void TransferDeinit(TRANSFER_ST *pTrans)
{
    pTrans->vptr->pTransferDeinit(pTrans);
}

static inline void TransferRx(TRANSFER_ST *pTrans, void *pDat, int32_t len)
{
    if(0 == QueueMultPush(pTrans->pRxQueue, pDat, len))
    {
        EventGeneration(pTrans->rxEvent);
    }
}

static inline int32_t TransferReqTxDat(TRANSFER_ST *pTrans, void *pDat, int32_t len)
{
    int32_t leftLen = CalcQueueDataSz(pTrans->pTxQueue);
    int32_t reqLen = len > leftLen ? leftLen : len;
    int32_t ret = (0 == QueueMultPop(pTrans->pTxQueue, pDat, reqLen)) ? reqLen : -1;
    return ret;
}

// need release
static inline int32_t TransferReadRxDat(TRANSFER_ST *pTrans, void *pDat, int32_t len)
{
    return pTrans->vptr->pTransferReadRxDat(pTrans, pDat, len);
}

static inline void TransferReleaseRxDat(TRANSFER_ST *pTrans, int32_t len)
{
    QueueReleaseMultObj(pTrans->pRxQueue, len);
}

static inline void TransferClrBuf(TRANSFER_ST *pTrans)
{
    pTrans->vptr->pTransferClrBuf(pTrans);
}


int32_t TransferTxDef(TRANSFER_ST *pTrans, TRANSFER_BUF_ST *pBuf, int32_t totalLen);
int32_t TransferReadRxDatDef(TRANSFER_ST *pTrans, void *pDat, int32_t len);
void TransferDeinitDef(TRANSFER_ST *pTrans);
void TransferClrBufferDef(TRANSFER_ST *pTrans);

int32_t TransferInit(TRANSFER_ST *pTrans, int32_t rxBufSz, int32_t txBufSz);
void TransferPrepareDeinit(TRANSFER_ST *pTrans);

#ifdef __cplusplus
}
#endif

#endif


