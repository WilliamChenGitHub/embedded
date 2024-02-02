#ifndef __QUEUE_H__
#define __QUEUE_H__

#include "TypeDef.h"
#include "MmMngr.h"
#include <string.h>

#define MAX_USED_STATISTICS 1
#define TOTAL_POP_PUSH_STATISTICS 1
#define OVERFLOW_FLAG_STATISTICS 1

typedef struct __Queue
{
    S32_T elemSz;
    S32_T wIdx;
    S32_T rIdx;
    S32_T nbOfElem;

#if MAX_USED_STATISTICS
    S32_T maxUsd;
#endif

#if OVERFLOW_FLAG_STATISTICS
    S32_T isOverflow;
    U64_T losted;
#endif

#if TOTAL_POP_PUSH_STATISTICS
    U64_T totalPushed;
    U64_T totalPoped;
#endif
    U8_T buf[0];
}QUEUE_ST;


#ifdef __cplusplus
extern "C" 
{
#endif

static inline S32_T CalcQueueFreeSz(QUEUE_ST *pQueue)
{
    return (pQueue->rIdx - pQueue->wIdx - 1 + pQueue->nbOfElem) % pQueue->nbOfElem;
}

static inline S32_T CalcQueueDataSz(QUEUE_ST * pQueue)
{
    return pQueue->nbOfElem - 1 - (pQueue->rIdx - pQueue->wIdx - 1 + pQueue->nbOfElem) % pQueue->nbOfElem;
}


// read an object from queue tail
// need release
static inline S32_T QueueReadSingleObj(QUEUE_ST * pQueue, VOID_T **pObj)
{
    if(1 > CalcQueueDataSz(pQueue))
    {
        return -1;
    }
    
    *pObj = pQueue->elemSz * pQueue->rIdx + pQueue->buf;
    return 0;
}

// release an object of the queue tail
static inline S32_T QueueReleaseSingleObj(QUEUE_ST * pQueue)
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
static inline S32_T QueuePopSingleObj(QUEUE_ST * pQueue, VOID_T *pObj)
{
    U8_T *pBuf = NULL;

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
static inline S32_T QueueMultPop(QUEUE_ST * pQueue, VOID_T *pObjs, S32_T popLen)
{
    U8_T *pBuf= NULL;
    S32_T temp = 0;

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

        pObjs = (U8_T *)pObjs + temp * pQueue->elemSz;
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
static inline S32_T QueueMultRead(QUEUE_ST * pQueue, VOID_T *pObjs, S32_T len)
{
    U8_T *pBuf = NULL;
    S32_T temp = 0;

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

        pObjs = (U8_T *)pObjs + temp * pQueue->elemSz;
        temp = len - temp;
        
        memcpy(pObjs,  pQueue->buf, temp * pQueue->elemSz);
    }

    return 0;
}

// release multiple objects of the queue tail
static inline VOID_T QueueReleaseMultObj(QUEUE_ST * pQueue, S32_T len)
{
    pQueue->rIdx = (pQueue->rIdx + len )% pQueue->nbOfElem;
    
#if TOTAL_POP_PUSH_STATISTICS
    pQueue->totalPoped += len;
#endif
}



// push an object to queue head
static inline S32_T QueuePushSingleObj(QUEUE_ST * pQueue,VOID_T *pObj)
{
    U8_T *pBuf = NULL;

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
    S32_T datLen = CalcQueueDataSz(pQueue);
    if(datLen > pQueue->maxUsd)
    {
        pQueue->maxUsd = datLen;
    }
#endif
     return 0;
}


// push multiple objects to the queue head
static inline S32_T QueueMultPush(QUEUE_ST * pQueue, VOID_T *pObjs, S32_T pushLen)
{
    U8_T *pBuf= NULL;
    S32_T temp = 0;

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

        pObjs = (U8_T *)pObjs + temp * pQueue->elemSz;
        temp = pushLen - temp;
        
        memcpy(pQueue->buf, pObjs, temp * pQueue->elemSz); // copy left
    }
    
    pQueue->wIdx = (pQueue->wIdx + pushLen )% pQueue->nbOfElem;

#if TOTAL_POP_PUSH_STATISTICS
    pQueue->totalPushed += pushLen;
#endif

#if MAX_USED_STATISTICS
    S32_T datLen = CalcQueueDataSz(pQueue);
    if(datLen > pQueue->maxUsd)
    {
        pQueue->maxUsd = datLen;
    }
#endif

    return 0;
}


static inline S32_T QueueGetMaxUsed(QUEUE_ST * pQueue)
{
#if MAX_USED_STATISTICS
    return pQueue->maxUsd;
#else
    return 0;
#endif
}


static inline U64_T QueueGetTotalPushed(QUEUE_ST * pQueue)
{
#if TOTAL_POP_PUSH_STATISTICS
    return pQueue->totalPushed;
#else
    return 0;
#endif
}

static inline U64_T QueueGetTotalPoped(QUEUE_ST * pQueue)
{
#if TOTAL_POP_PUSH_STATISTICS
    return pQueue->totalPoped;
#else
    return 0;
#endif
}

static inline BOOL_T QueueIsOverflow(QUEUE_ST * pQueue)
{
#if OVERFLOW_FLAG_STATISTICS
    return pQueue->isOverflow;
#else
    return FALSE;
#endif
}

static inline U64_T QueueGetLosted(QUEUE_ST * pQueue)
{
#if OVERFLOW_FLAG_STATISTICS
    return pQueue->losted;
#else
    return 0;
#endif
}

// malloc queue mem
QUEUE_ST * QueueCreate(S32_T elemSz,S32_T nbOfElem);

// free queue mem
S32_T QueueDestroy(QUEUE_ST *pQueue);


//do not malloc queue mem
QUEUE_ST * QueueInit(S32_T elemSz, S32_T nbOfElem, QUEUE_ST *pQueue);

//do not free queue mem
S32_T QueueDeinit(QUEUE_ST * pQueue);


S32_T QueueClear(QUEUE_ST * pQueue);


#ifdef __cplusplus
}
#endif

#endif


