#include "Transfer.h"


VOID_T TransferDeinitDef(TRANSFER_ST *pTrans)
{
    MutexDestroy(pTrans->txMutex);
    SemDestroy(pTrans->rxEvent);

    QueueDestroy(pTrans->pTxQueue);
    QueueDestroy(pTrans->pRxQueue);
}


S32_T TransferTxDef(TRANSFER_ST *pTrans, TRANSFER_BUF_ST *pBuf, S32_T totalLen)
{
    S32_T ret = 0;
    TRANSFER_BUF_ST *p = NULL;

    (VOID_T) totalLen;
    MutexLock(pTrans->txMutex);
    
    for(p = pBuf; p->bLen > 0; p++)
    {
        if(0 == QueueMultPush(pTrans->pTxQueue, p->pBuf, p->bLen))
        {
            ret += p->bLen;
        }
        else
        {
            ret = -1;
            break;
        }
    }
    
    MutexUnlock(pTrans->txMutex);

    return ret;
}


// need release
S32_T TransferReadRxDatDef(TRANSFER_ST *pTrans, VOID_T *pDat, S32_T len)
{
    if(0 == QueueMultRead(pTrans->pRxQueue, pDat, len))
    {
        return len;
    }
    else
    {
        do
        {
            if(RESULT_ERR_TIME_OUT == EventTimedWait(pTrans->rxEvent, GET_RX_DATA_TO_MAX))
            {
                break;
            }
            else
            {
                if(0 == QueueMultRead(pTrans->pRxQueue, pDat, len))
                {
                    return len;
                }
            }
        }
        while (pTrans->bInited);
    }

    return -1;
}

VOID_T TransferClrBufferDef(TRANSFER_ST *pTrans)
{
    QueueClear(pTrans->pRxQueue);

    MutexLock(pTrans->txMutex);
    QueueClear(pTrans->pTxQueue);
    MutexUnlock(pTrans->txMutex);
}

S32_T TransferInit(TRANSFER_ST *pTrans, S32_T rxBufSz, S32_T txBufSz)
{
    static TRANSFER_VTBL_ST transferVtbl = 
    {
        TransferTxDef,
        TransferReadRxDatDef,
        TransferClrBufferDef,
        TransferDeinitDef,
    }; 
    
    pTrans->vptr = &transferVtbl;

    if(0 < txBufSz)
    {
        pTrans->pTxQueue = QueueCreate(1, txBufSz);
        if(!pTrans->pTxQueue)
        {
            LOGE("Creat Transfer TxQueue failed\r\n");
            goto err1;
        }
    }

    if(0 < rxBufSz)
    {
        pTrans->pRxQueue = QueueCreate(1, rxBufSz);
        if(!pTrans->pRxQueue)
        {
            LOGE("Creat Transfer RxQueue failed\r\n");
            goto err2;
        }
    }

    pTrans->txMutex = MutexCreate();
    if(NULL == pTrans->txMutex)
    {
        LOGE("Creat Transfer txMutex failed\r\n");
        goto err3;
    }

    pTrans->rxEvent = EventCreate();
    if(NULL == pTrans->rxEvent)
    {
        LOGE("Create Transfer rxEvent failed\r\n");
        goto err4;
    }

    pTrans->bInited = TRUE;
    return 0;


err4:
    MutexDestroy(pTrans->txMutex);
    
err3:
    QueueDestroy(pTrans->pRxQueue);

err2:
    QueueDestroy(pTrans->pTxQueue);

err1:
    return -1;
}


VOID_T TransferPrepareDeinit(TRANSFER_ST *pTrans)
{
    pTrans->bInited = FALSE;
    EventGeneration(pTrans->rxEvent);
    ThreadUsleep(1000 * 3); // wait using transfer thread exit
}


