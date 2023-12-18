#ifndef __TRANSFER_H__
#define __TRANSFER_H__

#include "TypeDef.h"
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
    BOOL_T bInited;
}TRANSFER_ST;

typedef struct
{
    VOID_T *pBuf;
    U32_T bLen;
}TRANSFER_BUF_ST;

typedef S32_T (*TRANSFER_TX_FT)(TRANSFER_ST *pTrans, TRANSFER_BUF_ST *pBuf, S32_T totalLen);
typedef S32_T (*TRANSFER_READRXDAT_FT)(TRANSFER_ST *pTrans, VOID_T *pBuf, S32_T len);
typedef VOID_T (*TRANSFER_OP_FT)(TRANSFER_ST *pTrans);


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


static inline S32_T TransferTx(TRANSFER_ST *pTrans, TRANSFER_BUF_ST *pBuf, S32_T totalLen)
{
    return pTrans->vptr->pTransferTx(pTrans, pBuf, totalLen);
}

static inline VOID_T TransferDeinit(TRANSFER_ST *pTrans)
{
    pTrans->vptr->pTransferDeinit(pTrans);
}

static inline VOID_T TransferRx(TRANSFER_ST *pTrans, VOID_T *pDat, S32_T len)
{
    if(0 == QueueMultPush(pTrans->pRxQueue, pDat, len))
    {
        EventGeneration(pTrans->rxEvent);
    }
}

static inline S32_T TransferReqTxDat(TRANSFER_ST *pTrans, VOID_T *pDat, S32_T len)
{
    S32_T leftLen = CalcQueueDataSz(pTrans->pTxQueue);
    S32_T reqLen = len > leftLen ? leftLen : len;
    S32_T ret = (0 == QueueMultPop(pTrans->pTxQueue, pDat, reqLen)) ? reqLen : -1;
    return ret;
}

// need release
static inline S32_T TransferReadRxDat(TRANSFER_ST *pTrans, VOID_T *pDat, S32_T len)
{
    return pTrans->vptr->pTransferReadRxDat(pTrans, pDat, len);
}

static inline VOID_T TransferReleaseRxDat(TRANSFER_ST *pTrans, S32_T len)
{
    QueueReleaseMultObj(pTrans->pRxQueue, len);
}

static inline VOID_T TransferClrBuf(TRANSFER_ST *pTrans)
{
    pTrans->vptr->pTransferClrBuf(pTrans);
}


S32_T TransferTxDef(TRANSFER_ST *pTrans, TRANSFER_BUF_ST *pBuf, S32_T totalLen);
S32_T TransferReadRxDatDef(TRANSFER_ST *pTrans, VOID_T *pDat, S32_T len);
VOID_T TransferDeinitDef(TRANSFER_ST *pTrans);
VOID_T TransferClrBufferDef(TRANSFER_ST *pTrans);

S32_T TransferInit(TRANSFER_ST *pTrans, S32_T rxBufSz, S32_T txBufSz);
VOID_T TransferPrepareDeinit(TRANSFER_ST *pTrans);

#ifdef __cplusplus
}
#endif

#endif


