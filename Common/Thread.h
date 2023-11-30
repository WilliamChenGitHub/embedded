#ifndef __THREAD_H__
#define __THREAD_H__

#include "TypeDef.h"
#include "stdio.h"

typedef VOID_T * SEM_T;

typedef VOID_T * MUTEX_T;

typedef VOID_T * THREAD_T;

typedef VOID_T * EVENT_T;

typedef VOID_T * (*THREAD_FUNC_FT)(VOID_T *pArg);

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


EVENT_T EventCreate(VOID_T);

VOID_T EventDestroy(EVENT_T event);

VOID_T EventGeneration(EVENT_T event);

S32_T EventTimedWait(EVENT_T event, S32_T toMs);


SEM_T SemCreate(VOID_T);

VOID_T SemDestroy(SEM_T sem);

//if toMs is negative number indicats wait forever until signal arrived
S32_T SemTimedWait(SEM_T sem, S32_T toMs);


VOID_T SemPost(SEM_T sem);


MUTEX_T MutexCreate(VOID_T);

VOID_T MutexDestroy(MUTEX_T    mutex);

VOID_T MutexLock(MUTEX_T mutex);

VOID_T MutexUnlock(MUTEX_T mutex);

THREAD_T ThreadCreate(THREAD_FUNC_FT pFunc, VOID_T *pArg, U32_T stackSz, BOOL_T isDetached, U32_T priority, S8_T *name);

VOID_T ThreadJoin(THREAD_T thread);

VOID_T ThreadUsleep(U32_T us);



#ifdef __cplusplus
}
#endif

#endif



