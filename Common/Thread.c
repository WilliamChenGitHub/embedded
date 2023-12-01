#include "Thread.h"
#include "MmMngr.h"
#include "Log.h"

#if defined(__unix__) || defined(__linux__)
#include "pthread.h"
#include "unistd.h"
#include <semaphore.h>
#include <errno.h>

__attribute__((weak)) SEM_T SemCreate(VOID_T)
{
    SEM_T sem = NULL;
    sem = MmMngrMalloc(sizeof(sem_t));

    if(sem && sem_init(sem, 0, 0))
    {
        MmMngrFree(sem);
        return NULL;
    }

    return sem;
}

__attribute__((weak)) VOID_T SemDestroy(SEM_T sem)
{
    sem_destroy(sem);
    MmMngrFree(sem);
}


__attribute__((weak)) S32_T SemTimedWait(SEM_T sem, S32_T toMs)
{
    struct timespec ts;
    S32_T semWaitRet = 0;
    U64_T ns = 0;

    if(0 > toMs)
    {
        semWaitRet = sem_wait(sem);
    }
    else
    {
        if (0 > clock_gettime(CLOCK_REALTIME, &ts))
        {
            return RESULT_ERR_OTHER;
        }

        ns = toMs * 1000000;
        ts.tv_sec += (ts.tv_nsec + ns) / 1000000000;
        ts.tv_nsec = (ts.tv_nsec + ns) % 1000000000;

        semWaitRet = sem_timedwait(sem, &ts);
    }

    if(0 > semWaitRet)
    {
        if(ETIMEDOUT == errno)
        {
            return RESULT_ERR_TIME_OUT;
        }
        else
        {
            return semWaitRet;
        }
    }

    return 0;
}

__attribute__((weak)) VOID_T SemPost(SEM_T sem)
{
    sem_post(sem);
}


typedef struct
{
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    S32_T eventFlg;
}EVENT_ATTR_ST;

__attribute__((weak)) EVENT_T EventCreate(VOID_T)
{
    EVENT_ATTR_ST *pEventAttr = MmMngrMalloc(sizeof(*pEventAttr));

    if(!pEventAttr)
    {
        goto err1;
    }

    pEventAttr->eventFlg = 0;

    if(pthread_mutex_init(&pEventAttr->mutex, NULL))
    {
        goto err2;
    }
    
    if(pthread_cond_init(&pEventAttr->cond, NULL))
    {
        goto err3;
    }

    return (EVENT_T)pEventAttr;

err3:
    pthread_mutex_destroy(&pEventAttr->mutex);

err2:
    MmMngrFree(pEventAttr);

err1:

    return NULL;
}

__attribute__((weak)) VOID_T EventDestroy(EVENT_T event)
{
    EVENT_ATTR_ST *pEventAttr = (EVENT_ATTR_ST *)event;

    pthread_mutex_destroy(&pEventAttr->mutex);
    pthread_cond_destroy(&pEventAttr->cond);
    MmMngrFree(pEventAttr);
}

__attribute__((weak)) VOID_T EventGeneration(EVENT_T event)
{
    EVENT_ATTR_ST *pEventAttr = (EVENT_ATTR_ST *)event;

    pthread_mutex_lock(&pEventAttr->mutex);
    pEventAttr->eventFlg = 1;
    pthread_cond_signal(&pEventAttr->cond);
    pthread_mutex_unlock(&pEventAttr->mutex);
}

__attribute__((weak)) S32_T EventTimedWait(EVENT_T event, S32_T toMs)
{
    EVENT_ATTR_ST *pEventAttr = (EVENT_ATTR_ST *)event;
    struct timespec ts;
    S32_T ret = 0;
    U64_T ns = 0;

    if(0 > toMs)
    {
        pthread_mutex_lock(&pEventAttr->mutex);
        if(!pEventAttr->eventFlg)
        {
            ret = pthread_cond_wait(&pEventAttr->cond, &pEventAttr->mutex);
        }
        pEventAttr->eventFlg = 0;
        pthread_mutex_unlock(&pEventAttr->mutex);
    }
    else
    {
        if (0 > clock_gettime(CLOCK_REALTIME, &ts))
        {
            return RESULT_ERR_OTHER;
        }

        ns = toMs * 1000000;
        ts.tv_sec += (ts.tv_nsec + ns) / 1000000000;
        ts.tv_nsec = (ts.tv_nsec + ns) % 1000000000;

        pthread_mutex_lock(&pEventAttr->mutex);
        if(!pEventAttr->eventFlg)
        {
            ret = pthread_cond_timedwait(&pEventAttr->cond, &pEventAttr->mutex, &ts);
        }
        pEventAttr->eventFlg = 0;
        pthread_mutex_unlock(&pEventAttr->mutex);
    }

    if(ETIMEDOUT == ret)
    {
        return RESULT_ERR_TIME_OUT;
    }

    return ret;
}


