#ifndef __THREAD_H__
#define __THREAD_H__

#include "EmbeddedDef.h"
#include "stdio.h"

typedef void * SEM_T;

typedef void * MUTEX_T;

typedef void * THREAD_T;

typedef void * EVENT_T;

typedef void * (*THREAD_FUNC_FT)(void *pArg);

typedef enum
{
    RESULT_SUCCESS = 0,
    RESULT_ERR_TIME_OUT = -10,
    RESULT_ERR_OTHER = -11,
    RESULT_ERR_BUTT,
}RESULT_ET;

#ifdef __cplusplus
extern "C" 
{
#endif


EVENT_T EventCreate(void);

void EventDestroy(EVENT_T event);

void EventGeneration(EVENT_T event);

int32_t EventTimedWait(EVENT_T event, int32_t toMs);


SEM_T SemCreate(void);

void SemDestroy(SEM_T sem);

//if toMs is negative number indicats wait forever until signal arrived
int32_t SemTimedWait(SEM_T sem, int32_t toMs);


void SemPost(SEM_T sem);


MUTEX_T MutexCreate(void);

void MutexDestroy(MUTEX_T    mutex);

void MutexLock(MUTEX_T mutex);

void MutexUnlock(MUTEX_T mutex);

THREAD_T ThreadCreate(THREAD_FUNC_FT pFunc, void *pArg, uint32_t stackSz, bool isDetached, uint32_t priority, int8_t *name);

void ThreadJoin(THREAD_T thread);

void ThreadUsleep(uint32_t us);

void ThreadPostMsg(THREAD_T thread, uint32_t msg);



#ifdef __cplusplus
}
#endif

#endif



