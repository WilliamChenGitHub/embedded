#include "Miscellaneous.h"
#include "Thread.h"
#include "MmMngr.h"


#ifndef __LIBEMBEDDEDVER
#warning "version is not define use the default version 0xFFFF"
#define __LIBEMBEDDEDVER (0xFFFF)
#endif

U16_T GetLibEmbeddedBCDVer(VOID_T)
{
    return __LIBEMBEDDEDVER;
}


#if defined(__unix__) || defined(__linux__)  
#include <sys/time.h>
#include <time.h>

__attribute__((weak)) U64_T GetRealTickNs(VOID_T)
{
    struct timespec ts;
    U64_T ns = 0;

    clock_gettime(CLOCK_REALTIME, &ts);

    ns = ts.tv_nsec + ts.tv_sec * 1000000000;

    return ns;
}

__attribute__((weak)) U64_T GetMonotonicTickNs(VOID_T)
{
    struct timespec ts;
    U64_T ns = 0;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    ns = ts.tv_nsec + ts.tv_sec * 1000000000;

    return ns;
}

#elif defined(_WIN32)



#elif defined(__APPLE__)


#else
//rtos
__attribute__((weak)) U64_T GetRealTickNs(VOID_T)
{
    return 0;
}

__attribute__((weak)) U64_T GetMonotonicTickNs(VOID_T)
{
    U64_T ns = 0;

    return ns;
}

#endif



