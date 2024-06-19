#include "Thread.h"
#include "MmMngr.h"
#include "Log.h"

#if defined(__unix__) || defined(__linux__)
#include "pthread.h"
#include "unistd.h"
#include <semaphore.h>
#include <errno.h>

ATTRIBUTE_WEAK SEM_T SemCreate(void)
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

ATTRIBUTE_WEAK void SemDestroy(SEM_T sem)
{
    sem_destroy(sem);
    MmMngrFree(sem);
}


ATTRIBUTE_WEAK int32_t SemTimedWait(SEM_T sem, int32_t toMs)
{
    struct timespec ts;
    int32_t semWaitRet = 0;
    uint64_t ns = 0;

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

ATTRIBUTE_WEAK void SemPost(SEM_T sem)
{
    sem_post(sem);
}


typedef struct
{
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int32_t eventFlg;
}EVENT_ATTR_ST;

ATTRIBUTE_WEAK EVENT_T EventCreate(void)
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

ATTRIBUTE_WEAK void EventDestroy(EVENT_T event)
{
    EVENT_ATTR_ST *pEventAttr = (EVENT_ATTR_ST *)event;

    pthread_mutex_destroy(&pEventAttr->mutex);
    pthread_cond_destroy(&pEventAttr->cond);
    MmMngrFree(pEventAttr);
}

ATTRIBUTE_WEAK void EventGeneration(EVENT_T event)
{
    EVENT_ATTR_ST *pEventAttr = (EVENT_ATTR_ST *)event;

    pthread_mutex_lock(&pEventAttr->mutex);
    pEventAttr->eventFlg = 1;
    pthread_cond_signal(&pEventAttr->cond);
    pthread_mutex_unlock(&pEventAttr->mutex);
}

ATTRIBUTE_WEAK int32_t EventTimedWait(EVENT_T event, int32_t toMs)
{
    EVENT_ATTR_ST *pEventAttr = (EVENT_ATTR_ST *)event;
    struct timespec ts;
    int32_t ret = 0;
    uint64_t ns = 0;

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


ATTRIBUTE_WEAK MUTEX_T MutexCreate(void)
{
    MUTEX_T mutex = MmMngrMalloc(sizeof(pthread_mutex_t));
    if(mutex && pthread_mutex_init(mutex, NULL))
    {
        MmMngrFree(mutex);
        return NULL;
    }
    return mutex;
}


ATTRIBUTE_WEAK void MutexDestroy(MUTEX_T    mutex)
{
    pthread_mutex_destroy(mutex);
    MmMngrFree(mutex);
}

ATTRIBUTE_WEAK void MutexLock(MUTEX_T mutex)
{
    pthread_mutex_lock(mutex);
}

ATTRIBUTE_WEAK void MutexUnlock(MUTEX_T mutex)
{
    pthread_mutex_unlock(mutex);
}


typedef struct
{
    pthread_t threadId;
    THREAD_FUNC_FT pFunc;
    int8_t name[24];
    bool isDetached;
    void *pUsrData;
}THREAD_ATTR_LINUX_ST;


static void *ThreadFunc(void *argument)
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

ATTRIBUTE_WEAK THREAD_T ThreadCreate(THREAD_FUNC_FT pFunc, void *pArg, uint32_t stackSz, bool isDetached, uint32_t priority, int8_t *name)
{
    THREAD_ATTR_LINUX_ST *pThreadAttr = MmMngrMalloc(sizeof(THREAD_ATTR_LINUX_ST));
    pthread_attr_t thAttr = {0};
    
    struct sched_param priParam;
    
    int32_t ret = 0;

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

ATTRIBUTE_WEAK void ThreadJoin(THREAD_T thread)
{
    THREAD_ATTR_LINUX_ST *pThreadAttr = (THREAD_ATTR_LINUX_ST *)thread;
    pthread_join(pThreadAttr->threadId, NULL);
    MmMngrFree(thread);
}

ATTRIBUTE_WEAK void ThreadUsleep(uint32_t us)
{
    usleep(us);
}


#elif defined(_WIN32)  
#include <Windows.h>

ATTRIBUTE_WEAK SEM_T SemCreate(void)
{
    return CreateSemaphore(NULL, 0, 0x7FFFFFFF, NULL);
}

ATTRIBUTE_WEAK void SemDestroy(SEM_T sem)
{
    CloseHandle(sem);
}


ATTRIBUTE_WEAK int32_t SemTimedWait(SEM_T sem, int32_t toMs)
{
    int32_t semWaitRet = 0;

    if (0 > toMs)
    {
        semWaitRet = WaitForSingleObject(sem, INFINITE);
    }
    else
    {
        semWaitRet = WaitForSingleObject(sem, toMs);
    }


    if (WAIT_TIMEOUT == semWaitRet)
    {
        return RESULT_ERR_TIME_OUT;
    }
    else if (WAIT_OBJECT_0 == semWaitRet)
    {
        return 0;
    }
    else
    {
        return semWaitRet;
    }
}

ATTRIBUTE_WEAK void SemPost(SEM_T sem)
{
    ReleaseSemaphore(sem, 1, NULL);
}


ATTRIBUTE_WEAK EVENT_T EventCreate(void)
{
    HANDLE event;
    event = CreateEvent(NULL, false, false, NULL);
    //ResetEvent(&event);
    return (EVENT_T)event;
}

ATTRIBUTE_WEAK void EventDestroy(EVENT_T event)
{
    CloseHandle(event);
}

ATTRIBUTE_WEAK void EventGeneration(EVENT_T event)
{
    SetEvent(event);
}

ATTRIBUTE_WEAK int32_t EventTimedWait(EVENT_T event, int32_t toMs)
{
    int32_t waitRet = 0;

    if (0 > toMs)
    {
        waitRet = WaitForSingleObject(event, INFINITE);
    }
    else
    {
        waitRet = WaitForSingleObject(event, toMs);
    }

    if (WAIT_TIMEOUT == waitRet)
    {
        return RESULT_ERR_TIME_OUT;
    }
    else if (WAIT_OBJECT_0 == waitRet)
    {
        //ResetEvent(event);
        return 0;
    }
    else
    {
        return waitRet;
    }
}


ATTRIBUTE_WEAK MUTEX_T MutexCreate(void)
{
    MUTEX_T mutex = MmMngrMalloc(sizeof(CRITICAL_SECTION));

    if(mutex)
    {
        InitializeCriticalSection(mutex);
    }

    return mutex;
}


ATTRIBUTE_WEAK void MutexDestroy(MUTEX_T    mutex)
{
    DeleteCriticalSection(mutex);
}

ATTRIBUTE_WEAK void MutexLock(MUTEX_T mutex)
{
    EnterCriticalSection(mutex);
}

ATTRIBUTE_WEAK void MutexUnlock(MUTEX_T mutex)
{
    LeaveCriticalSection(mutex);
}


typedef struct
{
    HANDLE hThread;
    DWORD iThread;
    THREAD_FUNC_FT pFunc;
    int8_t name[24];
    bool isDetached;
    void* pUsrData;
}THREAD_ATTR_WINDOWS_ST;


static DWORD WINAPI ThreadFunc(LPVOID argument)
{
    THREAD_ATTR_WINDOWS_ST* pThreadAttr = (THREAD_ATTR_WINDOWS_ST*)argument;

    pThreadAttr->pFunc(pThreadAttr->pUsrData);

    if(pThreadAttr->isDetached)
    {
        MmMngrFree(pThreadAttr);
    }
    
    ExitThread(0);
}

ATTRIBUTE_WEAK THREAD_T ThreadCreate(THREAD_FUNC_FT pFunc, void* pArg, uint32_t stackSz, bool isDetached, uint32_t priority, int8_t* name)
{
    THREAD_ATTR_WINDOWS_ST* pThreadAttr = NULL;

    (void) priority;

    pThreadAttr = MmMngrMalloc(sizeof(THREAD_ATTR_WINDOWS_ST));

    if (!pThreadAttr)
    {
        goto err1;
    }
    
    if (name)
    {
        snprintf(pThreadAttr->name, sizeof pThreadAttr->name, "%s", name);
    }

    pThreadAttr->pUsrData = pArg;
    pThreadAttr->pFunc = pFunc;
    pThreadAttr->isDetached = isDetached;

    pThreadAttr->hThread = CreateThread(NULL, stackSz, ThreadFunc, pThreadAttr, 0, &pThreadAttr->iThread);

    if (!pThreadAttr->hThread)
    {
        goto err2;
    }

    //SetThreadPriority(pThreadAttr->hThread, THREAD_PRIORITY_TIME_CRITICAL + priority);
    if(isDetached)
    {
        CloseHandle(pThreadAttr->hThread); // detached
    }

    return (THREAD_T)pThreadAttr;

err2:
    MmMngrFree(pThreadAttr);

err1:
    return NULL;
}

ATTRIBUTE_WEAK void ThreadJoin(THREAD_T thread)
{
    THREAD_ATTR_WINDOWS_ST* pThreadAttr = (THREAD_ATTR_WINDOWS_ST*)thread;
    WaitForSingleObject(pThreadAttr->hThread, INFINITE);
    CloseHandle(pThreadAttr->hThread);
    MmMngrFree(thread);
}

ATTRIBUTE_WEAK void ThreadUsleep(uint32_t us)
{
    Sleep(us / 1000);
}

ATTRIBUTE_WEAK void ThreadPostMsg(THREAD_T thread, uint32_t msg)
{
    THREAD_ATTR_WINDOWS_ST* pThreadAttr = (THREAD_ATTR_WINDOWS_ST*)thread;
    PostThreadMessage(pThreadAttr->iThread, msg, 0, 0);
}


#elif defined(__APPLE__)


#else
//rtos
#include "cmsis_os2.h"


typedef struct
{
    osMutexId_t mutex;
    int32_t sem;
}SEM_ATTR_T;

ATTRIBUTE_WEAK SEM_T SemCreate(void)
{

    return NULL;
}

ATTRIBUTE_WEAK void SemDestroy(SEM_T sem)
{
    (void) sem;
}

ATTRIBUTE_WEAK int32_t SemTimedWait(SEM_T sem, int32_t toMs)
{
    (void) sem;
    (void) toMs;
    return 0;
}

ATTRIBUTE_WEAK void SemPost(SEM_T sem)
{
    (void) sem;
}


#define EVT_DEFT 0x00000001

ATTRIBUTE_WEAK EVENT_T EventCreate(void)
{
    osEventFlagsId_t event = NULL;

    event = osEventFlagsNew(NULL);

    return (EVENT_T)event;
}

ATTRIBUTE_WEAK void EventDestroy(EVENT_T event)
{
    osEventFlagsDelete((osEventFlagsId_t *)event);
}

ATTRIBUTE_WEAK void EventGeneration(EVENT_T event)
{
    osEventFlagsSet((osEventFlagsId_t *)event, EVT_DEFT);
}

ATTRIBUTE_WEAK int32_t EventTimedWait(EVENT_T event, int32_t toMs)
{
    int32_t ret = 0;

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
    else if(osFlagsErrorTimeout == (uint32_t)ret)
    {
        return RESULT_ERR_TIME_OUT;
    }
    else
    {
        return RESULT_ERR_OTHER;
    }
}


ATTRIBUTE_WEAK MUTEX_T MutexCreate(void)
{
    osMutexId_t mutex = NULL;

    mutex = osMutexNew(NULL);

    return (MUTEX_T)mutex;
}


ATTRIBUTE_WEAK void MutexDestroy(MUTEX_T    mutex)
{
    osMutexDelete((osMutexId_t *)mutex);
}

ATTRIBUTE_WEAK void MutexLock(MUTEX_T mutex)
{
    osMutexAcquire((osMutexId_t *)mutex, osWaitForever);
}

ATTRIBUTE_WEAK void MutexUnlock(MUTEX_T mutex)
{
    osMutexRelease((osMutexId_t *)mutex);
}


typedef struct
{
    osThreadId_t threadId;
    THREAD_FUNC_FT pFunc;
    int8_t name[24];
    bool isDetached;
    void *pUsrData;
}THREAD_ATTR_RTOS_ST;

static void ThreadFunc(void *argument)
{
    THREAD_ATTR_RTOS_ST *pThreadAttr = (THREAD_ATTR_RTOS_ST *)argument;

    pThreadAttr->pFunc(pThreadAttr->pUsrData);

    if(pThreadAttr->isDetached)
    {
        MmMngrFree(pThreadAttr);
    }
    osThreadExit();
}

ATTRIBUTE_WEAK THREAD_T ThreadCreate(THREAD_FUNC_FT pFunc, void *pArg, uint32_t stackSz, bool isDetached, uint32_t priority, int8_t *name)
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

ATTRIBUTE_WEAK void ThreadJoin(THREAD_T thread)
{
    THREAD_ATTR_RTOS_ST *pThread = (THREAD_ATTR_RTOS_ST *) thread;

    osThreadJoin(pThread->threadId);
    MmMngrFree(thread);
}

ATTRIBUTE_WEAK void ThreadUsleep(uint32_t us)
{
    osDelay((us + 999) / 1000);
}


#endif






