#include "TCPTransfer.h"

#define TCP_TRANSFER_GET_RX_DATA_TO_MAX          (1000 * 3) // ms

static int32_t TCPTransferTxDef(TRANSFER_ST *pTrans, TRANSFER_BUF_ST *pBuf, int32_t totalLen)
{
    int32_t ret = 0;
    
    TCP_TRANSFER_ST *pTCPTrans = (TCP_TRANSFER_ST *)pTrans;
    TRANSFER_BUF_ST *p = NULL;

    (void) totalLen;
    MutexLock(pTrans->txMutex);
    
    for(p = pBuf; p->bLen > 0; p++)
    {
        if((int32_t)p->bLen == send(pTCPTrans->socket, p->pBuf, p->bLen, 0))
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
static int32_t TCPTransferReadRxDatDef(TRANSFER_ST *pTrans, void *pDat, int32_t len)
{
    TCP_TRANSFER_ST *pTCPTrans = (TCP_TRANSFER_ST *)pTrans;
    if(0 == QueueMultRead(pTrans->pRxQueue, pDat, len))
    {
        return len;
    }
    else if(pTCPTrans->clientRxTh)
    {
        do
        {
            if(RESULT_ERR_TIME_OUT == EventTimedWait(pTrans->rxEvent, TCP_TRANSFER_GET_RX_DATA_TO_MAX))
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



static void TCPTransferClrBufferDef(TRANSFER_ST *pTrans)
{
    (void) pTrans;
    QueueClear(pTrans->pRxQueue);
}


static void TCPTransferDeinitDef(TRANSFER_ST *pTrans)
{
    TCP_TRANSFER_ST *pTCPTrans = (TCP_TRANSFER_ST *)pTrans;

    if(pTCPTrans->clientRxTh)
    {
        pTCPTrans->exit = 1;
        ThreadJoin(pTCPTrans->clientRxTh);
    }

    TransferPrepareDeinit(pTrans);
    MutexDestroy(pTCPTrans->txMutex);

    TransferDeinitDef(pTrans);

}


static void *clientRxTh(void *pArg)
{
    TCP_TRANSFER_ST *pTCPTrans = (TCP_TRANSFER_ST *)pArg;

    char buf[1024] = {0};

    int32_t evts = 0, rxBytes = 0;

    struct epoll_event ev;

    int32_t monitorFd;
    //epoll
    monitorFd = epoll_create(1);
    if(0 > monitorFd)
    {
        LOGE("epoll create failed: %s\r\n", strerror(errno));
        goto err1;
    }

    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = pTCPTrans->socket;

    if(0 > epoll_ctl(monitorFd, EPOLL_CTL_ADD, pTCPTrans->socket, &ev))
    {
        LOGE("epoll ctl failed : %s\r\n", strerror(errno));
        goto err2;
    }

    while(!pTCPTrans->exit)
    {
        evts = epoll_wait(monitorFd, &ev, 1, 50);
        if(0 >= evts)
        {
            if(!pTCPTrans->exit)
            {
                continue;
            }
        }

        do
        {
            rxBytes = recv(pTCPTrans->socket, buf, sizeof(buf), 0);
            LOGD("rx = %d\r\n", rxBytes);
            if(0 < rxBytes)
            {
                TransferRx(&pTCPTrans->transfer, buf, rxBytes);
            }
            else
            {
                break;
            }
        }
        while (1);
    }

    epoll_ctl(monitorFd, EPOLL_CTL_DEL, pTCPTrans->socket, NULL);

err2:
    close(monitorFd);

err1:

    return NULL;
}



int32_t TCPTransferInit(TRANSFER_ST *pTrans, SOCKET_T socket, bool isClient)
{
    static TRANSFER_VTBL_ST transferVtbl = 
    {
        TCPTransferTxDef,
        TCPTransferReadRxDatDef,
        TCPTransferClrBufferDef,
        TCPTransferDeinitDef,
    }; 

    TCP_TRANSFER_ST *pTCPTrans = (TCP_TRANSFER_ST *)pTrans;

    if(TransferInit(pTrans, 10 * 1024, 1))
    {
        LOGE("Transfer init failed\r\n");
        goto err1;
    }

    pTCPTrans->transfer.vptr = &transferVtbl;

    pTCPTrans->socket = socket;
    pTCPTrans->txMutex = MutexCreate();
    if(NULL == pTCPTrans->txMutex)
    {
        LOGE("Create pUSBTransfer txMutex failed\r\n");
        goto err2;
    }

    if(isClient)
    {
        pTCPTrans->exit = 0;
        pTCPTrans->clientRxTh = ThreadCreate(clientRxTh, pTCPTrans, 1024 * 3, false, 10, "tcp Trans Rx");
        if(!pTCPTrans->clientRxTh)
        {
            goto err3;
        }
    }

    return 0;


err3:
    MutexDestroy(pTCPTrans->txMutex);

err2:
    TransferDeinitDef(pTrans);

err1:
    return -1;
}





