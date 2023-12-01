#pragma once

#include "TypeDef.h"
#include "Thread.h"
#include "List.h"

#define MAX_WAITING_MISSIONS   1024 // number of waiting missions max value
#define MAX_ACTIVE_THREADS    100 // The number of threads that can run simultaneously


typedef VOID_T (*THREAD_POOL_MISSION)(VOID_T *pArg);

typedef struct __mission
{
    LIST_ST list;
    THREAD_POOL_MISSION pMission;
    VOID_T *pArg;
}MISSION_ATTR_ST;

typedef struct __threadPool
{
    LIST_ST missionList;
    MUTEX_T mutex;
    SEM_T sem;

    S32_T nbOfThreads;
    
    S32_T wMissions; // number of waiting missions
    S32_T rMissions; // number of running missions
    S32_T exit;

    THREAD_T *pTidArr;
}THREAD_POOL_ST;

#ifdef __cplusplus
extern "C"{
#endif

THREAD_POOL_ST *ThreadPoolCreate(S32_T nbOfThreads, S32_T stackSzOfTh);

S32_T ThreadPoolDispatchMission(THREAD_POOL_ST *pPool, THREAD_POOL_MISSION pMission, VOID_T *pArg);

VOID_T ThreadPoolDestory(THREAD_POOL_ST *pPool);




#ifdef __cplusplus
}
#endif


