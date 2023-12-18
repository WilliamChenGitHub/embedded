#include "Miscellaneous.h"
#include "Thread.h"
#include "MmMngr.h"


#ifndef __LIBEMBEDDEDVER
#pragma message ("version is not define use the default version 0xFFFF")
#define __LIBEMBEDDEDVER (0xFFFF)
#endif

U16_T GetLibEmbeddedBCDVer(VOID_T)
{
    return __LIBEMBEDDEDVER;
}


#if defined(__unix__) || defined(__linux__)  
#include <sys/time.h>
#include <time.h>

ATTRIBUTE_WEAK U64_T GetRealTickNs(VOID_T)
{
    struct timespec ts;
    U64_T ns = 0;

    clock_gettime(CLOCK_REALTIME, &ts);

    ns = ts.tv_nsec + ts.tv_sec * 1000000000;

    return ns;
}

ATTRIBUTE_WEAK U64_T GetMonotonicTickNs(VOID_T)
{
    struct timespec ts;
    U64_T ns = 0;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    ns = ts.tv_nsec + ts.tv_sec * 1000000000;

    return ns;
}

#elif defined(_WIN32)
#include <Windows.h>

ATTRIBUTE_WEAK U64_T GetRealTickNs(VOID_T)
{
    U64_T ns = 0;
    FILETIME ft;
    ULARGE_INTEGER ull;

    GetSystemTimeAsFileTime(&ft);

    ull.LowPart = ft.dwLowDateTime;
    ull.HighPart = ft.dwHighDateTime;

    // Windows time stamp interval is 100ns, from 1601-1-1
    const U64_T EPOCH = 116444736000000000ULL;
    ns = (ull.QuadPart - EPOCH) * 10; // convert to ns
    
    return ns;
}

ATTRIBUTE_WEAK U64_T GetMonotonicTickNs(VOID_T)
{
    static S32_T first = 1;
    static LARGE_INTEGER frequency;
    static U64_T ns = 0;
    LARGE_INTEGER count;
    

    if (first)
    {
        if (!QueryPerformanceFrequency(&frequency))
        {
            return 0;
        }
        first = 0;
        ns = 1000000000 / frequency.QuadPart;
    }

    QueryPerformanceCounter(&count);

    return count.QuadPart* ns;
}

#elif defined(__APPLE__)


#else
//rtos
ATTRIBUTE_WEAK U64_T GetRealTickNs(VOID_T)
{
    return 0;
}

ATTRIBUTE_WEAK U64_T GetMonotonicTickNs(VOID_T)
{
    U64_T ns = 0;

    return ns;
}

#endif



