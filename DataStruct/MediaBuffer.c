#include "MediaBuffer.h"


int MediaBufPush(MEDIA_BUFFER_ST *pMediaBuf, void *pBuf, int sz)
{
    QUEUE_BUF_LIST_ST bufList[3] = {0};

    bufList[0].pBuf = &sz;
    bufList[0].sz = sizeof (sz);
    
    bufList[1].pBuf = pBuf;
    bufList[1].sz = sz;

    MutexLock(pMediaBuf->mutex);
    if(QueuePushBufList(&pMediaBuf->queue, bufList, sz + sizeof(sz)))
    {
        MutexUnlock(pMediaBuf->mutex);
        LOGE("push failed\r\n");
        return -1;
    }
    pMediaBuf->itemCnt++;
    MutexUnlock(pMediaBuf->mutex);
    return 0;
}


int MediaBufPushBufList(MEDIA_BUFFER_ST *pMediaBuf, QUEUE_BUF_LIST_ST *pBufList, int totalSz)
{
    int szHead = 0;

    szHead = totalSz;
    
    MutexLock(pMediaBuf->mutex);

    if(CalcQueueFreeSz(&pMediaBuf->queue) < (totalSz + sizeof(szHead)))
    {
        MutexUnlock(pMediaBuf->mutex);
        LOGE("not enough mem\r\n");
        return -1;
    }

    QueueMultPush(&pMediaBuf->queue, &szHead, sizeof szHead);
    
    QueuePushBufList(&pMediaBuf->queue, pBufList, totalSz);
    
    pMediaBuf->itemCnt++;
    MutexUnlock(pMediaBuf->mutex);
    return 0;
}



int MediaBufGetNextItemSz(MEDIA_BUFFER_ST *pMediaBuf)
{
    int sz = 0;

    if(QueueMultRead(&pMediaBuf->queue, &sz, sizeof sz))
    {
        LOGE("read buf item sz failed\r\n");
        return -1;
    }
    
    return sz;
}

int MediaBufPop(MEDIA_BUFFER_ST *pMediaBuf, void *pBuf)
{
    int sz = 0;
    MutexLock(pMediaBuf->mutex);
    if(QueueMultPop(&pMediaBuf->queue, &sz, sizeof sz))
    {
        MutexUnlock(pMediaBuf->mutex);
        LOGE("get buf item sz failed\r\n");
        return -1;
    }

    if(QueueMultPop(&pMediaBuf->queue, pBuf, sz))
    {
        MutexUnlock(pMediaBuf->mutex);
        LOGE("get data failed\r\n");
        return -1;
    }
    
    pMediaBuf->itemCnt--;
    MutexUnlock(pMediaBuf->mutex);
    return 0;
}


// malloc buffer mem
MEDIA_BUFFER_ST * MediaBufCreate(int bSz)
{
    MEDIA_BUFFER_ST *pMediaBuf = MmMngrMalloc(sizeof *pMediaBuf + bSz);
    if(!pMediaBuf)
    {
        LOGE("malloc media buffer failed\r\n");
        goto err1;
    }

    pMediaBuf->mutex = MutexCreate();
    if(!pMediaBuf->mutex)
    {
        LOGE("mutex create failed\r\n");
        goto err2;
    }

    pMediaBuf->itemCnt = 0;
    
    QueueInit(1, bSz, &pMediaBuf->queue);

    return pMediaBuf;
    
err2:
    MmMngrFree(pMediaBuf);
err1:
    return NULL;
}

// free queue mem
int32_t MediaBufDestroy(MEDIA_BUFFER_ST *pMediaBuf)
{
    QueueDeinit(&pMediaBuf->queue);
    MutexDestroy(pMediaBuf->mutex);
    MmMngrFree(pMediaBuf);
    
    return 0;
}

void MediaBufClear(MEDIA_BUFFER_ST * pMediaBuf)
{
    MutexLock(pMediaBuf->mutex);
    pMediaBuf->itemCnt = 0;
    QueueClear(&pMediaBuf->queue);
    MutexUnlock(pMediaBuf->mutex);
}


