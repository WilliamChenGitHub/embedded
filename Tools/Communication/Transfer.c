#include "Transfer.h"


void TransferDeinitDef(TRANSFER_ST *pTrans)
{
    MutexDestroy(pTrans->txMutex);
    SemDestroy(pTrans->rxEvent);

    QueueDestroy(pTrans->pTxQueue);
    QueueDestroy(pTrans->pRxQueue);
}


int32_t TransferTxDef(TRANSFER_ST *pTrans, TRANSFER_BUF_ST *pBuf, int32_t totalLen)
{
    int32_t ret = 0;
    TRANSFER_BUF_ST *p = NULL;

    (void) totalLen;
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
int32_t TransferReadRxDatDef(TRANSFER_ST *pTrans, void *pDat, int32_t len)
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

void TransferClrBufferDef(TRANSFER_ST *pTrans)
{
    QueueClear(pTrans->pRxQueue);

    MutexLock(pTrans->txMutex);
    QueueClear(pTrans->pTxQueue);
    MutexUnlock(pTrans->txMutex);
}

int32_t TransferInit(TRANSFER_ST *pTrans, int32_t rxBufSz, int32_t txBufSz)
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

    pTrans->bInited = true;
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


void TransferPrepareDeinit(TRANSFER_ST *pTrans)
{
    pTrans->bInited = false;
    EventGeneration(pTrans->rxEvent);
    ThreadUsleep(1000 * 3); // wait using transfer thread exit
}


