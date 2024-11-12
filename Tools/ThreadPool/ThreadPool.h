#pragma once

#include "EmbeddedDef.h"
#include "Thread.h"
#include "List.h"

#define MAX_WAITING_MISSIONS   10240 // number of waiting missions max value
#define MAX_ACTIVE_THREADS    1024 // The number of threads that can run simultaneously


typedef void (*THREAD_POOL_MISSION)(void *pArg);

typedef struct __mission
{
    LIST_ST list;
    THREAD_POOL_MISSION pMission;
    void *pArg;
}MISSION_ATTR_ST;

typedef struct __threadPool
{
    LIST_ST missionList;
    MUTEX_T mutex;
    SEM_T sem;

    int32_t nbOfThreads;
    
    int32_t wMissions; // number of waiting missions
    int32_t rMissions; // number of running missions
    int32_t exit;

    THREAD_T *pTidArr;
}THREAD_POOL_ST;

#ifdef __cplusplus
extern "C"{
#endif

THREAD_POOL_ST *ThreadPoolCreate(int32_t nbOfThreads, int32_t stackSzOfTh);

int32_t ThreadPoolDispatchMission(THREAD_POOL_ST *pPool, THREAD_POOL_MISSION pMission, void *pArg);

void ThreadPoolDestory(THREAD_POOL_ST *pPool);

int ThreadPoolGetWaitingMissions(THREAD_POOL_ST *pPool);

int ThreadPoolGetRuningMissions(THREAD_POOL_ST *pPool);

#ifdef __cplusplus
}
#endif