__attribute__((weak)) MUTEX_T MutexCreate(VOID_T)
{
    MUTEX_T mutex = MmMngrMalloc(sizeof(pthread_mutex_t));
    if(mutex && pthread_mutex_init(mutex, NULL))
    {
        MmMngrFree(mutex);
        return NULL;
    }
    return mutex;
}


__attribute__((weak)) VOID_T MutexDestroy(MUTEX_T    mutex)
{
    pthread_mutex_destroy(mutex);
    MmMngrFree(mutex);
}

__attribute__((weak)) VOID_T MutexLock(MUTEX_T mutex)
{
    pthread_mutex_lock(mutex);
}

__attribute__((weak)) VOID_T MutexUnlock(MUTEX_T mutex)
{
    pthread_mutex_unlock(mutex);
}


typedef struct
{
    pthread_t threadId;
    THREAD_FUNC_FT pFunc;
    S8_T name[24];
    BOOL_T isDetached;
    VOID_T *pUsrData;
}THREAD_ATTR_LINUX_ST;


static VOID_T *ThreadFunc(VOID_T *argument)
{
    THREAD_ATTR_LINUX_ST *pThreadAttr = (THREAD_ATTR_LINUX_ST *)argument;
    pthread_setname_np(pthread_self(), pThreadAttr->name);

    pThreadAttr->pFunc(pThreadAttr->pUsrData);

    if(pThreadAttr->isDetached)
    {
        MmMngrFree(pThreadAttr);
    }
    pthread_exit(NULL);
    return NULL;
}

__attribute__((weak)) THREAD_T ThreadCreate(THREAD_FUNC_FT pFunc, VOID_T *pArg, U32_T stackSz, BOOL_T isDetached, U32_T priority, S8_T *name)
{
    THREAD_ATTR_LINUX_ST *pThreadAttr = MmMngrMalloc(sizeof(THREAD_ATTR_LINUX_ST));
    pthread_attr_t thAttr = {0};
    
    struct sched_param priParam;
    
    S32_T ret = 0;

    if(!pThreadAttr)
    {
        return NULL;
    }


    pthread_attr_init(&thAttr);

    priParam.sched_priority = priority;
    pthread_attr_setschedparam(&thAttr, &priParam);

    pThreadAttr->isDetached = isDetached;
    if(isDetached)
    {
        pthread_attr_setdetachstate(&thAttr, PTHREAD_CREATE_DETACHED);
    }
    else
    {
        pthread_attr_setdetachstate(&thAttr, PTHREAD_CREATE_JOINABLE);
    }
    
    pthread_attr_setstacksize(&thAttr, stackSz);

    if(name)
    {
        snprintf(pThreadAttr->name, sizeof pThreadAttr->name, "%s", name);
    }

    pThreadAttr->pUsrData = pArg;
    pThreadAttr->pFunc = pFunc;
    ret = pthread_create(&pThreadAttr->threadId, &thAttr, ThreadFunc, pThreadAttr);
    
    pthread_attr_destroy(&thAttr);

    if(ret)
    {
        MmMngrFree(pThreadAttr);
        return NULL;
    }

    return (THREAD_T)pThreadAttr;
}

__attribute__((weak)) VOID_T ThreadJoin(THREAD_T thread)
{
    THREAD_ATTR_LINUX_ST *pThreadAttr = (THREAD_ATTR_LINUX_ST *)thread;
    pthread_join(pThreadAttr->threadId, NULL);
    MmMngrFree(thread);
}

__attribute__((weak)) VOID_T ThreadUsleep(U32_T us)
{
    usleep(us);
}


#elif defined(_WIN32)  



#elif defined(__APPLE__)


#else
//rtos
#include "cmsis_os2.h"


typedef struct
{
    osMutexId_t mutex;
    S32_T sem;
}SEM_ATTR_T;

__attribute__((weak)) SEM_T SemCreate(VOID_T)
{

    return NULL;
}

__attribute__((weak)) VOID_T SemDestroy(SEM_T sem)
{
    (VOID_T) sem;
}

