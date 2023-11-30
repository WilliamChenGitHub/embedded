#include "Queue.h"
#include "MmMngr.h"

// malloc queue mem
QUEUE_ST * QueueCreate(S32_T elemSz, S32_T nbOfElem)
{
    if(elemSz == 0 || nbOfElem == 0 )
    {
        return NULL;
    }
    
    QUEUE_ST * pQueue = MmMngrMalloc(sizeof(*pQueue) + elemSz * nbOfElem);

    if(pQueue == NULL)
    {
        return NULL;
    }
    
    pQueue->elemSz = elemSz;
    pQueue->rIdx = pQueue->wIdx = 0;
    pQueue->nbOfElem = nbOfElem;

#if MAX_USED_STATISTICS
    pQueue->maxUsd = 0;
#endif

#if TOTAL_POP_PUSH_STATISTICS
    pQueue->totalPoped = 0;
    pQueue->totalPushed = 0;
#endif

#if OVERFLOW_FLAG_STATISTICS
    pQueue->isOverflow = 0;
    pQueue->losted = 0;
#endif

    return pQueue;
}


S32_T QueueClear(QUEUE_ST * pQueue)
{
    if(pQueue == NULL)
    {
        return -1;
    }
    pQueue->rIdx = pQueue->wIdx = 0;
    return 0;
}

//free queue mem
S32_T QueueDestroy(QUEUE_ST *pQueue)
{
    if(NULL == pQueue)
    {
        return -1;
    }

    pQueue->nbOfElem = pQueue->elemSz = pQueue->rIdx = pQueue->wIdx = 0;
    
    MmMngrFree(pQueue);
    return 0;
}


//do not malloc queue mem
QUEUE_ST * QueueInit(S32_T elemSz, S32_T nbOfElem, QUEUE_ST *pQueue)
{
    if(elemSz == 0 || nbOfElem == 0 )
    {
        return NULL;
    }
    
    if(pQueue == NULL)
    {
        return NULL;
    }
    
    pQueue->elemSz = elemSz;
    pQueue->rIdx = pQueue->wIdx = 0;
    pQueue->nbOfElem = nbOfElem;

#if MAX_USED_STATISTICS
    pQueue->maxUsd = 0;
#endif
    
#if TOTAL_POP_PUSH_STATISTICS
    pQueue->totalPoped = 0;
    pQueue->totalPushed = 0;
#endif
    
#if OVERFLOW_FLAG_STATISTICS
    pQueue->isOverflow = 0;
    pQueue->losted = 0;
#endif

    return pQueue;
}


//do not free queue mem
S32_T QueueDeinit(QUEUE_ST * pQueue)
{
    if(NULL == pQueue)
    {
        return -1;
    }

    pQueue->nbOfElem = pQueue->elemSz = pQueue->rIdx = pQueue->wIdx = 0;
    
    return 0;
}




