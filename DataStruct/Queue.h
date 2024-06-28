#ifndef __QUEUE_H__
#define __QUEUE_H__

#include "EmbeddedDef.h"
#include "MmMngr.h"
#include <string.h>

#define MAX_USED_STATISTICS 1
#define TOTAL_POP_PUSH_STATISTICS 1
#define OVERFLOW_FLAG_STATISTICS 1

typedef struct __Queue
{
    int32_t elemSz;
    int32_t wIdx;
    int32_t rIdx;
    int32_t nbOfElem;

#if MAX_USED_STATISTICS
    int32_t maxUsd;
#endif

#if OVERFLOW_FLAG_STATISTICS
    int32_t isOverflow;
    uint64_t losted;
#endif

#if TOTAL_POP_PUSH_STATISTICS
    uint64_t totalPushed;
    uint64_t totalPoped;
#endif
    uint8_t buf[0];
}QUEUE_ST;

typedef struct
{
    void *pBuf;
    int sz;
}QUEUE_BUF_LIST_ST;

#ifdef __cplusplus
extern "C" 
{
#endif

static inline int32_t CalcQueueFreeSz(QUEUE_ST *pQueue)
{
    return (pQueue->rIdx - pQueue->wIdx - 1 + pQueue->nbOfElem) % pQueue->nbOfElem;
}

static inline int32_t CalcQueueDataSz(QUEUE_ST * pQueue)
{
    return pQueue->nbOfElem - 1 - (pQueue->rIdx - pQueue->wIdx - 1 + pQueue->nbOfElem) % pQueue->nbOfElem;
}


// read an object from queue tail
// need release
static inline int32_t QueueReadSingleObj(QUEUE_ST * pQueue, void **pObj)
{
    if(1 > CalcQueueDataSz(pQueue))
    {
        return -1;
    }
    
    *pObj = pQueue->elemSz * pQueue->rIdx + pQueue->buf;
    return 0;
}

// release an object of the queue tail
static inline int32_t QueueReleaseSingleObj(QUEUE_ST * pQueue)
{
    if(1 > CalcQueueDataSz(pQueue))
    {
        return -1;
    }
    
    pQueue->rIdx++;
    pQueue->rIdx = pQueue->rIdx % pQueue->nbOfElem;

#if TOTAL_POP_PUSH_STATISTICS
    pQueue->totalPoped++;
#endif

    return 0;
}


//pop an object from queue tail
//do not need release
static inline int32_t QueuePopSingleObj(QUEUE_ST * pQueue, void *pObj)
{
    uint8_t *pBuf = NULL;

    if(CalcQueueDataSz(pQueue) == 0)
    {
        return -1;
    }
    
    pBuf = pQueue->buf + pQueue->rIdx * pQueue->elemSz;
    memcpy(pObj, pBuf, pQueue->elemSz);

    pQueue->rIdx++;
    pQueue->rIdx = pQueue->rIdx % pQueue->nbOfElem;

#if TOTAL_POP_PUSH_STATISTICS
    pQueue->totalPoped++;
#endif

    return 0;
}

//pop multiple objects from queue tail
//do not need release
static inline int32_t QueueMultPop(QUEUE_ST * pQueue, void *pObjs, int32_t popLen)
{
    uint8_t *pBuf= NULL;
    int32_t temp = 0;

    if(CalcQueueDataSz(pQueue) < popLen)
    {
        return -1;
    }
    
    pBuf = pQueue->buf + pQueue->elemSz * pQueue->rIdx;
    
    temp = pQueue->nbOfElem - pQueue->rIdx;

    if(temp >= popLen) // It can be done once
    {
        memcpy(pObjs, pBuf, popLen * pQueue->elemSz);
    }
    else 
    {
        memcpy(pObjs, pBuf, temp * pQueue->elemSz);

        pObjs = (uint8_t *)pObjs + temp * pQueue->elemSz;
        temp = popLen - temp;
        
        memcpy(pObjs, pQueue->buf, temp * pQueue->elemSz);
    }

    pQueue->rIdx = (pQueue->rIdx + popLen )% pQueue->nbOfElem;

#if TOTAL_POP_PUSH_STATISTICS
    pQueue->totalPoped += popLen;
#endif

    return 0;
}


// read multiple objects from queue tail
// need release
static inline int32_t QueueMultRead(QUEUE_ST * pQueue, void *pObjs, int32_t len)
{
    uint8_t *pBuf = NULL;
    int32_t temp = 0;

    if(CalcQueueDataSz(pQueue) < len)
    {
        return -1;
    }
    
    pBuf = pQueue->buf + pQueue->elemSz * pQueue->rIdx;
    
    temp = pQueue->nbOfElem - pQueue->rIdx;

    if(temp >= len) // It can be done once
    {
        memcpy(pObjs, pBuf, len * pQueue->elemSz);
    }
    else 
    {
        memcpy(pObjs, pBuf, temp * pQueue->elemSz);

        pObjs = (uint8_t *)pObjs + temp * pQueue->elemSz;
        temp = len - temp;
        
        memcpy(pObjs,  pQueue->buf, temp * pQueue->elemSz);
    }

    return 0;
}

// release multiple objects of the queue tail
static inline void QueueReleaseMultObj(QUEUE_ST * pQueue, int32_t len)
{
    pQueue->rIdx = (pQueue->rIdx + len )% pQueue->nbOfElem;
    
#if TOTAL_POP_PUSH_STATISTICS
    pQueue->totalPoped += len;
#endif
}



// push an object to queue head
static inline int32_t QueuePushSingleObj(QUEUE_ST * pQueue,void *pObj)
{
    uint8_t *pBuf = NULL;

    if(CalcQueueFreeSz(pQueue) == 0)
    {

#if OVERFLOW_FLAG_STATISTICS
        pQueue->isOverflow = 1;
        pQueue->losted++;
#endif

        return -1;
    }

    pBuf = pQueue->buf + pQueue->wIdx * pQueue->elemSz;
    
    memcpy(pBuf, pObj, pQueue->elemSz);

    pQueue->wIdx++;
    pQueue->wIdx = pQueue->wIdx % pQueue->nbOfElem;


#if TOTAL_POP_PUSH_STATISTICS
    pQueue->totalPushed++;
#endif

#if MAX_USED_STATISTICS
    int32_t datLen = CalcQueueDataSz(pQueue);
    if(datLen > pQueue->maxUsd)
    {
        pQueue->maxUsd = datLen;
    }
#endif
     return 0;
}


// push multiple objects to the queue head
static inline int32_t QueueMultPush(QUEUE_ST * pQueue, void *pObjs, int32_t pushLen)
{
    uint8_t *pBuf= NULL;
    int32_t temp = 0;

    if(CalcQueueFreeSz(pQueue) < pushLen)
    {
#if OVERFLOW_FLAG_STATISTICS
        pQueue->isOverflow = 1;
        pQueue->losted += pushLen;
#endif
        return -1;
    }
    
    pBuf = pQueue->buf + pQueue->elemSz * pQueue->wIdx;
    
    temp = pQueue->nbOfElem - pQueue->wIdx;
    
    if(temp >= pushLen) // It can be done once
    {
        memcpy(pBuf, pObjs, pushLen * pQueue->elemSz);
    }
    else
    {
        memcpy(pBuf, pObjs, temp * pQueue->elemSz);

        pObjs = (uint8_t *)pObjs + temp * pQueue->elemSz;
        temp = pushLen - temp;
        
        memcpy(pQueue->buf, pObjs, temp * pQueue->elemSz); // copy left
    }
    
    pQueue->wIdx = (pQueue->wIdx + pushLen )% pQueue->nbOfElem;

#if TOTAL_POP_PUSH_STATISTICS
    pQueue->totalPushed += pushLen;
#endif

#if MAX_USED_STATISTICS
    int32_t datLen = CalcQueueDataSz(pQueue);
    if(datLen > pQueue->maxUsd)
    {
        pQueue->maxUsd = datLen;
    }
#endif

    return 0;
}


// push a buffer list , be used to push a not continuous memory space
static inline int32_t QueuePushBufList(QUEUE_ST * pQueue, QUEUE_BUF_LIST_ST *pObjs, int32_t totalLen)
{
    uint8_t *pBuf= NULL;
    int32_t temp = 0;

    if(CalcQueueFreeSz(pQueue) < totalLen)
    {
#if OVERFLOW_FLAG_STATISTICS
        pQueue->isOverflow = 1;
        pQueue->losted += totalLen;
#endif
        return -1;
    }

    for(QUEUE_BUF_LIST_ST *p = pObjs; p->sz; p++)
    {
        pBuf = pQueue->buf + pQueue->elemSz * pQueue->wIdx;
        
        temp = pQueue->nbOfElem - pQueue->wIdx;
        
        if(temp >= p->sz) // It can be done once
        {
            memcpy(pBuf, p->pBuf, p->sz * pQueue->elemSz);
        }
        else
        {
            memcpy(pBuf, p->pBuf, temp * pQueue->elemSz);

            pBuf = (uint8_t *)p->pBuf + temp * pQueue->elemSz;
            temp = p->sz - temp;
            
            memcpy(pQueue->buf, pBuf, temp * pQueue->elemSz); // copy left
        }
    }
    
    pQueue->wIdx = (pQueue->wIdx + totalLen )% pQueue->nbOfElem;

#if TOTAL_POP_PUSH_STATISTICS
    pQueue->totalPushed += totalLen;
#endif

#if MAX_USED_STATISTICS
    int32_t datLen = CalcQueueDataSz(pQueue);
    if(datLen > pQueue->maxUsd)
    {
        pQueue->maxUsd = datLen;
    }
#endif

    return 0;
}


static inline int32_t QueueGetMaxUsed(QUEUE_ST * pQueue)
{
#if MAX_USED_STATISTICS
    return pQueue->maxUsd;
#else
    return 0;
#endif
}


static inline uint64_t QueueGetTotalPushed(QUEUE_ST * pQueue)
{
#if TOTAL_POP_PUSH_STATISTICS
    return pQueue->totalPushed;
#else
    return 0;
#endif
}

static inline uint64_t QueueGetTotalPoped(QUEUE_ST * pQueue)
{
#if TOTAL_POP_PUSH_STATISTICS
    return pQueue->totalPoped;
#else
    return 0;
#endif
}

static inline bool QueueIsOverflow(QUEUE_ST * pQueue)
{
#if OVERFLOW_FLAG_STATISTICS
    return pQueue->isOverflow;
#else
    return false;
#endif
}

static inline uint64_t QueueGetLosted(QUEUE_ST * pQueue)
{
#if OVERFLOW_FLAG_STATISTICS
    return pQueue->losted;
#else
    return 0;
#endif
}

// malloc queue mem
QUEUE_ST * QueueCreate(int32_t elemSz,int32_t nbOfElem);

// free queue mem
int32_t QueueDestroy(QUEUE_ST *pQueue);


//do not malloc queue mem
QUEUE_ST * QueueInit(int32_t elemSz, int32_t nbOfElem, QUEUE_ST *pQueue);

//do not free queue mem
int32_t QueueDeinit(QUEUE_ST * pQueue);


int32_t QueueClear(QUEUE_ST * pQueue);


#ifdef __cplusplus
}
#endif

#endif