__attribute__((weak)) S32_T SemTimedWait(SEM_T sem, S32_T toMs)
{
    (VOID_T) sem;
    (VOID_T) toMs;
    return 0;
}

__attribute__((weak)) VOID_T SemPost(SEM_T sem)
{
    (VOID_T) sem;
}


#define EVT_DEFT 0x00000001

__attribute__((weak)) EVENT_T EventCreate(VOID_T)
{
    osEventFlagsId_t event = NULL;

    event = osEventFlagsNew(NULL);

    return (EVENT_T)event;
}

__attribute__((weak)) VOID_T EventDestroy(EVENT_T event)
{
    osEventFlagsDelete((osEventFlagsId_t *)event);
}

__attribute__((weak)) VOID_T EventGeneration(EVENT_T event)
{
    osEventFlagsSet((osEventFlagsId_t *)event, EVT_DEFT);
}

__attribute__((weak)) S32_T EventTimedWait(EVENT_T event, S32_T toMs)
{
    S32_T ret = 0;

    if(0 > toMs)
    {
        ret = osEventFlagsWait((osEventFlagsId_t *)event, EVT_DEFT, osFlagsWaitAny, osWaitForever);
    }
    else
    {
        ret = osEventFlagsWait((osEventFlagsId_t *)event, EVT_DEFT, osFlagsWaitAny, toMs);
    }
    
    if((0 < ret) && (EVT_DEFT & ret))
    {
        return 0;
    }
    else if(osFlagsErrorTimeout == (U32_T)ret)
    {
        return RESULT_ERR_TIME_OUT;
    }
    else
    {
        return RESULT_ERR_OTHER;
    }
}


__attribute__((weak)) MUTEX_T MutexCreate(VOID_T)
{
    osMutexId_t mutex = NULL;

    mutex = osMutexNew(NULL);

    return (MUTEX_T)mutex;
}


__attribute__((weak)) VOID_T MutexDestroy(MUTEX_T    mutex)
{
    osMutexDelete((osMutexId_t *)mutex);
}

__attribute__((weak)) VOID_T MutexLock(MUTEX_T mutex)
{
    osMutexAcquire((osMutexId_t *)mutex, osWaitForever);
}

__attribute__((weak)) VOID_T MutexUnlock(MUTEX_T mutex)
{
    osMutexRelease((osMutexId_t *)mutex);
}


typedef struct
{
    osThreadId_t threadId;
    THREAD_FUNC_FT pFunc;
    S8_T name[24];
    BOOL_T isDetached;
    VOID_T *pUsrData;
}THREAD_ATTR_RTOS_ST;

static VOID_T ThreadFunc(VOID_T *argument)
{
    THREAD_ATTR_RTOS_ST *pThreadAttr = (THREAD_ATTR_RTOS_ST *)argument;

    pThreadAttr->pFunc(pThreadAttr->pUsrData);

    if(pThreadAttr->isDetached)
    {
        MmMngrFree(pThreadAttr);
    }
    osThreadExit();
}

__attribute__((weak)) THREAD_T ThreadCreate(THREAD_FUNC_FT pFunc, VOID_T *pArg, U32_T stackSz, BOOL_T isDetached, U32_T priority, S8_T *name)
{
    THREAD_ATTR_RTOS_ST *pThread = MmMngrMalloc(sizeof(THREAD_ATTR_RTOS_ST));
    osThreadAttr_t threadAttr = {0};

    if(pThread)
    {
        threadAttr.name = name;
        pThread->isDetached = isDetached;
        if(isDetached)
        {
            threadAttr.attr_bits = osThreadDetached;
        }
        else
        {
            threadAttr.attr_bits = osThreadJoinable;
        }
        threadAttr.priority = osPriorityRealtime7 - priority;
        threadAttr.stack_size = stackSz;

        pThread->pUsrData = pArg;
        pThread->pFunc = pFunc;
        pThread->threadId = osThreadNew(ThreadFunc, pThread, &threadAttr);
        if(!pThread->threadId)
        {
            MmMngrFree(pThread);
            pThread = NULL;
        }
    }

    return (THREAD_T)pThread;
}

__attribute__((weak)) VOID_T ThreadJoin(THREAD_T thread)
{
    THREAD_ATTR_RTOS_ST *pThread = (THREAD_ATTR_RTOS_ST *) thread;

    osThreadJoin(pThread->threadId);
    MmMngrFree(thread);
}

__attribute__((weak)) VOID_T ThreadUsleep(U32_T us)
{
    osDelay((us + 999) / 1000);
}


#endif






