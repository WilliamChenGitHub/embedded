#include "ThreadPool.h"
#include "MmMngr.h"
#include "Log.h"

static void *threadCb(void *pArg)
{
    THREAD_POOL_ST *pPool = (THREAD_POOL_ST *)pArg;

    while(1)
    {
        while((0 == pPool->wMissions) && (!pPool->exit))
        {
            SemTimedWait(pPool->sem, -1);
        }

        if((0 == pPool->wMissions) && pPool->exit)
        {
            break;// exit
        }

        MutexLock(pPool->mutex);
        if(LIST_IS_EMPTY(&pPool->missionList))
        {
            MutexUnlock(pPool->mutex);
            continue;
        }
        MISSION_ATTR_ST *pMission = LIST_NEXT_ENTRY(&pPool->missionList, MISSION_ATTR_ST, list);
        LIST_DELETE(&pMission->list);
        pPool->wMissions--;
        
        pPool->rMissions++;
        MutexUnlock(pPool->mutex);

        pMission->pMission(pMission->pArg);

        MutexLock(pPool->mutex);
        pPool->rMissions--;
        MutexUnlock(pPool->mutex);
        MmMngrFree(pMission);
    }

    return NULL;
}


THREAD_POOL_ST *ThreadPoolCreate(int32_t nbOfThreads, int32_t stackSzOfTh)
{
    int32_t i = 0;
    if((0 > nbOfThreads) || (MAX_ACTIVE_THREADS < nbOfThreads))
    {
        LOGE("nbOfThreads exceed avaliable range , %d\r\n", nbOfThreads);
        return NULL;
    }

    THREAD_POOL_ST *pPool = MmMngrMalloc(sizeof (*pPool));
    if(!pPool)
    {
        goto err1;
    }

    pPool->pTidArr = MmMngrMalloc(nbOfThreads * sizeof(*(pPool->pTidArr)));
    if(!pPool->pTidArr)
    {
        goto err2;
    }

    ListInit(&pPool->missionList);

    pPool->mutex = MutexCreate();

    if(!pPool->mutex)
    {
        goto err3;
    }

    pPool->sem = SemCreate();
    if(!pPool->sem)
    {
        goto err4;
    }

    pPool->nbOfThreads = nbOfThreads;
    pPool->wMissions = 0;
    pPool->rMissions = 0;
    pPool->exit = 0;

    for(i = 0; i < nbOfThreads; i++)
    {
        pPool->pTidArr[i] = ThreadCreate(threadCb, pPool, stackSzOfTh, false, 10, "tpTh");
        if(NULL == pPool->pTidArr[i])
        {
            LOGE("Thread create failed, %d\r\n", i);
            goto err5;
        }
    }

    return pPool;

err5:
    pPool->exit = 1;
    i--;
    for(int32_t j = i; j >= 0; j--)
    {
        SemPost(pPool->sem);
    }
    
    for(int32_t j = i; j >= 0; j--)
    {
        ThreadJoin(pPool->pTidArr[i]); // destory all thread
    }

    SemDestroy(pPool->sem);

err4:
    MutexDestroy(pPool->mutex);

err3:
    MmMngrFree(pPool->pTidArr);

err2:
    MmMngrFree(pPool);

err1:
    return NULL;
}

int32_t ThreadPoolDispatchMission(THREAD_POOL_ST *pPool, THREAD_POOL_MISSION pMission, void *pArg)
{
    if(MAX_WAITING_MISSIONS <= pPool->wMissions)
    {
        LOGE("too many waiting missions = %d\r\n", pPool->wMissions);
        goto err1;
    }

    MISSION_ATTR_ST *pMissionAttr = MmMngrMalloc(sizeof(*pMissionAttr));
    if(NULL == pMissionAttr)
    {
        goto err1;
    }

    pMissionAttr->pMission = pMission;
    pMissionAttr->pArg = pArg;

    MutexLock(pPool->mutex);
    LIST_INSERT_FRONT(&pPool->missionList, &pMissionAttr->list);
    pPool->wMissions++;
    MutexUnlock(pPool->mutex);

    SemPost(pPool->sem); // dispatch mission

    return 0;
    
err1:
    return -1;
}

void ThreadPoolDestory(THREAD_POOL_ST *pPool)
{
    pPool->exit = 1;

    for(int32_t i = 0; i < pPool->nbOfThreads; i++)
    {
        SemPost(pPool->sem);
    }
    for(int32_t i = 0; i < pPool->nbOfThreads; i++)
    {
        ThreadJoin(pPool->pTidArr[i]);
    }

#if 0
    // destory all mission
    MISSION_ATTR_ST *pMission = NULL, *pNext = NULL;
    MutexLock(pPool->mutex);
    LIST_FOREACH_FROM_HEAD_SAFE(pMission, &pPool->missionList, pNext, list)
    {
        LIST_DELETE(pMission);
        MmMngrFree(pMission);
    }
    MutexUnlock(pPool->mutex);
#endif

    MmMngrFree(pPool->pTidArr);
    pPool->nbOfThreads = 0;

    MutexDestroy(pPool->mutex);
    SemDestroy(pPool->sem);
    MmMngrFree(pPool);
}


int ThreadPoolGetWaitingMissions(THREAD_POOL_ST *pPool)
{
    return pPool->wMissions;
}

int ThreadPoolGetRuningMissions(THREAD_POOL_ST *pPool)
{
    return pPool->rMissions;
}

