#ifndef __MEDIA_BUFFER_H__
#define __MEDIA_BUFFER_H__

#include "Queue.h"
#include "Thread.h"
#include "Log.h"

typedef struct
{
    int itemCnt;
    MUTEX_T mutex;

    QUEUE_ST queue;
}MEDIA_BUFFER_ST;


#ifdef __cplusplus
extern "C" 
{
#endif

int MediaBufPushBufList(MEDIA_BUFFER_ST *pMediaBuf, QUEUE_BUF_LIST_ST *pBufList, int totalSz);

int MediaBufPush(MEDIA_BUFFER_ST *pMediaBuf, void *pBuf, int sz);

int MediaBufGetNextItemSz(MEDIA_BUFFER_ST *pMediaBuf);

int MediaBufPop(MEDIA_BUFFER_ST *pMediaBuf, void *pBuf);

// malloc buffer mem
MEDIA_BUFFER_ST * MediaBufCreate(int bSz);

// free queue mem
int32_t MediaBufDestroy(MEDIA_BUFFER_ST *pMediaBuf);

void MediaBufClear(MEDIA_BUFFER_ST * pMediaBuf);

#ifdef __cplusplus
}
#endif

#endif


